#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <sys/ioctl.h>

#include "ui.h"
#include "ui-curses.h"
#include "util.h"

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

#ifdef PDCURSES
int ESCDELAY;
#endif
#ifndef NCURSES_REENTRANT
# define set_escdelay(d) (ESCDELAY = (d))
#endif

#define CONTROL(k) ((k)&0x1F)

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

typedef struct UiCursesWin UiCursesWin;

typedef struct {
	Ui ui;                    /* generic ui interface, has to be the first struct member */
	Editor *ed;               /* editor instance to which this ui belongs */
	UiCursesWin *windows;     /* all windows managed by this ui */
	UiCursesWin *selwin;      /* the currently selected layout */
	char prompt_title[255];   /* prompt_title[0] == '\0' if prompt isn't shown */
	UiCursesWin *prompt_win;  /* like a normal window but without a status bar */
	char info[255];           /* info message displayed at the bottom of the screen */
	int width, height;        /* terminal dimensions available for all windows */
	enum UiLayout layout;     /* whether windows are displayed horizontally or vertically */
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
};

static volatile sig_atomic_t need_resize; /* TODO */

static void sigwinch_handler(int sig) {
	need_resize = true;
}

static unsigned int color_hash(short fg, short bg) {
	if (fg == -1)
		fg = COLORS;
	if (bg == -1)
		bg = COLORS + 1;
	return fg * (COLORS + 2) + bg;
}

static short color_get(short fg, short bg) {
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

	unsigned int index = color_hash(fg, bg);
	if (color2palette[index] == 0) {
		short oldfg, oldbg;
		if (++color_pair_current >= color_pairs_max)
			color_pair_current = 1;
		pair_content(color_pair_current, &oldfg, &oldbg);
		unsigned int old_index = color_hash(oldfg, oldbg);
		if (init_pair(color_pair_current, fg, bg) == OK) {
			color2palette[old_index] = 0;
			color2palette[index] = color_pair_current;
		}
	}

	return color2palette[index];
}

static void ui_window_resize(UiCursesWin *win, int width, int height) {
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
	win->x = x;
	win->y = y;
	mvwin(win->win, y, x + win->sidebar_width);
	if (win->winside)
		mvwin(win->winside, y, x);
	if (win->winstatus)
		mvwin(win->winstatus, y + win->height - 1, x);
}

static void ui_window_draw_status(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!win->winstatus)
		return;
	UiCurses *uic = win->ui;
	Editor *vis = uic->ed;
	bool focused = uic->selwin == win;
	const char *filename = win->file->name;
	CursorPos pos = view_cursor_getpos(win->view);
	wattrset(win->winstatus, focused ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win->winstatus, 0, 0, ' ', win->width);
	mvwprintw(win->winstatus, 0, 0, "%s %s %s %s",
	          vis->mode->name && vis->mode->name[0] == '-' ? vis->mode->name : "",
	          filename ? filename : "[No Name]",
	          text_modified(win->file->text) ? "[+]" : "",
	          vis->recording ? "recording": "");
	char buf[win->width + 1];
	int len = snprintf(buf, win->width, "%zd, %zd", pos.line, pos.col);
	if (len > 0) {
		buf[len] = '\0';
		mvwaddstr(win->winstatus, 0, win->width - len - 1, buf);
	}
}

static void ui_window_draw(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (win->winstatus)
		ui_window_draw_status((UiWin*)win);
	view_draw(win->view);
}

static void ui_window_reload(UiWin *w, File *file) {
	UiCursesWin *win = (UiCursesWin*)w;
	win->file = file;
	win->sidebar_width = 0;
	view_reload(win->view, file->text);
	ui_window_draw(w);
}

static void ui_window_draw_sidebar(UiCursesWin *win, const Line *line) {
	if (!win->winside || !line)
		return;
	int sidebar_width = snprintf(NULL, 0, "%zd", line->lineno + win->height - 2) + 1;
	if (win->sidebar_width != sidebar_width) {
		win->sidebar_width = sidebar_width;
		ui_window_resize(win, win->width, win->height);
		ui_window_move(win, win->x, win->y);
	} else {
		int i = 0;
		size_t prev_lineno = 0;
		size_t cursor_lineno = view_cursor_getpos(win->view).line;
		werase(win->winside);
		for (const Line *l = line; l; l = l->next, i++) {
			if (l->lineno && l->lineno != prev_lineno) {
				if (win->options & UI_OPTION_LINE_NUMBERS_ABSOLUTE) {
					mvwprintw(win->winside, i, 0, "%*u", sidebar_width-1, l->lineno);
				} else if (win->options & UI_OPTION_LINE_NUMBERS_RELATIVE) {
					size_t rel = l->lineno > cursor_lineno ?
					             l->lineno - cursor_lineno :
					             cursor_lineno - l->lineno;
					mvwprintw(win->winside, i, 0, "%*u", sidebar_width-1, rel);
				}
			}
			prev_lineno = l->lineno;
		}
		mvwvline(win->winside, 0, sidebar_width-1, ACS_VLINE, win->height-1);
	}
}

static void ui_window_update(UiCursesWin *win) {
	if (win->winstatus)
		wnoutrefresh(win->winstatus);
	if (win->winside)
		wnoutrefresh(win->winside);
	wnoutrefresh(win->win);
}

static void arrange(Ui *ui, enum UiLayout layout) {
	UiCurses *uic = (UiCurses*)ui;
	uic->layout = layout;
	int n = 0, x = 0, y = 0;
	for (UiCursesWin *win = uic->windows; win; win = win->next)
		n++;
	int max_height = uic->height - !!(uic->prompt_title[0] || uic->info[0]);
	int width = (uic->width / MAX(1, n)) - 1;
	int height = max_height / MAX(1, n);
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (layout == UI_LAYOUT_HORIZONTAL) {
			ui_window_resize(win, uic->width, win->next ? height : max_height - y);
			ui_window_move(win, x, y);
			y += height;
		} else {
			ui_window_resize(win, win->next ? width : uic->width - x, max_height);
			ui_window_move(win, x, y);
			x += width;
			if (win->next)
				mvvline(0, x++, ACS_VLINE, max_height);
		}
	}
}

static void draw(Ui *ui) { 
	UiCurses *uic = (UiCurses*)ui;
	erase();
	arrange(ui, uic->layout);
	
	for (UiCursesWin *win = uic->windows; win; win = win->next)
		ui_window_draw((UiWin*)win);

	if (uic->info[0]) {
	        attrset(A_BOLD);
        	mvaddstr(uic->height-1, 0, uic->info);
	}

	if (uic->prompt_title[0]) {
	        attrset(A_NORMAL);
        	mvaddstr(uic->height-1, 0, uic->prompt_title);
		ui_window_draw((UiWin*)uic->prompt_win);
	}

	wnoutrefresh(stdscr);
}

static void ui_resize_to(Ui *ui, int width, int height) {
	UiCurses *uic = (UiCurses*)ui;
	uic->width = width;
	uic->height = height;
	if (uic->prompt_title[0]) {
		size_t title_width = strlen(uic->prompt_title);
		ui_window_resize(uic->prompt_win, width - title_width, 1);
		ui_window_move(uic->prompt_win, title_width, height-1);
	}
	draw(ui);
}

static void ui_resize(Ui *ui) {
	struct winsize ws;
	int width, height;

	if (ioctl(0, TIOCGWINSZ, &ws) == -1) {
		getmaxyx(stdscr, height, width);
	} else {
		width = ws.ws_col;
		height = ws.ws_row;
	}

	resizeterm(height, width);
	wresize(stdscr, height, width);
	ui_resize_to(ui, width, height);
}

static void update(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (need_resize) {
		ui_resize(ui);
		need_resize = false;
	}
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (win != uic->selwin)
			ui_window_update(win);
	}

	if (uic->selwin)
		ui_window_update(uic->selwin);
	if (uic->prompt_title[0]) {
		wnoutrefresh(uic->prompt_win->win);
		ui_window_update(uic->prompt_win);
	}
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

static void ui_window_draw_text(UiWin *w, const Line *line) {
	UiCursesWin *win = (UiCursesWin*)w;
	wmove(win->win, 0, 0);
	int width = view_width_get(win->view);
	for (const Line *l = line; l; l = l->next) {
		for (int x = 0; x < width; x++) {
			int attr = l->cells[x].attr;
			if (l->cells[x].cursor && (win->ui->selwin == win || win->ui->prompt_win == win))
				attr = A_NORMAL | A_REVERSE;
			if (l->cells[x].selected)
				attr |= A_REVERSE;
			wattrset(win->win, attr);
			waddstr(win->win, l->cells[x].data);
		}
	}
	wclrtobot(win->win);

	ui_window_draw_sidebar(win, line);
}

static void ui_window_focus(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	UiCursesWin *oldsel = win->ui->selwin;
	win->ui->selwin = win;
	if (oldsel)
		ui_window_draw((UiWin*)oldsel);
	ui_window_draw(w);
}

static void ui_window_options(UiWin *w, enum UiOption options) {
	UiCursesWin *win = (UiCursesWin*)w;
	win->options = options;
	switch (options) {
	case UI_OPTION_LINE_NUMBERS_NONE:
		if (win->winside) {
			delwin(win->winside);
			win->winside = NULL;
			win->sidebar_width = 0;
		}
		break;
	case UI_OPTION_LINE_NUMBERS_ABSOLUTE:
	case UI_OPTION_LINE_NUMBERS_RELATIVE:
		if (!win->winside)
			win->winside = newwin(1, 1, 1, 1);
		break;
	}
	ui_window_draw(w);
}

static UiWin *ui_window_new(Ui *ui, View *view, File *file) {
	UiCurses *uic = (UiCurses*)ui;
	UiCursesWin *win = calloc(1, sizeof(UiCursesWin));
	if (!win)
		return NULL;

	win->uiwin = (UiWin) {
		.draw = ui_window_draw,
		.draw_status = ui_window_draw_status,
		.draw_text = ui_window_draw_text,
		.options = ui_window_options,
		.reload = ui_window_reload,
	};

	if (!(win->win = newwin(0, 0, 0, 0)) || !(win->winstatus = newwin(1, 0, 0, 0))) {
		ui_window_free((UiWin*)win);
		return NULL;
	}

	win->ui = uic;
	win->view = view;
	win->file = file;
	view_ui(view, &win->uiwin);
	
	if (uic->windows)
		uic->windows->prev = win;
	win->next = uic->windows;
	uic->windows = win;

	return &win->uiwin;
}

static void info(Ui *ui, const char *msg, va_list ap) {
	UiCurses *uic = (UiCurses*)ui;
	vsnprintf(uic->info, sizeof(uic->info), msg, ap);
	draw(ui);
}

static void info_hide(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (uic->info[0]) { 
		uic->info[0] = '\0';
		draw(ui);
	}
}

static UiWin *prompt_new(Ui *ui, View *view, File *file) {
	UiCurses *uic = (UiCurses*)ui;
	if (uic->prompt_win)
		return (UiWin*)uic->prompt_win;
	UiWin *uiwin = ui_window_new(ui, view, file);
	UiCursesWin *win = (UiCursesWin*)uiwin;
	if (!win)
		return NULL;
	uic->windows = win->next;
	if (uic->windows)
		uic->windows->prev = NULL;
	if (win->winstatus)
		delwin(win->winstatus);
	if (win->winside)
		delwin(win->winside);
	win->winstatus = NULL;
	win->winside = NULL;
	uic->prompt_win = win;
	return uiwin;
}

static void prompt(Ui *ui, const char *title, const char *data) {
	UiCurses *uic = (UiCurses*)ui;
	if (uic->prompt_title[0])
		return;
	size_t len = strlen(data);
	Text *text = uic->prompt_win->file->text;
	strncpy(uic->prompt_title, title, sizeof(uic->prompt_title)-1);
	while (text_undo(text) != EPOS);
	text_insert(text, 0, data, len);
	view_cursor_to(uic->prompt_win->view, 0);
	ui_resize_to(ui, uic->width, uic->height);
	view_cursor_to(uic->prompt_win->view, len);
}

static char *prompt_input(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (!uic->prompt_win)
		return NULL;
	Text *text = uic->prompt_win->file->text;
	char *buf = malloc(text_size(text) + 1);
	if (!buf)
		return NULL;
	size_t len = text_bytes_get(text, 0, text_size(text), buf);
	buf[len] = '\0';
	return buf;
}

static void prompt_hide(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	uic->prompt_title[0] = '\0';
	ui_resize_to(ui, uic->width, uic->height);
}

static bool ui_init(Ui *ui, Editor *ed) {
	UiCurses *uic = (UiCurses*)ui;
	uic->ed = ed;
	return true;
}

static void ui_suspend(Ui *ui) {
	endwin();
	raise(SIGSTOP);
}

static bool ui_haskey(Ui *ui) {
	nodelay(stdscr, TRUE);
	int c = getch();
	if (c != ERR)
		ungetch(c);
	nodelay(stdscr, FALSE);
	return c != ERR;
}

static Key ui_getkey(Ui *ui) {
	Key key = { .str = "", .code = 0 };
	int keycode = getch(), cur = 0;
	if (keycode == ERR)
		return key;

	if (keycode >= KEY_MIN) {
		key.code = keycode;
	} else {
		key.str[cur++] = keycode;
		int len = 1;
		unsigned char keychar = keycode;
		if (ISASCII(keychar)) len = 1;
		else if (keychar == 0x1B || keychar >= 0xFC) len = 6;
		else if (keychar >= 0xF8) len = 5;
		else if (keychar >= 0xF0) len = 4;
		else if (keychar >= 0xE0) len = 3;
		else if (keychar >= 0xC0) len = 2;
		len = MIN(len, LENGTH(key.str));

		if (cur < len) {
			nodelay(stdscr, TRUE);
			for (int t; cur < len && (t = getch()) != ERR; cur++)
				key.str[cur] = t;
			nodelay(stdscr, FALSE);
		}

		if (len == 1) {
			switch (key.str[0]) {
			case 127:
			case CONTROL('H'):
			case CONTROL('B'):
				key.code = KEY_BACKSPACE;
				key.str[0] = '\0';
				break;
			}
		}
	}

	return key;
}

static void ui_terminal_save(Ui *ui) {
	reset_shell_mode();
}

static void ui_terminal_restore(Ui *ui) {
	reset_prog_mode();
	wclear(stdscr);
}

Ui *ui_curses_new(Color *colors) {
	setlocale(LC_CTYPE, "");
	if (!getenv("ESCDELAY"))
		set_escdelay(50);
	char *term = getenv("TERM");
	if (!term)
		term = "xterm";
	if (!newterm(term, stderr, stdin))
		return NULL;
	start_color();
	use_default_colors();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	curs_set(0);
	/* needed because we use getch() which implicitly calls refresh() which
	   would clear the screen (overwrite it with an empty / unused stdscr */
	refresh();

	UiCurses *uic = calloc(1, sizeof(UiCurses));
	Ui *ui = (Ui*)uic;
	if (!uic)
		return NULL;

	*ui = (Ui) {
		.init = ui_init,
		.free = ui_curses_free,
		.suspend = ui_suspend,
		.resume = ui_resize,
		.resize = ui_resize,
		.update = update,
		.window_new = ui_window_new,
		.window_free = ui_window_free,
		.window_focus = ui_window_focus,
		.prompt_new = prompt_new,
		.prompt = prompt,
		.prompt_input = prompt_input,
		.prompt_hide = prompt_hide,
		.draw = draw,
		.arrange = arrange,
		.info = info,
		.info_hide = info_hide,
		.haskey = ui_haskey,
		.getkey = ui_getkey,
		.terminal_save = ui_terminal_save,
		.terminal_restore = ui_terminal_restore,
	};

	for (Color *color = colors; color && color->fg; color++) {
		if (color->attr == 0)
			color->attr = A_NORMAL;
		color->attr |= COLOR_PAIR(color_get(color->fg, color->bg));
	}

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);
	sigaction(SIGCONT, &sa, NULL);
	
	ui_resize(ui);

	return ui;
}

void ui_curses_free(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (!uic)
		return;
	ui_window_free((UiWin*)uic->prompt_win);
	while (uic->windows)
		ui_window_free((UiWin*)uic->windows);
	endwin();
	free(uic);
}

