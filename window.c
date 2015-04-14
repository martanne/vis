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
#include "window.h"
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

struct Win {                /* window showing part of a file */
	Text *text;         /* underlying text management */
	UiWin *ui;
	ViewEvent *events;
	int width, height;  /* window text area size */
	Filepos start, end; /* currently displayed area [start, end] in bytes from the start of the file */
	size_t lines_size;  /* number of allocated bytes for lines (grows only) */
	Line *lines;        /* win->height number of lines representing window content */
	Line *topline;      /* top of the window, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of screen, might be unused if lastline < bottomline */
	Filerange sel;      /* selected text range in bytes from start of file */
	Cursor cursor;      /* current window cursor position */
	Line *line;         /* used while drawing window content, line where next char will be drawn */
	int col;            /* used while drawing window content, column where next char will be drawn */
	Syntax *syntax;     /* syntax highlighting definitions for this window or NULL */
	int tabwidth;       /* how many spaces should be used to display a tab character */
};

static void window_clear(Win *win);
static bool window_addch(Win *win, Cell *cell);
static size_t window_cursor_update(Win *win);
/* set/move current cursor position to a given (line, column) pair */
static size_t window_cursor_set(Win *win, Line *line, int col);
/* move visible viewport n-lines up/down, redraws the window but does not change
 * cursor position which becomes invalid and should be corrected by either:
 *
 *   - window_cursor_to
 *   - window_cursor_set
 *
 * the return value indicates wether the visible area changed.
 */
static bool window_viewport_up(Win *win, int n);
static bool window_viewport_down(Win *win, int n);

void window_tabwidth_set(Win *win, int tabwidth) {
	win->tabwidth = tabwidth;
	window_draw(win);
}

void window_selection_clear(Win *win) {
	win->sel = text_range_empty();
	window_draw(win);
	window_cursor_update(win);
}

/* reset internal window data structures (cell matrix, line offsets etc.) */
static void window_clear(Win *win) {
	/* calculate line number of first line */
	// TODO move elsewhere
	win->topline = win->lines;
	win->topline->lineno = text_lineno_by_pos(win->text, win->start);
	win->lastline = win->topline;

	/* reset all other lines */
	size_t line_size = sizeof(Line) + win->width*sizeof(Cell);
	size_t end = win->height * line_size;
	Line *prev = NULL;
	for (size_t i = 0; i < end; i += line_size) {
		Line *line = (Line*)(((char*)win->lines) + i);
		line->width = 0;
		line->len = 0;
		line->prev = prev;
		if (prev)
			prev->next = line;
		prev = line;
	}
	win->bottomline = prev ? prev : win->topline;
	win->bottomline->next = NULL;
	win->line = win->topline;
	win->col = 0;
}

Filerange window_selection_get(Win *win) {
	Filerange sel = win->sel;
	if (sel.start > sel.end) {
		size_t tmp = sel.start;
		sel.start = sel.end;
		sel.end = tmp;
	}
	if (!text_range_valid(&sel))
		return text_range_empty();
	sel.end = text_char_next(win->text, sel.end);
	return sel;
}

void window_selection_set(Win *win, Filerange *sel) {
	Cursor *cursor = &win->cursor;
	win->sel = *sel;
	window_draw(win);
	if (win->ui)
		win->ui->cursor_to(win->ui, cursor->col, cursor->row);
}

Filerange window_viewport_get(Win *win) {
	return (Filerange){ .start = win->start, .end = win->end };
}

/* try to add another character to the window, return whether there was space left */
static bool window_addch(Win *win, Cell *cell) {
	if (!win->line)
		return false;

	int width;
	static Cell empty;
	size_t lineno = win->line->lineno;

	switch (cell->data[0]) {
	case '\t':
		width = win->tabwidth - (win->col % win->tabwidth);
		for (int w = 0; w < width; w++) {
			if (win->col + 1 > win->width) {
				win->line = win->line->next;
				win->col = 0;
				if (!win->line)
					return false;
				win->line->lineno = lineno;
			}
			if (w == 0) {
				/* first cell of a tab has a length of 1 */
				win->line->cells[win->col].len = cell->len;
				win->line->len += cell->len;
			} else {
				/* all remaining ones have a lenght of zero */
				win->line->cells[win->col].len = 0;
			}
			/* but all are marked as part of a tabstop */
			win->line->cells[win->col].width = 1;
			win->line->cells[win->col].data[0] = ' ';
			win->line->cells[win->col].data[1] = '\0';
			win->line->cells[win->col].istab = true;
			win->line->cells[win->col].attr = cell->attr;
			win->line->width++;
			win->col++;
		}
		return true;
	case '\n':
		cell->width = 1;
		if (win->col + cell->width > win->width) {
			win->line = win->line->next;
			win->col = 0;
			if (!win->line)
				return false;
			win->line->lineno = lineno;
		}
		win->line->cells[win->col] = *cell;
		win->line->len += cell->len;
		win->line->width += cell->width;
		for (int i = win->col + 1; i < win->width; i++)
			win->line->cells[i] = empty;

		win->line = win->line->next;
		if (win->line)
			win->line->lineno = lineno + 1;
		win->col = 0;
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

		if (win->col + cell->width > win->width) {
			for (int i = win->col; i < win->width; i++)
				win->line->cells[i] = empty;
			win->line = win->line->next;
			win->col = 0;
		}

		if (win->line) {
			win->line->width += cell->width;
			win->line->len += cell->len;
			win->line->lineno = lineno;
			win->line->cells[win->col] = *cell;
			win->col++;
			/* set cells of a character which uses multiple columns */
			for (int i = 1; i < cell->width; i++)
				win->line->cells[win->col++] = empty;
			return true;
		}
		return false;
	}
}

CursorPos window_cursor_getpos(Win *win) {
	Cursor *cursor = &win->cursor;
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
static void window_cursor_sync(Win *win) {
	int row = 0, col = 0;
	size_t cur = win->start, pos = win->cursor.pos;
	Line *line = win->topline;

	while (line && line != win->lastline && cur < pos) {
		if (cur + line->len > pos)
			break;
		cur += line->len;
		line = line->next;
		row++;
	}

	if (line) {
		int max_col = MIN(win->width, line->width);
		while (cur < pos && col < max_col) {
			cur += line->cells[col].len;
			/* skip over columns occupied by the same character */
			while (++col < max_col && line->cells[col].len == 0);
		}
	} else {
		line = win->bottomline;
		row = win->height - 1;
	}

	win->cursor.line = line;
	win->cursor.row = row;
	win->cursor.col = col;
}

/* place the cursor according to the screen coordinates in win->{row,col} and
 * fire user callback. if a selection is active, redraw the window to reflect
 * its changes. */
static size_t window_cursor_update(Win *win) {
	Cursor *cursor = &win->cursor;
	if (win->sel.start != EPOS) {
		win->sel.end = cursor->pos;
		window_draw(win);
	} else if (win->ui && win->syntax) {
		size_t pos = cursor->pos;
		size_t pos_match = text_bracket_match_except(win->text, pos, "<>");
		if (pos != pos_match && win->start <= pos_match && pos_match < win->end) {
			if (cursor->highlighted)
				window_draw(win); /* clear active highlighting */
			cursor->pos = pos_match;
			window_cursor_sync(win);
			cursor->line->cells[cursor->col].attr |= A_REVERSE;
			cursor->pos = pos;
			window_cursor_sync(win);
			win->ui->draw_text(win->ui, win->topline);
			cursor->highlighted = true;
		} else if (cursor->highlighted) {
			cursor->highlighted = false;
			window_draw(win);
		}
	}
	if (cursor->pos != cursor->lastpos)
		cursor->lastcol = 0;
	cursor->lastpos = cursor->pos;
	if (win->ui)
		win->ui->cursor_to(win->ui, cursor->col, cursor->row);
	return cursor->pos;
}

/* move the cursor to the character at pos bytes from the begining of the file.
 * if pos is not in the current viewport, redraw the window to make it visible */
void window_cursor_to(Win *win, size_t pos) {
	size_t max = text_size(win->text);

	if (pos > max)
		pos = max > 0 ? max - 1 : 0;

	if (pos == max && win->end != max) {
		/* do not display an empty screen when showing the end of the file */
		win->start = max - 1;
		window_viewport_up(win, win->height / 2);
	} else {
		/* set the start of the viewable region to the start of the line on which
		 * the cursor should be placed. if this line requires more space than
		 * available in the window then simply start displaying text at the new
		 * cursor position */
		for (int i = 0;  i < 2 && (pos < win->start || pos > win->end); i++) {
			win->start = i == 0 ? text_line_begin(win->text, pos) : pos;
			window_draw(win);
		}
	}

	win->cursor.pos = pos;
	window_cursor_sync(win);
	window_cursor_update(win);
}

/* redraw the complete with data starting from win->start bytes into the file.
 * stop once the screen is full, update win->end, win->lastline */
void window_draw(Win *win) {
	window_clear(win);
	/* current absolute file position */
	size_t pos = win->start;
	/* number of bytes to read in one go */
	size_t text_len = win->width * win->height;
	/* current buffer to work with */
	char text[text_len+1];
	/* remaining bytes to process in buffer*/
	size_t rem = text_bytes_get(win->text, pos, text_len, text);
	/* NUL terminate because regex(3) function expect it */
	text[rem] = '\0';
	/* current position into buffer from which to interpret a character */
	char *cur = text;
	/* current selection */
	Filerange sel = window_selection_get(win);
	/* syntax definition to use */
	Syntax *syntax = win->syntax;
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
						break; /* first match wins */
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
			rem = text_bytes_get(win->text, pos, text_len, text);
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
			/* convert windows style newline \r\n into a single char with len = 2 */
			cell = (Cell){ .data = "\n", .len = 2, .width = 1, .istab = false };
		}

		cell.attr = attrs;
		if (sel.start <= pos && pos < sel.end)
			cell.attr |= A_REVERSE;
		if (!window_addch(win, &cell))
			break;

 		rem -= cell.len;
		cur += cell.len;
		pos += cell.len;
	}

	/* set end of viewing region */
	win->end = pos;
	win->lastline = win->line ? win->line : win->bottomline;
	win->lastline->next = NULL;
	window_cursor_sync(win);
	if (win->ui)
		win->ui->draw_text(win->ui, win->topline);
	if (sel.start != EPOS && win->events && win->events->selection)
		win->events->selection(win->events->data, &sel);
}

bool window_resize(Win *win, int width, int height) {
	size_t lines_size = height*(sizeof(Line) + width*sizeof(Cell));
	if (lines_size > win->lines_size) {
		Line *lines = realloc(win->lines, lines_size);
		if (!lines)
			return false;
		win->lines = lines;
		win->lines_size = lines_size;
	}
	win->width = width;
	win->height = height;
	if (win->lines)
		memset(win->lines, 0, win->lines_size);
	window_draw(win);
	return true;
}

int window_height_get(Win *win) {
	return win->height;
}

void window_free(Win *win) {
	if (!win)
		return;
	free(win->lines);
	free(win);
}

void window_reload(Win *win, Text *text) {
	win->text = text;
	window_selection_clear(win);
	window_cursor_to(win, 0);
	if (win->ui)
		win->ui->reload(win->ui, text);
}

Win *window_new(Text *text, ViewEvent *events) {
	if (!text)
		return NULL;
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->text = text;
	win->events = events;
	win->tabwidth = 8;
	
	if (!window_resize(win, 1, 1)) {
		window_free(win);
		return NULL;
	}
	window_selection_clear(win);
	window_cursor_to(win, 0);

	return win;
}

void window_ui(Win *win, UiWin* ui) {
	win->ui = ui;
}
size_t window_char_prev(Win *win) {
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;

	do {
		if (cursor->col == 0) {
			if (!line->prev)
				return cursor->pos;
			cursor->line = line = line->prev;
			cursor->col = MIN(line->width, win->width - 1);
			cursor->row--;
		} else {
			cursor->col--;
		}
	} while (line->cells[cursor->col].len == 0);

	cursor->pos -= line->cells[cursor->col].len;
	return window_cursor_update(win);
}

size_t window_char_next(Win *win) {
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;

	do {
		cursor->pos += line->cells[cursor->col].len;
		if ((line->width == win->width && cursor->col == win->width - 1) ||
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

	return window_cursor_update(win);
}

static size_t window_cursor_set(Win *win, Line *line, int col) {
	int row = 0;
	size_t pos = win->start;
	Cursor *cursor = &win->cursor;
	/* get row number and file offset at start of the given line */
	for (Line *cur = win->topline; cur && cur != line; cur = cur->next) {
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

	window_cursor_update(win);

	return pos;
}

static bool window_viewport_down(Win *win, int n) {
	Line *line;
	if (win->end == text_size(win->text))
		return false;
	if (n >= win->height) {
		win->start = win->end;
	} else {
		for (line = win->topline; line && n > 0; line = line->next, n--)
			win->start += line->len;
	}
	window_draw(win);
	return true;
}

static bool window_viewport_up(Win *win, int n) {
	/* scrolling up is somewhat tricky because we do not yet know where
	 * the lines start, therefore scan backwards but stop at a reasonable
	 * maximum in case we are dealing with a file without any newlines
	 */
	if (win->start == 0)
		return false;
	size_t max = win->width * win->height;
	char c;
	Iterator it = text_iterator_get(win->text, win->start - 1);

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
	win->start -= off;
	window_draw(win);
	return true;
}

void window_redraw_top(Win *win) {
	Line *line = win->cursor.line;
	for (Line *cur = win->topline; cur && cur != line; cur = cur->next)
		win->start += cur->len;
	window_draw(win);
	window_cursor_to(win, win->cursor.pos);
}

void window_redraw_center(Win *win) {
	int center = win->height / 2;
	size_t pos = win->cursor.pos;
	for (int i = 0; i < 2; i++) {
		int linenr = 0;
		Line *line = win->cursor.line;
		for (Line *cur = win->topline; cur && cur != line; cur = cur->next)
			linenr++;
		if (linenr < center) {
			window_slide_down(win, center - linenr);
			continue;
		}
		for (Line *cur = win->topline; cur && cur != line && linenr > center; cur = cur->next) {
			win->start += cur->len;
			linenr--;
		}
		break;
	}
	window_draw(win);
	window_cursor_to(win, pos);
}

void window_redraw_bottom(Win *win) {
	Line *line = win->cursor.line;
	if (line == win->lastline)
		return;
	int linenr = 0;
	size_t pos = win->cursor.pos;
	for (Line *cur = win->topline; cur && cur != line; cur = cur->next)
		linenr++;
	window_slide_down(win, win->height - linenr - 1);
	window_cursor_to(win, pos);
}

size_t window_slide_up(Win *win, int lines) {
	Cursor *cursor = &win->cursor;
	if (window_viewport_down(win, lines)) {
		if (cursor->line == win->topline)
			window_cursor_set(win, win->topline, cursor->col);
		else
			window_cursor_to(win, cursor->pos);
	} else {
		window_screenline_down(win);
	}
	return cursor->pos;
}

size_t window_slide_down(Win *win, int lines) {
	Cursor *cursor = &win->cursor;
	if (window_viewport_up(win, lines)) {
		if (cursor->line == win->lastline)
			window_cursor_set(win, win->lastline, cursor->col);
		else
			window_cursor_to(win, cursor->pos);
	} else {
		window_screenline_up(win);
	}
	return cursor->pos;
}

size_t window_scroll_up(Win *win, int lines) {
	Cursor *cursor = &win->cursor;
	if (window_viewport_up(win, lines)) {
		Line *line = cursor->line < win->lastline ? cursor->line : win->lastline;
		window_cursor_set(win, line, win->cursor.col);
	} else {
		window_cursor_to(win, 0);
	}
	return cursor->pos;
}

size_t window_scroll_down(Win *win, int lines) {
	Cursor *cursor = &win->cursor;
	if (window_viewport_down(win, lines)) {
		Line *line = cursor->line > win->topline ? cursor->line : win->topline;
		window_cursor_set(win, line, cursor->col);
	} else {
		window_cursor_to(win, text_size(win->text));
	}
	return cursor->pos;
}

size_t window_line_up(Win *win) {
	Cursor *cursor = &win->cursor;
	if (cursor->line->prev && cursor->line->prev->prev &&
	    cursor->line->lineno != cursor->line->prev->lineno &&
	    cursor->line->prev->lineno != cursor->line->prev->prev->lineno)
		return window_screenline_up(win);
	size_t bol = text_line_begin(win->text, cursor->pos);
	size_t prev = text_line_prev(win->text, bol);
	size_t pos = text_line_offset(win->text, prev, cursor->pos - bol);
	window_cursor_to(win, pos);
	return cursor->pos;
}

size_t window_line_down(Win *win) {
	Cursor *cursor = &win->cursor;
	if (!cursor->line->next || cursor->line->next->lineno != cursor->line->lineno)
		return window_screenline_down(win);
	size_t bol = text_line_begin(win->text, cursor->pos);
	size_t next = text_line_next(win->text, bol);
	size_t pos = text_line_offset(win->text, next, cursor->pos - bol);
	window_cursor_to(win, pos);
	return cursor->pos;
}

size_t window_screenline_up(Win *win) {
	Cursor *cursor = &win->cursor;
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	if (!cursor->line->prev)
		window_scroll_up(win, 1);
	if (cursor->line->prev)
		window_cursor_set(win, cursor->line->prev, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t window_screenline_down(Win *win) {
	Cursor *cursor = &win->cursor;
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	if (!cursor->line->next && cursor->line == win->bottomline)
		window_scroll_down(win, 1);
	if (cursor->line->next)
		window_cursor_set(win, cursor->line->next, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t window_screenline_begin(Win *win) {
	return window_cursor_set(win, win->cursor.line, 0);
}

size_t window_screenline_middle(Win *win) {
	Cursor *cursor = &win->cursor;
	return window_cursor_set(win, cursor->line, cursor->line->width / 2);
}

size_t window_screenline_end(Win *win) {
	Cursor *cursor = &win->cursor;
	int col = cursor->line->width - 1;
	return window_cursor_set(win, cursor->line, col >= 0 ? col : 0);
}

size_t window_delete_key(Win *win) {
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;
	size_t len = line->cells[cursor->col].len;
	text_delete(win->text, cursor->pos, len);
	window_draw(win);
	window_cursor_to(win, cursor->pos);
	return cursor->pos;
}

size_t window_backspace_key(Win *win) {
	Cursor *cursor = &win->cursor;
	if (win->start == cursor->pos) {
		if (win->start == 0)
			return cursor->pos;
		/* if we are on the top left most position in the window
		 * first scroll up so that the to be deleted character is
		 * visible then proceed as normal */
		size_t pos = cursor->pos;
		window_viewport_up(win, 1);
		window_cursor_to(win, pos);
	}
	window_char_prev(win);
	size_t pos = cursor->pos;
	size_t len = cursor->line->cells[cursor->col].len;
	text_delete(win->text, pos, len);
	window_draw(win);
	window_cursor_to(win, pos);
	return pos;
}

size_t window_insert_key(Win *win, const char *c, size_t len) {
	size_t pos = win->cursor.pos;
	text_insert(win->text, pos, c, len);
	if (win->cursor.line == win->bottomline && memchr(c, '\n', len))
		window_viewport_down(win, 1);
	else
		window_draw(win);
	pos += len;
	window_cursor_to(win, pos);
	return pos;
}

size_t window_replace_key(Win *win, const char *c, size_t len) {
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;
	size_t pos = cursor->pos;
	/* do not overwrite new line which would merge the two lines */
	if (line->cells[cursor->col].data[0] != '\n') {
		size_t oldlen = line->cells[cursor->col].len;
		text_delete(win->text, pos, oldlen);
	}
	text_insert(win->text, pos, c, len);
	if (cursor->line == win->bottomline && memchr(c, '\n', len))
		window_viewport_down(win, 1);
	else
		window_draw(win);
	pos += len;
	window_cursor_to(win, pos);
	return pos;
}

size_t window_cursor_get(Win *win) {
	return win->cursor.pos;
}

const Line *window_lines_get(Win *win) {
	return win->topline;
}

void window_scroll_to(Win *win, size_t pos) {
	while (pos < win->start && window_viewport_up(win, 1));
	while (pos > win->end && window_viewport_down(win, 1));
	window_cursor_to(win, pos);
}

void window_selection_start(Win *win) {
	if (win->sel.start != EPOS && win->sel.end != EPOS)
		return;
	size_t pos = window_cursor_get(win);
	win->sel.start = win->sel.end = pos;
	window_draw(win);
	window_cursor_to(win, pos);
}

void window_syntax_set(Win *win, Syntax *syntax) {
	win->syntax = syntax;
}

Syntax *window_syntax_get(Win *win) {
	return win->syntax;
}

size_t window_screenline_goto(Win *win, int n) {
	size_t pos = win->start;
	for (Line *line = win->topline; --n > 0 && line != win->lastline; line = line->next)
		pos += line->len;
	return pos;
}
