#include "vis.h"
#include "vis-core.h"
#include "text.h"

typedef struct {
	union {
		struct {u8 r, g, b;} rgb;
		u16 index;
	} color;
	bool indexed;
} VisTerminalStyle;

// TODO(rnp): if we remove curses backend we should do a single mapping
// for both front and back buffers.
VIS_INTERNAL bool
vis_cell_buffer_resize(VisCellBuffer *cb, u32 width, u32 height)
{
	u64 page_size = sysconf(_SC_PAGE_SIZE);
	u64 size      = round_up_to(width * height * sizeof(Cell), page_size);

	// NOTE(rnp): we always ask for a new address range here which allows for both
	// growing and shrinking. it also means that this function always 0s the memory
	void *memory = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	bool  result = memory != MAP_FAILED;
	if (result) {
		if (cb->cells) munmap(cb->cells, cb->size);
		cb->cells = memory;
		cb->size  = size;
	}
	return result;
}

#if CONFIG_CURSES
#include "ui-terminal-curses.c"
#else
#include "ui-terminal-vt100.c"
#endif

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
	bool status = win->options & UI_OPTION_STATUSBAR;
	win->width  = width;
	win->height = height;
	view_resize(&win->view, width - win->sidebar_width, status ? height - 1 : height);
}

static void ui_window_move(Win *win, int x, int y) {
	win->x = x;
	win->y = y;
}

VIS_INTERNAL bool
vis_terminal_style_from_str8(Vis *vis, VisTerminalStyle *out, str8 s)
{
	bool result = false;
	if (s.length > 0) {
		// TODO: warn if length > 7
		if (s.data[0] == '#' && s.length == 7) {
			s = str8_skip(s, 1);
			IntegerConversion integer = integer_conversion(s, IntegerConversionFlag_ForceHex);
			result = integer.result == IntegerConversionResult_Success;
			if (result) {
				u8 r = (integer.as.U64 >> 16u) & 0xFFu;
				u8 g = (integer.as.U64 >>  8u) & 0xFFu;
				u8 b = (integer.as.U64 >>  0u) & 0xFFu;
				*out = vis_terminal_style_rgb(vis, r, g, b);
			}

		} else if Between(s.data[0], '0', '9') {
			IntegerConversion integer = integer_conversion(s, IntegerConversionFlag_NoAutoHex);
			result = integer.result == IntegerConversionResult_Success && integer.as.U64 <= 255;
			if (result) *out = vis_terminal_style_indexed(integer.as.U64);
		} else {
			static const str8 color_names[] = {
				str8_comp("black"),
				str8_comp("red"),
				str8_comp("green"),
				str8_comp("yellow"),
				str8_comp("blue"),
				str8_comp("magenta"),
				str8_comp("cyan"),
				str8_comp("white"),
			};

			out->indexed = true;
			for (u64 i = 0; i < countof(color_names); i++) {
				if (str8_case_ignore_equal(color_names[i], s)) {
					out->color.index = i;
					result = true;
					break;
				}
			}
		}
	}
	return result;
}

VIS_INTERNAL uint16_t
vis_ui_style_push(Vis *vis)
{
	uint16_t result = -1;
	if (vis->ui.style_count < countof(vis->ui.styles))
		result = vis->ui.style_count++;
	return result;
}

VIS_INTERNAL bool
vis_ui_style_define(Vis *vis, u16 style_id, str8 style)
{
	bool result = style_id < vis->ui.style_count;
	if (result) {
		VisCellStyle cell_style = {0};
		if (style_id == UI_STYLE_DEFAULT) {
			// NOTE(rnp): DEFAULT is special and must always be set
			cell_style = vis_ui_backend_style_default(&vis->ui);
		}

		while (style.length > 0) {
			style = str8_trim_space(style);

			str8 key, value;
			str8_split(style, &key, &style, ',');
			str8_split(key,   &key, &value, ':');

			if (key.length <= 0)
				continue;

			value = str8_trim_space(value);

			if (str8_case_ignore_equal(key, str8("reverse"))) {
				cell_style.attributes |= VisCellAttribute_Reverse;
			} else if (str8_case_ignore_equal(key, str8("bold"))) {
				cell_style.attributes |= VisCellAttribute_Bold;
			} else if (str8_case_ignore_equal(key, str8("dim"))) {
				cell_style.attributes |= VisCellAttribute_Dim;
			} else if (str8_case_ignore_equal(key, str8("italics"))) {
				cell_style.attributes |= VisCellAttribute_Italic;
			} else if (str8_case_ignore_equal(key, str8("underlined"))) {
				cell_style.attributes |= VisCellAttribute_Underline;
			} else if (str8_case_ignore_equal(key, str8("blink"))) {
				cell_style.attributes |= VisCellAttribute_Blink;
			} else if (str8_case_ignore_equal(key, str8("keep_attribute"))) {
				cell_style.properties |= VisCellProperty_KeepAttribute;

			} else if (str8_case_ignore_equal(key, str8("fore"))) {
				VisTerminalStyle fg = {0};
				if (vis_terminal_style_from_str8(vis, &fg, value)) {
					cell_style.properties |= VisCellProperty_FGSet;
					if (fg.indexed) {
						cell_style.properties |= VisCellProperty_IndexedFG;
						VisCellStyleFGIndexSet(&cell_style, fg.color.index);
					} else {
						cell_style.properties &= ~VisCellProperty_IndexedFG;
						cell_style.fg_r = fg.color.rgb.r;
						cell_style.fg_g = fg.color.rgb.g;
						cell_style.fg_b = fg.color.rgb.b;
					}
				}

			} else if (str8_case_ignore_equal(key, str8("back"))) {
				VisTerminalStyle bg = {0};
				if (vis_terminal_style_from_str8(vis, &bg, value)) {
					cell_style.properties |= VisCellProperty_BGSet;
					if (bg.indexed) {
						cell_style.properties |= VisCellProperty_IndexedBG;
						VisCellStyleBGIndexSet(&cell_style, bg.color.index);
					} else {
						cell_style.properties &= ~VisCellProperty_IndexedBG;
						cell_style.bg_r = bg.color.rgb.r;
						cell_style.bg_g = bg.color.rgb.g;
						cell_style.bg_b = bg.color.rgb.b;
					}
				}
			}
		}

		vis->ui.styles[style_id] = cell_style;
	}
	return result;
}

VIS_INTERNAL void
vis_cell_style_copy_fg(VisCellStyle *dst, VisCellStyle src)
{
	dst->fg_r = src.fg_r;
	dst->fg_g = src.fg_g;
	dst->fg_b = src.fg_b;
	dst->properties &= ~(VisCellProperty_FGSet|VisCellProperty_IndexedFG);
	dst->properties |= (src.properties & (VisCellProperty_FGSet|VisCellProperty_IndexedFG));
}

VIS_INTERNAL void
vis_cell_style_copy_bg(VisCellStyle *dst, VisCellStyle src)
{
	dst->bg_r = src.bg_r;
	dst->bg_g = src.bg_g;
	dst->bg_b = src.bg_b;
	dst->properties &= ~(VisCellProperty_BGSet|VisCellProperty_IndexedBG);
	dst->properties |= (src.properties & (VisCellProperty_BGSet|VisCellProperty_IndexedBG));
}

VIS_INTERNAL void
vis_ui_window_style_set(Ui *tui, Cell *cell, u16 style_id)
{
	assert(style_id < tui->style_count);

	VisCellStyle set        = tui->styles[style_id];
	VisCellStyle cell_style = cell->style;
	if ((set.properties & VisCellProperty_FGSet) == 0)
		vis_cell_style_copy_fg(&set, cell_style);
	if ((set.properties & VisCellProperty_BGSet) == 0)
		vis_cell_style_copy_bg(&set, cell_style);
	if (set.properties & VisCellProperty_KeepAttribute)
		set.attributes |= cell_style.attributes;

	cell->style = set;
}

VIS_INTERNAL bool
vis_ui_window_style_set_pos(Win *win, int x, int y, u16 style_id)
{
	Ui *ui = &win->vis->ui;
	bool result = Between(x, 0, win->width - 1) && Between(y, 0, win->height - 1);
	if (result) {
		x += win->x;
		y += win->y;
		Cell *cell = ui->cell_buffer.cells + ui->width * y + x;
		vis_ui_window_style_set(ui, cell, style_id);
	}
	return result;
}

VIS_INTERNAL void
ui_draw_line(Ui *tui, int x, int y, char c, uint16_t style_id)
{
	assert(style_id < tui->style_count);
	if (x < 0 || x >= tui->width || y < 0 || y >= tui->height)
		return;

	Cell *cells = tui->cell_buffer.cells + y * tui->width;
	while (x < tui->width) {
		cells[x].data[0] = c;
		cells[x].data[1] = 0;
		cells[x].style   = tui->styles[style_id];
		x++;
	}
}

VIS_INTERNAL void
ui_draw_string(Ui *tui, int x, int y, const char *str, uint16_t style_id)
{
	if (x < 0 || x >= tui->width || y < 0 || y >= tui->height)
		return;

	/* NOTE: the style that style_id refers to may contain unset values; we need to properly
	 * clear the cell first then go through ui_window_style_set to get the correct style */
	VisCellStyle default_style = tui->styles[UI_STYLE_DEFAULT];
	// FIXME: does not handle double width characters etc, share code with view.c?
	Cell *cells = tui->cell_buffer.cells + y * tui->width;
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
		vis_ui_window_style_set(tui, cells + x++, style_id);
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

	bool status  = win->options & UI_OPTION_STATUSBAR;
	bool nu      = win->options & UI_OPTION_LINE_NUMBERS_ABSOLUTE;
	bool rnu     = win->options & UI_OPTION_LINE_NUMBERS_RELATIVE;
	bool sidebar = nu || rnu;

	int width = win->width, height = win->height;

	int sidebar_width = 0;
	if (sidebar) {
		sidebar_width = u32_count_digits(view->lastline->lineno) + 1;
		sidebar_width = MIN(win->width, MAX(sidebar_width, win->min_sidebar_width));
	}
	if (sidebar_width != win->sidebar_width) {
		view_resize(view, width - sidebar_width, status ? height - 1 : height);
		win->sidebar_width = sidebar_width;
	}
	vis_window_draw(win);

	Selection *sel = view_selections_primary_get(view);
	size_t prev_lineno = 0, cursor_lineno = sel->line->lineno;
	int x = win->x, y = win->y;
	int view_width = view->width;
	Cell *cells = ui->cell_buffer.cells + y * ui->width;
	// TODO(rnp): this should not be possible
	if (x + sidebar_width + view_width > ui->width)
		view_width = ui->width - x - sidebar_width;

	// NOTE(rnp) 10 digits in U32_MAX, 1 space, 1 for 0 termination
	char sidebar_buffer[12];
	for (Line *l = view->topline; l; l = l->next, y++) {
		if (sidebar_width) {
			s32 line_number = l->lineno;
			sidebar_buffer[0] = 0;
			if (l->lineno && l->len && l->lineno != prev_lineno) {
				if (rnu) {
					line_number = (win->options & UI_OPTION_LARGE_FILE) ? 0 : l->lineno;
					if (l->lineno > cursor_lineno) line_number = l->lineno - cursor_lineno;
					if (l->lineno < cursor_lineno) line_number = cursor_lineno - l->lineno;
				}
				snprintf(sidebar_buffer, sizeof(sidebar_buffer), "%d ", line_number);
			}

			s32 padding = sidebar_width - 1 - u32_count_digits(line_number);
			if (sidebar_buffer[0] == 0) padding = sidebar_width;

			u16 style_id = (l->lineno == cursor_lineno) ? UI_STYLE_LINENUMBER_CURSOR : UI_STYLE_LINENUMBER;
			for (s32 xi = 0; xi < padding; xi++) {
				cells[x + xi] = (Cell){.data = " ", .len = 1, .width = 1, .style = ui->styles[UI_STYLE_DEFAULT]};
				vis_ui_window_style_set(ui, cells + x + xi, style_id);
			}
			ui_draw_string(ui, x + padding, y, sidebar_buffer, style_id);
			prev_lineno = l->lineno;
		}
		memcpy(cells + x + sidebar_width, l->cells, sizeof(Cell) * view_width);
		cells += ui->width;
	}
}

VIS_INTERNAL void
ui_window_status(Vis *vis, Win *win, const char *status)
{
	VisUiStyle style = vis->win == win ? UI_STYLE_STATUS_FOCUSED : UI_STYLE_STATUS;
	ui_draw_string(&vis->ui, win->x, win->y + win->height - 1, status, style);
}

VIS_INTERNAL void
ui_arrange(Vis *vis, enum UiLayout layout)
{
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
				Cell *cells = tui->cell_buffer.cells;
				for (int i = 0; i < max_height; i++) {
					vis_ui_window_style_set(tui, cells + x, UI_STYLE_SEPARATOR);
					strcpy(cells[x].data,"│");
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
		ui_draw_string(tui, 0, tui->height-1, tui->info, UI_STYLE_INFO);
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

	if (vis_cell_buffer_resize(&tui->cell_buffer, width, height)) {
		tui->width  = width;
		tui->height = height;
	}
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
	termkey_start(&tui->termkey, UI_TERMKEY_FLAGS);
	ui_term_backend_restore(tui);
}

VIS_INTERNAL void
ui_terminal_free(Ui *tui)
{
	vis_ui_backend_free(tui);
	termkey_destroy(&tui->termkey);
	munmap(tui->cell_buffer.cells, tui->cell_buffer.size);
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
				if (!termkey_init_from_fd(&tui->termkey, -1, UI_TERMKEY_FLAGS, term))
					ui_die_msg(tui, "Failed to start termkey\n");
		}
	}

	tui->doupdate    = true;
	tui->style_count = UI_STYLE_LAST + 1;

	bool result = tui->term.length && ui_backend_init(tui, term);
	if (result) ui_resize(tui);
	else        ui_terminal_free(tui);

	if (result) {
		VisCellStyle default_style = vis_ui_backend_style_default(tui);
		for (u64 it = 0; it < countof(tui->styles); it++)
			tui->styles[it] = default_style;

		tui->styles[UI_STYLE_CURSOR].attributes         |= VisCellAttribute_Reverse;
		tui->styles[UI_STYLE_CURSOR_PRIMARY].attributes |= VisCellAttribute_Reverse|VisCellAttribute_Blink;
		tui->styles[UI_STYLE_SELECTION].attributes      |= VisCellAttribute_Reverse;
		tui->styles[UI_STYLE_COLOR_COLUMN].attributes   |= VisCellAttribute_Reverse;
		tui->styles[UI_STYLE_STATUS].attributes         |= VisCellAttribute_Reverse;
		tui->styles[UI_STYLE_STATUS_FOCUSED].attributes |= VisCellAttribute_Reverse|VisCellAttribute_Bold;
		tui->styles[UI_STYLE_INFO].attributes           |= VisCellAttribute_Bold;
	}

	return result;
}
