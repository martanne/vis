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

#include "ui-terminal.h"
#include "vis.h"
#include "vis-core.h"
#include "text.h"
#include "util.h"
#include "text-util.h"

#ifndef DEBUG_UI
#define DEBUG_UI 0
#endif

#if DEBUG_UI
#define debug(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#define debug(...) do { } while (0)
#endif

#define MAX_WIDTH 1024
#define MAX_HEIGHT 1024
typedef struct UiTermWin UiTermWin;

typedef struct {
	Ui ui;                    /* generic ui interface, has to be the first struct member */
	Vis *vis;                 /* editor instance to which this ui belongs */
	UiTermWin *windows;       /* all windows managed by this ui */
	UiTermWin *selwin;        /* the currently selected layout */
	char info[MAX_WIDTH];     /* info message displayed at the bottom of the screen */
	int width, height;        /* terminal dimensions available for all windows */
	enum UiLayout layout;     /* whether windows are displayed horizontally or vertically */
	TermKey *termkey;         /* libtermkey instance to handle keyboard input (stdin or /dev/tty) */
	size_t ids;               /* bit mask of in use window ids */
	size_t styles_size;       /* #bytes allocated for styles array */
	CellStyle *styles;        /* each window has UI_STYLE_MAX different style definitions */
	size_t cells_size;        /* #bytes allocated for 2D grid (grows only) */
	Cell *cells;              /* 2D grid of cells, at least as large as current terminal size */
} UiTerm;

struct UiTermWin {
	UiWin uiwin;              /* generic interface, has to be the first struct member */
	UiTerm *ui;               /* ui which manages this window */
	Win *win;                 /* editor window being displayed */
	int id;                   /* unique identifier for this window */
	int width, height;        /* window dimension including status bar */
	int x, y;                 /* window position */
	int sidebar_width;        /* width of the sidebar showing line numbers etc. */
	UiTermWin *next, *prev;   /* pointers to neighbouring windows */
	enum UiOption options;    /* display settings for this window */
};

#if CONFIG_CURSES
#include "ui-terminal-curses.c"
#else
#include "ui-terminal-vt100.c"
#endif

__attribute__((noreturn)) static void ui_die(Ui *ui, const char *msg, va_list ap) {
	UiTerm *tui = (UiTerm*)ui;
	ui_term_backend_free(tui);
	if (tui->termkey)
		termkey_stop(tui->termkey);
	vfprintf(stderr, msg, ap);
	exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void ui_die_msg(Ui *ui, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	ui_die(ui, msg, ap);
	va_end(ap);
}

static void ui_window_resize(UiTermWin *win, int width, int height) {
	debug("ui-win-resize[%s]: %dx%d\n", win->win->file->name ? win->win->file->name : "noname", width, height);
	bool status = win->options & UI_OPTION_STATUSBAR;
	win->width = width;
	win->height = height;
	view_resize(win->win->view, width - win->sidebar_width, status ? height - 1 : height);
}

static void ui_window_move(UiTermWin *win, int x, int y) {
	debug("ui-win-move[%s]: (%d, %d)\n", win->win->file->name ? win->win->file->name : "noname", x, y);
	win->x = x;
	win->y = y;
}

static bool color_fromstring(UiTerm *ui, CellColor *color, const char *s)
{
	if (!s)
		return false;
	if (*s == '#' && strlen(s) == 7) {
		const char *cp;
		unsigned char r, g, b;
		for (cp = s + 1; isxdigit((unsigned char)*cp); cp++);
		if (*cp != '\0')
			return false;
		int n = sscanf(s + 1, "%2hhx%2hhx%2hhx", &r, &g, &b);
		if (n != 3)
			return false;
		*color = color_rgb(ui, r, g, b);
		return true;
	} else if ('0' <= *s && *s <= '9') {
		int index = atoi(s);
		if (index <= 0 || index > 255)
			return false;
		*color = color_terminal(ui, index);
		return true;
	}

	static const struct {
		const char *name;
		CellColor color;
	} color_names[] = {
		{ "black",   CELL_COLOR_BLACK   },
		{ "red",     CELL_COLOR_RED     },
		{ "green",   CELL_COLOR_GREEN   },
		{ "yellow",  CELL_COLOR_YELLOW  },
		{ "blue",    CELL_COLOR_BLUE    },
		{ "magenta", CELL_COLOR_MAGENTA },
		{ "cyan",    CELL_COLOR_CYAN    },
		{ "white",   CELL_COLOR_WHITE   },
		{ "default", CELL_COLOR_DEFAULT },
	};

	for (size_t i = 0; i < LENGTH(color_names); i++) {
		if (strcasecmp(color_names[i].name, s) == 0) {
			*color = color_names[i].color;
			return true;
		}
	}

	return false;
}

static bool ui_style_define(UiWin *w, int id, const char *style) {
	UiTermWin *win = (UiTermWin*)w;
	UiTerm *tui = win->ui;
	if (id >= UI_STYLE_MAX)
		return false;
	if (!style)
		return true;
	CellStyle cell_style = tui->styles[win->id * UI_STYLE_MAX + UI_STYLE_DEFAULT];
	char *style_copy = strdup(style), *option = style_copy;
	while (option) {
		while (*option == ' ')
			option++;
		char *next = strchr(option, ',');
		if (next)
			*next++ = '\0';
		char *value = strchr(option, ':');
		if (value)
			for (*value++ = '\0'; *value == ' '; value++);
		if (!strcasecmp(option, "reverse")) {
			cell_style.attr |= CELL_ATTR_REVERSE;
		} else if (!strcasecmp(option, "bold")) {
			cell_style.attr |= CELL_ATTR_BOLD;
		} else if (!strcasecmp(option, "notbold")) {
			cell_style.attr &= ~CELL_ATTR_BOLD;
		} else if (!strcasecmp(option, "italics")) {
			cell_style.attr |= CELL_ATTR_ITALIC;
		} else if (!strcasecmp(option, "notitalics")) {
			cell_style.attr &= ~CELL_ATTR_ITALIC;
		} else if (!strcasecmp(option, "underlined")) {
			cell_style.attr |= CELL_ATTR_UNDERLINE;
		} else if (!strcasecmp(option, "notunderlined")) {
			cell_style.attr &= ~CELL_ATTR_UNDERLINE;
		} else if (!strcasecmp(option, "blink")) {
			cell_style.attr |= CELL_ATTR_BLINK;
		} else if (!strcasecmp(option, "notblink")) {
			cell_style.attr &= ~CELL_ATTR_BLINK;
		} else if (!strcasecmp(option, "fore")) {
			color_fromstring(win->ui, &cell_style.fg, value);
		} else if (!strcasecmp(option, "back")) {
			color_fromstring(win->ui, &cell_style.bg, value);
		}
		option = next;
	}
	tui->styles[win->id * UI_STYLE_MAX + id] = cell_style;
	free(style_copy);
	return true;
}

static void ui_draw_line(UiTerm *tui, int x, int y, char c, enum UiStyle style_id) {
	if (x < 0 || x >= tui->width || y < 0 || y >= tui->height)
		return;
	CellStyle style = tui->styles[style_id];
	Cell *cells = tui->cells + y * tui->width;
	while (x < tui->width) {
		cells[x].data[0] = c;
		cells[x].data[1] = '\0';
		cells[x].style = style;
		x++;
	}
}

static void ui_draw_string(UiTerm *tui, int x, int y, const char *str, UiTermWin *win, enum UiStyle style_id) {
	debug("draw-string: [%d][%d]\n", y, x);
	if (x < 0 || x >= tui->width || y < 0 || y >= tui->height)
		return;
	CellStyle style = tui->styles[(win ? win->id : 0)*UI_STYLE_MAX + style_id];
	// FIXME: does not handle double width characters etc, share code with view.c?
	Cell *cells = tui->cells + y * tui->width;
	const size_t cell_size = sizeof(cells[0].data)-1;
	for (const char *next = str; *str && x < tui->width; str = next) {
		do next++; while (!ISUTF8(*next));
		size_t len = next - str;
		if (!len)
			break;
		len = MIN(len, cell_size);
		strncpy(cells[x].data, str, len);
		cells[x].data[len] = '\0';
		cells[x].style = style;
		x++;
	}
}

static void ui_window_draw(UiWin *w) {
	UiTermWin *win = (UiTermWin*)w;
	UiTerm *ui = win->ui;
	View *view = win->win->view;
	int width = win->width, height = win->height;
	const Line *line = view_lines_first(view);
	bool status = win->options & UI_OPTION_STATUSBAR;
	bool nu = win->options & UI_OPTION_LINE_NUMBERS_ABSOLUTE;
	bool rnu = win->options & UI_OPTION_LINE_NUMBERS_RELATIVE;
	bool sidebar = nu || rnu;
	int sidebar_width = sidebar ? snprintf(NULL, 0, "%zd ", line->lineno + height - 2) : 0;
	if (sidebar_width != win->sidebar_width) {
		view_resize(view, width - sidebar_width, status ? height - 1 : height);
		win->sidebar_width = sidebar_width;
	}
	vis_window_draw(win->win);
	line = view_lines_first(view);
	size_t prev_lineno = 0;
	Selection *sel = view_selections_primary_get(view);
	const Line *cursor_line = view_cursors_line_get(sel);
	size_t cursor_lineno = cursor_line->lineno;
	char buf[(sizeof(size_t) * CHAR_BIT + 2) / 3 + 1 + 1];
	int x = win->x, y = win->y;
	int view_width = view_width_get(view);
	Cell *cells = ui->cells + y * ui->width;
	if (x + sidebar_width + view_width > ui->width)
		view_width = ui->width - x - sidebar_width;
	for (const Line *l = line; l; l = l->next, y++) {
		if (sidebar) {
			if (!l->lineno || !l->len || l->lineno == prev_lineno) {
				memset(buf, ' ', sizeof(buf));
				buf[sidebar_width] = '\0';
			} else {
				size_t number = l->lineno;
				if (rnu) {
					number = (win->options & UI_OPTION_LARGE_FILE) ? 0 : l->lineno;
					if (l->lineno > cursor_lineno)
						number = l->lineno - cursor_lineno;
					else if (l->lineno < cursor_lineno)
						number = cursor_lineno - l->lineno;
				}
				snprintf(buf, sizeof buf, "%*zu ", sidebar_width-1, number);
			}
			ui_draw_string(ui, x, y, buf, win,
				(l->lineno == cursor_lineno) ? UI_STYLE_LINENUMBER_CURSOR : UI_STYLE_LINENUMBER);
			prev_lineno = l->lineno;
		}
		debug("draw-window: [%d][%d] ... cells[%d][%d]\n", y, x+sidebar_width, y, view_width);
		memcpy(&cells[x+sidebar_width], l->cells, sizeof(Cell) * view_width);
		cells += ui->width;
	}
}

static CellStyle ui_window_style_get(UiWin *w, enum UiStyle style) {
	UiTermWin *win = (UiTermWin*)w;
	UiTerm *tui = win->ui;
	return tui->styles[win->id * UI_STYLE_MAX + style];
}

static void ui_window_status(UiWin *w, const char *status) {
	UiTermWin *win = (UiTermWin*)w;
	if (!(win->options & UI_OPTION_STATUSBAR))
		return;
	UiTerm *ui = win->ui;
	enum UiStyle style = ui->selwin == win ? UI_STYLE_STATUS_FOCUSED : UI_STYLE_STATUS;
	ui_draw_string(ui, win->x, win->y + win->height - 1, status, win, style);
}

static void ui_arrange(Ui *ui, enum UiLayout layout) {
	debug("ui-arrange\n");
	UiTerm *tui = (UiTerm*)ui;
	tui->layout = layout;
	int n = 0, m = !!tui->info[0], x = 0, y = 0;
	for (UiTermWin *win = tui->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			m++;
		else
			n++;
	}
	int max_height = tui->height - m;
	int width = (tui->width / MAX(1, n)) - 1;
	int height = max_height / MAX(1, n);
	for (UiTermWin *win = tui->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			continue;
		n--;
		if (layout == UI_LAYOUT_HORIZONTAL) {
			int h = n ? height : max_height - y;
			ui_window_resize(win, tui->width, h);
			ui_window_move(win, x, y);
			y += h;
		} else {
			int w = n ? width : tui->width - x;
			ui_window_resize(win, w, max_height);
			ui_window_move(win, x, y);
			x += w;
			if (n) {
				Cell *cells = tui->cells;
				for (int i = 0; i < max_height; i++) {
					strcpy(cells[x].data,"│");
					cells[x].style = tui->styles[UI_STYLE_SEPARATOR];
					cells += tui->width;
				}
				x++;
			}
		}
	}

	if (layout == UI_LAYOUT_VERTICAL)
		y = max_height;

	for (UiTermWin *win = tui->windows; win; win = win->next) {
		if (!(win->options & UI_OPTION_ONELINE))
			continue;
		ui_window_resize(win, tui->width, 1);
		ui_window_move(win, 0, y++);
	}
}

static void ui_draw(Ui *ui) {
	debug("ui-draw\n");
	UiTerm *tui = (UiTerm*)ui;
	ui_arrange(ui, tui->layout);
	for (UiTermWin *win = tui->windows; win; win = win->next)
		ui_window_draw((UiWin*)win);
	if (tui->info[0])
		ui_draw_string(tui, 0, tui->height-1, tui->info, NULL, UI_STYLE_INFO);
	ui_term_backend_blit(tui);
}

static void ui_redraw(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	ui_term_backend_clear(tui);
	for (UiTermWin *win = tui->windows; win; win = win->next)
		view_invalidate(win->win->view);
}

static void ui_resize(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	struct winsize ws;
	int width = 80, height = 24;

	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) != -1) {
		if (ws.ws_col > 0)
			width = ws.ws_col;
		if (ws.ws_row > 0)
			height = ws.ws_row;
	}

	width = MIN(width, MAX_WIDTH);
	height = MIN(height, MAX_HEIGHT);
	if (!ui_term_backend_resize(tui, width, height))
		return;

	size_t size = width*height*sizeof(Cell);
	if (size > tui->cells_size) {
		Cell *cells = realloc(tui->cells, size);
		if (!cells)
			return;
		memset((char*)cells+tui->cells_size, 0, size - tui->cells_size);
		tui->cells_size = size;
		tui->cells = cells;
	}
	tui->width = width;
	tui->height = height;
}

static void ui_window_free(UiWin *w) {
	UiTermWin *win = (UiTermWin*)w;
	if (!win)
		return;
	UiTerm *tui = win->ui;
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (tui->windows == win)
		tui->windows = win->next;
	if (tui->selwin == win)
		tui->selwin = NULL;
	win->next = win->prev = NULL;
	tui->ids &= ~(1UL << win->id);
	free(win);
}

static void ui_window_focus(UiWin *w) {
	UiTermWin *new = (UiTermWin*)w;
	UiTermWin *old = new->ui->selwin;
	if (new->options & UI_OPTION_STATUSBAR)
		new->ui->selwin = new;
	if (old)
		view_invalidate(old->win->view);
	view_invalidate(new->win->view);
}

static void ui_window_options_set(UiWin *w, enum UiOption options) {
	UiTermWin *win = (UiTermWin*)w;
	win->options = options;
	if (options & UI_OPTION_ONELINE) {
		/* move the new window to the end of the list */
		UiTerm *tui = win->ui;
		UiTermWin *last = tui->windows;
		while (last->next)
			last = last->next;
		if (last != win) {
			if (win->prev)
				win->prev->next = win->next;
			if (win->next)
				win->next->prev = win->prev;
			if (tui->windows == win)
				tui->windows = win->next;
			last->next = win;
			win->prev = last;
			win->next = NULL;
		}
	}
	ui_draw((Ui*)win->ui);
}

static enum UiOption ui_window_options_get(UiWin *win) {
	return ((UiTermWin*)win)->options;
}

static int ui_window_width(UiWin *win) {
	return ((UiTermWin*)win)->width;
}

static int ui_window_height(UiWin *win) {
	return ((UiTermWin*)win)->height;
}

static void ui_window_swap(UiWin *aw, UiWin *bw) {
	UiTermWin *a = (UiTermWin*)aw;
	UiTermWin *b = (UiTermWin*)bw;
	if (a == b || !a || !b)
		return;
	UiTerm *tui = a->ui;
	UiTermWin *tmp = a->next;
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
	if (tui->windows == a)
		tui->windows = b;
	else if (tui->windows == b)
		tui->windows = a;
	if (tui->selwin == a)
		ui_window_focus(bw);
	else if (tui->selwin == b)
		ui_window_focus(aw);
}

static UiWin *ui_window_new(Ui *ui, Win *w, enum UiOption options) {
	UiTerm *tui = (UiTerm*)ui;
	/* get rightmost zero bit, i.e. highest available id */
	size_t bit = ~tui->ids & (tui->ids + 1);
	size_t id = 0;
	for (size_t tmp = bit; tmp >>= 1; id++);
	if (id >= sizeof(size_t) * 8)
		return NULL;
	size_t styles_size = (id + 1) * UI_STYLE_MAX * sizeof(CellStyle);
	if (styles_size > tui->styles_size) {
		CellStyle *styles = realloc(tui->styles, styles_size);
		if (!styles)
			return NULL;
		tui->styles = styles;
		tui->styles_size = styles_size;
	}
	UiTermWin *win = calloc(1, sizeof(UiTermWin));
	if (!win)
		return NULL;

	win->uiwin = (UiWin) {
		.style_get = ui_window_style_get,
		.status = ui_window_status,
		.options_set = ui_window_options_set,
		.options_get = ui_window_options_get,
		.style_define = ui_style_define,
		.window_width = ui_window_width,
		.window_height = ui_window_height,
	};

	tui->ids |= bit;
	win->id = id;
	win->ui = tui;
	win->win = w;

	CellStyle *styles = &tui->styles[win->id * UI_STYLE_MAX];
	for (int i = 0; i < UI_STYLE_MAX; i++) {
		styles[i] = (CellStyle) {
			.fg = CELL_COLOR_DEFAULT,
			.bg = CELL_COLOR_DEFAULT,
			.attr = CELL_ATTR_NORMAL,
		};
	}

	styles[UI_STYLE_CURSOR].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_CURSOR_PRIMARY].attr |= CELL_ATTR_REVERSE|CELL_ATTR_BLINK;
	styles[UI_STYLE_SELECTION].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_COLOR_COLUMN].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_STATUS].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_STATUS_FOCUSED].attr |= CELL_ATTR_REVERSE|CELL_ATTR_BOLD;
	styles[UI_STYLE_INFO].attr |= CELL_ATTR_BOLD;
	view_ui(w->view, &win->uiwin);

	if (tui->windows)
		tui->windows->prev = win;
	win->next = tui->windows;
	tui->windows = win;

	if (text_size(w->file->text) > UI_LARGE_FILE_SIZE) {
		options |= UI_OPTION_LARGE_FILE;
		options &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
	}

	ui_window_options_set((UiWin*)win, options);

	return &win->uiwin;
}

static void ui_info(Ui *ui, const char *msg, va_list ap) {
	UiTerm *tui = (UiTerm*)ui;
	ui_draw_line(tui, 0, tui->height-1, ' ', UI_STYLE_INFO);
	vsnprintf(tui->info, sizeof(tui->info), msg, ap);
}

static void ui_info_hide(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	if (tui->info[0])
		tui->info[0] = '\0';
}

static TermKey *ui_termkey_new(int fd) {
	TermKey *termkey = termkey_new(fd, UI_TERMKEY_FLAGS);
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
	UiTerm *tui = (UiTerm*)ui;
	return tui->termkey;
}

static void ui_suspend(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	ui_term_backend_suspend(tui);
	kill(0, SIGTSTP);
}

static void ui_resume(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	ui_term_backend_resume(tui);
}

static bool ui_getkey(Ui *ui, TermKeyKey *key) {
	UiTerm *tui = (UiTerm*)ui;
	TermKeyResult ret = termkey_getkey(tui->termkey, key);

	if (ret == TERMKEY_RES_EOF) {
		termkey_destroy(tui->termkey);
		errno = 0;
		if (!(tui->termkey = ui_termkey_reopen(ui, STDIN_FILENO)))
			ui_die_msg(ui, "Failed to re-open stdin as /dev/tty: %s\n", errno != 0 ? strerror(errno) : "");
		return false;
	}

	if (ret == TERMKEY_RES_AGAIN) {
		struct pollfd fd;
		fd.fd = STDIN_FILENO;
		fd.events = POLLIN;
		if (poll(&fd, 1, termkey_get_waittime(tui->termkey)) == 0)
			ret = termkey_getkey_force(tui->termkey, key);
	}

	return ret == TERMKEY_RES_KEY;
}

static void ui_terminal_save(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	ui_term_backend_save(tui);
	termkey_stop(tui->termkey);
}

static void ui_terminal_restore(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	termkey_start(tui->termkey);
	ui_term_backend_restore(tui);
}

static bool ui_init(Ui *ui, Vis *vis) {
	UiTerm *tui = (UiTerm*)ui;
	tui->vis = vis;

	setlocale(LC_CTYPE, "");

	char *term = getenv("TERM");
	if (!term) {
		term = "xterm";
		setenv("TERM", term, 1);
	}

	errno = 0;
	if (!(tui->termkey = ui_termkey_new(STDIN_FILENO))) {
		/* work around libtermkey bug which fails if stdin is /dev/null */
		if (errno == EBADF) {
			errno = 0;
			if (!(tui->termkey = ui_termkey_reopen(ui, STDIN_FILENO)) && errno == ENXIO)
				tui->termkey = termkey_new_abstract(term, UI_TERMKEY_FLAGS);
		}
		if (!tui->termkey)
			goto err;
	}

	if (!ui_term_backend_init(tui, term))
		goto err;
	ui_resize(ui);
	return true;
err:
	ui_die_msg(ui, "Failed to start curses interface: %s\n", errno != 0 ? strerror(errno) : "");
	return false;
}

Ui *ui_term_new(void) {
	size_t styles_size = UI_STYLE_MAX * sizeof(CellStyle);
	CellStyle *styles = calloc(1, styles_size);
	if (!styles)
		return NULL;
	UiTerm *tui = ui_term_backend_new();
	if (!tui) {
		free(styles);
		return NULL;
	}
	tui->styles_size = styles_size;
	tui->styles = styles;
	Ui *ui = (Ui*)tui;
	*ui = (Ui) {
		.init = ui_init,
		.free = ui_term_free,
		.termkey_get = ui_termkey_get,
		.suspend = ui_suspend,
		.resume = ui_resume,
		.resize = ui_resize,
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
		.colors = ui_term_backend_colors,
	};

	return ui;
}

void ui_term_free(Ui *ui) {
	UiTerm *tui = (UiTerm*)ui;
	if (!tui)
		return;
	while (tui->windows)
		ui_window_free((UiWin*)tui->windows);
	ui_term_backend_free(tui);
	if (tui->termkey)
		termkey_destroy(tui->termkey);
	free(tui->cells);
	free(tui->styles);
	free(tui);
}
