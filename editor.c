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
#include "text.h"
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
	Editor *editor;
	Text *text;         /* underlying text management */
	WINDOW *win;        /* curses window for the text area  */
	WINDOW *statuswin;  /* curses window for the statusbar */
	int width, height;  /* text area size *not* including the statusbar */
	Filepos start, end; /* currently displayed area [start, end] in bytes from the start of the file */
	Line *lines;        /* win->height number of lines representing window content */
	Line *topline;      /* top of the window, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of screen, might be unused if lastline < bottomline */
	Filerange sel;      /* selected text range in bytes from start of file */
	Cursor cursor;      /* current window cursor position */

	Line *line;         // TODO: rename to something more descriptive, these are the current drawing pos
	int col;

	Syntax *syntax;     /* syntax highlighting definitions for this window or NULL */
	int tabwidth;       /* how many spaces should be used to display a tab character */
	Win *prev, *next;   /* neighbouring windows */
};

struct Editor {
	int width, height;  /* terminal size, available for all windows */
	editor_statusbar_t statusbar;
	Win *windows;       /* list of windows */
	Win *win;           /* currently active window */
	Syntax *syntaxes;   /* NULL terminated array of syntax definitions */
	void (*windows_arrange)(Editor*);
};

static void editor_windows_arrange(Editor *ed);
static void editor_windows_arrange_horizontal(Editor *ed); 
static void editor_windows_arrange_vertical(Editor *ed); 
static Filerange window_selection_get(Win *win);
static void window_statusbar_draw(Win *win); 
static void editor_windows_invalidate(Editor *ed, size_t pos, size_t len);
static void editor_windows_insert(Editor *ed, size_t pos, const char *c, size_t len); 
static void editor_windows_delete(Editor *ed, size_t pos, size_t len);
static void editor_search_forward(Editor *ed, Regex *regex);
static void editor_search_backward(Editor *ed, Regex *regex);
static void window_selection_clear(Win *win);
static void window_clear(Win *win);
static bool window_addch(Win *win, Char *c);
static size_t window_cursor_update(Win *win);
static size_t window_line_begin(Win *win);
static size_t cursor_move_to(Win *win, size_t pos);
static size_t cursor_move_to_line(Win *win, size_t pos); 
static void window_draw(Win *win);
static bool window_resize(Win *win, int width, int height);
static void window_move(Win *win, int x, int y); 
static void window_free(Win *win);
static Win *window_new(Editor *ed, const char *filename, int x, int y, int w, int h, int pos);
static size_t pos_by_line(Win *win, Line *line);
static size_t cursor_offset(Cursor *cursor);
static bool scroll_line_down(Win *win, int n);
static bool scroll_line_up(Win *win, int n);

static void window_selection_clear(Win *win) {
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

static Filerange window_selection_get(Win *win) {
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

static void window_statusbar_draw(Win *win) {
	if (!win->editor->statusbar)
		return;
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;
	size_t lineno = line->lineno;
	int col = cursor->col;
	while (line->prev && line->prev->lineno == lineno) {
		line = line->prev;
		col += line->width;
	}
	win->editor->statusbar(win->statuswin, win->editor->win == win, text_filename(win->text), cursor->line->lineno, col+1);
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
	window_statusbar_draw(win);
	return cursor->pos;
}

/* move the cursor to the character at pos bytes from the begining of the file.
 * if pos is not in the current viewport, redraw the window to make it visible */
static size_t cursor_move_to(Win *win, size_t pos) {
	Line *line = win->topline;
	int row = 0;
	int col = 0;
	size_t max = text_size(win->text);

	if (pos > max)
		pos = max > 0 ? max - 1 : 0;

	if (pos < win->start || pos > win->end) {
		win->start = pos;
		window_draw(win);
	} else {
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
	}

	win->cursor.line = line;
	win->cursor.row = row;
	win->cursor.col = col;
	win->cursor.pos = pos;
	return window_cursor_update(win);
}

/* move cursor to pos, make sure the whole line is visible */ 
static size_t cursor_move_to_line(Win *win, size_t pos) {
	if (pos < win->start || pos > win->end) {
		win->cursor.pos = pos;
		window_line_begin(win);
	}
	return cursor_move_to(win, pos);
}

/* redraw the complete with data starting from win->start bytes into the file.
 * stop once the screen is full, update win->end, win->lastline */
static void window_draw(Win *win) {
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

static bool window_resize(Win *win, int width, int height) {
	height--; // statusbar
	if (wresize(win->win, height, width) == ERR ||
	    wresize(win->statuswin, 1, width) == ERR)
		return false;

	// TODO: only grow memory area
	win->height = height;
	win->width = width;
	free(win->lines);
	size_t line_size = sizeof(Line) + width*sizeof(Cell);
	if (!(win->lines = calloc(height, line_size)))
		return false;
	window_draw(win);
	cursor_move_to(win, win->cursor.pos);
	return true;
}

static void window_move(Win *win, int x, int y) {
	mvwin(win->win, y, x);
	mvwin(win->statuswin, y + win->height, x);
}

static void window_free(Win *win) {
	if (!win)
		return;
	if (win->win)
		delwin(win->win);
	if (win->statuswin)
		delwin(win->statuswin);
	text_free(win->text);
	free(win);
}

static Win *window_new(Editor *ed, const char *filename, int x, int y, int w, int h, int pos) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->editor = ed;
	win->start = pos;
	win->tabwidth = 8; // TODO make configurable

	if (filename) {
		for (Syntax *syn = ed->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				win->syntax = syn;
				break;
			}
		}
	}

	if (!(win->text = text_load(filename)) ||
	    !(win->win = newwin(h-1, w, y, x)) ||
	    !(win->statuswin = newwin(1, w, y+h-1, x)) ||
	    !window_resize(win, w, h)) {
		window_free(win);
		return NULL;
	}

	window_selection_clear(win);
	cursor_move_to(win, pos);

	return win;
}

Editor *editor_new(int width, int height, editor_statusbar_t statusbar) {
	Editor *ed = calloc(1, sizeof(Editor));
	if (!ed)
		return NULL;
	ed->width = width;
	ed->height = height;
	ed->statusbar = statusbar;
	ed->windows_arrange = editor_windows_arrange_horizontal; 
	return ed;
}

bool editor_load(Editor *ed, const char *filename) {
	Win *win = window_new(ed, filename, 0, 0, ed->width, ed->height, 0);
	if (!win)
		return false;

	if (ed->windows)
		ed->windows->prev = win;
	win->next = ed->windows;
	win->prev = NULL;
	ed->windows = win;
	ed->win = win;
	return true;
}

static size_t pos_by_line(Win *win, Line *line) {
	size_t pos = win->start;
	for (Line *cur = win->topline; cur && cur != line; cur = cur->next)
		pos += cur->len;
	return pos;
}

size_t editor_char_prev(Editor *ed) {
	Win *win = ed->win;
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

size_t editor_char_next(Editor *ed) {
	Win *win = ed->win;
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

static bool scroll_line_down(Win *win, int n) {
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

static bool scroll_line_up(Win *win, int n) {
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

size_t editor_file_begin(Editor *ed) {
	return cursor_move_to(ed->win, 0);
}

size_t editor_file_end(Editor *ed) {
	Win *win = ed->win;
	size_t size = text_size(win->text);
	if (win->end == size)
		return cursor_move_to(win, win->end);
	win->start = size - 1;
	scroll_line_up(win, win->height / 2);
	return cursor_move_to(win, win->end);
}

size_t editor_page_up(Editor *ed) {
	Win *win = ed->win;
	if (!scroll_line_up(win, win->height))
		cursor_move_to(win, win->start);
	return win->cursor.pos;
}

size_t editor_page_down(Editor *ed) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	if (win->end == text_size(win->text))
		return cursor_move_to(win, win->end);
	win->start = win->end;
	window_draw(win);
	int col = cursor->col;
	cursor_move_to(win, win->start);
	cursor->col = col;
	cursor->pos += cursor_offset(cursor);
	return window_cursor_update(win);
}

size_t editor_line_up(Editor *ed) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	if (!cursor->line->prev) {
		scroll_line_up(win, 1);
		return cursor->pos;
	}
	cursor->row--;
	cursor->line = cursor->line->prev;
	cursor->pos = pos_by_line(win, cursor->line) + cursor_offset(cursor);
	return window_cursor_update(win);
}

size_t editor_line_down(Editor *ed) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	if (!cursor->line->next) {
		if (cursor->line == win->bottomline)
			scroll_line_down(ed->win, 1);
		return cursor->pos;
	}
	cursor->row++;
	cursor->line = cursor->line->next;
	cursor->pos = pos_by_line(win, cursor->line) + cursor_offset(cursor);
	return window_cursor_update(win);
}

static size_t window_line_begin(Win *win) {
	char c;
	size_t pos = win->cursor.pos;
	Iterator it = text_iterator_get(win->text, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	if (c == '\r')
		text_iterator_byte_prev(&it, &c);
	if (c == '\n')
		text_iterator_byte_prev(&it, &c);
	while (text_iterator_byte_get(&it, &c)) {
		if (c == '\n' || c == '\r') {
			it.pos++;
			break;
		}
		text_iterator_byte_prev(&it, NULL);
	}
	return cursor_move_to(win, it.pos);
}

size_t editor_line_begin(Editor *ed) {
	return window_line_begin(ed->win);
}

size_t editor_line_start(Editor *ed) {
	char c;
	Win *win = ed->win;
	editor_line_begin(ed);
	Iterator it = text_iterator_get(win->text, win->cursor.pos);
	while (text_iterator_byte_get(&it, &c) && c != '\n' && isspace(c))
		text_iterator_byte_next(&it, NULL);
	while (win->end < it.pos && scroll_line_down(win, 1));
	return cursor_move_to(win, it.pos);
}

size_t editor_line_end(Editor *ed) {
	char c;
	Win *win = ed->win;
	Iterator it = text_iterator_get(win->text, win->cursor.pos);
	while (text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	while (win->end < it.pos && scroll_line_down(win, 1));
	return cursor_move_to(win, it.pos);
}

size_t editor_line_finish(Editor *ed) {
	char c;
	Win *win = ed->win;
	Iterator it = text_iterator_get(win->text, editor_line_end(ed));
	do text_iterator_byte_prev(&it, NULL);
	while (text_iterator_byte_get(&it, &c) && c != '\n' && c != '\r' && isspace(c));
	return cursor_move_to(win, it.pos);
}

size_t editor_word_end_prev(Editor *ed) {
	char c;
	Win *win = ed->win;
	Iterator it = text_iterator_get(win->text, win->cursor.pos);
	while (text_iterator_byte_prev(&it, &c) && !isspace(c));
	while (text_iterator_char_prev(&it, &c) && isspace(c));
	while (win->start > it.pos && scroll_line_up(win, 1));
	return cursor_move_to(win, it.pos);
}

size_t editor_word_end_next(Editor *ed) {
	char c;
	Win *win = ed->win;
	size_t pos = win->cursor.pos;
	Iterator it = text_iterator_get(win->text, pos);
	while (text_iterator_char_next(&it, &c) && isspace(c));
	do pos = it.pos; while (text_iterator_char_next(&it, &c) && !isspace(c));
	while (win->end < pos && scroll_line_down(win, 1));
	return cursor_move_to(win, pos);
}

size_t editor_word_start_next(Editor *ed) {
	char c;
	Win *win = ed->win;
	Iterator it = text_iterator_get(win->text, win->cursor.pos);
	while (text_iterator_byte_get(&it, &c) && !isspace(c))
		text_iterator_byte_next(&it, NULL);
	while (text_iterator_byte_get(&it, &c) && isspace(c))
		text_iterator_byte_next(&it, NULL);
	while (win->end < it.pos && scroll_line_down(win, 1));
	return cursor_move_to(win, it.pos);
}

size_t editor_word_start_prev(Editor *ed) {
	char c;
	Win *win = ed->win;
	size_t pos = win->cursor.pos;
	Iterator it = text_iterator_get(win->text, pos);
	while (text_iterator_byte_prev(&it, &c) && isspace(c));
	do pos = it.pos; while (text_iterator_char_prev(&it, &c) && !isspace(c));
	while (win->start > pos && scroll_line_up(win, 1));
	return cursor_move_to(win, pos);
}

static size_t editor_paragraph_sentence_next(Editor *ed, bool sentence) {
	char c;
	bool content = false, paragraph = false;
	Win *win = ed->win;
	size_t pos = win->cursor.pos;
	Iterator it = text_iterator_get(win->text, pos);
	while (text_iterator_byte_next(&it, &c)) {
		content |= !isspace(c);
		if (sentence && (c == '.' || c == '?' || c == '!') && text_iterator_byte_next(&it, &c) && isspace(c)) {
			if (c == '\n' && text_iterator_byte_next(&it, &c)) {
				if (c == '\r')
					text_iterator_byte_next(&it, &c);
			} else {
				while (text_iterator_byte_get(&it, &c) && c == ' ')
					text_iterator_byte_next(&it, NULL);
			}
			break;
		}
		if (c == '\n' && text_iterator_byte_next(&it, &c)) {
			if (c == '\r')
				text_iterator_byte_next(&it, &c);
			content |= !isspace(c);
			if (c == '\n')
				paragraph = true;
		}
		if (content && paragraph)
			break;
	}
	while (win->end < it.pos && scroll_line_down(win, 1));
	return cursor_move_to(win, it.pos);
}

size_t editor_sentence_next(Editor *ed) {
	return editor_paragraph_sentence_next(ed, true);
}

size_t editor_paragraph_next(Editor *ed) {
	return editor_paragraph_sentence_next(ed, false);
}

static size_t editor_paragraph_sentence_prev(Editor *ed, bool sentence) {
	char prev, c;
	bool content = false, paragraph = false;
	Win *win = ed->win;
	size_t pos = win->cursor.pos;

	Iterator it = text_iterator_get(win->text, pos);
	if (!text_iterator_byte_get(&it, &prev))
		return pos;

	while (text_iterator_byte_prev(&it, &c)) {
		content |= !isspace(c) && c != '.' && c != '?' && c != '!';
		if (sentence && content && (c == '.' || c == '?' || c == '!') && isspace(prev)) {
			do text_iterator_byte_next(&it, NULL);
			while (text_iterator_byte_get(&it, &c) && isspace(c));
			break;
		}
		if (c == '\r')
			text_iterator_byte_prev(&it, &c);
		if (c == '\n' && text_iterator_byte_prev(&it, &c)) {
			content |= !isspace(c);
			if (c == '\r')
				text_iterator_byte_prev(&it, &c);
			if (c == '\n') {
				paragraph = true;
				if (content) {
					do text_iterator_byte_next(&it, NULL);
					while (text_iterator_byte_get(&it, &c) && isspace(c));
					break;
				}
			}
		}
		if (content && paragraph) {
			do text_iterator_byte_next(&it, NULL);
			while (text_iterator_byte_get(&it, &c) && !isspace(c));
			break;
		}
		prev = c;
	}
	while (win->end < it.pos && scroll_line_down(win, 1));
	return cursor_move_to(win, it.pos);
}

size_t editor_sentence_prev(Editor *ed) {
	return editor_paragraph_sentence_prev(ed, true);
}

size_t editor_paragraph_prev(Editor *ed) {
	return editor_paragraph_sentence_prev(ed, false);
}

size_t editor_bracket_match(Editor *ed) {
	Win *win = ed->win;
	int direction, count = 1;
	char search, current, c;
	size_t pos = win->cursor.pos;
	Iterator it = text_iterator_get(win->text, pos);
	if (!text_iterator_byte_get(&it, &current))
		return pos;

	switch (current) {
	case '(': search = ')'; direction =  1; break;
	case ')': search = '('; direction = -1; break;
	case '{': search = '}'; direction =  1; break;
	case '}': search = '{'; direction = -1; break;
	case '[': search = ']'; direction =  1; break;
	case ']': search = '['; direction = -1; break;
	case '<': search = '>'; direction =  1; break;
	case '>': search = '<'; direction = -1; break;
	case '"': search = '"'; direction =  1; break;
	default: return pos;
	}

	if (direction >= 0) { /* forward search */
		while (text_iterator_byte_next(&it, &c)) {
			if (c == search && --count == 0) {
				pos = it.pos;
				break;
			} else if (c == current) {
				count++;
			}
		}
	} else { /* backwards */
		while (text_iterator_byte_prev(&it, &c)) {
			if (c == search && --count == 0) {
				pos = it.pos;
				break;
			} else if (c == current) {
				count++;
			}
		}
	}

	return cursor_move_to_line(win, pos);
}

void editor_draw(Editor *ed) {
	for (Win *win = ed->windows; win; win = win->next) {
		if (ed->win != win) {
			window_draw(win);
			cursor_move_to(win, win->cursor.pos);
		}
	}
	window_draw(ed->win);
	cursor_move_to(ed->win, ed->win->cursor.pos);
}

void editor_update(Editor *ed) {
	for (Win *win = ed->windows; win; win = win->next) {
		if (ed->win != win) {
			wnoutrefresh(win->statuswin);
			wnoutrefresh(win->win);
		}
	}

	wnoutrefresh(ed->win->statuswin);
	wnoutrefresh(ed->win->win);
}

void editor_free(Editor *ed) {
	if (!ed)
		return;
	window_free(ed->win);
	free(ed);
}

bool editor_resize(Editor *ed, int width, int height) {
	ed->width = width;
	ed->height = height;
	editor_windows_arrange(ed);
	return true;
}

bool editor_syntax_load(Editor *ed, Syntax *syntaxes, Color *colors) {
	bool success = true;
	ed->syntaxes = syntaxes;
	for (Syntax *syn = syntaxes; syn && syn->name; syn++) {
		if (regcomp(&syn->file_regex, syn->file, REG_EXTENDED|REG_NOSUB|REG_ICASE|REG_NEWLINE))
			success = false;
		Color *color = colors;
		for (int j = 0; j < LENGTH(syn->rules); j++) {
			SyntaxRule *rule = &syn->rules[j];
			if (!rule->rule)
				break;
			if (rule->color.fg == 0 && color && color->fg != 0)
				rule->color = *color++;
			if (rule->color.attr == 0)
				rule->color.attr = A_NORMAL;
			if (rule->color.fg != 0)
				rule->color.attr |= COLOR_PAIR(editor_color_reserve(rule->color.fg, rule->color.bg));
			if (regcomp(&rule->regex, rule->rule, REG_EXTENDED|rule->cflags))
				success = false;
		}
	}

	return success;
}

void editor_syntax_unload(Editor *ed) {
	for (Syntax *syn = ed->syntaxes; syn && syn->name; syn++) {
		regfree(&syn->file_regex);
		for (int j = 0; j < LENGTH(syn->rules); j++) {
			SyntaxRule *rule = &syn->rules[j];
			if (!rule->rule)
				break;
			regfree(&rule->regex);
		}
	}

	ed->syntaxes = NULL;
}

static void editor_windows_invalidate(Editor *ed, size_t pos, size_t len) {
	size_t end = pos + len;
	for (Win *win = ed->windows; win; win = win->next) {
		if (ed->win != win && ed->win->text == win->text &&
		    ((win->start <= pos && pos <= win->end) ||
		     (win->start <= end && end <= win->end))) {
			window_draw(win);
			cursor_move_to(win, win->cursor.pos);
		}
	}
}

static void editor_windows_insert(Editor *ed, size_t pos, const char *c, size_t len) {
	text_insert_raw(ed->win->text, pos, c, len);
	editor_windows_invalidate(ed, pos, len);
}

static void editor_windows_delete(Editor *ed, size_t pos, size_t len) {
	text_delete(ed->win->text, pos, len);
	editor_windows_invalidate(ed, pos, len);
}

size_t editor_delete(Editor *ed) {
	Cursor *cursor = &ed->win->cursor;
	Line *line = cursor->line;
	size_t len = line->cells[cursor->col].len;
	editor_windows_delete(ed, cursor->pos, len);
	window_draw(ed->win);
	return cursor_move_to(ed->win, cursor->pos);
}

size_t editor_backspace(Editor *ed) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	if (win->start == cursor->pos) {
		if (win->start == 0)
			return cursor->pos;
		/* if we are on the top left most position in the window
		 * first scroll up so that the to be deleted character is
		 * visible then proceed as normal */
		size_t pos = cursor->pos;
		scroll_line_up(win, 1);
		cursor_move_to(win, pos);
	}
	editor_char_prev(ed);
	size_t pos = cursor->pos;
	size_t len = cursor->line->cells[cursor->col].len;
	editor_windows_delete(ed, pos, len);
	window_draw(win);
	return cursor_move_to(ed->win, pos);
}

size_t editor_insert(Editor *ed, const char *c, size_t len) {
	Win *win = ed->win;
	size_t pos = win->cursor.pos;
	editor_windows_insert(ed, pos, c, len);
	if (win->cursor.line == win->bottomline && memchr(c, '\n', len))
		scroll_line_down(win, 1);
	else
		window_draw(win);
	return cursor_move_to(win, pos + len);
}

size_t editor_replace(Editor *ed, const char *c, size_t len) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	Line *line = cursor->line;
	size_t pos = cursor->pos;
	/* do not overwrite new line which would merge the two lines */
	if (line->cells[cursor->col].data != '\n') {
		size_t oldlen = line->cells[cursor->col].len;
		text_delete(win->text, pos, oldlen);
	}
	editor_windows_insert(ed, pos, c, len);
	if (cursor->line == win->bottomline && memchr(c, '\n', len))
		scroll_line_down(win, 1);
	else
		window_draw(win);
	return cursor_move_to(win, pos + len);
}

size_t editor_cursor_get(Editor *ed) {
	return ed->win->cursor.pos;
}

size_t editor_selection_start(Editor *ed) {
	return ed->win->sel.start = editor_cursor_get(ed);
}

size_t editor_selection_end(Editor *ed) {
	return ed->win->sel.end = editor_cursor_get(ed);
}

Filerange editor_selection_get(Editor *ed) {
	return window_selection_get(ed->win);
}

void editor_selection_clear(Editor *ed) {
	window_selection_clear(ed->win);
}

size_t editor_line_goto(Editor *ed, size_t lineno) {
	size_t pos = text_pos_by_lineno(ed->win->text, lineno);
	return cursor_move_to(ed->win, pos);
}

static void editor_search_forward(Editor *ed, Regex *regex) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	int pos = cursor->pos + 1;
	int end = text_size(win->text);
	RegexMatch match[1];
	bool found = false;
	if (text_search_forward(win->text, pos, end - pos, regex, 1, match, 0)) {
		pos = 0;
		end = cursor->pos;
		if (!text_search_forward(win->text, pos, end, regex, 1, match, 0))
			found = true;
	} else {
		found = true;
	}
	if (found)
		cursor_move_to_line(win, match[0].start);
}

static void editor_search_backward(Editor *ed, Regex *regex) {
	Win *win = ed->win;
	Cursor *cursor = &win->cursor;
	int pos = 0;
	int end = cursor->pos;
	RegexMatch match[1];
	bool found = false;
	if (text_search_backward(win->text, pos, end, regex, 1, match, 0)) {
		pos = cursor->pos + 1;
		end = text_size(win->text);
		if (!text_search_backward(win->text, pos, end - pos, regex, 1, match, 0))
			found = true;
	} else {
		found = true;
	}
	if (found)
		cursor_move_to_line(win, match[0].start);
}

void editor_search(Editor *ed, const char *s, int direction) {
	Regex *regex = text_regex_new();
	if (!regex)
		return;
	if (!text_regex_compile(regex, s, REG_EXTENDED)) {
		if (direction >= 0)
			editor_search_forward(ed, regex);
		else
			editor_search_backward(ed, regex);
	}
	text_regex_free(regex);
}

void editor_snapshot(Editor *ed) {
	text_snapshot(ed->win->text);
}

void editor_undo(Editor *ed) {
	Win *win = ed->win;
	if (text_undo(win->text))
		window_draw(win);
}

void editor_redo(Editor *ed) {
	Win *win = ed->win;
	if (text_redo(win->text))
		window_draw(win);
}

static void editor_windows_arrange(Editor *ed) {
	erase();
	wnoutrefresh(stdscr);
	ed->windows_arrange(ed);
}

static void editor_windows_arrange_horizontal(Editor *ed) {
	int n = 0, x = 0, y = 0;
	for (Win *win = ed->windows; win; win = win->next)
		n++;
	int height = ed->height / n; 
	for (Win *win = ed->windows; win; win = win->next) {
		window_resize(win, ed->width, win->next ? height : ed->height - y);
		window_move(win, x, y);
		y += height;
	}
}

static void editor_windows_arrange_vertical(Editor *ed) {
	int n = 0, x = 0, y = 0;
	for (Win *win = ed->windows; win; win = win->next)
		n++;
	int width = ed->width / n; 
	for (Win *win = ed->windows; win; win = win->next) {
		window_resize(win, win->next ? width : ed->width - x, ed->height);
		window_move(win, x, y);
		x += width;
	}
}

static void editor_window_split_internal(Editor *ed, const char *filename) {
	Win *sel = ed->win;
	editor_load(ed, filename);
	if (sel && !filename) {
		Win *win = ed->win;
		text_free(win->text);
		win->text = sel->text;
		win->start = sel->start;
		win->syntax = sel->syntax;
		win->cursor.pos = sel->cursor.pos;
		// TODO show begin of line instead of cursor->pos
	}
}

void editor_window_split(Editor *ed, const char *filename) {
	editor_window_split_internal(ed, filename);
	ed->windows_arrange = editor_windows_arrange_horizontal;
	editor_windows_arrange(ed);
}

void editor_window_vsplit(Editor *ed, const char *filename) {
	editor_window_split_internal(ed, filename);
	ed->windows_arrange = editor_windows_arrange_vertical;
	editor_windows_arrange(ed);
}

void editor_window_next(Editor *ed) {
	Win *sel = ed->win;
	if (!sel)
		return;
	ed->win = ed->win->next;
	if (!ed->win)
		ed->win = ed->windows;
	window_statusbar_draw(sel);
	window_statusbar_draw(ed->win);
}

void editor_window_prev(Editor *ed) {
	Win *sel = ed->win;
	if (!sel)
		return;
	ed->win = ed->win->prev;
	if (!ed->win)
		for (ed->win = ed->windows; ed->win->next; ed->win = ed->win->next);
	window_statusbar_draw(sel);
	window_statusbar_draw(ed->win);
}

void editor_mark_set(Editor *ed, Mark mark, size_t pos) {
	text_mark_set(ed->win->text, mark, pos);
}

void editor_mark_goto(Editor *ed, Mark mark) {
	Win *win = ed->win;
	size_t pos = text_mark_get(win->text, mark);
	if (pos != (size_t)-1)
		cursor_move_to_line(win, pos);
}

void editor_mark_clear(Editor *ed, Mark mark) {
	text_mark_clear(ed->win->text, mark);
}
