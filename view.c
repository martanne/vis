#include "vis-core.h"

/* A selection is made up of two marks named cursor and anchor.
 * While the anchor remains fixed the cursor mark follows cursor motions.
 * For a selection (indicated by []), the marks (^) are placed as follows:
 *
 *     [some text]              [!]
 *      ^       ^                ^
 *                               ^
 *
 * That is the marks point to the *start* of the first and last character
 * of the selection. In particular for a single character selection (as
 * depicted on the right above) both marks point to the same location.
 *
 * The view_selections_{get,set} functions take care of adding/removing
 * the necessary offset for the last character.
 */

static const char *symbols_none[] = {
	[SYNTAX_SYMBOL_SPACE]    = " ",
	[SYNTAX_SYMBOL_TAB]      = " ",
	[SYNTAX_SYMBOL_TAB_FILL] = " ",
	[SYNTAX_SYMBOL_EOL]      = " ",
	[SYNTAX_SYMBOL_EOF]      = " ",
};

static const char *symbols_default[] = {
	[SYNTAX_SYMBOL_SPACE]    = "·", /* Middle Dot U+00B7 */
	[SYNTAX_SYMBOL_TAB]      = "›", /* Single Right-Pointing Angle Quotation Mark U+203A */
	[SYNTAX_SYMBOL_TAB_FILL] = " ",
	[SYNTAX_SYMBOL_EOL]      = "↵", /* Downwards Arrow with Corner Leftwards U+21B5 */
	[SYNTAX_SYMBOL_EOF]      = "~",
};

static Cell cell_blank = { .width = 0, .len = 0, .data = " ", };

/* move visible viewport n-lines up/down, redraws the view but does not change
 * cursor position which becomes invalid and should be corrected by calling
 * view_cursors_to. the return value indicates whether the visible area changed.
 */
static bool view_viewport_up(View *view, int n);
static bool view_viewport_down(View *view, int n);

static void view_clear(View *view);
static bool view_add_cell(View *view, const Cell *cell);
static bool view_addch(View *view, Cell *cell);
static void selection_free(Selection*);
/* set/move current cursor position to a given (line, column) pair */
static size_t cursor_set(Selection*, Line *line, int col);

void window_status_update(Vis *vis, Win *win) {
	char left_parts[4][255] = { "", "", "", "" };
	char right_parts[4][32] = { "", "", "", "" };
	char left[sizeof(left_parts)+LENGTH(left_parts)*8];
	char right[sizeof(right_parts)+LENGTH(right_parts)*8];
	char status[sizeof(left)+sizeof(right)+1];
	size_t left_count = 0;
	size_t right_count = 0;

	View *view = &win->view;
	File *file = win->file;
	Text *txt = file->text;
	int width = win->width;
	enum UiOption options = win->options;
	bool focused = vis->win == win;
	const char *filename = file_name_get(file);
	const char *mode = vis->mode->status;

	if (focused && mode)
		strcpy(left_parts[left_count++], mode);

	snprintf(left_parts[left_count++], sizeof(left_parts[0]), "%s%s%s",
	         filename ? filename : "[No Name]",
	         text_modified(txt) ? " [+]" : "",
	         vis_macro_recording(vis) ? " @": "");

	int count = vis->action.count;
	const char *keys = buffer_content0(&vis->input_queue);
	if (keys && keys[0])
		snprintf(right_parts[right_count++], sizeof(right_parts[0]), "%s", keys);
	else if (count != VIS_COUNT_UNKNOWN)
		snprintf(right_parts[right_count++], sizeof(right_parts[0]), "%d", count);

	int sel_count = view->selection_count;
	if (sel_count > 1) {
		Selection *s = view_selections_primary_get(view);
		int sel_number = view_selections_number(s) + 1;
		snprintf(right_parts[right_count++], sizeof(right_parts[0]),
		         "%d/%d", sel_number, sel_count);
	}

	size_t size = text_size(txt);
	size_t pos = view_cursor_get(view);
	size_t percent = 0;
	if (size > 0) {
		double tmp = ((double)pos/(double)size)*100;
		percent = (size_t)(tmp+1);
	}
	snprintf(right_parts[right_count++], sizeof(right_parts[0]),
	         "%zu%%", percent);

	if (!(options & UI_OPTION_LARGE_FILE)) {
		Selection *sel = view_selections_primary_get(&win->view);
		size_t line = view_cursors_line(sel);
		size_t col = view_cursors_col(sel);
		if (col > UI_LARGE_FILE_LINE_SIZE) {
			options |= UI_OPTION_LARGE_FILE;
			win_options_set(win, options);
		}
		snprintf(right_parts[right_count++], sizeof(right_parts[0]),
		         "%zu, %zu", line, col);
	}

	int left_len = snprintf(left, sizeof(left), " %s%s%s%s%s%s%s",
	         left_parts[0],
	         left_parts[1][0] ? " » " : "",
	         left_parts[1],
	         left_parts[2][0] ? " » " : "",
	         left_parts[2],
	         left_parts[3][0] ? " » " : "",
	         left_parts[3]);

	int right_len = snprintf(right, sizeof(right), "%s%s%s%s%s%s%s ",
	         right_parts[0],
	         right_parts[1][0] ? " « " : "",
	         right_parts[1],
	         right_parts[2][0] ? " « " : "",
	         right_parts[2],
	         right_parts[3][0] ? " « " : "",
	         right_parts[3]);

	if (left_len < 0 || right_len < 0)
		return;
	int left_width = text_string_width(left, left_len);
	int right_width = text_string_width(right, right_len);

	int spaces = width - left_width - right_width;
	if (spaces < 1)
		spaces = 1;

	snprintf(status, sizeof(status), "%s%*s%s", left, spaces, " ", right);
	ui_window_status(win, status);
}

void view_tabwidth_set(View *view, int tabwidth) {
	if (tabwidth < 1 || tabwidth > 8)
		return;
	view->tabwidth = tabwidth;
	view_draw(view);
}

/* reset internal view data structures (cell matrix, line offsets etc.) */
static void view_clear(View *view) {
	memset(view->lines, 0, view->lines_size);
	if (view->start != view->start_last) {
		if (view->start == 0)
			view->start_mark = EMARK;
		else
			view->start_mark = text_mark_set(view->text, view->start);
	} else {
		size_t start;
		if (view->start_mark == EMARK)
			start = 0;
		else
			start = text_mark_get(view->text, view->start_mark);
		if (start != EPOS)
			view->start = start;
	}

	view->start_last = view->start;
	view->topline = view->lines;
	view->topline->lineno = view->large_file ? 1 : text_lineno_by_pos(view->text, view->start);
	view->lastline = view->topline;

	size_t line_size = sizeof(Line) + view->width*sizeof(Cell);
	size_t end = view->height * line_size;
	Line *prev = NULL;
	for (size_t i = 0; i < end; i += line_size) {
		Line *line = (Line*)(((char*)view->lines) + i);
		line->prev = prev;
		if (prev)
			prev->next = line;
		prev = line;
	}
	view->bottomline = prev ? prev : view->topline;
	view->bottomline->next = NULL;
	view->line = view->topline;
	view->col = 0;
	view->wrapcol = 0;
	view->prevch_breakat = false;

	/* FIXME: awful garbage that only exists because every
	 * struct in this program is an interdependent hellscape */
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	ui_window_style_set(&win->vis->ui, win->id, &cell_blank, UI_STYLE_DEFAULT, false);
}

static int view_max_text_width(const View *view) {
	if (view->wrapcolumn > 0)
		return MIN(view->wrapcolumn, view->width);
	return view->width;
}

static void view_wrap_line(View *view) {
	Line *wrapped_line = view->line;
	int col = view->col;
	int wrapcol = (view->wrapcol > 0) ? view->wrapcol : view->col;

	view->line = view->line->next;
	view->col = 0;
	view->wrapcol = 0;

	if (view->line) {
		view->line->lineno = wrapped_line->lineno;
		/* move extra cells to the next line */
		for (int i = wrapcol; i < col; ++i) {
			const Cell *cell = &wrapped_line->cells[i];
			view->line->width += cell->width;
			view->line->len += cell->len;
			view->line->cells[view->col++] = *cell;
		}
	}

	/* clear remaining cells on line */
	for (int i = wrapcol; i < view->width; ++i) {
		if (i < col) {
			wrapped_line->width -= wrapped_line->cells[i].width;
			wrapped_line->len -= wrapped_line->cells[i].len;
		}
		wrapped_line->cells[i] = cell_blank;
	}
}

static bool view_add_cell(View *view, const Cell *cell) {
	/* if the terminal is resized to a single (ASCII) char an out
	 * of bounds write could be performed for a wide char. this can
	 * be caught by iterating through the lines with view_wrap_line()
	 * until no lines remain. usually 0 or 1 iterations.
	 */
	while (view->col + cell->width > view_max_text_width(view)) {
		view_wrap_line(view);
		if (!view->line)
			return false;
	}

	view->line->width += cell->width;
	view->line->len += cell->len;
	view->line->cells[view->col++] = *cell;
	/* set cells of a character which uses multiple columns */
	for (int i = 1; i < cell->width; i++)
		view->line->cells[view->col++] = (Cell){0};
	return true;
}

static bool view_expand_tab(View *view, Cell *cell) {
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	ui_window_style_set(&win->vis->ui, win->id, cell, UI_STYLE_WHITESPACE, false);

	cell->width = 1;

	int displayed_width = view->tabwidth - (view->col % view->tabwidth);
	for (int w = 0; w < displayed_width; ++w) {
		int t = (w == 0) ? SYNTAX_SYMBOL_TAB : SYNTAX_SYMBOL_TAB_FILL;
		const char *symbol = view->symbols[t];
		strncpy(cell->data, symbol, sizeof(cell->data) - 1);
		cell->len = (w == 0) ? 1 : 0;

		if (!view_add_cell(view, cell))
			return false;
	}

	cell->len = 1;
	return true;
}

static bool view_expand_newline(View *view, Cell *cell) {
	size_t lineno = view->line->lineno;
	const char *symbol = view->symbols[SYNTAX_SYMBOL_EOL];

	Win *win = (Win *)((char *)view - offsetof(Win, view));
	ui_window_style_set(&win->vis->ui, win->id, cell, UI_STYLE_WHITESPACE, false);

	strncpy(cell->data, symbol, sizeof(cell->data) - 1);
	cell->width = 1;
	if (!view_add_cell(view, cell))
		return false;

	view->wrapcol = 0;
	view_wrap_line(view);
	if (view->line)
		view->line->lineno = lineno + 1;
	return true;
}

static bool view_expand_space(View *view, Cell *cell) {
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	ui_window_style_set(&win->vis->ui, win->id, cell, UI_STYLE_WHITESPACE, false);

	const char *symbol = view->symbols[SYNTAX_SYMBOL_SPACE];
	strncpy(cell->data, symbol, sizeof(cell->data) - 1);
	return view_add_cell(view, cell);
}

/* try to add another character to the view, return whether there was space left */
static bool view_addch(View *view, Cell *cell) {
	if (!view->line)
		return false;

	bool ch_breakat = (cell->data[0] != 0) && strstr(view->breakat, cell->data);
	if (view->prevch_breakat && !ch_breakat) {
		/* this is a good place to wrap line if needed */
		view->wrapcol = view->col;
	}
	view->prevch_breakat = ch_breakat;
	cell->style = cell_blank.style;

	unsigned char ch = (unsigned char)cell->data[0];
	switch (ch) {
	case '\t':
		return view_expand_tab(view, cell);
	case '\n':
		return view_expand_newline(view, cell);
	case ' ':
		return view_expand_space(view, cell);
	}

	if (ch < 128 && !isprint(ch)) {
		/* non-printable ascii char, represent it as ^(char + 64) */
		*cell = (Cell) {
			.data = { '^', ch == 127 ? '?' : ch + 64, '\0' },
			.len = 1,
			.width = 2,
			.style = cell->style,
		};
	}
	return view_add_cell(view, cell);
}

static void cursor_to(Selection *s, size_t pos) {
	Text *txt = s->view->text;
	s->cursor = text_mark_set(txt, pos);
	if (!s->anchored)
		s->anchor = s->cursor;
	if (pos != s->pos)
		s->lastcol = 0;
	s->pos = pos;
	if (!view_coord_get(s->view, pos, &s->line, &s->row, &s->col)) {
		if (s->view->selection == s) {
			s->line = s->view->topline;
			s->row = 0;
			s->col = 0;
		}
		return;
	}
	// TODO: minimize number of redraws
	view_draw(s->view);
}

bool view_coord_get(View *view, size_t pos, Line **retline, int *retrow, int *retcol) {
	int row = 0, col = 0;
	size_t cur = view->start;
	Line *line = view->topline;

	if (pos < view->start || pos > view->end) {
		if (retline) *retline = NULL;
		if (retrow) *retrow = -1;
		if (retcol) *retcol = -1;
		return false;
	}

	while (line && line != view->lastline && cur < pos) {
		if (cur + line->len > pos)
			break;
		cur += line->len;
		line = line->next;
		row++;
	}

	if (line) {
		int max_col = MIN(view->width, line->width);
		while (cur < pos && col < max_col) {
			cur += line->cells[col].len;
			/* skip over columns occupied by the same character */
			while (++col < max_col && line->cells[col].len == 0);
		}
	} else {
		line = view->bottomline;
		row = view->height - 1;
	}

	if (retline) *retline = line;
	if (retrow) *retrow = row;
	if (retcol) *retcol = col;
	return true;
}

/* redraw the complete with data starting from view->start bytes into the file.
 * stop once the screen is full, update view->end, view->lastline */
void view_draw(View *view) {
	view_clear(view);
	/* read a screenful of text considering each character as 4-byte UTF character*/
	const size_t size = view->width * view->height * 4;
	/* current buffer to work with */
	char *text = view->textbuf;
	/* remaining bytes to process in buffer */
	size_t rem = text_bytes_get(view->text, view->start, size, text);
	/* NUL terminate text section */
	text[rem] = '\0';
	/* absolute position of character currently being added to display */
	size_t pos = view->start;
	/* current position into buffer from which to interpret a character */
	char *cur = text;
	/* start from known multibyte state */
	mbstate_t mbstate = { 0 };

	Cell cell = { .data = "", .len = 0, .width = 0, }, prev_cell = cell;

	while (rem > 0) {

		/* current 'parsed' character' */
		wchar_t wchar;

		size_t len = mbrtowc(&wchar, cur, rem, &mbstate);
		if (len == (size_t)-1 && errno == EILSEQ) {
			/* ok, we encountered an invalid multibyte sequence,
			 * replace it with the Unicode Replacement Character
			 * (FFFD) and skip until the start of the next utf8 char */
			mbstate = (mbstate_t){0};
			for (len = 1; rem > len && !ISUTF8(cur[len]); len++);
			cell = (Cell){ .data = "\xEF\xBF\xBD", .len = len, .width = 1 };
		} else if (len == (size_t)-2) {
			/* not enough bytes available to convert to a
			 * wide character. Advance file position and read
			 * another junk into buffer.
			 */
			rem = text_bytes_get(view->text, pos+prev_cell.len, size, text);
			text[rem] = '\0';
			cur = text;
			continue;
		} else if (len == 0) {
			/* NUL byte encountered, store it and continue */
			cell = (Cell){ .data = "\x00", .len = 1, .width = 2 };
		} else {
			if (len >= sizeof(cell.data))
				len = sizeof(cell.data)-1;
			for (size_t i = 0; i < len; i++)
				cell.data[i] = cur[i];
			cell.data[len] = '\0';
			cell.len = len;
			cell.width = wcwidth(wchar);
			if (cell.width == -1)
				cell.width = 1;
		}

		if (cell.width == 0) {
			strncat(prev_cell.data, cell.data, sizeof(prev_cell.data)-strlen(prev_cell.data)-1);
			prev_cell.len += cell.len;
		} else {
			if (prev_cell.len && !view_addch(view, &prev_cell))
				break;
			pos += prev_cell.len;
			prev_cell = cell;
		}

 		rem -= cell.len;
		cur += cell.len;

		memset(&cell, 0, sizeof cell);
	}

	if (prev_cell.len && view_addch(view, &prev_cell))
		pos += prev_cell.len;

	/* set end of viewing region */
	view->end = pos;
	if (view->line) {
		bool eof = view->end == text_size(view->text);
		if (view->line->len == 0 && eof && view->line->prev)
			view->lastline = view->line->prev;
		else
			view->lastline = view->line;
	} else {
		view->lastline = view->bottomline;
	}

	/* clear remaining of line, important to show cursor at end of file */
	if (view->line) {
		for (int x = view->col; x < view->width; x++)
			view->line->cells[x] = cell_blank;
	}

	/* resync position of cursors within visible area */
	for (Selection *s = view->selections; s; s = s->next) {
		size_t pos = view_cursors_pos(s);
		if (!view_coord_get(view, pos, &s->line, &s->row, &s->col) &&
		    s == view->selection) {
			s->line = view->topline;
			s->row = 0;
			s->col = 0;
		}
	}

	view->need_update = true;
}

bool view_update(View *view) {
	if (!view->need_update)
		return false;
	for (Line *l = view->lastline->next; l; l = l->next) {
		for (int x = 0; x < view->width; x++)
			l->cells[x] = cell_blank;
	}
	view->need_update = false;
	return true;
}

bool view_resize(View *view, int width, int height) {
	if (width <= 0)
		width = 1;
	if (height <= 0)
		height = 1;
	if (view->width == width && view->height == height) {
		view->need_update = true;
		return true;
	}
	char *textbuf = malloc(width * height * 4 + 1);
	if (!textbuf)
		return false;
	size_t lines_size = height*(sizeof(Line) + width*sizeof(Cell));
	if (lines_size > view->lines_size) {
		Line *lines = realloc(view->lines, lines_size);
		if (!lines) {
			free(textbuf);
			return false;
		}
		view->lines = lines;
		view->lines_size = lines_size;
	}
	free(view->textbuf);
	view->textbuf = textbuf;
	view->width = width;
	view->height = height;
	memset(view->lines, 0, view->lines_size);
	view_draw(view);
	return true;
}

void view_free(View *view) {
	if (!view)
		return;
	while (view->selections)
		selection_free(view->selections);
	free(view->textbuf);
	free(view->lines);
	free(view->breakat);
}

void view_reload(View *view, Text *text) {
	view->text = text;
	view_selections_clear_all(view);
	view_cursors_to(view->selection, 0);
}

bool view_init(Win *win, Text *text) {
	View *view = &win->view;
	if (!text)
		return false;

	view->text = text;
	view->tabwidth = 8;
	view->breakat = strdup("");
	view->wrapcolumn = 0;
	win_options_set(win, 0);

	if (!view->breakat ||
	    !view_selections_new(view, 0) ||
	    !view_resize(view, 1, 1))
	{
		return false;
	}

	view_cursors_to(view->selection, 0);
	return true;
}

static size_t cursor_set(Selection *sel, Line *line, int col) {
	int row = 0;
	View *view = sel->view;
	size_t pos = view->start;
	/* get row number and file offset at start of the given line */
	for (Line *l = view->topline; l && l != line; l = l->next) {
		pos += l->len;
		row++;
	}

	/* for characters which use more than 1 column, make sure we are on the left most */
	while (col > 0 && line->cells[col].len == 0)
		col--;
	/* calculate offset within the line */
	for (int i = 0; i < col; i++)
		pos += line->cells[i].len;

	sel->col = col;
	sel->row = row;
	sel->line = line;

	cursor_to(sel, pos);

	return pos;
}

static bool view_viewport_down(View *view, int n) {
	Line *line;
	if (view->end >= text_size(view->text))
		return false;
	if (n >= view->height) {
		view->start = view->end;
	} else {
		for (line = view->topline; line && n > 0; line = line->next, n--)
			view->start += line->len;
	}
	view_draw(view);
	return true;
}

static bool view_viewport_up(View *view, int n) {
	/* scrolling up is somewhat tricky because we do not yet know where
	 * the lines start, therefore scan backwards but stop at a reasonable
	 * maximum in case we are dealing with a file without any newlines
	 */
	if (view->start == 0)
		return false;
	size_t max = view->width * view->height;
	char c;
	Iterator it = text_iterator_get(view->text, view->start - 1);

	if (!text_iterator_byte_get(&it, &c))
		return false;
	size_t off = 0;
	/* skip newlines immediately before display area */
	if (c == '\n' && text_iterator_byte_prev(&it, &c))
		off++;
	do {
		if (c == '\n' && --n == 0)
			break;
		if (++off > max)
			break;
	} while (text_iterator_byte_prev(&it, &c));
	view->start -= MIN(view->start, off);
	view_draw(view);
	return true;
}

void view_redraw_top(View *view) {
	Line *line = view->selection->line;
	for (Line *cur = view->topline; cur && cur != line; cur = cur->next)
		view->start += cur->len;
	view_draw(view);
	/* FIXME: does this logic make sense */
	view_cursors_to(view->selection, view->selection->pos);
}

void view_redraw_center(View *view) {
	int center = view->height / 2;
	size_t pos = view->selection->pos;
	for (int i = 0; i < 2; i++) {
		int linenr = 0;
		Line *line = view->selection->line;
		for (Line *cur = view->topline; cur && cur != line; cur = cur->next)
			linenr++;
		if (linenr < center) {
			view_slide_down(view, center - linenr);
			continue;
		}
		for (Line *cur = view->topline; cur && cur != line && linenr > center; cur = cur->next) {
			view->start += cur->len;
			linenr--;
		}
		break;
	}
	view_draw(view);
	view_cursors_to(view->selection, pos);
}

void view_redraw_bottom(View *view) {
	size_t pos = view->selection->pos;
	view_viewport_up(view, view->height);
	while (pos >= view->end && view_viewport_down(view, 1));
	cursor_to(view->selection, pos);
}

size_t view_slide_up(View *view, int lines) {
	Selection *sel = view->selection;
	if (view_viewport_down(view, lines)) {
		if (sel->line == view->topline)
			cursor_set(sel, view->topline, sel->col);
		else
			view_cursors_to(view->selection, sel->pos);
	} else {
		view_screenline_down(sel);
	}
	return sel->pos;
}

size_t view_slide_down(View *view, int lines) {
	Selection *sel = view->selection;
	bool lastline = sel->line == view->lastline;
	size_t col = sel->col;
	if (view_viewport_up(view, lines)) {
		if (lastline)
			cursor_set(sel, view->lastline, col);
		else
			view_cursors_to(view->selection, sel->pos);
	} else {
		view_screenline_up(sel);
	}
	return sel->pos;
}

size_t view_scroll_up(View *view, int lines) {
	Selection *sel = view->selection;
	if (view_viewport_up(view, lines)) {
		Line *line = sel->line < view->lastline ? sel->line : view->lastline;
		cursor_set(sel, line, view->selection->col);
	} else {
		view_cursors_to(view->selection, 0);
	}
	return sel->pos;
}

size_t view_scroll_page_up(View *view) {
	Selection *sel = view->selection;
	if (view->start == 0) {
		view_cursors_to(view->selection, 0);
	} else {
		view_cursors_to(view->selection, view->start-1);
		view_redraw_bottom(view);
		view_screenline_begin(sel);
	}
	return sel->pos;
}

size_t view_scroll_page_down(View *view) {
	view_scroll_down(view, view->height);
	return view_screenline_begin(view->selection);
}

size_t view_scroll_halfpage_up(View *view) {
	Selection *sel = view->selection;
	if (view->start == 0) {
		view_cursors_to(view->selection, 0);
	} else {
		view_cursors_to(view->selection, view->start-1);
		view_redraw_center(view);
		view_screenline_begin(sel);
	}
	return sel->pos;
}

size_t view_scroll_halfpage_down(View *view) {
	size_t end = view->end;
	size_t pos = view_scroll_down(view, view->height/2);
	if (pos < text_size(view->text))
		view_cursors_to(view->selection, end);
	return view->selection->pos;
}

size_t view_scroll_down(View *view, int lines) {
	Selection *sel = view->selection;
	if (view_viewport_down(view, lines)) {
		Line *line = sel->line > view->topline ? sel->line : view->topline;
		cursor_set(sel, line, sel->col);
	} else {
		view_cursors_to(view->selection, text_size(view->text));
	}
	return sel->pos;
}

size_t view_line_up(Selection *sel) {
	View *view = sel->view;
	int lastcol = sel->lastcol;
	if (!lastcol)
		lastcol = sel->col;
	size_t pos = text_line_up(sel->view->text, sel->pos);
	bool offscreen = view->selection == sel && pos < view->start;
	view_cursors_to(sel, pos);
	if (offscreen)
		view_redraw_top(view);
	if (sel->line)
		cursor_set(sel, sel->line, lastcol);
	sel->lastcol = lastcol;
	return sel->pos;
}

size_t view_line_down(Selection *sel) {
	View *view = sel->view;
	int lastcol = sel->lastcol;
	if (!lastcol)
		lastcol = sel->col;
	size_t pos = text_line_down(sel->view->text, sel->pos);
	bool offscreen = view->selection == sel && pos > view->end;
	view_cursors_to(sel, pos);
	if (offscreen)
		view_redraw_bottom(view);
	if (sel->line)
		cursor_set(sel, sel->line, lastcol);
	sel->lastcol = lastcol;
	return sel->pos;
}

size_t view_screenline_up(Selection *sel) {
	if (!sel->line)
		return view_line_up(sel);
	int lastcol = sel->lastcol;
	if (!lastcol)
		lastcol = sel->col;
	if (!sel->line->prev)
		view_scroll_up(sel->view, 1);
	if (sel->line->prev)
		cursor_set(sel, sel->line->prev, lastcol);
	sel->lastcol = lastcol;
	return sel->pos;
}

size_t view_screenline_down(Selection *sel) {
	if (!sel->line)
		return view_line_down(sel);
	int lastcol = sel->lastcol;
	if (!lastcol)
		lastcol = sel->col;
	if (!sel->line->next && sel->line == sel->view->bottomline)
		view_scroll_down(sel->view, 1);
	if (sel->line->next)
		cursor_set(sel, sel->line->next, lastcol);
	sel->lastcol = lastcol;
	return sel->pos;
}

size_t view_screenline_begin(Selection *sel) {
	if (!sel->line)
		return sel->pos;
	return cursor_set(sel, sel->line, 0);
}

size_t view_screenline_middle(Selection *sel) {
	if (!sel->line)
		return sel->pos;
	return cursor_set(sel, sel->line, sel->line->width / 2);
}

size_t view_screenline_end(Selection *sel) {
	if (!sel->line)
		return sel->pos;
	int col = sel->line->width - 1;
	return cursor_set(sel, sel->line, col >= 0 ? col : 0);
}

size_t view_cursor_get(View *view) {
	return view_cursors_pos(view->selection);
}

void win_options_set(Win *win, enum UiOption options) {
	const int mapping[] = {
		[SYNTAX_SYMBOL_SPACE]    = UI_OPTION_SYMBOL_SPACE,
		[SYNTAX_SYMBOL_TAB]      = UI_OPTION_SYMBOL_TAB,
		[SYNTAX_SYMBOL_TAB_FILL] = UI_OPTION_SYMBOL_TAB_FILL,
		[SYNTAX_SYMBOL_EOL]      = UI_OPTION_SYMBOL_EOL,
		[SYNTAX_SYMBOL_EOF]      = UI_OPTION_SYMBOL_EOF,
	};

	for (int i = 0; i < LENGTH(mapping); i++) {
		win->view.symbols[i] = (options & mapping[i]) ? symbols_default[i] :
			symbols_none[i];
	}

	if (options & UI_OPTION_LINE_NUMBERS_ABSOLUTE)
		options &= ~UI_OPTION_LARGE_FILE;

	win->view.large_file = (options & UI_OPTION_LARGE_FILE);

	ui_window_options_set(win, options);
}

bool view_breakat_set(View *view, const char *breakat) {
	char *copy = strdup(breakat);
	if (!copy)
		return false;
	free(view->breakat);
	view->breakat = copy;
	return true;
}

size_t view_screenline_goto(View *view, int n) {
	size_t pos = view->start;
	for (Line *line = view->topline; --n > 0 && line != view->lastline; line = line->next)
		pos += line->len;
	return pos;
}

static Selection *selections_new(View *view, size_t pos, bool force) {
	if (pos > text_size(view->text))
		return NULL;
	Selection *s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;
	s->view = view;
	s->generation = view->selection_generation;
	if (!view->selections) {
		view->selection = s;
		view->selection_latest = s;
		view->selections = s;
		view->selection_count = 1;
		return s;
	}

	Selection *prev = NULL, *next = NULL;
	Selection *latest = view->selection_latest ? view->selection_latest : view->selection;
	size_t cur = view_cursors_pos(latest);
	if (pos == cur) {
		prev = latest;
		next = prev->next;
	} else if (pos > cur) {
		prev = latest;
		for (next = prev->next; next; prev = next, next = next->next) {
			cur = view_cursors_pos(next);
			if (pos <= cur)
				break;
		}
	} else if (pos < cur) {
		next = latest;
		for (prev = next->prev; prev; next = prev, prev = prev->prev) {
			cur = view_cursors_pos(prev);
			if (pos >= cur)
				break;
		}
	}

	if (pos == cur && !force)
		goto err;

	for (Selection *after = next; after; after = after->next)
		after->number++;

	s->prev = prev;
	s->next = next;
	if (next)
		next->prev = s;
	if (prev) {
		prev->next = s;
		s->number = prev->number + 1;
	} else {
		view->selections = s;
	}
	view->selection_latest = s;
	view->selection_count++;
	view_selections_dispose(view->selection_dead);
	view_cursors_to(s, pos);
	return s;
err:
	free(s);
	return NULL;
}

Selection *view_selections_new(View *view, size_t pos) {
	return selections_new(view, pos, false);
}

Selection *view_selections_new_force(View *view, size_t pos) {
	return selections_new(view, pos, true);
}

int view_selections_number(Selection *sel) {
	return sel->number;
}

int view_selections_column_count(View *view) {
	Text *txt = view->text;
	int cpl_max = 0, cpl = 0; /* cursors per line */
	size_t line_prev = 0;
	for (Selection *sel = view->selections; sel; sel = sel->next) {
		size_t pos = view_cursors_pos(sel);
		size_t line = text_lineno_by_pos(txt, pos);
		if (line == line_prev)
			cpl++;
		else
			cpl = 1;
		line_prev = line;
		if (cpl > cpl_max)
			cpl_max = cpl;
	}
	return cpl_max;
}

static Selection *selections_column_next(View *view, Selection *sel, int column) {
	size_t line_cur = 0;
	int column_cur = 0;
	Text *txt = view->text;
	if (sel) {
		size_t pos = view_cursors_pos(sel);
		line_cur = text_lineno_by_pos(txt, pos);
		column_cur = INT_MIN;
	} else {
		sel = view->selections;
	}

	for (; sel; sel = sel->next) {
		size_t pos = view_cursors_pos(sel);
		size_t line = text_lineno_by_pos(txt, pos);
		if (line != line_cur) {
			line_cur = line;
			column_cur = 0;
		} else {
			column_cur++;
		}
		if (column == column_cur)
			return sel;
	}
	return NULL;
}

Selection *view_selections_column(View *view, int column) {
	return selections_column_next(view, NULL, column);
}

Selection *view_selections_column_next(Selection *sel, int column) {
	return selections_column_next(sel->view, sel, column);
}

static void selection_free(Selection *s) {
	if (!s)
		return;
	for (Selection *after = s->next; after; after = after->next)
		after->number--;
	if (s->prev)
		s->prev->next = s->next;
	if (s->next)
		s->next->prev = s->prev;
	if (s->view->selections == s)
		s->view->selections = s->next;
	if (s->view->selection == s)
		s->view->selection = s->next ? s->next : s->prev;
	if (s->view->selection_dead == s)
		s->view->selection_dead = NULL;
	if (s->view->selection_latest == s)
		s->view->selection_latest = s->prev ? s->prev : s->next;
	s->view->selection_count--;
	free(s);
}

bool view_selections_dispose(Selection *sel) {
	if (!sel)
		return true;
	View *view = sel->view;
	if (!view->selections || !view->selections->next)
		return false;
	selection_free(sel);
	view_selections_primary_set(view->selection);
	return true;
}

bool view_selections_dispose_force(Selection *sel) {
	if (view_selections_dispose(sel))
		return true;
	View *view = sel->view;
	if (view->selection_dead)
		return false;
	view_selection_clear(sel);
	view->selection_dead = sel;
	return true;
}

Selection *view_selection_disposed(View *view) {
	Selection *sel = view->selection_dead;
	view->selection_dead = NULL;
	return sel;
}

Selection *view_selections(View *view) {
	view->selection_generation++;
	return view->selections;
}

Selection *view_selections_primary_get(View *view) {
	view->selection_generation++;
	return view->selection;
}

void view_selections_primary_set(Selection *s) {
	if (!s)
		return;
	s->view->selection = s;
	Mark anchor = s->anchor;
	view_cursors_to(s, view_cursors_pos(s));
	s->anchor = anchor;
}

Selection *view_selections_prev(Selection *s) {
	View *view = s->view;
	for (s = s->prev; s; s = s->prev) {
		if (s->generation != view->selection_generation)
			return s;
	}
	view->selection_generation++;
	return NULL;
}

Selection *view_selections_next(Selection *s) {
	View *view = s->view;
	for (s = s->next; s; s = s->next) {
		if (s->generation != view->selection_generation)
			return s;
	}
	view->selection_generation++;
	return NULL;
}

size_t view_cursors_pos(Selection *s) {
	return text_mark_get(s->view->text, s->cursor);
}

size_t view_cursors_line(Selection *s) {
	size_t pos = view_cursors_pos(s);
	return text_lineno_by_pos(s->view->text, pos);
}

size_t view_cursors_col(Selection *s) {
	size_t pos = view_cursors_pos(s);
	return text_line_char_get(s->view->text, pos) + 1;
}

int view_cursors_cell_set(Selection *s, int cell) {
	if (!s->line || cell < 0)
		return -1;
	cursor_set(s, s->line, cell);
	return s->col;
}

void view_cursors_scroll_to(Selection *s, size_t pos) {
	View *view = s->view;
	if (view->selection == s) {
		view_draw(view);
		while (pos < view->start && view_viewport_up(view, 1));
		while (pos > view->end && view_viewport_down(view, 1));
	}
	view_cursors_to(s, pos);
}

void view_cursors_to(Selection *s, size_t pos) {
	View *view = s->view;
	if (pos == EPOS)
		return;
	size_t size = text_size(view->text);
	if (pos > size)
		pos = size;
	if (s->view->selection == s) {
		/* make sure we redraw changes to the very first character of the window */
		if (view->start == pos)
			view->start_last = 0;

		if (view->end == pos && view->lastline == view->bottomline) {
			view->start += view->topline->len;
			view_draw(view);
		}

		if (pos < view->start || pos > view->end) {
			view->start = pos;
			view_viewport_up(view, view->height / 2);
		}

		if (pos <= view->start || pos > view->end) {
			view->start = text_line_begin(view->text, pos);
			view_draw(view);
		}

		if (pos <= view->start || pos > view->end) {
			view->start = pos;
			view_draw(view);
		}
	}

	cursor_to(s, pos);
}

void view_cursors_place(Selection *s, size_t line, size_t col) {
	Text *txt = s->view->text;
	size_t pos = text_pos_by_lineno(txt, line);
	pos = text_line_char_set(txt, pos, col > 0 ? col-1 : col);
	view_cursors_to(s, pos);
}

void view_selection_clear(Selection *s) {
	s->anchored = false;
	s->anchor = s->cursor;
	s->view->need_update = true;
}

void view_selections_flip(Selection *s) {
	Mark temp = s->anchor;
	s->anchor = s->cursor;
	s->cursor = temp;
	view_cursors_to(s, text_mark_get(s->view->text, s->cursor));
}

void view_selections_clear_all(View *view) {
	for (Selection *s = view->selections; s; s = s->next)
		view_selection_clear(s);
	view_draw(view);
}

void view_selections_dispose_all(View *view) {
	Selection *last = view->selections;
	while (last->next)
		last = last->next;
	for (Selection *s = last, *prev; s; s = prev) {
		prev = s->prev;
		if (s != view->selection)
			selection_free(s);
	}
	view_draw(view);
}

Filerange view_selections_get(Selection *s) {
	if (!s)
		return text_range_empty();
	Text *txt = s->view->text;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	Filerange sel = text_range_new(anchor, cursor);
	if (text_range_valid(&sel))
		sel.end = text_char_next(txt, sel.end);
	return sel;
}

bool view_selections_set(Selection *s, const Filerange *r) {
	Text *txt = s->view->text;
	size_t max = text_size(txt);
	if (!text_range_valid(r) || r->start >= max)
		return false;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	bool left_extending = anchor != EPOS && anchor > cursor;
	size_t end = r->end > max ? max : r->end;
	if (r->start != end)
		end = text_char_prev(txt, end);
	view_cursors_to(s, left_extending ? r->start : end);
	s->anchor = text_mark_set(txt, left_extending ? end : r->start);
	return true;
}

Filerange view_regions_restore(View *view, SelectionRegion *s) {
	Text *txt = view->text;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	Filerange sel = text_range_new(anchor, cursor);
	if (text_range_valid(&sel))
		sel.end = text_char_next(txt, sel.end);
	return sel;
}

bool view_regions_save(View *view, Filerange *r, SelectionRegion *s) {
	Text *txt = view->text;
	size_t max = text_size(txt);
	if (!text_range_valid(r) || r->start >= max)
		return false;
	size_t end = r->end > max ? max : r->end;
	if (r->start != end)
		end = text_char_prev(txt, end);
	s->anchor = text_mark_set(txt, r->start);
	s->cursor = text_mark_set(txt, end);
	return true;
}

void view_selections_set_all(View *view, FilerangeList ranges, bool anchored)
{
	VisDACount i = 0;
	for (Selection *s = view->selections; s; s = s->next) {
		if (i++ >= ranges.count || !view_selections_set(s, ranges.data + i - 1)) {
			for (Selection *next; s; s = next) {
				next = view_selections_next(s);
				if (i == 1 && s == view->selection)
					view_selection_clear(s);
				else
					view_selections_dispose(s);
			}
			break;
		}
		s->anchored = anchored;
	}
	while (i < ranges.count) {
		Filerange *r = ranges.data + i++;
		Selection *s = view_selections_new_force(view, r->start);
		if (!s || !view_selections_set(s, r))
			break;
		s->anchored = anchored;
	}
	view_selections_primary_set(view->selections);
}

FilerangeList view_selections_get_all(Vis *vis, View *view)
{
	FilerangeList result = {0};
	da_reserve(vis, &result, view->selection_count);
	for (Selection *s = view->selections; s; s = s->next) {
		Filerange r = view_selections_get(s);
		if (text_range_valid(&r))
			*da_push(vis, &result) = r;
	}
	return result;
}

void view_selections_normalize(View *view) {
	Selection *prev = NULL;
	Filerange range_prev = text_range_empty();
	for (Selection *s = view->selections, *next; s; s = next) {
		next = s->next;
		Filerange range = view_selections_get(s);
		if (!text_range_valid(&range)) {
			view_selections_dispose(s);
		} else if (prev && text_range_overlap(&range_prev, &range)) {
			range_prev = text_range_union(&range_prev, &range);
			view_selections_dispose(s);
		} else {
			if (prev)
				view_selections_set(prev, &range_prev);
			range_prev = range;
			prev = s;
		}
	}
	if (prev)
		view_selections_set(prev, &range_prev);
}

void win_style(Win *win, enum UiStyle style, size_t start, size_t end, bool keep_non_default) {
	View *view = &win->view;
	if (end < view->start || start > view->end)
		return;

	size_t pos = view->start;
	Line *line = view->topline;

	/* skip lines before range to be styled */
	while (line && pos + line->len <= start) {
		pos += line->len;
		line = line->next;
	}

	if (!line)
		return;

	int col = 0, width = view->width;

	/* skip columns before range to be styled */
	while (pos < start && col < width)
		pos += line->cells[col++].len;

	/* skip empty columns */
	while (!line->cells[col].len && col < width)
		col++;

	do {
		while (pos <= end && col < width) {
			pos += line->cells[col].len;
			ui_window_style_set(&win->vis->ui, win->id, &line->cells[col++], style, keep_non_default);
		}
		col = 0;
	} while (pos <= end && (line = line->next));
}
