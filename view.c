#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "view.h"
#include "text.h"
#include "text-motions.h"
#include "text-util.h"
#include "util.h"

typedef struct {
	char *symbol;
} SyntaxSymbol;

enum {
	SYNTAX_SYMBOL_SPACE,
	SYNTAX_SYMBOL_TAB,
	SYNTAX_SYMBOL_TAB_FILL,
	SYNTAX_SYMBOL_EOL,
	SYNTAX_SYMBOL_LAST,
};

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

typedef struct {
	Mark anchor;
	Mark cursor;
} SelectionRegion;

struct Cursor {             /* cursor position */
	Mark cursor;        /* other selection endpoint where it changes */
	Mark anchor;        /* position where the selection was created */
	bool anchored;      /* whether anchor remains fixed */
	size_t pos;         /* in bytes from the start of the file */
	int row, col;       /* in terms of zero based screen coordinates */
	int lastcol;        /* remembered column used when moving across lines */
	Line *line;         /* screen line on which cursor currently resides */
	int generation;     /* used to filter out newly created cursors during iteration */
	int number;         /* how many cursors are located before this one */
	SelectionRegion region; /* saved selection region */
	View *view;         /* associated view to which this cursor belongs */
	Cursor *prev, *next;/* previous/next cursors ordered by location at creation time */
};

/* Viewable area, showing part of a file. Keeps track of cursors and selections.
 * At all times there exists at least one cursor, which is placed in the visible viewport.
 * Additional cursors can be created and positioned anywhere in the file. */
struct View {
	Text *text;         /* underlying text management */
	UiWin *ui;
	Cell cell_blank;    /* used for empty/blank cells */
	int width, height;  /* size of display area */
	size_t start, end;  /* currently displayed area [start, end] in bytes from the start of the file */
	size_t start_last;  /* previously used start of visible area, used to update the mark */
	Mark start_mark;    /* mark to keep track of the start of the visible area */
	size_t lines_size;  /* number of allocated bytes for lines (grows only) */
	Line *lines;        /* view->height number of lines representing view content */
	Line *topline;      /* top of the view, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of view, might be unused if lastline < bottomline */
	Cursor *cursor;     /* main cursor, always placed within the visible viewport */
	Cursor *cursor_latest; /* most recently created cursor */
	Cursor *cursor_dead;/* main cursor which was disposed, will be removed when another cursor is created */
	int cursor_count;   /* how many cursors do currently exist */
	Line *line;         /* used while drawing view content, line where next char will be drawn */
	int col;            /* used while drawing view content, column where next char will be drawn */
	const SyntaxSymbol *symbols[SYNTAX_SYMBOL_LAST]; /* symbols to use for white spaces etc */
	int tabwidth;       /* how many spaces should be used to display a tab character */
	Cursor *cursors;    /* all cursors currently active */
	int cursor_generation; /* used to filter out newly created cursors during iteration */
	bool need_update;   /* whether view has been redrawn */
	bool large_file;    /* optimize for displaying large files */
	int colorcolumn;
};

static const SyntaxSymbol symbols_none[] = {
	[SYNTAX_SYMBOL_SPACE]    = { " " },
	[SYNTAX_SYMBOL_TAB]      = { " " },
	[SYNTAX_SYMBOL_TAB_FILL] = { " " },
	[SYNTAX_SYMBOL_EOL]      = { " " },
};

static const SyntaxSymbol symbols_default[] = {
	[SYNTAX_SYMBOL_SPACE]    = { "·" /* Middle Dot U+00B7 */ },
	[SYNTAX_SYMBOL_TAB]      = { "›" /* Single Right-Pointing Angle Quotation Mark U+203A */ },
	[SYNTAX_SYMBOL_TAB_FILL] = { " " },
	[SYNTAX_SYMBOL_EOL]      = { "↵" /* Downwards Arrow with Corner Leftwards U+21B5 */ },
};

static Cell cell_unused;


/* move visible viewport n-lines up/down, redraws the view but does not change
 * cursor position which becomes invalid and should be corrected by calling
 * view_cursor_to. the return value indicates wether the visible area changed.
 */
static bool view_viewport_up(View *view, int n);
static bool view_viewport_down(View *view, int n);

static void view_clear(View *view);
static bool view_addch(View *view, Cell *cell);
static void view_cursors_free(Cursor *c);
/* set/move current cursor position to a given (line, column) pair */
static size_t cursor_set(Cursor *cursor, Line *line, int col);

void view_tabwidth_set(View *view, int tabwidth) {
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
	if (view->ui)
		view->cell_blank.style = view->ui->style_get(view->ui, UI_STYLE_DEFAULT);
}

Filerange view_viewport_get(View *view) {
	return (Filerange){ .start = view->start, .end = view->end };
}

/* try to add another character to the view, return whether there was space left */
static bool view_addch(View *view, Cell *cell) {
	if (!view->line)
		return false;

	int width;
	size_t lineno = view->line->lineno;
	unsigned char ch = (unsigned char)cell->data[0];
	cell->style = view->cell_blank.style;

	switch (ch) {
	case '\t':
		cell->width = 1;
		width = view->tabwidth - (view->col % view->tabwidth);
		for (int w = 0; w < width; w++) {
			if (view->col + 1 > view->width) {
				view->line = view->line->next;
				view->col = 0;
				if (!view->line)
					return false;
				view->line->lineno = lineno;
			}

			cell->len = w == 0 ? 1 : 0;
			int t = w == 0 ? SYNTAX_SYMBOL_TAB : SYNTAX_SYMBOL_TAB_FILL;
			strncpy(cell->data, view->symbols[t]->symbol, sizeof(cell->data)-1);
			view->line->cells[view->col] = *cell;
			view->line->len += cell->len;
			view->line->width += cell->width;
			view->col++;
		}
		cell->len = 1;
		return true;
	case '\n':
		cell->width = 1;
		if (view->col + cell->width > view->width) {
			view->line = view->line->next;
			view->col = 0;
			if (!view->line)
				return false;
			view->line->lineno = lineno;
		}

		strncpy(cell->data, view->symbols[SYNTAX_SYMBOL_EOL]->symbol, sizeof(cell->data)-1);

		view->line->cells[view->col] = *cell;
		view->line->len += cell->len;
		view->line->width += cell->width;
		for (int i = view->col + 1; i < view->width; i++)
			view->line->cells[i] = view->cell_blank;

		view->line = view->line->next;
		if (view->line)
			view->line->lineno = lineno + 1;
		view->col = 0;
		return true;
	default:
		if (ch < 128 && !isprint(ch)) {
			/* non-printable ascii char, represent it as ^(char + 64) */
			*cell = (Cell) {
				.data = { '^', ch == 127 ? '?' : ch + 64, '\0' },
				.len = 1,
				.width = 2,
				.style = cell->style,
			};
		}

		if (ch == ' ') {
			strncpy(cell->data, view->symbols[SYNTAX_SYMBOL_SPACE]->symbol, sizeof(cell->data)-1);

		}

		if (view->col + cell->width > view->width) {
			for (int i = view->col; i < view->width; i++)
				view->line->cells[i] = view->cell_blank;
			view->line = view->line->next;
			view->col = 0;
		}

		if (view->line) {
			view->line->width += cell->width;
			view->line->len += cell->len;
			view->line->lineno = lineno;
			view->line->cells[view->col] = *cell;
			view->col++;
			/* set cells of a character which uses multiple columns */
			for (int i = 1; i < cell->width; i++)
				view->line->cells[view->col++] = cell_unused;
			return true;
		}
		return false;
	}
}

static void cursor_to(Cursor *c, size_t pos) {
	Text *txt = c->view->text;
	c->cursor = text_mark_set(txt, pos);
	if (!c->anchored)
		c->anchor = c->cursor;
	if (pos != c->pos)
		c->lastcol = 0;
	c->pos = pos;
	if (!view_coord_get(c->view, pos, &c->line, &c->row, &c->col)) {
		if (c->view->cursor == c) {
			c->line = c->view->topline;
			c->row = 0;
			c->col = 0;
		}
		return;
	}
	// TODO: minimize number of redraws
	view_draw(c->view);
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

/* move the cursor to the character at pos bytes from the begining of the file.
 * if pos is not in the current viewport, redraw the view to make it visible */
void view_cursor_to(View *view, size_t pos) {
	view_cursors_to(view->cursor, pos);
}

/* redraw the complete with data starting from view->start bytes into the file.
 * stop once the screen is full, update view->end, view->lastline */
void view_draw(View *view) {
	view_clear(view);
	/* read a screenful of text considering each character as 4-byte UTF character*/
	const size_t size = view->width * view->height * 4;
	/* current buffer to work with */
	char text[size+1];
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
			for (len = 1; rem > len && !ISUTF8(cur[len]); len++);
			cell = (Cell){ .data = "\xEF\xBF\xBD", .len = len, .width = 1 };
		} else if (len == (size_t)-2) {
			/* not enough bytes available to convert to a
			 * wide character. advance file position and read
			 * another junk into buffer.
			 */
			rem = text_bytes_get(view->text, pos, size, text);
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

		if (cell.width == 0 && prev_cell.len + cell.len < sizeof(cell.data)) {
			prev_cell.len += cell.len;
			strcat(prev_cell.data, cell.data);
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
			view->line->cells[x] = view->cell_blank;
	}

	/* resync position of cursors within visible area */
	for (Cursor *c = view->cursors; c; c = c->next) {
		size_t pos = view_cursors_pos(c);
		if (!view_coord_get(view, pos, &c->line, &c->row, &c->col) && 
		    c == view->cursor) {
			c->line = view->topline;
			c->row = 0;
			c->col = 0;
		}
	}

	view->need_update = true;
}

void view_invalidate(View *view) {
	view->need_update = true;
}

bool view_update(View *view) {
	if (!view->need_update)
		return false;
	for (Line *l = view->lastline->next; l; l = l->next) {
		for (int x = 0; x < view->width; x++)
			l->cells[x] = view->cell_blank;
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
	size_t lines_size = height*(sizeof(Line) + width*sizeof(Cell));
	if (lines_size > view->lines_size) {
		Line *lines = realloc(view->lines, lines_size);
		if (!lines)
			return false;
		view->lines = lines;
		view->lines_size = lines_size;
	}
	view->width = width;
	view->height = height;
	memset(view->lines, 0, view->lines_size);
	view_draw(view);
	return true;
}

int view_height_get(View *view) {
	return view->height;
}

int view_width_get(View *view) {
	return view->width;
}

void view_free(View *view) {
	if (!view)
		return;
	while (view->cursors)
		view_cursors_free(view->cursors);
	free(view->lines);
	free(view);
}

void view_reload(View *view, Text *text) {
	view->text = text;
	view_selections_clear_all(view);
	view_cursor_to(view, 0);
}

View *view_new(Text *text) {
	if (!text)
		return NULL;
	View *view = calloc(1, sizeof(View));
	if (!view)
		return NULL;
	if (!view_selections_new(view, 0)) {
		view_free(view);
		return NULL;
	}

	view->cell_blank = (Cell) {
		.width = 0,
		.len = 0,
		.data = " ",
	};
	view->text = text;
	view->tabwidth = 8;
	view_options_set(view, 0);

	if (!view_resize(view, 1, 1)) {
		view_free(view);
		return NULL;
	}

	view_cursor_to(view, 0);

	return view;
}

void view_ui(View *view, UiWin* ui) {
	view->ui = ui;
}

static size_t cursor_set(Cursor *cursor, Line *line, int col) {
	int row = 0;
	View *view = cursor->view;
	size_t pos = view->start;
	/* get row number and file offset at start of the given line */
	for (Line *cur = view->topline; cur && cur != line; cur = cur->next) {
		pos += cur->len;
		row++;
	}

	/* for characters which use more than 1 column, make sure we are on the left most */
	while (col > 0 && line->cells[col].len == 0)
		col--;
	/* calculate offset within the line */
	for (int i = 0; i < col; i++)
		pos += line->cells[i].len;

	cursor->col = col;
	cursor->row = row;
	cursor->line = line;

	cursor_to(cursor, pos);

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
	Line *line = view->cursor->line;
	for (Line *cur = view->topline; cur && cur != line; cur = cur->next)
		view->start += cur->len;
	view_draw(view);
	view_cursor_to(view, view->cursor->pos);
}

void view_redraw_center(View *view) {
	int center = view->height / 2;
	size_t pos = view->cursor->pos;
	for (int i = 0; i < 2; i++) {
		int linenr = 0;
		Line *line = view->cursor->line;
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
	view_cursor_to(view, pos);
}

void view_redraw_bottom(View *view) {
	Line *line = view->cursor->line;
	if (line == view->lastline)
		return;
	size_t pos = view->cursor->pos;
	view_viewport_up(view, view->height);
	while (pos >= view->end && view_viewport_down(view, 1));
	cursor_to(view->cursor, pos);
}

size_t view_slide_up(View *view, int lines) {
	Cursor *cursor = view->cursor;
	if (view_viewport_down(view, lines)) {
		if (cursor->line == view->topline)
			cursor_set(cursor, view->topline, cursor->col);
		else
			view_cursor_to(view, cursor->pos);
	} else {
		view_screenline_down(cursor);
	}
	return cursor->pos;
}

size_t view_slide_down(View *view, int lines) {
	Cursor *cursor = view->cursor;
	bool lastline = cursor->line == view->lastline;
	size_t col = cursor->col;
	if (view_viewport_up(view, lines)) {
		if (lastline)
			cursor_set(cursor, view->lastline, col);
		else
			view_cursor_to(view, cursor->pos);
	} else {
		view_screenline_up(cursor);
	}
	return cursor->pos;
}

size_t view_scroll_up(View *view, int lines) {
	Cursor *cursor = view->cursor;
	if (view_viewport_up(view, lines)) {
		Line *line = cursor->line < view->lastline ? cursor->line : view->lastline;
		cursor_set(cursor, line, view->cursor->col);
	} else {
		view_cursor_to(view, 0);
	}
	return cursor->pos;
}

size_t view_scroll_page_up(View *view) {
	Cursor *cursor = view->cursor;
	if (view->start == 0) {
		view_cursor_to(view, 0);
	} else {
		view_cursor_to(view, view->start-1);
		view_redraw_bottom(view);
		view_screenline_begin(cursor);
	}
	return cursor->pos;
}

size_t view_scroll_page_down(View *view) {
	view_scroll_down(view, view->height);
	return view_screenline_begin(view->cursor);
}

size_t view_scroll_halfpage_up(View *view) {
	Cursor *cursor = view->cursor;
	if (view->start == 0) {
		view_cursor_to(view, 0);
	} else {
		view_cursor_to(view, view->start-1);
		view_redraw_center(view);
		view_screenline_begin(cursor);
	}
	return cursor->pos;
}

size_t view_scroll_halfpage_down(View *view) {
	size_t end = view->end;
	size_t pos = view_scroll_down(view, view->height/2);
	if (pos < text_size(view->text))
		view_cursor_to(view, end);
	return view->cursor->pos;
}

size_t view_scroll_down(View *view, int lines) {
	Cursor *cursor = view->cursor;
	if (view_viewport_down(view, lines)) {
		Line *line = cursor->line > view->topline ? cursor->line : view->topline;
		cursor_set(cursor, line, cursor->col);
	} else {
		view_cursor_to(view, text_size(view->text));
	}
	return cursor->pos;
}

size_t view_line_up(Cursor *cursor) {
	View *view = cursor->view;
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	size_t pos = text_line_up(cursor->view->text, cursor->pos);
	bool offscreen = view->cursor == cursor && pos < view->start;
	view_cursors_to(cursor, pos);
	if (offscreen)
		view_redraw_top(view);
	if (cursor->line)
		cursor_set(cursor, cursor->line, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_line_down(Cursor *cursor) {
	View *view = cursor->view;
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	size_t pos = text_line_down(cursor->view->text, cursor->pos);
	bool offscreen = view->cursor == cursor && pos > view->end;
	view_cursors_to(cursor, pos);
	if (offscreen)
		view_redraw_bottom(view);
	if (cursor->line)
		cursor_set(cursor, cursor->line, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_screenline_up(Cursor *cursor) {
	if (!cursor->line)
		return view_line_up(cursor);
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	if (!cursor->line->prev)
		view_scroll_up(cursor->view, 1);
	if (cursor->line->prev)
		cursor_set(cursor, cursor->line->prev, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_screenline_down(Cursor *cursor) {
	if (!cursor->line)
		return view_line_down(cursor);
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	if (!cursor->line->next && cursor->line == cursor->view->bottomline)
		view_scroll_down(cursor->view, 1);
	if (cursor->line->next)
		cursor_set(cursor, cursor->line->next, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_screenline_begin(Cursor *cursor) {
	if (!cursor->line)
		return cursor->pos;
	return cursor_set(cursor, cursor->line, 0);
}

size_t view_screenline_middle(Cursor *cursor) {
	if (!cursor->line)
		return cursor->pos;
	return cursor_set(cursor, cursor->line, cursor->line->width / 2);
}

size_t view_screenline_end(Cursor *cursor) {
	if (!cursor->line)
		return cursor->pos;
	int col = cursor->line->width - 1;
	return cursor_set(cursor, cursor->line, col >= 0 ? col : 0);
}

size_t view_cursor_get(View *view) {
	return view_cursors_pos(view->cursor);
}

Line *view_lines_first(View *view) {
	return view->topline;
}

Line *view_lines_last(View *view) {
	return view->lastline;
}

Line *view_cursors_line_get(Cursor *c) {
	return c->line;
}

void view_scroll_to(View *view, size_t pos) {
	view_cursors_scroll_to(view->cursor, pos);
}

void view_options_set(View *view, enum UiOption options) {
	const int mapping[] = {
		[SYNTAX_SYMBOL_SPACE]    = UI_OPTION_SYMBOL_SPACE,
		[SYNTAX_SYMBOL_TAB]      = UI_OPTION_SYMBOL_TAB,
		[SYNTAX_SYMBOL_TAB_FILL] = UI_OPTION_SYMBOL_TAB_FILL,
		[SYNTAX_SYMBOL_EOL]      = UI_OPTION_SYMBOL_EOL,
	};

	for (int i = 0; i < LENGTH(mapping); i++) {
		view->symbols[i] = (options & mapping[i]) ? &symbols_default[i] :
			&symbols_none[i];
	}

	if (options & UI_OPTION_LINE_NUMBERS_ABSOLUTE)
		options &= ~UI_OPTION_LARGE_FILE;

	view->large_file = (options & UI_OPTION_LARGE_FILE);

	if (view->ui)
		view->ui->options_set(view->ui, options);
}

enum UiOption view_options_get(View *view) {
	return view->ui ? view->ui->options_get(view->ui) : 0;
}

void view_colorcolumn_set(View *view, int col) {
	if (col >= 0)
		view->colorcolumn = col;
}

int view_colorcolumn_get(View *view) {
	return view->colorcolumn;
}

size_t view_screenline_goto(View *view, int n) {
	size_t pos = view->start;
	for (Line *line = view->topline; --n > 0 && line != view->lastline; line = line->next)
		pos += line->len;
	return pos;
}

static Cursor *cursors_new(View *view, size_t pos, bool force) {
	Cursor *c = calloc(1, sizeof(*c));
	if (!c)
		return NULL;
	c->view = view;
	c->generation = view->cursor_generation;
	if (!view->cursors) {
		view->cursor = c;
		view->cursor_latest = c;
		view->cursors = c;
		view->cursor_count = 1;
		return c;
	}

	Cursor *prev = NULL, *next = NULL;
	Cursor *latest = view->cursor_latest ? view->cursor_latest : view->cursor;
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

	for (Cursor *after = next; after; after = after->next)
		after->number++;

	c->prev = prev;
	c->next = next;
	if (next)
		next->prev = c;
	if (prev) {
		prev->next = c;
		c->number = prev->number + 1;
	} else {
		view->cursors = c;
	}
	view->cursor_latest = c;
	view->cursor_count++;
	view_selections_dispose(view->cursor_dead);
	view_cursors_to(c, pos);
	return c;
err:
	free(c);
	return NULL;
}

Cursor *view_selections_new(View *view, size_t pos) {
	return cursors_new(view, pos, false);
}

Cursor *view_selections_new_force(View *view, size_t pos) {
	return cursors_new(view, pos, true);
}

int view_selections_count(View *view) {
	return view->cursor_count;
}

int view_selections_number(Cursor *c) {
	return c->number;
}

int view_selections_column_count(View *view) {
	Text *txt = view->text;
	int cpl_max = 0, cpl = 0; /* cursors per line */
	size_t line_prev = 0;
	for (Cursor *c = view->cursors; c; c = c->next) {
		size_t pos = view_cursors_pos(c);
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

static Cursor *cursors_column_next(View *view, Cursor *c, int column) {
	size_t line_cur = 0;
	int column_cur = 0;
	Text *txt = view->text;
	if (c) {
		size_t pos = view_cursors_pos(c);
		line_cur = text_lineno_by_pos(txt, pos);
		column_cur = INT_MIN;
	} else {
		c = view->cursors;
	}

	for (; c; c = c->next) {
		size_t pos = view_cursors_pos(c);
		size_t line = text_lineno_by_pos(txt, pos);
		if (line != line_cur) {
			line_cur = line;
			column_cur = 0;
		} else {
			column_cur++;
		}
		if (column == column_cur)
			return c;
	}
	return NULL;
}

Cursor *view_cursors_column(View *view, int column) {
	return cursors_column_next(view, NULL, column);
}

Cursor *view_selections_column_next(Cursor *c, int column) {
	return cursors_column_next(c->view, c, column);
}

static void view_cursors_free(Cursor *c) {
	if (!c)
		return;
	for (Cursor *after = c->next; after; after = after->next)
		after->number--;
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
	if (c->view->cursors == c)
		c->view->cursors = c->next;
	if (c->view->cursor == c)
		c->view->cursor = c->next ? c->next : c->prev;
	if (c->view->cursor_dead == c)
		c->view->cursor_dead = NULL;
	if (c->view->cursor_latest == c)
		c->view->cursor_latest = c->prev ? c->prev : c->next;
	c->view->cursor_count--;
	free(c);
}

bool view_selections_dispose(Cursor *c) {
	if (!c)
		return true;
	View *view = c->view;
	if (!view->cursors || !view->cursors->next)
		return false;
	view_cursors_free(c);
	view_selections_primary_set(view->cursor);
	return true;
}

bool view_selections_dispose_force(Cursor *c) {
	if (view_selections_dispose(c))
		return true;
	View *view = c->view;
	if (view->cursor_dead)
		return false;
	view_selection_clear(c);
	view->cursor_dead = c;
	return true;
}

Cursor *view_selection_disposed(View *view) {
	Cursor *c = view->cursor_dead;
	view->cursor_dead = NULL;
	return c;
}

Cursor *view_cursors(View *view) {
	view->cursor_generation++;
	return view->cursors;
}

Cursor *view_selections_primary_get(View *view) {
	view->cursor_generation++;
	return view->cursor;
}

void view_selections_primary_set(Cursor *c) {
	if (!c)
		return;
	c->view->cursor = c;
}

Cursor *view_selections_prev(Cursor *c) {
	View *view = c->view;
	for (c = c->prev; c; c = c->prev) {
		if (c->generation != view->cursor_generation)
			return c;
	}
	view->cursor_generation++;
	return NULL;
}

Cursor *view_selections_next(Cursor *c) {
	View *view = c->view;
	for (c = c->next; c; c = c->next) {
		if (c->generation != view->cursor_generation)
			return c;
	}
	view->cursor_generation++;
	return NULL;
}

size_t view_cursors_pos(Cursor *c) {
	return text_mark_get(c->view->text, c->cursor);
}

size_t view_cursors_line(Cursor *c) {
	size_t pos = view_cursors_pos(c);
	return text_lineno_by_pos(c->view->text, pos);
}

size_t view_cursors_col(Cursor *c) {
	size_t pos = view_cursors_pos(c);
	return text_line_char_get(c->view->text, pos) + 1;
}

int view_cursors_cell_get(Cursor *c) {
	return c->line ? c->col : -1;
}

int view_cursors_cell_set(Cursor *c, int cell) {
	if (!c->line || cell < 0)
		return -1;
	cursor_set(c, c->line, cell);
	return c->col;
}

void view_cursors_scroll_to(Cursor *c, size_t pos) {
	View *view = c->view;
	if (view->cursor == c) {
		view_draw(view);
		while (pos < view->start && view_viewport_up(view, 1));
		while (pos > view->end && view_viewport_down(view, 1));
	}
	view_cursors_to(c, pos);
}

void view_cursors_to(Cursor *c, size_t pos) {
	View *view = c->view;
	if (pos == EPOS)
		return;
	size_t size = text_size(view->text);
	if (pos > size)
		pos = size;
	if (c->view->cursor == c) {
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

	cursor_to(c, pos);
}

void view_cursors_place(Cursor *c, size_t line, size_t col) {
	Text *txt = c->view->text;
	size_t pos = text_pos_by_lineno(txt, line);
	pos = text_line_char_set(txt, pos, col > 0 ? col-1 : col);
	view_cursors_to(c, pos);
}

void view_selections_anchor(Cursor *c) {
	c->anchored = true;
}

void view_selection_clear(Cursor *c) {
	c->anchored = false;
	c->anchor = c->cursor;
	c->view->need_update = true;
}

void view_selections_flip(Cursor *s) {
	Mark temp = s->anchor;
	s->anchor = s->cursor;
	s->cursor = temp;
	view_cursors_to(s, text_mark_get(s->view->text, s->cursor));
}

bool view_selections_anchored(Selection *s) {
	return s->anchored;
}

void view_selections_clear_all(View *view) {
	for (Cursor *c = view->cursors; c; c = c->next)
		view_selection_clear(c);
	view_draw(view);
}

void view_selections_dispose_all(View *view) {
	for (Cursor *c = view->cursors, *next; c; c = next) {
		next = c->next;
		if (c != view->cursor)
			view_cursors_free(c);
	}
	view_draw(view);
}

Filerange view_selection_get(View *view) {
	return view_selections_get(view->cursor);
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

void view_selections_set(Selection *s, const Filerange *r) {
	if (!text_range_valid(r))
		return;
	Text *txt = s->view->text;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	bool left_extending = anchor != EPOS && anchor > cursor;
	size_t end = r->end;
	if (r->start != end)
		end = text_char_prev(txt, end);
	if (left_extending) {
		s->anchor = text_mark_set(txt, end);
		s->cursor = text_mark_set(txt, r->start);
	} else {
		s->anchor = text_mark_set(txt, r->start);
		s->cursor = text_mark_set(txt, end);
	}
	s->anchored = true;
	view_cursors_to(s, text_mark_get(s->view->text, s->cursor));
}

void view_selections_save(Selection *s) {
	s->region.cursor = s->cursor;
	s->region.anchor = s->anchor;
}

bool view_selections_restore(Selection *s) {
	Text *txt = s->view->text;
	size_t pos = text_mark_get(txt, s->region.cursor);
	if (pos == EPOS)
		return false;
	if (s->region.anchor != s->region.cursor && text_mark_get(txt, s->region.anchor) == EPOS)
		return false;
	s->cursor = s->region.cursor;
	s->anchor = s->region.anchor;
	s->anchored = true;
	view_cursors_to(s, pos);
	return true;
}

Text *view_text(View *view) {
	return view->text;
}

bool view_style_define(View *view, enum UiStyle id, const char *style) {
	return view->ui->style_define(view->ui, id, style);
}

void view_style(View *view, enum UiStyle style_id, size_t start, size_t end) {
	if (end < view->start || start > view->end)
		return;

	CellStyle style = view->ui->style_get(view->ui, style_id);
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

	do {
		while (pos <= end && col < width) {
			pos += line->cells[col].len;
			line->cells[col++].style = style;
		}
		col = 0;
	} while (pos <= end && (line = line->next));
}
