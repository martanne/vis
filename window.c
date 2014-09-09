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
#define _XOPEN_SOURCE
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include "editor.h"
#include "window.h"
#include "text.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"

typedef struct {            /* used to calculate the display width of a character */
	char c[7];	    /* utf8 encoded bytes */
	size_t len;	    /* number of bytes of the multibyte sequence */
	wchar_t wchar;	    /* equivalent converted wide character, needed for wcwidth(3) */
} Char;

typedef struct {
	unsigned char len;  /* number of bytes the character displayed in this cell uses, for
	                       character which use more than 1 column to display, their lenght
	                       is stored in the leftmost cell wheras all following cells
	                       occupied by the same character have a length of 0. */
	char data;          /* first byte of the utf8 sequence, used for white space handling */
} Cell;

typedef struct Line Line;
struct Line {               /* a line on the screen, *not* in the file */
	Line *prev, *next;  /* pointer to neighbouring screen lines */
	size_t len;         /* line length in terms of bytes */
	size_t lineno;      /* line number from start of file */
	int width;          /* zero based position of last used column cell */
	Cell cells[];       /* win->width cells storing information about the displayed characters */
};

typedef struct {            /* cursor position */
	Filepos pos;        /* in bytes from the start of the file */
	int row, col;       /* in terms of zero based screen coordinates */
	Line *line;         /* screen line on which cursor currently resides */
} Cursor;

typedef struct Win Win;
struct Win {                /* window showing part of a file */
	Text *text;         /* underlying text management */
	WINDOW *win;        /* curses window for the text area */
	int width, height;  /* window text area size */
	Filepos start, end; /* currently displayed area [start, end] in bytes from the start of the file */
	Line *lines;        /* win->height number of lines representing window content */
	Line *topline;      /* top of the window, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of screen, might be unused if lastline < bottomline */
	Filerange sel;      /* selected text range in bytes from start of file */
	Cursor cursor;      /* current window cursor position */
	void (*cursor_moved)(Win*, void *);
	void *cursor_moved_data;

	Line *line;         // TODO: rename to something more descriptive, these are the current drawing pos
	int col;

	Syntax *syntax;     /* syntax highlighting definitions for this window or NULL */
	int tabwidth;       /* how many spaces should be used to display a tab character */
};

static void window_clear(Win *win);
static bool window_addch(Win *win, Char *c);
static size_t window_cursor_update(Win *win);
static size_t pos_by_line(Win *win, Line *line);
static size_t cursor_offset(Cursor *cursor);
static bool window_scroll_lines_down(Win *win, int n);
static bool window_scroll_lines_up(Win *win, int n);

void window_selection_clear(Win *win) {
	win->sel.start = win->sel.end = (size_t)-1;
	window_draw(win);
	window_cursor_update(win);
}

static void window_clear(Win *win) {
	size_t line_size = sizeof(Line) + win->width*sizeof(Cell);
	win->topline = win->lines;
	win->topline->lineno = text_lineno_by_pos(win->text, win->start);
	win->lastline = win->topline;
	Line *prev = NULL;
	for (int i = 0; i < win->height; i++) {
		Line *line = (Line*)(((char*)win->lines) + i*line_size);
		line->len = 0;
		line->width = 0;
		line->prev = prev;
		if (prev)
			prev->next = line;
		prev = line;
	}
	win->bottomline = prev;
	win->line = win->topline;
	win->col = 0;
}

Filerange window_selection_get(Win *win) {
	Filerange sel = win->sel;
	if (sel.start == (size_t)-1) {
		sel.end = (size_t)-1;
	} else if (sel.end == (size_t)-1) {
		sel.start = (size_t)-1;
	} else if (sel.start > sel.end) {
		size_t tmp = sel.start;
		sel.start = sel.end;
		sel.end = tmp;
	}
	return sel;
}

Filerange window_viewport_get(Win *win) {
	return (Filerange){ .start = win->start, .end = win->end };
}
/* try to add another character to the window, return whether there was space left */
static bool window_addch(Win *win, Char *c) {
	if (!win->line)
		return false;

	Cell empty = {};
	int width;
	size_t lineno = win->line->lineno;

	switch (c->wchar) {
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
				win->line->cells[win->col].len = c->len;
				win->line->len += c->len;
			} else {
				/* all remaining ones have a lenght of zero */
				win->line->cells[win->col].len = 0;
			}
			/* but all are marked as part of a tabstop */
			win->line->cells[win->col].data = '\t';
			win->col++;
			win->line->width++;
			waddch(win->win, ' ');
		}
		return true;
	case '\n':
		width = 1;
		if (win->col + width > win->width) {
			win->line = win->line->next;
			win->col = 0;
			if (!win->line)
				return false;
			win->line->lineno = lineno + 1;
		}
		win->line->cells[win->col].len = c->len;
		win->line->len += c->len;
		win->line->cells[win->col].data = '\n';
		for (int i = win->col + 1; i < win->width; i++)
			win->line->cells[i] = empty;

		if (win->line == win->bottomline) {
			/* XXX: curses bug? the wclrtoeol(win->win); implied by waddch(win->win, '\n')
			 * doesn't seem to work on the last line!?
			 *
			 * Thus explicitly clear the remaining of the line.
			 */
			for (int i = win->col; i < win->width; i++)
				waddch(win->win, ' ');
		} else if (win->line->width == 0) {
			/* add a single space in an otherwise empty line, makes selection cohorent */
			waddch(win->win, ' ');
		}

		waddch(win->win, '\n');
		win->line = win->line->next;
		if (win->line)
			win->line->lineno = lineno + 1;
		win->col = 0;
		return true;
	default:
		if (c->wchar < 128 && !isprint(c->wchar)) {
			/* non-printable ascii char, represent it as ^(char + 64) */
			Char s = { .c = "^_", .len = 1 };
			s.c[1] = c->c[0] + 64;
			*c = s;
			width = 2;
		} else {
			if ((width = wcwidth(c->wchar)) == -1) {
				/* this should never happen */
				width = 1;
			}
		}

		if (win->col + width > win->width) {
			for (int i = win->col; i < win->width; i++)
				win->line->cells[i] = empty;
			win->line = win->line->next;
			win->col = 0;
		}

		if (win->line) {
			win->line->width += width;
			win->line->len += c->len;
			win->line->lineno = lineno;
			win->line->cells[win->col].len = c->len;
			win->line->cells[win->col].data = c->c[0];
			win->col++;
			/* set cells of a character which uses multiple columns */
			for (int i = 1; i < width; i++)
				win->line->cells[win->col++] = empty;
			waddstr(win->win, c->c);
			return true;
		}
		return false;
	}
}

void window_cursor_getxy(Win *win, size_t *lineno, size_t *col) {
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;
	*lineno = line->lineno;
	*col = cursor->col;
	while (line->prev && line->prev->lineno == *lineno) {
		line = line->prev;
		*col += line->width;
	}
	*col += 1;
}

/* place the cursor according to the screen coordinates in win->{row,col} and
 * update the statusbar. if a selection is active, redraw the window to reflect
 * its changes. */
static size_t window_cursor_update(Win *win) {
	Cursor *cursor = &win->cursor;
	if (win->sel.start != (size_t)-1) {
		win->sel.end = cursor->pos;
		window_draw(win);
	}
	wmove(win->win, cursor->row, cursor->col);
	if (win->cursor_moved)
		win->cursor_moved(win, win->cursor_moved_data);
	return cursor->pos;
}

/* move the cursor to the character at pos bytes from the begining of the file.
 * if pos is not in the current viewport, redraw the window to make it visible */
void window_cursor_to(Win *win, size_t pos) {
	Line *line = win->topline;
	int row = 0;
	int col = 0;
	size_t max = text_size(win->text);

	if (pos > max)
		pos = max > 0 ? max - 1 : 0;

	if (pos == max && win->end != max) {
		/* do not display an empty screen when showing the end of the file */
		win->start = max - 1;
		window_scroll_lines_up(win, win->height / 2);
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

	size_t cur = win->start;
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
			col++;
		}
		while (col < max_col && line->cells[col].data == '\t')
			col++;
	} else {
		line = win->bottomline;
		row = win->height - 1;
	}

	win->cursor.line = line;
	win->cursor.row = row;
	win->cursor.col = col;
	win->cursor.pos = pos;
	window_cursor_update(win);
}

/* redraw the complete with data starting from win->start bytes into the file.
 * stop once the screen is full, update win->end, win->lastline */
void window_draw(Win *win) {
	window_clear(win);
	wmove(win->win, 0, 0);
	/* current absolute file position */
	size_t pos = win->start;
	/* number of bytes to read in one go */
	// TODO read smaller junks
	size_t text_len = win->width * win->height;
	/* current buffer to work with */
	char text[text_len+1];
	/* remaining bytes to process in buffer*/
	size_t rem = text_bytes_get(win->text, pos, text_len, text);
	/* NUL terminate because regex(3) function expect it */
	text[rem] = '\0';
	/* current position into buffer from which to interpret a character */
	char *cur = text;
	/* current 'parsed' character' */
	Char c;
	/* current selection */
	Filerange sel = window_selection_get(win);
	/* matched tokens for each syntax rule */
	regmatch_t match[SYNTAX_REGEX_RULES][1];
	if (win->syntax) {
		for (int i = 0; i < LENGTH(win->syntax->rules); i++) {
			SyntaxRule *rule = &win->syntax->rules[i];
			if (!rule->rule)
				break;
			if (regexec(&rule->regex, cur, 1, match[i], 0) ||
			    match[i][0].rm_so == match[i][0].rm_eo) {
				match[i][0].rm_so = -1;
				match[i][0].rm_eo = -1;
			}
		}
	}

	while (rem > 0) {

		int attrs = COLOR_PAIR(0) | A_NORMAL;

		if (win->syntax) {
			size_t off = cur - text; /* number of already processed bytes */
			for (int i = 0; i < LENGTH(win->syntax->rules); i++) {
				SyntaxRule *rule = &win->syntax->rules[i];
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
					attrs = rule->color.attr;
				}
			}
		}

		if (sel.start <= pos && pos < sel.end)
			attrs |= A_REVERSE; // TODO: make configurable

		size_t len = mbrtowc(&c.wchar, cur, rem, NULL);
		if (len == (size_t)-1 && errno == EILSEQ) {
			/* ok, we encountered an invalid multibyte sequence,
			 * replace it with the Unicode Replacement Character
			 * (FFFD) and skip until the start of the next utf8 char */
			for (len = 1; rem > len && !isutf8(cur[len]); len++);
			c = (Char){ .c = "\xEF\xBF\xBD", .wchar = 0xFFFD, .len = len };
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
			len = 1;
			c = (Char){ .c = "\x00", .wchar = 0x00, .len = len };
		} else {
			for (size_t i = 0; i < len; i++)
				c.c[i] = cur[i];
			c.c[len] = '\0';
			c.len = len;
		}

		if (cur[0] == '\n' && rem > 1 && cur[1] == '\r') {
			/* convert windows style newline \n\r into a single char with len = 2 */
			c.len = len = 2;
		}

		wattrset(win->win, attrs);
		if (!window_addch(win, &c))
			break;

 		rem -= len;
		cur += len;
		pos += len;
	}

	/* set end of viewing region */
	win->end = pos;
	win->lastline = win->line ? win->line : win->bottomline;
	win->lastline->next = NULL;
	/* and clear the rest of the unused window */
	wclrtobot(win->win);
}

bool window_resize(Win *win, int width, int height) {
	if (wresize(win->win, height, width) == ERR) 
		return false;
	// TODO: only grow memory area
	win->height = height;
	win->width = width;
	free(win->lines);
	size_t line_size = sizeof(Line) + width*sizeof(Cell);
	if (!(win->lines = calloc(height, line_size)))
		return false;
	return true;
}

void window_move(Win *win, int x, int y) {
	mvwin(win->win, y, x);
}

void window_free(Win *win) {
	if (!win)
		return;
	if (win->win)
		delwin(win->win);
	free(win->lines);
	free(win);
}

Win *window_new(Text *text) {
	if (!text)
		return NULL;
	Win *win = calloc(1, sizeof(Win));
	if (!win || !(win->win = newwin(0, 0, 0, 0))) {
		window_free(win);
		return NULL;
	}

	win->text = text;
	win->tabwidth = 8; // TODO make configurable

	int width, height;
	getmaxyx(win->win, height, width);
	if (!window_resize(win, width, height)) {
		window_free(win);
		return NULL;
	}

	window_selection_clear(win);
	window_cursor_to(win, 0);

	return win;
}

static size_t pos_by_line(Win *win, Line *line) {
	size_t pos = win->start;
	for (Line *cur = win->topline; cur && cur != line; cur = cur->next)
		pos += cur->len;
	return pos;
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

/* calculate the line offset in bytes of a given cursor position, used after
 * the cursor changes line. this assumes cursor->line already points to the new
 * line, but cursor->col still has the old column position of the previous
 * line based on which the new column position is calculated */
static size_t cursor_offset(Cursor *cursor) {
	Line *line = cursor->line;
	int col = cursor->col;
	int off = 0;
	/* for characters which use more than 1 column, make sure we are on the left most */
	while (col > 0 && line->cells[col].len == 0 && line->cells[col].data != '\t')
		col--;
	while (col < line->width && line->cells[col].data == '\t')
		col++;
	for (int i = 0; i < col; i++)
		off += line->cells[i].len;
	cursor->col = col;
	return off;
}

static bool window_scroll_lines_down(Win *win, int n) {
	Line *line;
	if (win->end == text_size(win->text))
		return false;
	for (line = win->topline; line && n > 0; line = line->next, n--)
		win->start += line->len;
	window_draw(win);
	/* try to place the cursor at the same column */
	Cursor *cursor = &win->cursor;
	cursor->pos = win->end - win->lastline->len + cursor_offset(cursor);
	window_cursor_update(win);
	return true;
}

static bool window_scroll_lines_up(Win *win, int n) {
	/* scrolling up is somewhat tricky because we do not yet know where
	 * the lines start, therefore scan backwards but stop at a reasonable
	 * maximum in case we are dealing with a file without any newlines
	 */
	if (win->start == 0)
		return false;
	size_t max = win->width * win->height / 2;
	char c;
	Iterator it = text_iterator_get(win->text, win->start - 1);

	if (!text_iterator_byte_get(&it, &c))
		return false;
	size_t off = 0;
	/* skip newlines immediately before display area */
	if (c == '\r' && text_iterator_byte_prev(&it, &c))
		off++;
	if (c == '\n' && text_iterator_byte_prev(&it, &c))
		off++;
	do {
		if ((c == '\n' || c == '\r') && --n == 0)
			break;
		if (++off > max)
			break;
	} while (text_iterator_byte_prev(&it, &c));
	win->start -= off;
	window_draw(win);
	Cursor *cursor = &win->cursor;
	cursor->pos = win->start + cursor_offset(cursor);
	window_cursor_update(win);
	return true;
}

size_t window_page_up(Win *win) {
	if (!window_scroll_lines_up(win, win->height))
		window_cursor_to(win, win->start);
	return win->cursor.pos;
}

size_t window_page_down(Win *win) {
	Cursor *cursor = &win->cursor;
	if (win->end == text_size(win->text)) {
		window_cursor_to(win, win->end);
		return win->end;
	}
	win->start = win->end;
	window_draw(win);
	int col = cursor->col;
	window_cursor_to(win, win->start);
	cursor->col = col;
	cursor->pos += cursor_offset(cursor);
	return window_cursor_update(win);
}

size_t window_line_up(Win *win) {
	Cursor *cursor = &win->cursor;
	if (!cursor->line->prev) {
		window_scroll_lines_up(win, 1);
		return cursor->pos;
	}
	cursor->row--;
	cursor->line = cursor->line->prev;
	cursor->pos = pos_by_line(win, cursor->line) + cursor_offset(cursor);
	return window_cursor_update(win);
}

size_t window_line_down(Win *win) {
	Cursor *cursor = &win->cursor;
	if (!cursor->line->next) {
		if (cursor->line == win->bottomline)
			window_scroll_lines_down(win, 1);
		return cursor->pos;
	}
	cursor->row++;
	cursor->line = cursor->line->next;
	cursor->pos = pos_by_line(win, cursor->line) + cursor_offset(cursor);
	return window_cursor_update(win);
}

void window_update(Win *win) {
	wnoutrefresh(win->win);
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
		window_scroll_lines_up(win, 1);
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
	text_insert_raw(win->text, pos, c, len);
	if (win->cursor.line == win->bottomline && memchr(c, '\n', len))
		window_scroll_lines_down(win, 1);
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
	if (line->cells[cursor->col].data != '\n') {
		size_t oldlen = line->cells[cursor->col].len;
		text_delete(win->text, pos, oldlen);
	}
	text_insert_raw(win->text, pos, c, len);
	if (cursor->line == win->bottomline && memchr(c, '\n', len))
		window_scroll_lines_down(win, 1);
	else
		window_draw(win);
	pos += len;
	window_cursor_to(win, pos);
	return pos;
}

size_t window_cursor_get(Win *win) {
	return win->cursor.pos;
}

void window_scroll_to(Win *win, size_t pos) {
	while (pos < win->start && window_scroll_lines_up(win, 1));
	while (pos > win->end && window_scroll_lines_down(win, 1));
	window_cursor_to(win, pos);
}

void window_selection_start(Win *win) {
	win->sel.start = window_cursor_get(win);
}

void window_selection_end(Win *win) {
	win->sel.end = window_cursor_get(win);
}

void window_syntax_set(Win *win, Syntax *syntax) {
	win->syntax = syntax;
}

Syntax *window_syntax_get(Win *win) {
	return win->syntax; 
}

void window_cursor_watch(Win *win, void (*cursor_moved)(Win*, void *), void *data) {
	win->cursor_moved = cursor_moved;
	win->cursor_moved_data = data;
}
