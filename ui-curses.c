#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "ui-curses.h"
#include "vis.h"
#include "vis-core.h"
#include "text.h"
#include "util.h"
#include "text-util.h"

#ifdef NCURSES_VERSION
# ifndef NCURSES_EXT_COLORS
#  define NCURSES_EXT_COLORS 0
# endif
# if !NCURSES_EXT_COLORS
#  define MAX_COLOR_PAIRS 256
# endif
#endif
#ifndef MAX_COLOR_PAIRS
# define MAX_COLOR_PAIRS COLOR_PAIRS
#endif

#ifndef DEBUG_UI
#define DEBUG_UI 0
#endif

#if DEBUG_UI
#define debug(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#define debug(...) do { } while (0)
#endif

#if 0
#define wresize(win, y, x) do { \
	if (wresize(win, y, x) == ERR) { \
		printf("ERROR resizing: %d x %d\n", x, y); \
	} else { \
		printf("OK resizing: %d x %d\n", x, y); \
	} \
	fflush(stdout); \
} while (0);

#define mvwin(win, y, x) do { \
	if (mvwin(win, y, x) == ERR) { \
		printf("ERROR moving: %d x %d\n", x, y); \
	} else { \
		printf("OK moving: %d x %d\n", x, y); \
	} \
	fflush(stdout); \
} while (0);
#endif

#define MAX_COLOR_CLOBBER 240
static short color_clobber_idx = 0;
static uint32_t clobbering_colors[MAX_COLOR_CLOBBER];
static bool change_colors;

typedef struct {
	attr_t attr;
	short fg, bg;
} CellStyle;

typedef struct UiCursesWin UiCursesWin;

typedef struct {
	Ui ui;                    /* generic ui interface, has to be the first struct member */
	Vis *vis;              /* editor instance to which this ui belongs */
	UiCursesWin *windows;     /* all windows managed by this ui */
	UiCursesWin *selwin;      /* the currently selected layout */
	char info[512];           /* info message displayed at the bottom of the screen */
	int width, height;        /* terminal dimensions available for all windows */
	enum UiLayout layout;     /* whether windows are displayed horizontally or vertically */
	TermKey *termkey;         /* libtermkey instance to handle keyboard input (stdin or /dev/tty) */
} UiCurses;

struct UiCursesWin {
	UiWin uiwin;              /* generic interface, has to be the first struct member */
	UiCurses *ui;             /* ui which manages this window */
	File *file;               /* file being displayed in this window */
	View *view;               /* current viewport */
	WINDOW *win;              /* curses window for the text area */
	WINDOW *winstatus;        /* curses window for the status bar */
	WINDOW *winside;          /* curses window for the side bar (line numbers) */
	int width, height;        /* window dimension including status bar */
	int x, y;                 /* window position */
	int sidebar_width;        /* width of the sidebar showing line numbers etc. */
	UiCursesWin *next, *prev; /* pointers to neighbouring windows */
	enum UiOption options;    /* display settings for this window */
	CellStyle styles[UI_STYLE_MAX];
};

__attribute__((noreturn)) static void ui_die(Ui *ui, const char *msg, va_list ap) {
	UiCurses *uic = (UiCurses*)ui;
	endwin();
	if (uic->termkey)
		termkey_stop(uic->termkey);
	vfprintf(stderr, msg, ap);
	exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void ui_die_msg(Ui *ui, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	ui_die(ui, msg, ap);
	va_end(ap);
}

/* Calculate r,g,b components of one of the standard upper 240 colors */
static void get_6cube_rgb(unsigned int n, int *r, int *g, int *b)
{
	if (n < 16) {
		return;
	} else if (n < 232) {
		n -= 16;
		*r = (n / 36) ? (n / 36) * 40 + 55 : 0;
		*g = ((n / 6) % 6) ? ((n / 6) % 6) * 40 + 55 : 0;
		*b = (n % 6) ? (n % 6) * 40 + 55 : 0;
	} else if (n < 256) {
		n -= 232;
		*r = n * 10 + 8;
		*g = n * 10 + 8;
		*b = n * 10 + 8;
	}
}

/* Reset color palette to default values using OSC 104 */
static void undo_palette(void)
{
	fputs("\033]104;\a", stderr);
	fflush(stderr);
}

/* Work out the nearest color from the 256 color set, or perhaps exactly. */
static int color_find_rgb(unsigned char r, unsigned char g, unsigned char b)
{
	if (change_colors) {
		uint32_t hexrep = ((r << 16) | (g << 8) | b) + 1;
		for (short i = 0; i < MAX_COLOR_CLOBBER; ++i) {
			if (clobbering_colors[i] == hexrep)
				return i + 16;
			else if (!clobbering_colors[i])
				break;
		}

		short i = color_clobber_idx;
		clobbering_colors[i] = hexrep;
		init_color(i + 16, (r * 1000) / 0xff, (g * 1000) / 0xff,
		           (b * 1000) / 0xff);

		/* in the unlikely case a user requests this many colors, reuse old slots */
		if (++color_clobber_idx >= MAX_COLOR_CLOBBER)
			color_clobber_idx = 0;

		return i + 16;
	}

	static const unsigned char color_256_to_16[256] = {
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		 0,  4,  4,  4, 12, 12,  2,  6,  4,  4, 12, 12,  2,  2,  6,  4,
		12, 12,  2,  2,  2,  6, 12, 12, 10, 10, 10, 10, 14, 12, 10, 10,
		10, 10, 10, 14,  1,  5,  4,  4, 12, 12,  3,  8,  4,  4, 12, 12,
		 2,  2,  6,  4, 12, 12,  2,  2,  2,  6, 12, 12, 10, 10, 10, 10,
		14, 12, 10, 10, 10, 10, 10, 14,  1,  1,  5,  4, 12, 12,  1,  1,
		 5,  4, 12, 12,  3,  3,  8,  4, 12, 12,  2,  2,  2,  6, 12, 12,
		10, 10, 10, 10, 14, 12, 10, 10, 10, 10, 10, 14,  1,  1,  1,  5,
		12, 12,  1,  1,  1,  5, 12, 12,  1,  1,  1,  5, 12, 12,  3,  3,
		 3,  7, 12, 12, 10, 10, 10, 10, 14, 12, 10, 10, 10, 10, 10, 14,
		 9,  9,  9,  9, 13, 12,  9,  9,  9,  9, 13, 12,  9,  9,  9,  9,
		13, 12,  9,  9,  9,  9, 13, 12, 11, 11, 11, 11,  7, 12, 10, 10,
		10, 10, 10, 14,  9,  9,  9,  9,  9, 13,  9,  9,  9,  9,  9, 13,
		 9,  9,  9,  9,  9, 13,  9,  9,  9,  9,  9, 13,  9,  9,  9,  9,
		 9, 13, 11, 11, 11, 11, 11, 15,  0,  0,  0,  0,  0,  0,  8,  8,
		 8,  8,  8,  8,  7,  7,  7,  7,  7,  7, 15, 15, 15, 15, 15, 15
	};

	int i = 0;
	if ((!r || (r - 55) % 40 == 0) &&
	    (!g || (g - 55) % 40 == 0) &&
	    (!b || (b - 55) % 40 == 0)) {
		i = 16;
		i += r ? ((r - 55) / 40) * 36 : 0;
		i += g ? ((g - 55) / 40) * 6 : 0;
		i += g ? ((b - 55) / 40) : 0;
	} else if (r == g && g == b && (r - 8) % 10 == 0 && r < 239) {
		i = 232 + ((r - 8) / 10);
	} else {
		unsigned lowest = UINT_MAX;
		for (int j = 16; j < 256; ++j) {
			int jr = 0, jg = 0, jb = 0;
			get_6cube_rgb(j, &jr, &jg, &jb);
			int dr = jr - r;
			int dg = jg - g;
			int db = jb - b;
			unsigned int distance = dr * dr + dg * dg + db * db;
			if (distance < lowest) {
				lowest = distance;
				i = j;
			}
		}
	}

	if (COLORS <= 16)
		return color_256_to_16[i];
	return i;
}

/* Convert color from string. */
static int color_fromstring(const char *s)
{
	if (!s)
		return -1;
	if (*s == '#' && strlen(s) == 7) {
		const char *cp;
		unsigned char r, g, b;
		for (cp = s + 1; isxdigit((unsigned char)*cp); cp++);
		if (*cp != '\0')
			return -1;
		int n = sscanf(s + 1, "%2hhx%2hhx%2hhx", &r, &g, &b);
		if (n != 3)
			return -1;
		return color_find_rgb(r, g, b);
	} else if ('0' <= *s && *s <= '9') {
		int col = atoi(s);
		return (col <= 0 || col > 255) ? -1 : col;
	}

	if (strcasecmp(s, "black") == 0)
		return 0;
	if (strcasecmp(s, "red") == 0)
		return 1;
	if (strcasecmp(s, "green") == 0)
		return 2;
	if (strcasecmp(s, "yellow") == 0)
		return 3;
	if (strcasecmp(s, "blue") == 0)
		return 4;
	if (strcasecmp(s, "magenta") == 0)
		return 5;
	if (strcasecmp(s, "cyan") == 0)
		return 6;
	if (strcasecmp(s, "white") == 0)
		return 7;
	return -1;
}

static inline unsigned int color_pair_hash(short fg, short bg) {
	if (fg == -1)
		fg = COLORS;
	if (bg == -1)
		bg = COLORS + 1;
	return fg * (COLORS + 2) + bg;
}

static short color_pair_get(short fg, short bg) {
	static bool has_default_colors;
	static short *color2palette, default_fg, default_bg;
	static short color_pairs_max, color_pair_current;

	if (!color2palette) {
		pair_content(0, &default_fg, &default_bg);
		if (default_fg == -1)
			default_fg = COLOR_WHITE;
		if (default_bg == -1)
			default_bg = COLOR_BLACK;
		has_default_colors = (use_default_colors() == OK);
		color_pairs_max = MIN(COLOR_PAIRS, MAX_COLOR_PAIRS);
		if (COLORS)
			color2palette = calloc((COLORS + 2) * (COLORS + 2), sizeof(short));
	}

	if (fg >= COLORS)
		fg = default_fg;
	if (bg >= COLORS)
		bg = default_bg;

	if (!has_default_colors) {
		if (fg == -1)
			fg = default_fg;
		if (bg == -1)
			bg = default_bg;
	}

	if (!color2palette || (fg == -1 && bg == -1))
		return 0;

	unsigned int index = color_pair_hash(fg, bg);
	if (color2palette[index] == 0) {
		short oldfg, oldbg;
		if (++color_pair_current >= color_pairs_max)
			color_pair_current = 1;
		pair_content(color_pair_current, &oldfg, &oldbg);
		unsigned int old_index = color_pair_hash(oldfg, oldbg);
		if (init_pair(color_pair_current, fg, bg) == OK) {
			color2palette[old_index] = 0;
			color2palette[index] = color_pair_current;
		}
	}

	return color2palette[index];
}

static inline attr_t style_to_attr(CellStyle *style) {
	return style->attr | COLOR_PAIR(color_pair_get(style->fg, style->bg));
}

static bool ui_window_syntax_style(UiWin *w, int id, const char *style) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (id >= UI_STYLE_MAX)
		return false;
	if (!style)
		return true;
	CellStyle cell_style = win->styles[UI_STYLE_DEFAULT];
	char *style_copy = strdup(style), *option = style_copy, *next, *p;
	while (option) {
		if ((next = strchr(option, ',')))
			*next++ = '\0';
		if ((p = strchr(option, ':')))
			*p++ = '\0';
		if (!strcasecmp(option, "reverse")) {
			cell_style.attr |= A_REVERSE;
		} else if (!strcasecmp(option, "bold")) {
			cell_style.attr |= A_BOLD;
		} else if (!strcasecmp(option, "notbold")) {
			cell_style.attr &= ~A_BOLD;
#ifdef A_ITALIC
		} else if (!strcasecmp(option, "italics")) {
			cell_style.attr |= A_ITALIC;
		} else if (!strcasecmp(option, "notitalics")) {
			cell_style.attr &= ~A_ITALIC;
#endif
		} else if (!strcasecmp(option, "underlined")) {
			cell_style.attr |= A_UNDERLINE;
		} else if (!strcasecmp(option, "notunderlined")) {
			cell_style.attr &= ~A_UNDERLINE;
		} else if (!strcasecmp(option, "blink")) {
			cell_style.attr |= A_BLINK;
		} else if (!strcasecmp(option, "notblink")) {
			cell_style.attr &= ~A_BLINK;
		} else if (!strcasecmp(option, "fore")) {
			cell_style.fg = color_fromstring(p);
		} else if (!strcasecmp(option, "back")) {
			cell_style.bg = color_fromstring(p);
		}
		option = next;
	}
	win->styles[id] = cell_style;
	free(style_copy);
	return true;
}

static void ui_window_resize(UiCursesWin *win, int width, int height) {
	debug("ui-win-resize[%s]: %dx%d\n", win->file->name ? win->file->name : "noname", width, height);
	win->width = width;
	win->height = height;
	if (win->winstatus)
		wresize(win->winstatus, 1, width);
	wresize(win->win, win->winstatus ? height - 1 : height, width - win->sidebar_width);
	if (win->winside)
		wresize(win->winside, height-1, win->sidebar_width);
	view_resize(win->view, width - win->sidebar_width, win->winstatus ? height - 1 : height);
}

static void ui_window_move(UiCursesWin *win, int x, int y) {
	debug("ui-win-move[%s]: (%d, %d)\n", win->file->name ? win->file->name : "noname", x, y);
	win->x = x;
	win->y = y;
	mvwin(win->win, y, x + win->sidebar_width);
	if (win->winside)
		mvwin(win->winside, y, x);
	if (win->winstatus)
		mvwin(win->winstatus, y + win->height - 1, x);
}

static bool ui_window_draw_sidebar(UiCursesWin *win) {
	if (!win->winside)
		return true;
	const Line *line = view_lines_get(win->view);
	int sidebar_width = snprintf(NULL, 0, "%zd", line->lineno + win->height - 2) + 1;
	if (win->sidebar_width != sidebar_width) {
		win->sidebar_width = sidebar_width;
		ui_window_resize(win, win->width, win->height);
		ui_window_move(win, win->x, win->y);
		return false;
	} else {
		int i = 0;
		size_t prev_lineno = 0;
		const Line *cursor_line = view_line_get(win->view);
		size_t cursor_lineno = cursor_line->lineno;
		werase(win->winside);
		wbkgd(win->winside, style_to_attr(&win->styles[UI_STYLE_DEFAULT]));
		wattrset(win->winside, style_to_attr(&win->styles[UI_STYLE_LINENUMBER]));
		for (const Line *l = line; l; l = l->next, i++) {
			if (l->lineno && l->lineno != prev_lineno) {
				if (win->options & UI_OPTION_LINE_NUMBERS_ABSOLUTE) {
					mvwprintw(win->winside, i, 0, "%*u", sidebar_width-1, l->lineno);
				} else if (win->options & UI_OPTION_LINE_NUMBERS_RELATIVE) {
					size_t rel = (win->options & UI_OPTION_LARGE_FILE) ? 0 : l->lineno;
					if (l->lineno > cursor_lineno)
						rel = l->lineno - cursor_lineno;
					else if (l->lineno < cursor_lineno)
						rel = cursor_lineno - l->lineno;
					mvwprintw(win->winside, i, 0, "%*u", sidebar_width-1, rel);
				}
			}
			prev_lineno = l->lineno;
		}
		mvwvline(win->winside, 0, sidebar_width-1, ACS_VLINE, win->height-1);
		return true;
	}
}

static void ui_window_status(UiWin *w, const char *status) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!win->winstatus)
		return;
	UiCurses *uic = win->ui;
	bool focused = uic->selwin == win;
	wattrset(win->winstatus, focused ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win->winstatus, 0, 0, ' ', win->width);
	if (status)
		mvwprintw(win->winstatus, 0, 0, "%s", status);
}

static void ui_window_draw(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!ui_window_draw_sidebar(win))
		return;

	debug("ui-win-draw[%s]\n", win->file->name ? win->file->name : "noname");
	wbkgd(win->win, style_to_attr(&win->styles[UI_STYLE_DEFAULT]));
	wmove(win->win, 0, 0);
	int width = view_width_get(win->view);
	CellStyle *prev_style = NULL;
	size_t cursor_lineno = -1;

	if (win->options & UI_OPTION_CURSOR_LINE && win->ui->selwin == win) {
		Filerange selection = view_selection_get(win->view);
		if (!view_cursors_multiple(win->view) && !text_range_valid(&selection)) {
			const Line *line = view_line_get(win->view);
			cursor_lineno = line->lineno;
		}
	}

	short selection_bg = win->styles[UI_STYLE_SELECTION].bg;
	short cul_bg = win->styles[UI_STYLE_CURSOR_LINE].bg;
	attr_t cul_attr = win->styles[UI_STYLE_CURSOR_LINE].attr;
	bool multiple_cursors = view_cursors_multiple(win->view);
	attr_t attr = A_NORMAL;

	for (const Line *l = view_lines_get(win->view); l; l = l->next) {
		bool cursor_line = l->lineno == cursor_lineno;
		for (int x = 0; x < width; x++) {
			enum UiStyle style_id = l->cells[x].style;
			if (style_id == 0)
				style_id = UI_STYLE_DEFAULT;
			CellStyle *style = &win->styles[style_id];

			if (l->cells[x].cursor && win->ui->selwin == win) {
				if (multiple_cursors && l->cells[x].cursor_primary)
					attr = style_to_attr(&win->styles[UI_STYLE_CURSOR_PRIMARY]);
				else
					attr = style_to_attr(&win->styles[UI_STYLE_CURSOR]);
				prev_style = NULL;
			} else if (l->cells[x].selected) {
				if (style->fg == selection_bg)
					attr = style->attr | A_REVERSE;
				else
					attr = style->attr | COLOR_PAIR(color_pair_get(style->fg, selection_bg));
				prev_style = NULL;
			} else if (cursor_line) {
				attr = cul_attr | (style->attr & ~A_COLOR) | COLOR_PAIR(color_pair_get(style->fg, cul_bg));
				prev_style = NULL;
			} else if (style != prev_style) {
				attr = style_to_attr(style);
				prev_style = style;
			}
			wattrset(win->win, attr);
			waddstr(win->win, l->cells[x].data);
		}
		/* try to fixup display issues, in theory we should always output a full line */
		int x, y;
		getyx(win->win, y, x);
		(void)y;
		wattrset(win->win, A_NORMAL);
		for (; 0 < x && x < width; x++)
			waddstr(win->win, " ");
	}

	wclrtobot(win->win);
}

static void ui_window_reload(UiWin *w, File *file) {
	UiCursesWin *win = (UiCursesWin*)w;
	win->file = file;
	win->sidebar_width = 0;
	view_reload(win->view, file->text);
	ui_window_draw(w);
}

static void ui_window_update(UiCursesWin *win) {
	debug("ui-win-update[%s]\n", win->file->name ? win->file->name : "noname");
	if (win->winstatus)
		wnoutrefresh(win->winstatus);
	if (win->winside)
		wnoutrefresh(win->winside);
	wnoutrefresh(win->win);
}

static void ui_arrange(Ui *ui, enum UiLayout layout) {
	debug("ui-arrange\n");
	UiCurses *uic = (UiCurses*)ui;
	uic->layout = layout;
	int n = 0, m = !!uic->info[0], x = 0, y = 0;
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			m++;
		else
			n++;
	}
	int max_height = uic->height - m;
	int width = (uic->width / MAX(1, n)) - 1;
	int height = max_height / MAX(1, n);
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			continue;
		n--;
		if (layout == UI_LAYOUT_HORIZONTAL) {
			int h = n ? height : max_height - y;
			ui_window_resize(win, uic->width, h);
			ui_window_move(win, x, y);
			y += h;
		} else {
			int w = n ? width : uic->width - x;
			ui_window_resize(win, w, max_height);
			ui_window_move(win, x, y);
			x += w;
			if (n)
				mvvline(0, x++, ACS_VLINE, max_height);
		}
	}

	if (layout == UI_LAYOUT_VERTICAL)
		y = max_height;

	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (!(win->options & UI_OPTION_ONELINE))
			continue;
		ui_window_resize(win, uic->width, 1);
		ui_window_move(win, 0, y++);
	}
}

static void ui_draw(Ui *ui) {
	debug("ui-draw\n");
	UiCurses *uic = (UiCurses*)ui;
	erase();
	ui_arrange(ui, uic->layout);

	for (UiCursesWin *win = uic->windows; win; win = win->next)
		ui_window_draw((UiWin*)win);

	if (uic->info[0]) {
	        attrset(A_BOLD);
        	mvaddstr(uic->height-1, 0, uic->info);
	}

	wnoutrefresh(stdscr);
}

static void ui_redraw(Ui *ui) {
	clear();
	ui_draw(ui);
}

static void ui_resize_to(Ui *ui, int width, int height) {
	UiCurses *uic = (UiCurses*)ui;
	uic->width = width;
	uic->height = height;
	ui_draw(ui);
}

static void ui_resize(Ui *ui) {
	struct winsize ws;
	int width, height;

	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == -1) {
		getmaxyx(stdscr, height, width);
	} else {
		width = ws.ws_col;
		height = ws.ws_row;
	}

	resizeterm(height, width);
	wresize(stdscr, height, width);
	ui_resize_to(ui, width, height);
}

static void ui_update(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	for (UiCursesWin *win = uic->windows; win; win = win->next)
		ui_window_update(win);
	debug("ui-doupdate\n");
	doupdate();
}

static void ui_window_free(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!win)
		return;
	UiCurses *uic = win->ui;
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (uic->windows == win)
		uic->windows = win->next;
	if (uic->selwin == win)
		uic->selwin = NULL;
	win->next = win->prev = NULL;
	if (win->winstatus)
		delwin(win->winstatus);
	if (win->winside)
		delwin(win->winside);
	if (win->win)
		delwin(win->win);
	free(win);
}

static void ui_window_focus(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	UiCursesWin *oldsel = win->ui->selwin;
	win->ui->selwin = win;
	if (oldsel) {
		view_draw(oldsel->view);
		ui_window_draw((UiWin*)oldsel);
	}
	view_draw(win->view);
	ui_window_draw(w);
}

static void ui_window_options_set(UiWin *w, enum UiOption options) {
	UiCursesWin *win = (UiCursesWin*)w;
	win->options = options;
	if (options & (UI_OPTION_LINE_NUMBERS_ABSOLUTE|UI_OPTION_LINE_NUMBERS_RELATIVE)) {
		if (!win->winside)
			win->winside = newwin(1, 1, 1, 1);
	} else {
		if (win->winside) {
			delwin(win->winside);
			win->winside = NULL;
			win->sidebar_width = 0;
		}
	}
	if (options & UI_OPTION_STATUSBAR) {
		if (!win->winstatus)
			win->winstatus = newwin(1, 0, 0, 0);
	} else {
		if (win->winstatus)
			delwin(win->winstatus);
		win->winstatus = NULL;
	}

	if (options & UI_OPTION_ONELINE) {
		/* move the new window to the end of the list */
		UiCurses *uic = win->ui;
		UiCursesWin *last = uic->windows;
		while (last->next)
			last = last->next;
		if (last != win) {
			if (win->prev)
				win->prev->next = win->next;
			if (win->next)
				win->next->prev = win->prev;
			if (uic->windows == win)
				uic->windows = win->next;
			last->next = win;
			win->prev = last;
			win->next = NULL;
		}
	}

	ui_draw((Ui*)win->ui);
}

static enum UiOption ui_window_options_get(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	return win->options;
}

static int ui_window_width(UiWin *win) {
	return ((UiCursesWin*)win)->width;
}

static int ui_window_height(UiWin *win) {
	return ((UiCursesWin*)win)->height;
}

static void ui_window_swap(UiWin *aw, UiWin *bw) {
	UiCursesWin *a = (UiCursesWin*)aw;
	UiCursesWin *b = (UiCursesWin*)bw;
	if (a == b || !a || !b)
		return;
	UiCurses *ui = a->ui;
	UiCursesWin *tmp = a->next;
	a->next = b->next;
	b->next = tmp;
	if (a->next)
		a->next->prev = a;
	if (b->next)
		b->next->prev = b;
	tmp = a->prev;
	a->prev = b->prev;
	b->prev = tmp;
	if (a->prev)
		a->prev->next = a;
	if (b->prev)
		b->prev->next = b;
	if (ui->windows == a)
		ui->windows = b;
	else if (ui->windows == b)
		ui->windows = a;
	if (ui->selwin == a)
		ui_window_focus(bw);
	else if (ui->selwin == b)
		ui_window_focus(aw);
}

static UiWin *ui_window_new(Ui *ui, View *view, File *file, enum UiOption options) {
	UiCurses *uic = (UiCurses*)ui;
	UiCursesWin *win = calloc(1, sizeof(UiCursesWin));
	if (!win)
		return NULL;

	win->uiwin = (UiWin) {
		.draw = ui_window_draw,
		.status = ui_window_status,
		.options_set = ui_window_options_set,
		.options_get = ui_window_options_get,
		.reload = ui_window_reload,
		.syntax_style = ui_window_syntax_style,
		.window_width = ui_window_width,
		.window_height = ui_window_height,
	};

	if (!(win->win = newwin(0, 0, 0, 0))) {
		ui_window_free((UiWin*)win);
		return NULL;
	}


	for (int i = 0; i < UI_STYLE_MAX; i++) {
		win->styles[i] = (CellStyle) {
			.fg = -1, .bg = -1, .attr = A_NORMAL,
		};
	}

	win->styles[UI_STYLE_CURSOR].attr |= A_REVERSE;
	win->styles[UI_STYLE_CURSOR_PRIMARY].attr |= A_REVERSE|A_BLINK;
	win->styles[UI_STYLE_SELECTION].attr |= A_REVERSE;
	win->styles[UI_STYLE_COLOR_COLUMN].attr |= A_REVERSE;

	win->ui = uic;
	win->view = view;
	win->file = file;
	view_ui(view, &win->uiwin);

	if (uic->windows)
		uic->windows->prev = win;
	win->next = uic->windows;
	uic->windows = win;

	if (text_size(file->text) > UI_LARGE_FILE_SIZE) {
		options |= UI_OPTION_LARGE_FILE;
		options &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
	}

	ui_window_options_set((UiWin*)win, options);

	return &win->uiwin;
}

static void ui_info(Ui *ui, const char *msg, va_list ap) {
	UiCurses *uic = (UiCurses*)ui;
	vsnprintf(uic->info, sizeof(uic->info), msg, ap);
	ui_draw(ui);
}

static void ui_info_hide(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (uic->info[0]) {
		uic->info[0] = '\0';
		ui_draw(ui);
	}
}

static TermKey *ui_termkey_new(int fd) {
	TermKey *termkey = termkey_new(fd, TERMKEY_FLAG_UTF8);
	if (termkey)
		termkey_set_canonflags(termkey, TERMKEY_CANON_DELBS);
	return termkey;
}

static TermKey *ui_termkey_reopen(Ui *ui, int fd) {
	int tty = open("/dev/tty", O_RDWR);
	if (tty == -1)
		return NULL;
	if (tty != fd && dup2(tty, fd) == -1) {
		close(tty);
		return NULL;
	}
	close(tty);
	return ui_termkey_new(fd);
}

static TermKey *ui_termkey_get(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	return uic->termkey;
}

static void ui_suspend(Ui *ui) {
	if (change_colors)
		undo_palette();
	endwin();
	raise(SIGSTOP);
}

static bool ui_getkey(Ui *ui, TermKeyKey *key) {
	UiCurses *uic = (UiCurses*)ui;
	TermKeyResult ret = termkey_getkey(uic->termkey, key);

	if (ret == TERMKEY_RES_EOF) {
		termkey_destroy(uic->termkey);
		errno = 0;
		if (!(uic->termkey = ui_termkey_reopen(ui, STDIN_FILENO)))
			ui_die_msg(ui, "Failed to re-open stdin as /dev/tty: %s\n", errno != 0 ? strerror(errno) : "");
		return false;
	}

	if (ret == TERMKEY_RES_AGAIN) {
		struct pollfd fd;
		fd.fd = STDIN_FILENO;
		fd.events = POLLIN;
		if (poll(&fd, 1, termkey_get_waittime(uic->termkey)) == 0)
			ret = termkey_getkey_force(uic->termkey, key);
	}

	return ret == TERMKEY_RES_KEY;
}

static void ui_terminal_save(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	curs_set(1);
	reset_shell_mode();
	termkey_stop(uic->termkey);
}

static void ui_terminal_restore(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	termkey_start(uic->termkey);
	reset_prog_mode();
	wclear(stdscr);
	curs_set(0);
}

static int ui_colors(Ui *ui) {
	return COLORS;
}

static bool ui_init(Ui *ui, Vis *vis) {
	UiCurses *uic = (UiCurses*)ui;
	uic->vis = vis;

	setlocale(LC_CTYPE, "");

	char *term = getenv("TERM");
	if (!term)
		term = "xterm";

	errno = 0;
	if (!(uic->termkey = ui_termkey_new(STDIN_FILENO))) {
		/* work around libtermkey bug which fails if stdin is /dev/null */
		if (errno == EBADF && !isatty(STDIN_FILENO)) {
			errno = 0;
			if (!(uic->termkey = ui_termkey_reopen(ui, STDIN_FILENO)) && errno == ENXIO)
				uic->termkey = termkey_new_abstract(term, TERMKEY_FLAG_UTF8);
		}
		if (!uic->termkey)
			goto err;
	}

	if (!newterm(term, stderr, stdin)) {
		snprintf(uic->info, sizeof(uic->info), "Warning: unknown term `%s'", term);
		if (!newterm(strstr(term, "-256color") ? "xterm-256color" : "xterm", stderr, stdin))
			goto err;
	}
	start_color();
	use_default_colors();
	raw();
	noecho();
	nonl();
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	curs_set(0);
	change_colors = can_change_color() && COLORS >= 256;

	ui_resize(ui);
	return true;
err:
	ui_die_msg(ui, "Failed to start curses interface: %s\n", errno != 0 ? strerror(errno) : "");
	return false;
}

Ui *ui_curses_new(void) {

	Ui *ui = calloc(1, sizeof(UiCurses));
	if (!ui)
		return NULL;

	*ui = (Ui) {
		.init = ui_init,
		.free = ui_curses_free,
		.termkey_get = ui_termkey_get,
		.suspend = ui_suspend,
		.resize = ui_resize,
		.update = ui_update,
		.window_new = ui_window_new,
		.window_free = ui_window_free,
		.window_focus = ui_window_focus,
		.window_swap = ui_window_swap,
		.draw = ui_draw,
		.redraw = ui_redraw,
		.arrange = ui_arrange,
		.die = ui_die,
		.info = ui_info,
		.info_hide = ui_info_hide,
		.getkey = ui_getkey,
		.terminal_save = ui_terminal_save,
		.terminal_restore = ui_terminal_restore,
		.colors = ui_colors,
	};

	return ui;
}

void ui_curses_free(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (!uic)
		return;
	while (uic->windows)
		ui_window_free((UiWin*)uic->windows);
	if (change_colors)
		undo_palette();
	endwin();
	if (uic->termkey)
		termkey_destroy(uic->termkey);
	free(uic);
}
