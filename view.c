/*
 * Copyright (c) 2014 Marc André Tanner <mat at brain-dump.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include "editor.h"
#include "view.h"
#include "syntax.h"
#include "text.h"
#include "text-motions.h"
#include "util.h"

typedef struct {            /* cursor position */
	Filepos pos;        /* in bytes from the start of the file */
	Filepos lastpos;    /* previous cursor position */
	int row, col;       /* in terms of zero based screen coordinates */
	int lastcol;        /* remembered column used when moving across lines */
	Line *line;         /* screen line on which cursor currently resides */
	bool highlighted;   /* true e.g. when cursor is on a bracket */
} Cursor;

struct View {               /* viewable area, showing part of a file */
	Text *text;         /* underlying text management */
	UiWin *ui;
	ViewEvent *events;
	int width, height;  /* size of display area */
	Filepos start, end; /* currently displayed area [start, end] in bytes from the start of the file */
	size_t lines_size;  /* number of allocated bytes for lines (grows only) */
	Line *lines;        /* view->height number of lines representing view content */
	Line *topline;      /* top of the view, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of view, might be unused if lastline < bottomline */
	Filerange sel;      /* selected text range in bytes from start of file */
	Cursor cursor;      /* current cursor position within view */
	Line *line;         /* used while drawing view content, line where next char will be drawn */
	int col;            /* used while drawing view content, column where next char will be drawn */
	Syntax *syntax;     /* syntax highlighting definitions for this view or NULL */
	int tabwidth;       /* how many spaces should be used to display a tab character */
};

static void view_clear(View *view);
static bool view_addch(View *view, Cell *cell);
static size_t view_cursor_update(View *view);
/* set/move current cursor position to a given (line, column) pair */
static size_t view_cursor_set(View *view, Line *line, int col);
/* move visible viewport n-lines up/down, redraws the view but does not change
 * cursor position which becomes invalid and should be corrected by either:
 *
 *   - view_cursor_to
 *   - view_cursor_set
 *
 * the return value indicates wether the visible area changed.
 */
static bool view_viewport_up(View *view, int n);
static bool view_viewport_down(View *view, int n);

void view_tabwidth_set(View *view, int tabwidth) {
	view->tabwidth = tabwidth;
	view_draw(view);
}

void view_selection_clear(View *view) {
	view->sel = text_range_empty();
	view_draw(view);
	view_cursor_update(view);
}

/* reset internal view data structures (cell matrix, line offsets etc.) */
static void view_clear(View *view) {
	/* calculate line number of first line */
	// TODO move elsewhere
	view->topline = view->lines;
	view->topline->lineno = text_lineno_by_pos(view->text, view->start);
	view->lastline = view->topline;

	/* reset all other lines */
	size_t line_size = sizeof(Line) + view->width*sizeof(Cell);
	size_t end = view->height * line_size;
	Line *prev = NULL;
	for (size_t i = 0; i < end; i += line_size) {
		Line *line = (Line*)(((char*)view->lines) + i);
		line->width = 0;
		line->len = 0;
		line->prev = prev;
		if (prev)
			prev->next = line;
		prev = line;
	}
	view->bottomline = prev ? prev : view->topline;
	view->bottomline->next = NULL;
	view->line = view->topline;
	view->col = 0;
}

Filerange view_selection_get(View *view) {
	Filerange sel = view->sel;
	if (sel.start > sel.end) {
		size_t tmp = sel.start;
		sel.start = sel.end;
		sel.end = tmp;
	}
	if (!text_range_valid(&sel))
		return text_range_empty();
	sel.end = text_char_next(view->text, sel.end);
	return sel;
}

void view_selection_set(View *view, Filerange *sel) {
	Cursor *cursor = &view->cursor;
	view->sel = *sel;
	view_draw(view);
	if (view->ui)
		view->ui->cursor_to(view->ui, cursor->col, cursor->row);
}

Filerange view_viewport_get(View *view) {
	return (Filerange){ .start = view->start, .end = view->end };
}

/* try to add another character to the view, return whether there was space left */
static bool view_addch(View *view, Cell *cell) {
	if (!view->line)
		return false;

	int width;
	static Cell empty;
	size_t lineno = view->line->lineno;

	switch (cell->data[0]) {
	case '\t':
		width = view->tabwidth - (view->col % view->tabwidth);
		for (int w = 0; w < width; w++) {
			if (view->col + 1 > view->width) {
				view->line = view->line->next;
				view->col = 0;
				if (!view->line)
					return false;
				view->line->lineno = lineno;
			}
			if (w == 0) {
				/* first cell of a tab has a length of 1 */
				view->line->cells[view->col].len = cell->len;
				view->line->len += cell->len;
			} else {
				/* all remaining ones have a lenght of zero */
				view->line->cells[view->col].len = 0;
			}
			/* but all are marked as part of a tabstop */
			view->line->cells[view->col].width = 1;
			view->line->cells[view->col].data[0] = ' ';
			view->line->cells[view->col].data[1] = '\0';
			view->line->cells[view->col].istab = true;
			view->line->cells[view->col].attr = cell->attr;
			view->line->width++;
			view->col++;
		}
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
		view->line->cells[view->col] = *cell;
		view->line->len += cell->len;
		view->line->width += cell->width;
		for (int i = view->col + 1; i < view->width; i++)
			view->line->cells[i] = empty;

		view->line = view->line->next;
		if (view->line)
			view->line->lineno = lineno + 1;
		view->col = 0;
		return true;
	default:
		if ((unsigned char)cell->data[0] < 128 && !isprint((unsigned char)cell->data[0])) {
			/* non-printable ascii char, represent it as ^(char + 64) */
			*cell = (Cell) {
				.data = { '^', cell->data[0] + 64, '\0' },
				.len = 1,
				.width = 2,
				.istab = false,
				.attr = cell->attr,
			};
		}

		if (view->col + cell->width > view->width) {
			for (int i = view->col; i < view->width; i++)
				view->line->cells[i] = empty;
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
				view->line->cells[view->col++] = empty;
			return true;
		}
		return false;
	}
}

CursorPos view_cursor_getpos(View *view) {
	Cursor *cursor = &view->cursor;
	Line *line = cursor->line;
	CursorPos pos = { .line = line->lineno, .col = cursor->col };
	while (line->prev && line->prev->lineno == pos.line) {
		line = line->prev;
		pos.col += line->width;
	}
	pos.col++;
	return pos;
}

/* snyc current cursor position with internal Line/Cell structures */
static void view_cursor_sync(View *view) {
	int row = 0, col = 0;
	size_t cur = view->start, pos = view->cursor.pos;
	Line *line = view->topline;

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

	view->cursor.line = line;
	view->cursor.row = row;
	view->cursor.col = col;
}

/* place the cursor according to the screen coordinates in view->{row,col} and
 * fire user callback. if a selection is active, redraw the view to reflect
 * its changes. */
static size_t view_cursor_update(View *view) {
	Cursor *cursor = &view->cursor;
	if (view->sel.start != EPOS) {
		view->sel.end = cursor->pos;
		view_draw(view);
	} else if (view->ui && view->syntax) {
		size_t pos = cursor->pos;
		size_t pos_match = text_bracket_match_except(view->text, pos, "<>");
		if (pos != pos_match && view->start <= pos_match && pos_match < view->end) {
			if (cursor->highlighted)
				view_draw(view); /* clear active highlighting */
			cursor->pos = pos_match;
			view_cursor_sync(view);
			cursor->line->cells[cursor->col].attr |= A_REVERSE;
			cursor->pos = pos;
			view_cursor_sync(view);
			view->ui->draw_text(view->ui, view->topline);
			cursor->highlighted = true;
		} else if (cursor->highlighted) {
			cursor->highlighted = false;
			view_draw(view);
		}
	}
	if (cursor->pos != cursor->lastpos)
		cursor->lastcol = 0;
	cursor->lastpos = cursor->pos;
	if (view->ui)
		view->ui->cursor_to(view->ui, cursor->col, cursor->row);
	return cursor->pos;
}

/* move the cursor to the character at pos bytes from the begining of the file.
 * if pos is not in the current viewport, redraw the view to make it visible */
void view_cursor_to(View *view, size_t pos) {
	size_t max = text_size(view->text);

	if (pos > max)
		pos = max > 0 ? max - 1 : 0;

	if (pos == max && view->end != max) {
		/* do not display an empty screen when shoviewg the end of the file */
		view->start = max - 1;
		view_viewport_up(view, view->height / 2);
	} else {
		/* set the start of the viewable region to the start of the line on which
		 * the cursor should be placed. if this line requires more space than
		 * available in the view then simply start displaying text at the new
		 * cursor position */
		for (int i = 0;  i < 2 && (pos < view->start || pos > view->end); i++) {
			view->start = i == 0 ? text_line_begin(view->text, pos) : pos;
			view_draw(view);
		}
	}

	view->cursor.pos = pos;
	view_cursor_sync(view);
	view_cursor_update(view);
}

/* redraw the complete with data starting from view->start bytes into the file.
 * stop once the screen is full, update view->end, view->lastline */
void view_draw(View *view) {
	view_clear(view);
	/* current absolute file position */
	size_t pos = view->start;
	/* number of bytes to read in one go */
	size_t text_len = view->width * view->height;
	/* current buffer to work with */
	char text[text_len+1];
	/* remaining bytes to process in buffer*/
	size_t rem = text_bytes_get(view->text, pos, text_len, text);
	/* NUL terminate because regex(3) function expect it */
	text[rem] = '\0';
	/* current position into buffer from which to interpret a character */
	char *cur = text;
	/* current selection */
	Filerange sel = view_selection_get(view);
	/* syntax definition to use */
	Syntax *syntax = view->syntax;
	/* matched tokens for each syntax rule */
	regmatch_t match[syntax ? LENGTH(syntax->rules) : 1][1], *matched = NULL;
	memset(match, 0, sizeof match);
	/* default and current curses attributes to use */
	int default_attrs = COLOR_PAIR(0) | A_NORMAL, attrs = default_attrs;

	while (rem > 0) {

		/* current 'parsed' character' */
		wchar_t wchar;
		Cell cell;
		memset(&cell, 0, sizeof cell);
	
		if (syntax) {
			if (matched && cur >= text + matched->rm_eo) {
				/* end of current match */
				matched = NULL;
				attrs = default_attrs;
				for (int i = 0; i < LENGTH(syntax->rules); i++) {
					if (match[i][0].rm_so == -1)
						continue; /* no match on whole text */
					/* reset matches which overlap with matched */
					if (text + match[i][0].rm_so <= cur && cur < text + match[i][0].rm_eo) {
						match[i][0].rm_so = 0;
						match[i][0].rm_eo = 0;
					}
				}
			}

			if (!matched) {
				size_t off = cur - text; /* number of already processed bytes */
				for (int i = 0; i < LENGTH(syntax->rules); i++) {
					SyntaxRule *rule = &syntax->rules[i];
					if (!rule->rule)
						break;
					if (match[i][0].rm_so == -1)
						continue; /* no match on whole text */
					if (off >= (size_t)match[i][0].rm_eo) {
						/* past match, continue search from current position */
						if (regexec(&rule->regex, cur, 1, match[i], 0) ||
						    match[i][0].rm_so == match[i][0].rm_eo) {
							match[i][0].rm_so = -1;
							match[i][0].rm_eo = -1;
							continue;
						}
						match[i][0].rm_so += off;
						match[i][0].rm_eo += off;
					}

					if (text + match[i][0].rm_so <= cur && cur < text + match[i][0].rm_eo) {
						/* within matched expression */
						matched = &match[i][0];
						attrs = rule->color->attr;
						break; /* first match views */
					}
				}
			}
		}

		size_t len = mbrtowc(&wchar, cur, rem, NULL);
		if (len == (size_t)-1 && errno == EILSEQ) {
			/* ok, we encountered an invalid multibyte sequence,
			 * replace it with the Unicode Replacement Character
			 * (FFFD) and skip until the start of the next utf8 char */
			for (len = 1; rem > len && !ISUTF8(cur[len]); len++);
			cell = (Cell){ .data = "\xEF\xBF\xBD", .len = len, .width = 1, .istab = false };
		} else if (len == (size_t)-2) {
			/* not enough bytes available to convert to a
			 * wide character. advance file position and read
			 * another junk into buffer.
			 */
			rem = text_bytes_get(view->text, pos, text_len, text);
			text[rem] = '\0';
			cur = text;
			continue;
		} else if (len == 0) {
			/* NUL byte encountered, store it and continue */
			cell = (Cell){ .data = "\x00", .len = 1, .width = 0, .istab = false };
		} else {
			for (size_t i = 0; i < len; i++)
				cell.data[i] = cur[i];
			cell.data[len] = '\0';
			cell.istab = false;
			cell.len = len;
			cell.width = wcwidth(wchar);
			if (cell.width == -1)
				cell.width = 1;
		}

		if (cur[0] == '\r' && rem > 1 && cur[1] == '\n') {
			/* convert views style newline \r\n into a single char with len = 2 */
			cell = (Cell){ .data = "\n", .len = 2, .width = 1, .istab = false };
		}

		cell.attr = attrs;
		if (sel.start <= pos && pos < sel.end)
			cell.attr |= A_REVERSE;
		if (!view_addch(view, &cell))
			break;

 		rem -= cell.len;
		cur += cell.len;
		pos += cell.len;
	}

	/* set end of vieviewg region */
	view->end = pos;
	view->lastline = view->line ? view->line : view->bottomline;
	view->lastline->next = NULL;
	view_cursor_sync(view);
	if (view->ui)
		view->ui->draw_text(view->ui, view->topline);
	if (sel.start != EPOS && view->events && view->events->selection)
		view->events->selection(view->events->data, &sel);
}

bool view_resize(View *view, int width, int height) {
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
	if (view->lines)
		memset(view->lines, 0, view->lines_size);
	view_draw(view);
	return true;
}

int view_height_get(View *view) {
	return view->height;
}

void view_free(View *view) {
	if (!view)
		return;
	free(view->lines);
	free(view);
}

void view_reload(View *view, Text *text) {
	view->text = text;
	view_selection_clear(view);
	view_cursor_to(view, 0);
	if (view->ui)
		view->ui->reload(view->ui, text);
}

View *view_new(Text *text, ViewEvent *events) {
	if (!text)
		return NULL;
	View *view = calloc(1, sizeof(View));
	if (!view)
		return NULL;
	view->text = text;
	view->events = events;
	view->tabwidth = 8;
	
	if (!view_resize(view, 1, 1)) {
		view_free(view);
		return NULL;
	}
	view_selection_clear(view);
	view_cursor_to(view, 0);

	return view;
}

void view_ui(View *view, UiWin* ui) {
	view->ui = ui;
}
size_t view_char_prev(View *view) {
	Cursor *cursor = &view->cursor;
	Line *line = cursor->line;

	do {
		if (cursor->col == 0) {
			if (!line->prev)
				return cursor->pos;
			cursor->line = line = line->prev;
			cursor->col = MIN(line->width, view->width - 1);
			cursor->row--;
		} else {
			cursor->col--;
		}
	} while (line->cells[cursor->col].len == 0);

	cursor->pos -= line->cells[cursor->col].len;
	return view_cursor_update(view);
}

size_t view_char_next(View *view) {
	Cursor *cursor = &view->cursor;
	Line *line = cursor->line;

	do {
		cursor->pos += line->cells[cursor->col].len;
		if ((line->width == view->width && cursor->col == view->width - 1) ||
		    cursor->col == line->width) {
			if (!line->next)
				return cursor->pos;
			cursor->line = line = line->next;
			cursor->row++;
			cursor->col = 0;
		} else {
			cursor->col++;
		}
	} while (line->cells[cursor->col].len == 0);

	return view_cursor_update(view);
}

static size_t view_cursor_set(View *view, Line *line, int col) {
	int row = 0;
	size_t pos = view->start;
	Cursor *cursor = &view->cursor;
	/* get row number and file offset at start of the given line */
	for (Line *cur = view->topline; cur && cur != line; cur = cur->next) {
		pos += cur->len;
		row++;
	}

	/* for characters which use more than 1 column, make sure we are on the left most */
	while (col > 0 && line->cells[col].len == 0)
		col--;
	while (col < line->width && line->cells[col].istab)
		col++;

	/* calculate offset within the line */
	for (int i = 0; i < col; i++)
		pos += line->cells[i].len;

	cursor->col = col;
	cursor->row = row;
	cursor->pos = pos;
	cursor->line = line;

	view_cursor_update(view);

	return pos;
}

static bool view_viewport_down(View *view, int n) {
	Line *line;
	if (view->end == text_size(view->text))
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
	if (c == '\r' && text_iterator_byte_prev(&it, &c))
		off++;
	do {
		if (c == '\n' && --n == 0)
			break;
		if (++off > max)
			break;
	} while (text_iterator_byte_prev(&it, &c));
	if (c == '\r')
		off++;
	view->start -= off;
	view_draw(view);
	return true;
}

void view_redraw_top(View *view) {
	Line *line = view->cursor.line;
	for (Line *cur = view->topline; cur && cur != line; cur = cur->next)
		view->start += cur->len;
	view_draw(view);
	view_cursor_to(view, view->cursor.pos);
}

void view_redraw_center(View *view) {
	int center = view->height / 2;
	size_t pos = view->cursor.pos;
	for (int i = 0; i < 2; i++) {
		int linenr = 0;
		Line *line = view->cursor.line;
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
	Line *line = view->cursor.line;
	if (line == view->lastline)
		return;
	int linenr = 0;
	size_t pos = view->cursor.pos;
	for (Line *cur = view->topline; cur && cur != line; cur = cur->next)
		linenr++;
	view_slide_down(view, view->height - linenr - 1);
	view_cursor_to(view, pos);
}

size_t view_slide_up(View *view, int lines) {
	Cursor *cursor = &view->cursor;
	if (view_viewport_down(view, lines)) {
		if (cursor->line == view->topline)
			view_cursor_set(view, view->topline, cursor->col);
		else
			view_cursor_to(view, cursor->pos);
	} else {
		view_screenline_down(view);
	}
	return cursor->pos;
}

size_t view_slide_down(View *view, int lines) {
	Cursor *cursor = &view->cursor;
	if (view_viewport_up(view, lines)) {
		if (cursor->line == view->lastline)
			view_cursor_set(view, view->lastline, cursor->col);
		else
			view_cursor_to(view, cursor->pos);
	} else {
		view_screenline_up(view);
	}
	return cursor->pos;
}

size_t view_scroll_up(View *view, int lines) {
	Cursor *cursor = &view->cursor;
	if (view_viewport_up(view, lines)) {
		Line *line = cursor->line < view->lastline ? cursor->line : view->lastline;
		view_cursor_set(view, line, view->cursor.col);
	} else {
		view_cursor_to(view, 0);
	}
	return cursor->pos;
}

size_t view_scroll_down(View *view, int lines) {
	Cursor *cursor = &view->cursor;
	if (view_viewport_down(view, lines)) {
		Line *line = cursor->line > view->topline ? cursor->line : view->topline;
		view_cursor_set(view, line, cursor->col);
	} else {
		view_cursor_to(view, text_size(view->text));
	}
	return cursor->pos;
}

size_t view_line_up(View *view) {
	Cursor *cursor = &view->cursor;
	if (cursor->line->prev && cursor->line->prev->prev &&
	    cursor->line->lineno != cursor->line->prev->lineno &&
	    cursor->line->prev->lineno != cursor->line->prev->prev->lineno)
		return view_screenline_up(view);
	size_t bol = text_line_begin(view->text, cursor->pos);
	size_t prev = text_line_prev(view->text, bol);
	size_t pos = text_line_offset(view->text, prev, cursor->pos - bol);
	view_cursor_to(view, pos);
	return cursor->pos;
}

size_t view_line_down(View *view) {
	Cursor *cursor = &view->cursor;
	if (!cursor->line->next || cursor->line->next->lineno != cursor->line->lineno)
		return view_screenline_down(view);
	size_t bol = text_line_begin(view->text, cursor->pos);
	size_t next = text_line_next(view->text, bol);
	size_t pos = text_line_offset(view->text, next, cursor->pos - bol);
	view_cursor_to(view, pos);
	return cursor->pos;
}

size_t view_screenline_up(View *view) {
	Cursor *cursor = &view->cursor;
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	if (!cursor->line->prev)
		view_scroll_up(view, 1);
	if (cursor->line->prev)
		view_cursor_set(view, cursor->line->prev, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_screenline_down(View *view) {
	Cursor *cursor = &view->cursor;
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	if (!cursor->line->next && cursor->line == view->bottomline)
		view_scroll_down(view, 1);
	if (cursor->line->next)
		view_cursor_set(view, cursor->line->next, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_screenline_begin(View *view) {
	return view_cursor_set(view, view->cursor.line, 0);
}

size_t view_screenline_middle(View *view) {
	Cursor *cursor = &view->cursor;
	return view_cursor_set(view, cursor->line, cursor->line->width / 2);
}

size_t view_screenline_end(View *view) {
	Cursor *cursor = &view->cursor;
	int col = cursor->line->width - 1;
	return view_cursor_set(view, cursor->line, col >= 0 ? col : 0);
}

size_t view_delete_key(View *view) {
	Cursor *cursor = &view->cursor;
	Line *line = cursor->line;
	size_t len = line->cells[cursor->col].len;
	text_delete(view->text, cursor->pos, len);
	view_draw(view);
	view_cursor_to(view, cursor->pos);
	return cursor->pos;
}

size_t view_backspace_key(View *view) {
	Cursor *cursor = &view->cursor;
	if (view->start == cursor->pos) {
		if (view->start == 0)
			return cursor->pos;
		/* if we are on the top left most position in the view
		 * first scroll up so that the to be deleted character is
		 * visible then proceed as normal */
		size_t pos = cursor->pos;
		view_viewport_up(view, 1);
		view_cursor_to(view, pos);
	}
	view_char_prev(view);
	size_t pos = cursor->pos;
	size_t len = cursor->line->cells[cursor->col].len;
	text_delete(view->text, pos, len);
	view_draw(view);
	view_cursor_to(view, pos);
	return pos;
}

size_t view_insert_key(View *view, const char *c, size_t len) {
	size_t pos = view->cursor.pos;
	text_insert(view->text, pos, c, len);
	if (view->cursor.line == view->bottomline && memchr(c, '\n', len))
		view_viewport_down(view, 1);
	else
		view_draw(view);
	pos += len;
	view_cursor_to(view, pos);
	return pos;
}

size_t view_replace_key(View *view, const char *c, size_t len) {
	Cursor *cursor = &view->cursor;
	Line *line = cursor->line;
	size_t pos = cursor->pos;
	/* do not overwrite new line which would merge the two lines */
	if (line->cells[cursor->col].data[0] != '\n') {
		size_t oldlen = line->cells[cursor->col].len;
		text_delete(view->text, pos, oldlen);
	}
	text_insert(view->text, pos, c, len);
	if (cursor->line == view->bottomline && memchr(c, '\n', len))
		view_viewport_down(view, 1);
	else
		view_draw(view);
	pos += len;
	view_cursor_to(view, pos);
	return pos;
}

size_t view_cursor_get(View *view) {
	return view->cursor.pos;
}

const Line *view_lines_get(View *view) {
	return view->topline;
}

void view_scroll_to(View *view, size_t pos) {
	while (pos < view->start && view_viewport_up(view, 1));
	while (pos > view->end && view_viewport_down(view, 1));
	view_cursor_to(view, pos);
}

void view_selection_start(View *view) {
	if (view->sel.start != EPOS && view->sel.end != EPOS)
		return;
	size_t pos = view_cursor_get(view);
	view->sel.start = view->sel.end = pos;
	view_draw(view);
	view_cursor_to(view, pos);
}

void view_syntax_set(View *view, Syntax *syntax) {
	view->syntax = syntax;
}

Syntax *view_syntax_get(View *view) {
	return view->syntax;
}

size_t view_screenline_goto(View *view, int n) {
	size_t pos = view->start;
	for (Line *line = view->topline; --n > 0 && line != view->lastline; line = line->next)
		pos += line->len;
	return pos;
}
