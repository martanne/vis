#include "vis.h"
#include "vis-core.h"
#include "text.h"

#ifndef DEBUG_UI
#define DEBUG_UI 0
#endif

#if DEBUG_UI
#define debug(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#define debug(...) do { } while (0)
#endif

#if CONFIG_CURSES
#include "ui-terminal-curses.c"
#else
#include "ui-terminal-vt100.c"
#endif

/* helper macro for handling UiTerm.cells */
#define CELL_AT_POS(UI, X, Y) (((UI)->cells) + (X) + ((Y) * (UI)->width));

#define CELL_STYLE_DEFAULT (CellStyle){.fg = CELL_COLOR_DEFAULT, .bg = CELL_COLOR_DEFAULT, .attr = CELL_ATTR_NORMAL}

static bool is_default_fg(CellColor c) {
	return is_default_color(c);
}

static bool is_default_bg(CellColor c) {
	return is_default_color(c);
}

void ui_die(Ui *tui, const char *msg, va_list ap) {
	ui_terminal_free(tui);
	vfprintf(stderr, msg, ap);
	exit(EXIT_FAILURE);
}

static void ui_die_msg(Ui *ui, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	ui_die(ui, msg, ap);
	va_end(ap);
}

static void ui_window_resize(Win *win, int width, int height) {
	debug("ui-win-resize[%s]: %dx%d\n", win->file->name ? win->file->name : "noname", width, height);
	bool status = win->options & UI_OPTION_STATUSBAR;
	win->width  = width;
	win->height = height;
	view_resize(&win->view, width - win->sidebar_width, status ? height - 1 : height);
}

static void ui_window_move(Win *win, int x, int y) {
	debug("ui-win-move[%s]: (%d, %d)\n", win->file->name ? win->file->name : "noname", x, y);
	win->x = x;
	win->y = y;
}

VIS_INTERNAL bool
vis_ui_color_from_str8(Vis *vis, CellColor *color, str8 s)
{
	if (s.length <= 0)
		return false;

	// TODO: warn if length > 7
	if (s.data[0] == '#' && s.length == 7) {
		s = str8_skip(s, 1);
		IntegerConversion integer = integer_conversion(s, IntegerConversionFlag_ForceHex);
		bool result = integer.result == IntegerConversionResult_Success;
		if (result) {
			uint8_t r = (integer.as.U64 >> 16u) & 0xFFu;
			uint8_t g = (integer.as.U64 >>  8u) & 0xFFu;
			uint8_t b = (integer.as.U64 >>  0u) & 0xFFu;
			*color = color_rgb(vis, r, g, b);
		}
		return result;

	} else if Between(s.data[0], '0', '9') {
		IntegerConversion integer = integer_conversion(s, IntegerConversionFlag_NoAutoHex);
		bool result = integer.result == IntegerConversionResult_Success && integer.as.U64 <= 255;
		if (result)
			*color = color_terminal(integer.as.U64);
		return result;
	}

	static const struct {
		str8      name;
		CellColor color;
	} color_names[] = {
		{str8_comp("black"),   CELL_COLOR_BLACK  },
		{str8_comp("red"),     CELL_COLOR_RED    },
		{str8_comp("green"),   CELL_COLOR_GREEN  },
		{str8_comp("yellow"),  CELL_COLOR_YELLOW },
		{str8_comp("blue"),    CELL_COLOR_BLUE   },
		{str8_comp("magenta"), CELL_COLOR_MAGENTA},
		{str8_comp("cyan"),    CELL_COLOR_CYAN   },
		{str8_comp("white"),   CELL_COLOR_WHITE  },
		{str8_comp("default"), CELL_COLOR_DEFAULT},
	};

	for (uint64_t i = 0; i < countof(color_names); i++) {
		if (str8_case_ignore_equal(color_names[i].name, s)) {
			*color = color_names[i].color;
			return true;
		}
	}

	return false;
}

VIS_INTERNAL bool
vis_ui_style_define(Win *win, int id, str8 style)
{
	if (id >= UI_STYLE_MAX)
		return false;
	if (style.length <= 0)
		return true;

	CellStyle cell_style = CELL_STYLE_DEFAULT;
	while (style.length > 0) {
		style = str8_trim_space(style);

		str8 key, value;
		str8_split(style, &key, &style, ',');
		str8_split(key,   &key, &value, ':');

		if (key.length <= 0)
			continue;

		value = str8_trim_space(value);

		if (str8_case_ignore_equal(key, str8("reverse"))) {
			cell_style.attr |= CELL_ATTR_REVERSE;
		} else if (str8_case_ignore_equal(key, str8("notreverse"))) {
			cell_style.attr &= CELL_ATTR_REVERSE;
		} else if (str8_case_ignore_equal(key, str8("bold"))) {
			cell_style.attr |= CELL_ATTR_BOLD;
		} else if (str8_case_ignore_equal(key, str8("notbold"))) {
			cell_style.attr &= ~CELL_ATTR_BOLD;
		} else if (str8_case_ignore_equal(key, str8("dim"))) {
			cell_style.attr |= CELL_ATTR_DIM;
		} else if (str8_case_ignore_equal(key, str8("notdim"))) {
			cell_style.attr &= ~CELL_ATTR_DIM;
		} else if (str8_case_ignore_equal(key, str8("italics"))) {
			cell_style.attr |= CELL_ATTR_ITALIC;
		} else if (str8_case_ignore_equal(key, str8("notitalics"))) {
			cell_style.attr &= ~CELL_ATTR_ITALIC;
		} else if (str8_case_ignore_equal(key, str8("underlined"))) {
			cell_style.attr |= CELL_ATTR_UNDERLINE;
		} else if (str8_case_ignore_equal(key, str8("notunderlined"))) {
			cell_style.attr &= ~CELL_ATTR_UNDERLINE;
		} else if (str8_case_ignore_equal(key, str8("blink"))) {
			cell_style.attr |= CELL_ATTR_BLINK;
		} else if (str8_case_ignore_equal(key, str8("notblink"))) {
			cell_style.attr &= ~CELL_ATTR_BLINK;
		} else if (str8_case_ignore_equal(key, str8("fore"))) {
			vis_ui_color_from_str8(win->vis, &cell_style.fg, value);
		} else if (str8_case_ignore_equal(key, str8("back"))) {
			vis_ui_color_from_str8(win->vis, &cell_style.bg, value);
		}
	}
	win->vis->ui.styles[win->id * UI_STYLE_MAX + id] = cell_style;

	return true;
}

static void ui_draw_line(Ui *tui, int x, int y, char c, enum UiStyle style_id) {
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

static void ui_draw_string(Ui *tui, int x, int y, const char *str, int win_id, enum UiStyle style_id) {
	debug("draw-string: [%d][%d]\n", y, x);
	if (x < 0 || x >= tui->width || y < 0 || y >= tui->height)
		return;

	/* NOTE: the style that style_id refers to may contain unset values; we need to properly
	 * clear the cell first then go through ui_window_style_set to get the correct style */
	CellStyle default_style = tui->styles[UI_STYLE_MAX * win_id + UI_STYLE_DEFAULT];
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
		cells[x].style = default_style;
		ui_window_style_set(tui, win_id, cells + x++, style_id, false);
	}
}

VIS_INTERNAL uint8_t
u32_count_digits(uint32_t a)
{
	// NOTE(rnp): guaranteed branchless, fixed cost regardless of input
	uint8_t result = (a >= 1000000000) +
	                 (a >= 100000000)  +
	                 (a >= 10000000)   +
	                 (a >= 1000000)    +
	                 (a >= 100000)     +
	                 (a >= 10000)      +
	                 (a >= 1000)       +
	                 (a >= 100)        +
	                 (a >= 10)         +
	                 1;
	return result;
}

static void ui_window_draw(Win *win) {
	Ui *ui = &win->vis->ui;
	View *view = &win->view;
	const Line *line = win->view.topline;

	bool status  = win->options & UI_OPTION_STATUSBAR;
	bool nu      = win->options & UI_OPTION_LINE_NUMBERS_ABSOLUTE;
	bool rnu     = win->options & UI_OPTION_LINE_NUMBERS_RELATIVE;
	bool sidebar = nu || rnu;

	int width = win->width, height = win->height;

	int sidebar_width = 0;
	if (sidebar) {
		sidebar_width = u32_count_digits(view->lastline->lineno) + 1;
		sidebar_width = MAX(sidebar_width, win->min_sidebar_width);
	}
	if (sidebar_width != win->sidebar_width) {
		view_resize(view, width - sidebar_width, status ? height - 1 : height);
		win->sidebar_width = sidebar_width;
	}
	vis_window_draw(win);

	Selection *sel = view_selections_primary_get(view);
	size_t prev_lineno = 0, cursor_lineno = sel->line->lineno;
	char buf[(sizeof(size_t) * CHAR_BIT + 2) / 3 + 1 + 1];
	int x = win->x, y = win->y;
	int view_width = view->width;
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
			ui_draw_string(ui, x, y, buf, win->id,
				       (l->lineno == cursor_lineno) ? UI_STYLE_LINENUMBER_CURSOR :
				                                      UI_STYLE_LINENUMBER);
			prev_lineno = l->lineno;
		}
		debug("draw-window: [%d][%d] ... cells[%d][%d]\n", y, x+sidebar_width, y, view_width);
		memcpy(cells + x + sidebar_width, l->cells, sizeof(Cell) * view_width);
		cells += ui->width;
	}
}

void ui_window_style_set(Ui *tui, int win_id, Cell *cell, enum UiStyle id, bool keep_non_default) {
	CellStyle set = tui->styles[win_id * UI_STYLE_MAX + id];

	if (id != UI_STYLE_DEFAULT) {
		if (keep_non_default) {
			CellStyle default_style = tui->styles[win_id * UI_STYLE_MAX + UI_STYLE_DEFAULT];
			if (!cell_color_equal(cell->style.fg, default_style.fg))
				set.fg = cell->style.fg;
			if (!cell_color_equal(cell->style.bg, default_style.bg))
				set.bg = cell->style.bg;
		}
		set.fg = is_default_fg(set.fg)? cell->style.fg : set.fg;
		set.bg = is_default_bg(set.bg)? cell->style.bg : set.bg;
		set.attr = cell->style.attr | set.attr;
	}

	cell->style = set;
}

bool ui_window_style_set_pos(Win *win, int x, int y, enum UiStyle id, bool keep_non_default) {
	Ui *tui = &win->vis->ui;
	if (x < 0 || y < 0 || y >= win->height || x >= win->width) {
		return false;
	}
	Cell *cell = CELL_AT_POS(tui, win->x + x, win->y + y)
	ui_window_style_set(tui, win->id, cell, id, keep_non_default);
	return true;
}

VIS_INTERNAL void
ui_window_status(Vis *vis, Win *win, const char *status)
{
	enum UiStyle style = vis->win == win ? UI_STYLE_STATUS_FOCUSED : UI_STYLE_STATUS;
	ui_draw_string(&vis->ui, win->x, win->y + win->height - 1, status, win->id, style);
}

VIS_INTERNAL void
ui_arrange(Vis *vis, enum UiLayout layout)
{
	debug("ui-arrange\n");
	Ui *tui = &vis->ui;
	tui->layout = layout;
	int n = 0, m = !!tui->info[0], x = 0, y = 0;
	for (Win *win = vis->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			m++;
		else
			n++;
	}
	int max_height = tui->height - m;
	int width = (tui->width / MAX(1, n)) - 1;
	int height = max_height / MAX(1, n);
	for (Win *win = vis->windows; win; win = win->next) {
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

	for (Win *win = vis->windows; win; win = win->next) {
		if (!(win->options & UI_OPTION_ONELINE))
			continue;
		ui_window_resize(win, tui->width, 1);
		ui_window_move(win, 0, y++);
	}
}

VIS_INTERNAL void
ui_draw(Vis *vis)
{
	debug("ui-draw\n");
	Ui *tui = &vis->ui;
	ui_arrange(vis, vis->ui.layout);
	for (Win *win = vis->windows; win; win = win->next)
		ui_window_draw(win);

	/* determine primary cursor's position */
	if (vis->win) {
		Win  *win  = vis->win;
		View *view = &win->view;
		view_coord_get(view, view_cursor_get(view), 0, &tui->cur_row, &tui->cur_col);
		tui->cur_col += win->sidebar_width + win->x;
		tui->cur_row += win->y;
	}
	switch (vis->prompt_state) {
	case PROMPTSTATE_NONE:
	case PROMPTSTATE_ONELINE:
		break;
	case PROMPTSTATE_COMMAND:
		tui->cur_row = vis->ui.height;
	}
	if (tui->info[0])
		ui_draw_string(tui, 0, tui->height-1, tui->info, 0, UI_STYLE_INFO);
	vis_event_emit(vis, VIS_EVENT_UI_DRAW);
	ui_term_backend_blit(tui);
}

void ui_resize(Ui *tui) {
	struct winsize ws;
	int width = 80, height = 24;

	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) != -1) {
		if (ws.ws_col > 0)
			width = ws.ws_col;
		if (ws.ws_row > 0)
			height = ws.ws_row;
	}

	width  = MIN(width,  UI_MAX_WIDTH);
	height = MIN(height, UI_MAX_HEIGHT);
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

VIS_INTERNAL void
ui_window_options_set(Win *win, enum UiOption options)
{
	Vis *vis = win->vis;
	win->options = options;
	if (options & UI_OPTION_ONELINE) {
		/* move the new window to the end of the list */
		Win *last = vis->windows;
		while (last->next)
			last = last->next;
		if (last != win) {
			if (win->prev)
				win->prev->next = win->next;
			if (win->next)
				win->next->prev = win->prev;
			if (vis->windows == win)
				vis->windows = win->next;
			last->next = win;
			win->prev  = last;
			win->next  = 0;
		}
	}
	ui_draw(vis);
}

bool ui_window_init(Ui *tui, Win *w, enum UiOption options) {
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

	tui->ids |= bit;
	w->id = id;

	CellStyle *styles = &tui->styles[w->id * UI_STYLE_MAX];
	for (int i = 0; i < UI_STYLE_MAX; i++) {
		styles[i] = CELL_STYLE_DEFAULT;
	}

	styles[UI_STYLE_CURSOR].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_CURSOR_PRIMARY].attr |= CELL_ATTR_REVERSE|CELL_ATTR_BLINK;
	styles[UI_STYLE_SELECTION].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_COLOR_COLUMN].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_STATUS].attr |= CELL_ATTR_REVERSE;
	styles[UI_STYLE_STATUS_FOCUSED].attr |= CELL_ATTR_REVERSE|CELL_ATTR_BOLD;
	styles[UI_STYLE_INFO].attr |= CELL_ATTR_BOLD;

	if (text_size(w->file->text) > UI_LARGE_FILE_SIZE) {
		options |= UI_OPTION_LARGE_FILE;
		options &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
	}

	win_options_set(w, options);

	return true;
}

void ui_info_show(Ui *tui, const char *msg, va_list ap) {
	ui_draw_line(tui, 0, tui->height-1, ' ', UI_STYLE_INFO);
	vsnprintf(tui->info, sizeof(tui->info), msg, ap);
}

void ui_info_hide(Ui *tui) {
	if (tui->info[0])
		tui->info[0] = '\0';
}

VIS_INTERNAL bool
vis_ui_termkey_init(TermKey *tk, int fd, char *term)
{
	bool result = termkey_init_from_fd(tk, fd, UI_TERMKEY_FLAGS, term);
	if (result) termkey_set_canonflags(tk, TERMKEY_CANON_DELBS);
	return result;
}

VIS_INTERNAL bool
vis_ui_termkey_reopen(Ui *ui, int fd, char *term)
{
	bool result = false;
	int tty = open("/dev/tty", O_RDWR);
	if (tty != -1) {
		if (tty == fd || dup2(tty, fd) != -1)
			result = vis_ui_termkey_init(&ui->termkey, fd, term);
		close(tty);
	}
	return result;
}

void ui_terminal_suspend(Ui *tui) {
	ui_term_backend_suspend(tui);
	kill(0, SIGTSTP);
}

VIS_INTERNAL bool
vis_ui_getkey(Vis *vis, TermKeyKey *key)
{
	TermKeyResult ret = termkey_getkey(&vis->ui.termkey, key);

	if (ret == TERMKEY_RES_EOF) {
		termkey_destroy(&vis->ui.termkey);
		errno = 0;
		if (!vis_ui_termkey_reopen(&vis->ui, STDIN_FILENO, (char *)vis->ui.term.data))
			ui_die_msg(&vis->ui, "Failed to re-open stdin as /dev/tty: %s\n", errno != 0 ? strerror(errno) : "");
		return false;
	}

	if (ret == TERMKEY_RES_AGAIN) {
		struct pollfd fd;
		fd.fd = STDIN_FILENO;
		fd.events = POLLIN;
		if (poll(&fd, 1, vis->escape_delay) == 0)
			ret = termkey_getkey_force(&vis->ui.termkey, key);
	}

	return ret == TERMKEY_RES_KEY;
}

void ui_terminal_save(Ui *tui, bool fscr) {
	ui_term_backend_save(tui, fscr);
	termkey_stop(&tui->termkey);
}

void ui_terminal_restore(Ui *tui) {
	termkey_start(&tui->termkey);
	ui_term_backend_restore(tui);
}

VIS_INTERNAL void
ui_terminal_free(Ui *tui)
{
	ui_term_backend_free(tui);
	termkey_destroy(&tui->termkey);
	free(tui->cells);
	free(tui->styles);
	free(tui->term.data);
}

VIS_INTERNAL bool
ui_init(Ui *tui)
{
	setlocale(LC_CTYPE, "");

	char *term = getenv("TERM");
	if (!term) term = "xterm";
	tui->term = str8_from_c_str(strdup(term));

	errno = 0;
	if (!vis_ui_termkey_init(&tui->termkey, STDIN_FILENO, term)) {
		/* work around libtermkey bug which fails if stdin is /dev/null */
		if (errno == EBADF) {
			errno = 0;
			if (!vis_ui_termkey_reopen(tui, STDIN_FILENO, term) && errno == ENXIO)
				if (!termkey_init_abstract(&tui->termkey, term, UI_TERMKEY_FLAGS))
					ui_die_msg(tui, "Failed to start termkey\n");
		}
	}

	tui->styles_size = UI_STYLE_MAX * sizeof(CellStyle);
	tui->styles      = calloc(1, tui->styles_size);
	tui->doupdate    = true;

	bool result = tui->term.length && tui->styles && ui_backend_init(tui, term);
	if (result) ui_resize(tui);
	else        ui_terminal_free(tui);
	return result;
}
