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

static str8 symbols_default[] = {
	[SYNTAX_SYMBOL_SPACE]    = str8_comp("·"), /* Middle Dot U+00B7 */
	[SYNTAX_SYMBOL_TAB]      = str8_comp("›"), /* Single Right-Pointing Angle Quotation Mark U+203A */
	[SYNTAX_SYMBOL_TAB_FILL] = str8_comp(" "),
	[SYNTAX_SYMBOL_EOL]      = str8_comp("↵"), /* Downwards Arrow with Corner Leftwards U+21B5 */
	[SYNTAX_SYMBOL_EOF]      = str8_comp("~"),
};

static void selection_free(Selection *s)
{
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
	str8 filename = file->name;
	const char *mode = vis->mode->status;

	if (focused && mode)
		strcpy(left_parts[left_count++], mode);

	snprintf(left_parts[left_count++], sizeof(left_parts[0]), "%s%s%s",
	         filename.length > 0 ? (char *)filename.data : "[No Name]",
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
	ui_window_status(vis, win, status);
}

void view_tabwidth_set(View *view, int tabwidth) {
	if (tabwidth < 1 || tabwidth > 8)
		return;
	view->tabwidth = tabwidth;
	view_draw(view);
}

/* reset internal view data structures (cell matrix, line offsets etc.) */
VIS_INTERNAL void
vis_view_clear(View *view)
{
	memset(view->lines, 0, view->buffer_size - ((u8 *)view->lines - (u8 *)view->buffer));
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

	view->start_last      = view->start;
	view->line            = view->lines;
	view->lastline        = view->lines;
	view->col             = 0;
	view->wrapcol         = 0;
	view->prevch_breakat  = false;
	view->lines[0].lineno = view->large_file ? 1 : text_lineno_by_pos(view->text, view->start);
}

static int view_max_text_width(const View *view) {
	if (view->wrapcolumn > 0)
		return MIN(view->wrapcolumn, view->width);
	return view->width;
}

VIS_INTERNAL VisCell
view_blank_cell(View *view)
{
	// TODO(rnp): cleanup win and view should be merged
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	VisCell result     = {0};
	result.data[0]     = ' ';
	result.data_length = 1;
	result.width       = 1;
	result.style       = win->vis->ui.styles[UI_STYLE_DEFAULT];
	return result;
}

VIS_INTERNAL Line *
vis_view_line_next(View *view, Line *line)
{
	assert(Between(line, view->lines, view->lines + view->height - 1));
	Line *result = 0;
	if (line - view->lines < (view->height - 1))
		result = line + 1;
	return result;
}

VIS_INTERNAL s32
vis_view_line_index(View *view, Line *line)
{
	assert(Between(line, view->lines, view->lines + view->height - 1));
	s32 result = line - view->lines;
	return result;
}

static void view_wrap_line(View *view) {
	int col = view->col;
	int wrapcol = (view->wrapcol > 0) ? view->wrapcol : view->col;

	s32 old_line_index = vis_view_line_index(view, view->line);
	view->line    = old_line_index < (view->height - 1) ? view->lines + old_line_index + 1 : 0;
	view->col     = 0;
	view->wrapcol = 0;

	if (view->line) {
		view->line->lineno = view->lines[old_line_index].lineno;
		/* move extra cells to the next line */
		for (int i = wrapcol; i < col; ++i) {
			VisCellData  cell  = view->cell_data[old_line_index * view->width + i];
			VisCellStyle style = view->cell_styles[old_line_index * view->width + i];
			view->line->width += cell.width;
			view->line->len   += cell.file_byte_count;

			s32 index = (old_line_index + 1) * view->width + view->col++;
			view->cell_data[index]   = cell;
			view->cell_styles[index] = style;
		}
	}

	/* clear remaining cells on line */
	VisCell blank = view_blank_cell(view);
	for (int i = wrapcol; i < view->width; ++i) {
		if (i < col) {
			view->lines[old_line_index].width -= view->cell_data[old_line_index * view->width + i].width;
			view->lines[old_line_index].len   -= view->cell_data[old_line_index * view->width + i].file_byte_count;
		}
		memory_copy(view->cell_data + old_line_index * view->width + i, &blank, sizeof(VisCellData));
		view->cell_styles[old_line_index * view->width + i] = blank.style;
	}
}

VIS_INTERNAL bool
view_add_cell(View *view, VisCell cell)
{
	/* if the terminal is resized to a single (ASCII) char an out
	 * of bounds write could be performed for a wide char. this can
	 * be caught by iterating through the lines with view_wrap_line()
	 * until no lines remain. usually 0 or 1 iterations.
	 */
	while (view->col + cell.width > view_max_text_width(view)) {
		view_wrap_line(view);
		if (!view->line)
			return false;
	}

	view->line->width += cell.width;
	view->line->len   += cell.file_byte_count;

	s32 line_index = vis_view_line_index(view, view->line);
	s32 index = line_index * view->width + view->col++;
	memory_copy(view->cell_data + index, &cell, sizeof(VisCellData));
	view->cell_styles[index] = cell.style;

	/* set cells of a character which uses multiple columns */
	for (s32 i = 1; i < cell.width; i++) {
		index = line_index * view->width + view->col++;
		view->cell_data[index] = (VisCellData){0};
		view->cell_styles[index] = cell.style;
	}
	return true;
}

VIS_INTERNAL bool
view_expand_tab(View *view, VisCell *cell)
{
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	cell->style = vis_cell_style_merge(cell->style, win->vis->ui.styles[UI_STYLE_WHITESPACE]);

	cell->width = 1;

	bool result = true;
	int displayed_width = view->tabwidth - (view->col % view->tabwidth);
	for (int w = 0; result && w < displayed_width; ++w) {
		str8 symbol = (w == 0) ? view->symbols[SYNTAX_SYMBOL_TAB]
		                       : view->symbols[SYNTAX_SYMBOL_TAB_FILL];
		memory_copy(cell->data, symbol.data, MIN(sizeof(cell->data), symbol.length));
		cell->data_length     = MIN(sizeof(cell->data), symbol.length);
		cell->file_byte_count = (w == 0) ? 1 : 0;

		result = view_add_cell(view, *cell);
	}
	cell->file_byte_count = 1;

	return result;
}

VIS_INTERNAL bool
view_expand_newline(View *view, VisCell *cell)
{
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	cell->style = vis_cell_style_merge(cell->style, win->vis->ui.styles[UI_STYLE_WHITESPACE]);

	str8 symbol = view->symbols[SYNTAX_SYMBOL_EOL];
	memory_copy(cell->data, symbol.data, MIN(sizeof(cell->data), symbol.length));
	cell->data_length = MIN(sizeof(cell->data), symbol.length);
	cell->width       = 1;

	u64  lineno = view->line->lineno;
	bool result = view_add_cell(view, *cell);
	if (result) {
		view->wrapcol = 0;
		view_wrap_line(view);
		if (view->line)
			view->line->lineno = lineno + 1;
	}
	return result;
}

VIS_INTERNAL bool
view_expand_space(View *view, VisCell *cell)
{
	Win *win = (Win *)((char *)view - offsetof(Win, view));
	cell->style = vis_cell_style_merge(cell->style, win->vis->ui.styles[UI_STYLE_WHITESPACE]);

	str8 symbol = view->symbols[SYNTAX_SYMBOL_SPACE];
	memory_copy(cell->data, symbol.data, MIN(sizeof(cell->data), symbol.length));
	cell->data_length = MIN(sizeof(cell->data), symbol.length);
	cell->width       = 1;

	bool result = view_add_cell(view, *cell);
	return result;
}

/* try to add another character to the view, return whether there was space left */
VIS_INTERNAL bool
view_addch(View *view, VisCell *cell)
{
	if (!view->line)
		return false;

	char buffer[sizeof(cell->data) + 1];
	memory_copy(buffer, cell->data, sizeof(cell->data));
	buffer[cell->data_length] = 0;
	bool ch_breakat = (buffer[0] != 0) && strstr(view->breakat, buffer);
	if (view->prevch_breakat && !ch_breakat) {
		/* this is a good place to wrap line if needed */
		view->wrapcol = view->col;
	}
	view->prevch_breakat = ch_breakat;
	cell->style = view_blank_cell(view).style;

	u8 ch = cell->data[0];
	switch (ch) {
	default:{}break;
	case '\t':{return view_expand_tab(view, cell);    }break;
	case '\n':{return view_expand_newline(view, cell);}break;
	case ' ':{ return view_expand_space(view, cell);  }break;
	}

	if (ch < 128 && !isprint(ch)) {
		/* non-printable ascii char, represent it as ^(char + 64) */
		*cell = (VisCell){
			.data            = {'^', ch == 127 ? '?' : ch + 64},
			.data_length     = 2,
			.width           = 2,
			.file_byte_count = 1,
			.style           = cell->style,
		};
	}
	return view_add_cell(view, *cell);
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
			s->line = s->view->lines;
			s->row = 0;
			s->col = 0;
		}
		return;
	}
	// TODO: minimize number of redraws
	view_draw(s->view);
}

VIS_INTERNAL bool
view_coord_get(View *view, size_t pos, Line **retline, s32 *retrow, s32 *retcol)
{
	if (pos < view->start || pos > view->end) {
		if (retline) *retline = NULL;
		if (retrow) *retrow = -1;
		if (retcol) *retcol = -1;
		return false;
	}

	s32 row = 0, col = 0;
	size_t cur = view->start;

	s32 last_line_index = vis_view_line_index(view, view->lastline);
	while (row < last_line_index && cur < pos) {
		if (cur + view->lines[row].len > pos)
			break;
		cur += view->lines[row].len;
		row++;
	}

	if (row != view->height) {
		int max_col = MIN(view->width, view->lines[row].width);
		while (cur < pos && col < max_col) {
			cur += view->cell_data[row * view->width + col].file_byte_count;
			/* skip over columns occupied by the same character */
			while (++col < max_col && view->cell_data[row * view->width + col].file_byte_count == 0);
		}
	} else {
		row = view->height - 1;
	}

	if (retline) *retline = view->lines + row;
	if (retrow)  *retrow  = row;
	if (retcol)  *retcol  = col;
	return true;
}

/* redraw the complete with data starting from view->start bytes into the file.
 * stop once the screen is full, update view->end, view->lastline */
VIS_INTERNAL void
view_draw(View *view)
{
	vis_view_clear(view);
	/* read a screenful of text considering each character as 4-byte UTF character*/
	size_t size = view->width * view->height * 4;
	/* current buffer to work with */
	char *text = view->text_buffer;
	/* absolute position of character currently being added to display */
	size_t pos = view->start;

	str8 string = {.data = (u8 *)text, .length = text_bytes_get(view->text, view->start, size, text)};
	VisCell prev_cell = {0};
	while (string.length > 0) {
		VisCell cell = vis_cell_from_string(&string);

		if VisCellInvalid(cell) {
			// NOTE(rnp): needs more data. read another chunk into buffer.
			string.length = text_bytes_get(view->text, pos + prev_cell.file_byte_count, size, text);
			string.data   = (u8 *)text;
		} else {
			if (cell.width == 0) {
				u8 current   = prev_cell.data_length;
				u8 remaining = countof(prev_cell.data) - current;
				memory_copy(prev_cell.data + current, cell.data, MIN(remaining, cell.data_length));
				prev_cell.file_byte_count += MIN(remaining, cell.data_length);
				prev_cell.data_length     += MIN(remaining, cell.data_length);
			} else {
				if (prev_cell.file_byte_count && !view_addch(view, &prev_cell))
					break;
				pos += prev_cell.file_byte_count;
				prev_cell = cell;
			}
		}
	}

	if (prev_cell.file_byte_count && view_addch(view, &prev_cell))
		pos += prev_cell.file_byte_count;

	/* set end of viewing region */
	view->end = pos;
	if (view->line) {
		bool eof = view->end == text_size(view->text);
		if (view->line->len == 0 && eof && (view->line != view->lines)) {
			view->lastline = view->line - 1;
		} else if (eof && view->line->len == view->width) {
			// TODO(rnp): HACK: this doesn't belong here. we shouldn't allow
			// the selection to sit off the edge of the terminal but stopping
			// that requires major changes elsewhere
			view->lastline = vis_view_line_next(view, view->line);
			view_wrap_line(view);
		} else {
			view->lastline = view->line;
		}
	} else {
		view->lastline = view->lines + view->height - 1;
	}

	VisCell blank = view_blank_cell(view);
	/* clear remaining of line, important to show cursor at end of file */
	if (view->line) {
		s32 line_index = vis_view_line_index(view, view->line);
		for (s32 x = view->col; x < view->width; x++) {
			s32 index = line_index * view->width + x;
			memory_copy(view->cell_data + index, &blank, sizeof(VisCellData));
			view->cell_styles[index] = blank.style;
		}
	}

	/* resync position of cursors within visible area */
	for (Selection *s = view->selections; s; s = s->next) {
		size_t pos = view_cursors_pos(s);
		if (!view_coord_get(view, pos, &s->line, &s->row, &s->col) &&
		    s == view->selection) {
			s->line = view->lines;
			s->row = 0;
			s->col = 0;
		}
	}

	view->need_update = true;
}

bool view_update(View *view) {
	if (!view->need_update)
		return false;

	VisCell blank = view_blank_cell(view);
	for (s32 y = vis_view_line_index(view, view->lastline) + 1; y < view->height; y++) {
		for (s32 x = 0; x < view->width; x++) {
			s32 index = y * view->width + x;
			memory_copy(view->cell_data + index, &blank, sizeof(VisCellData));
			view->cell_styles[index] = blank.style;
		}
	}
	view->need_update = false;
	return true;
}

VIS_INTERNAL bool
vis_view_resize(View *view, s32 width, s32 height)
{
	width  = Max(width, 1);
	height = Max(height, 1);

	bool result = true;
	if (view->width != width || view->height != height) {
		u64 cell_count         = (u64)height * width;
		u64 page_size          = sysconf(_SC_PAGE_SIZE);
		u64 lines_offset       = AlignUpPowerOfTwo(cell_count * 4 + 1, 64);
		u64 cell_data_offset   = AlignUpPowerOfTwo(height * sizeof(Line), 64) + lines_offset;
		u64 cell_styles_offset = AlignUpPowerOfTwo(cell_count * sizeof(VisCellData), 64) + cell_data_offset;
		u64 buffer_size        = round_up_to(cell_styles_offset + cell_count * sizeof(VisCellStyle), page_size);

		if (buffer_size != view->buffer_size) {
			void *memory = mmap(0, buffer_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
			result = memory != MAP_FAILED;
			if (result) {
				if (view->buffer_size) munmap(view->buffer, view->buffer_size);
				view->buffer_size = buffer_size;
				view->buffer      = memory;
			}
		}

		if (result) {
			view->width       = width;
			view->height      = height;
			view->text_buffer = view->buffer;
			view->lines       = (Line *)((u8 *)view->buffer + lines_offset);
			view->cell_data   = (VisCellData *)((u8 *)view->buffer + cell_data_offset);
			view->cell_styles = (VisCellStyle *)((u8 *)view->buffer + cell_styles_offset);

			view_draw(view);
		}
	}

	if (result) view->need_update = true;

	return result;
}

void view_free(View *view) {
	if (!view)
		return;
	while (view->selections)
		selection_free(view->selections);
	free(view->breakat);
	munmap(view->buffer, view->buffer_size);
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
	    !vis_view_resize(view, 1, 1))
	{
		return false;
	}

	view_cursors_to(view->selection, 0);
	return true;
}

/* set/move current cursor position to a given (line, column) pair */
static size_t cursor_set(Selection *sel, Line *line, int col)
{
	int row = 0;
	View *view = sel->view;
	size_t pos = view->start;
	/* get row number and file offset at start of the given line */
	s32 line_index = vis_view_line_index(view, line);
	for (s32 y = 0; y < line_index; y++) {
		pos += view->lines[y].len;
		row++;
	}

	/* for characters which use more than 1 column, make sure we are on the left most */
	while (col > 0 && view->cell_data[line_index * view->width + col].file_byte_count == 0)
		col--;
	/* calculate offset within the line */
	for (int i = 0; i < col; i++)
		pos += view->cell_data[line_index * view->width + i].file_byte_count;

	sel->col = col;
	sel->row = row;
	sel->line = line;

	cursor_to(sel, pos);

	return pos;
}

/* NOTE: does not change the cursor_position use view_cursors_to() afterwords.
 * returns whether the visible area changed */
static bool view_viewport_down(View *view, int n)
{
	if (view->end >= text_size(view->text))
		return false;
	if (n >= view->height) {
		view->start = view->end;
	} else {
		for (s32 l = 0; l < view->height && n > 0; l++, n--)
			view->start += view->lines[l].len;
	}
	view_draw(view);
	return true;
}

/* NOTE: does not change the cursor_position use view_cursors_to() afterwords.
 * returns whether the visible area changed */
static bool view_viewport_up(View *view, int n)
{
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
	s32 line_index = vis_view_line_index(view, view->selection->line);
	for (s32 l = 0; l < line_index; l++)
		view->start += view->lines[l].len;
	view_draw(view);
	/* FIXME: does this logic make sense */
	view_cursors_to(view->selection, view->selection->pos);
}

VIS_INTERNAL void
view_redraw_center(View *view)
{
	u64 pos         = view->selection->pos;
	s32 line_number = (s32)(view->selection->line - view->lines);
	s32 center      = view->height / 2;
	if (line_number < center) {
		view_slide_down(view, center - line_number);
	} else {
		for (s32 i = 0; i < line_number - center; i++)
			view->start += view->lines[i].len;
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
		if (sel->line == view->lines)
			cursor_set(sel, view->lines, sel->col);
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
		Line *line = sel->line > view->lines ? sel->line : view->lines;
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
	if (sel->line == sel->view->lines)
		view_scroll_up(sel->view, 1);
	if (sel->line != sel->view->lines)
		cursor_set(sel, sel->line - 1, lastcol);
	sel->lastcol = lastcol;
	return sel->pos;
}

size_t view_screenline_down(Selection *sel) {
	if (!sel->line)
		return view_line_down(sel);
	int lastcol = sel->lastcol;
	if (!lastcol)
		lastcol = sel->col;
	if (vis_view_line_index(sel->view, sel->line) == sel->view->height - 1)
		view_scroll_down(sel->view, 1);
	if (vis_view_line_index(sel->view, sel->line) != sel->view->height - 1)
		cursor_set(sel, sel->line + 1 , lastcol);
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
		win->view.symbols[i] = (options & mapping[i]) ? symbols_default[i] : str8(" ");
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
	s32 last_line_index = vis_view_line_index(view, view->lastline);
	for (s32 l = 0; --n > 0 && l < last_line_index; l++)
		pos += view->lines[l].len;
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

bool view_selections_dispose(Selection *sel)
{
	if (sel) {
		View *view = sel->view;
		if (!view->selections || !view->selections->next)
			return false;
		selection_free(sel);
		view_selections_primary_set(view->selection);
	}
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

		if (view->end == pos && view->lastline == (view->lines + view->height - 1)) {
			view->start += view->lines[0].len;
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
	if (text_range_valid(sel))
		sel.end = text_char_next(txt, sel.end);
	return sel;
}

bool view_selections_set(Selection *s, Filerange r)
{
	Text *txt = s->view->text;
	size_t max = text_size(txt);
	if (!text_range_valid(r) || r.start >= max)
		return false;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	bool left_extending = anchor != EPOS && anchor > cursor;
	size_t end = r.end > max ? max : r.end;
	if (r.start != end)
		end = text_char_prev(txt, end);
	view_cursors_to(s, left_extending ? r.start : end);
	s->anchor = text_mark_set(txt, left_extending ? end : r.start);
	return true;
}

Filerange view_regions_restore(View *view, SelectionRegion s)
{
	Text *txt = view->text;
	size_t anchor = text_mark_get(txt, s.anchor);
	size_t cursor = text_mark_get(txt, s.cursor);
	Filerange sel = text_range_new(anchor, cursor);
	if (text_range_valid(sel))
		sel.end = text_char_next(txt, sel.end);
	return sel;
}

bool view_regions_save(View *view, Filerange r, SelectionRegion *s)
{
	Text *txt = view->text;
	size_t max = text_size(txt);
	if (!text_range_valid(r) || r.start >= max)
		return false;
	size_t end = r.end > max ? max : r.end;
	if (r.start != end)
		end = text_char_prev(txt, end);
	s->anchor = text_mark_set(txt, r.start);
	s->cursor = text_mark_set(txt, end);
	return true;
}

void view_selections_set_all(View *view, FilerangeList ranges, bool anchored)
{
	VisDACount i = 0;
	for (Selection *s = view->selections; s; s = s->next) {
		if (i++ >= ranges.count || !view_selections_set(s, ranges.data[i - 1])) {
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
	for (; i < ranges.count; i++) {
		Filerange r  = ranges.data[i];
		Selection *s = view_selections_new_force(view, r.start);
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
		if (text_range_valid(r))
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
		if (!text_range_valid(range)) {
			view_selections_dispose(s);
		} else if (prev && text_range_overlap(range_prev, range)) {
			range_prev = text_range_union(range_prev, range);
			view_selections_dispose(s);
		} else {
			if (prev)
				view_selections_set(prev, range_prev);
			range_prev = range;
			prev = s;
		}
	}
	if (prev)
		view_selections_set(prev, range_prev);
}

VIS_INTERNAL void
vis_win_style(Win *win, u64 start, u64 end, u16 style_id)
{
	View *view = &win->view;
	if (end < view->start || start > view->end)
		return;

	size_t pos = view->start;

	s32 line_index = 0;
	/* skip lines before range to be styled */
	while (line_index < view->height && pos + view->lines[line_index].len <= start)
		pos += view->lines[line_index++].len;

	if (line_index == view->height)
		return;

	int col = 0, view_width = view->width;
	/* skip columns before range to be styled */
	while (pos < start && col < view_width)
		pos += view->cell_data[view_width * line_index + col++].file_byte_count;

	/* skip empty columns */
	while (col < view_width && view->cell_data[view_width * line_index + col].file_byte_count == 0)
		col++;

	assert(style_id < win->vis->ui.style_count);
	do {
		// NOTE(rnp): first style at most until the end of the real line contents
		while (pos <= end && col < view->lines[line_index].width) {
			pos += view->cell_data[line_index * view_width + col].file_byte_count;
			VisCellStyle *style = view->cell_styles + line_index * view_width + col++;
			*style = vis_cell_style_merge(*style, win->vis->ui.styles[style_id]);
		}

		// NOTE(rnp): if the range extends to another line continue styling the full view width
		if (pos < end) while (col < view_width)
		{
			VisCellStyle *style = view->cell_styles + line_index * view_width + col++;
			*style = vis_cell_style_merge(*style, win->vis->ui.styles[style_id]);
		}

		line_index++;
		col = 0;
	} while (pos < end && line_index < view->height);
}
