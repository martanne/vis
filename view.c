#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include "vis-lua.h"
#include "view.h"
#include "text.h"
#include "text-motions.h"
#include "text-util.h"
#include "util.h"

typedef struct {
	char *symbol;
	int style;
} SyntaxSymbol;

enum {
	SYNTAX_SYMBOL_SPACE,
	SYNTAX_SYMBOL_TAB,
	SYNTAX_SYMBOL_TAB_FILL,
	SYNTAX_SYMBOL_EOL,
	SYNTAX_SYMBOL_EOF,
	SYNTAX_SYMBOL_LAST,
};

struct Selection {
	Mark anchor;             /* position where the selection was created */
	Mark cursor;             /* other selection endpoint where it changes */
	View *view;              /* associated view to which this selection belongs */
	Selection *prev, *next;  /* previsous/next selections in no particular order */
};

struct Cursor {             /* cursor position */
	Filepos pos;        /* in bytes from the start of the file */
	int row, col;       /* in terms of zero based screen coordinates */
	int lastcol;        /* remembered column used when moving across lines */
	Line *line;         /* screen line on which cursor currently resides */
	Mark mark;          /* mark used to keep track of current cursor position */
	Selection *sel;     /* selection (if any) which folows the cursor upon movement */
	Mark lastsel_anchor;/* previously used selection data, */
	Mark lastsel_cursor;/* used to restore it */
	Register reg;       /* per cursor register to support yank/put operation */
	View *view;         /* associated view to which this cursor belongs */
	Cursor *prev, *next;/* previous/next cursors in no particular order */
};

/* Viewable area, showing part of a file. Keeps track of cursors and selections.
 * At all times there exists at least one cursor, which is placed in the visible viewport.
 * Additional cursors can be created and positioned anywhere in the file. */
struct View {
	Text *text;         /* underlying text management */
	UiWin *ui;
	int width, height;  /* size of display area */
	Filepos start, end; /* currently displayed area [start, end] in bytes from the start of the file */
	Filepos start_last; /* previously used start of visible area, used to update the mark */
	Mark start_mark;    /* mark to keep track of the start of the visible area */
	size_t lines_size;  /* number of allocated bytes for lines (grows only) */
	Line *lines;        /* view->height number of lines representing view content */
	Line *topline;      /* top of the view, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of view, might be unused if lastline < bottomline */
	Cursor *cursor;     /* main cursor, always placed within the visible viewport */
	Line *line;         /* used while drawing view content, line where next char will be drawn */
	int col;            /* used while drawing view content, column where next char will be drawn */
	const SyntaxSymbol *symbols[SYNTAX_SYMBOL_LAST]; /* symbols to use for white spaces etc */
	int tabwidth;       /* how many spaces should be used to display a tab character */
	Cursor *cursors;    /* all cursors currently active */
	Selection *selections; /* all selected regions */
	lua_State *lua;     /* lua state used for syntax highlighting */
	char *lexer_name;
	bool need_update;   /* whether view has been redrawn */
	bool large_file;    /* optimize for displaying large files */
	int colorcolumn;
};

static const SyntaxSymbol symbols_none[] = {
	[SYNTAX_SYMBOL_SPACE]    = { " " },
	[SYNTAX_SYMBOL_TAB]      = { " " },
	[SYNTAX_SYMBOL_TAB_FILL] = { " " },
	[SYNTAX_SYMBOL_EOL]      = { " " },
	[SYNTAX_SYMBOL_EOF]      = { "~" },
};

static const SyntaxSymbol symbols_default[] = {
	[SYNTAX_SYMBOL_SPACE]    = { "\xC2\xB7" },
	[SYNTAX_SYMBOL_TAB]      = { "\xE2\x96\xB6" },
	[SYNTAX_SYMBOL_TAB_FILL] = { " " },
	[SYNTAX_SYMBOL_EOL]      = { "\xE2\x8F\x8E" },
	[SYNTAX_SYMBOL_EOF]      = { "~" },
};

static Cell cell_unused;
static Cell cell_blank = { .data = " " };

static void view_clear(View *view);
static bool view_addch(View *view, Cell *cell);
static bool view_coord_get(View *view, size_t pos, Line **retline, int *retrow, int *retcol);
static void view_cursors_free(Cursor *c);
/* set/move current cursor position to a given (line, column) pair */
static size_t cursor_set(Cursor *cursor, Line *line, int col);

#if !CONFIG_LUA

static void view_syntax_color(View *view) { }
bool view_syntax_set(View *view, const char *name) { return false; }

#else

static void view_syntax_color(View *view) {
	lua_State *L = view->lua;
	if (!L || !view->lexer_name)
		return;
	lua_getglobal(L, "vis");
	lua_getfield(L, -1, "lexers");
	if (lua_isnil(L, -1))
		return;

	/* maximal number of bytes to consider for syntax highlighting before
	 * the visible area */
	const size_t lexer_before_max = 16384;
	/* absolute position to start syntax highlighting */
	const size_t lexer_start = view->start >= lexer_before_max ? view->start - lexer_before_max : 0;
	/* number of bytes used for syntax highlighting before visible are */
	const size_t lexer_before = view->start - lexer_start;
	/* number of bytes to read in one go */
	const size_t text_size = lexer_before + (view->end - view->start);
	/* current buffer to work with */
	char text[text_size+1];
	/* bytes to process */
	const size_t text_len = text_bytes_get(view->text, lexer_start, text_size, text);
	/* NUL terminate text section */
	text[text_len] = '\0';

	lua_getfield(L, -1, "load");
	lua_pushstring(L, view->lexer_name);
	lua_pcall(L, 1, 1, 0);

	lua_getfield(L, -1, "_TOKENSTYLES");
	lua_getfield(L, -2, "lex");

	lua_pushvalue(L, -3); /* lexer obj */

	const char *lex_text = text;
	if (lexer_start > 0) {
		/* try to start lexing at a line boundry */
		/* TODO: start at known state, handle nested lexers */
		const char *newline = memchr(text, '\n', lexer_before);
		if (newline)
			lex_text = newline;
	}

	lua_pushlstring(L, lex_text, text_len - (lex_text - text));
	lua_pushinteger(L, 1 /* inital style: whitespace */);

	int token_count;

	if (lua_isfunction(L, -4) &&  !lua_pcall(L, 3, 1, 0) && lua_istable(L, -1) &&
	   (token_count = lua_objlen(L, -1)) > 0) {

		size_t pos = lexer_before - (lex_text - text);
		Line *line = view->topline;
		int col = 0;

		for (int i = 1; i < token_count; i += 2) {
			lua_rawgeti(L, -1, i);
			//const char *name = lua_tostring(L, -1);
			lua_gettable(L, -3); /* _TOKENSTYLES[token] */
			size_t token_style = lua_tointeger(L, -1);
			lua_pop(L, 1); /* style */
			lua_rawgeti(L, -1, i + 1);
			size_t token_end = lua_tointeger(L, -1) - 1;
			lua_pop(L, 1); /* pos */

			for (bool token_next = false; line; line = line->next, col = 0) {
				for (; col < line->width; col++) {
					if (pos < token_end) {
						line->cells[col].style = token_style;
						pos += line->cells[col].len;
					} else {
						token_next = true;
						break;
					}
				}
				if (token_next)
					break;
			}
		}
		lua_pop(L, 1);
	}

	lua_pop(L, 3); /* _TOKENSTYLES, language specific lexer, lexers global */
}

bool view_syntax_set(View *view, const char *name) {
	lua_State *L = view->lua;
	if (!L)
		return name == NULL;

	lua_getglobal(L, "vis");
	lua_getfield(L, -1, "lexers");

	static const struct {
		enum UiStyles id;
		const char *name;
	} styles[] = {
		{ UI_STYLE_DEFAULT,        "STYLE_DEFAULT"        },
		{ UI_STYLE_CURSOR,         "STYLE_CURSOR"         },
		{ UI_STYLE_CURSOR_PRIMARY, "STYLE_CURSOR_PRIMARY" },
		{ UI_STYLE_CURSOR_LINE,    "STYLE_CURSOR_LINE"    },
		{ UI_STYLE_SELECTION,      "STYLE_SELECTION"      },
		{ UI_STYLE_LINENUMBER,     "STYLE_LINENUMBER"     },
		{ UI_STYLE_COLOR_COLUMN,   "STYLE_COLOR_COLUMN"   },
	};

	for (size_t i = 0; i < LENGTH(styles); i++) {
		lua_getfield(L, -1, styles[i].name);
		view->ui->syntax_style(view->ui, styles[i].id, lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	if (!name) {
		free(view->lexer_name);
		view->lexer_name = NULL;
		return true;
	}

	/* Try to load the specified lexer and parse its token styles.
	 * Roughly equivalent to the following lua code:
	 *
	 * lang = vis.lexers.load(name)
	 * for token_name, id in pairs(lang._TOKENSTYLES) do
	 * 	ui->syntax_style(id, vis.lexers:get_style(lang, token_name);
	 */
	lua_getfield(L, -1, "load");
	lua_pushstring(L, name);

	if (lua_pcall(L, 1, 1, 0))
		return false;

	if (!lua_istable(L, -1)) {
		lua_pop(L, 2);
		return false;
	}

	view->lexer_name = strdup(name);
	/* loop through all _TOKENSTYLES and parse them */
	lua_getfield(L, -1, "_TOKENSTYLES");
	lua_pushnil(L); /* first key */

	while (lua_next(L, -2)) {
		size_t id = lua_tointeger(L, -1);
		//const char *name = lua_tostring(L, -2);
		lua_pop(L, 1); /* remove value (=id), keep key (=name) */
		lua_getfield(L, -4, "get_style");
		lua_pushvalue(L, -5); /* lexer */
		lua_pushvalue(L, -5); /* lang */
		lua_pushvalue(L, -4); /* token_name */
		if (lua_pcall(L, 3, 1, 0))
			return false;
		const char *style = lua_tostring(L, -1);
		//printf("%s\t%d\t%s\n", name, id, style);
		view->ui->syntax_style(view->ui, id, style);
		lua_pop(L, 1); /* style */
	}

	lua_pop(L, 4); /* _TOKENSTYLES, grammar, lexers, vis */

	return true;
}

#endif


void view_tabwidth_set(View *view, int tabwidth) {
	view->tabwidth = tabwidth;
	view_draw(view);
}

/* reset internal view data structures (cell matrix, line offsets etc.) */
static void view_clear(View *view) {
	memset(view->lines, 0, view->lines_size);
	if (view->start != view->start_last) {
		view->start_mark = text_mark_set(view->text, view->start);
	} else {
		size_t start = text_mark_get(view->text, view->start_mark);
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
			cell->style = view->symbols[t]->style;
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
		cell->style = view->symbols[SYNTAX_SYMBOL_EOL]->style;

		view->line->cells[view->col] = *cell;
		view->line->len += cell->len;
		view->line->width += cell->width;
		for (int i = view->col + 1; i < view->width; i++)
			view->line->cells[i] = cell_blank;

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
			cell->style = view->symbols[SYNTAX_SYMBOL_SPACE]->style;

		}

		if (view->col + cell->width > view->width) {
			for (int i = view->col; i < view->width; i++)
				view->line->cells[i] = cell_blank;
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

CursorPos view_cursor_getpos(View *view) {
	Cursor *cursor = view->cursor;
	Line *line = cursor->line;
	CursorPos pos = { .line = line->lineno, .col = cursor->col };
	while (line->prev && line->prev->lineno == pos.line) {
		line = line->prev;
		pos.col += line->width;
	}
	pos.col++;
	return pos;
}

static void cursor_to(Cursor *c, size_t pos) {
	Text *txt = c->view->text;
	c->mark = text_mark_set(txt, pos);
	if (pos != c->pos)
		c->lastcol = 0;
	c->pos = pos;
	if (c->sel) {
		size_t anchor = text_mark_get(txt, c->sel->anchor);
		size_t cursor = text_mark_get(txt, c->sel->cursor);
		/* do we have to change the orientation of the selection? */
		if (pos < anchor && anchor < cursor) {
			/* right extend -> left extend  */
			anchor = text_char_next(txt, anchor);
			c->sel->anchor = text_mark_set(txt, anchor);
		} else if (cursor < anchor && anchor <= pos) {
			/* left extend  -> right extend */
			anchor = text_char_prev(txt, anchor);
			c->sel->anchor = text_mark_set(txt, anchor);
		}
		if (anchor <= pos)
			pos = text_char_next(txt, pos);
		c->sel->cursor = text_mark_set(txt, pos);
	}
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

static bool view_coord_get(View *view, size_t pos, Line **retline, int *retrow, int *retcol) {
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
	const size_t text_size = view->width * view->height * 4;
	/* current buffer to work with */
	char text[text_size+1];
	/* remaining bytes to process in buffer */
	size_t rem = text_bytes_get(view->text, view->start, text_size, text);
	/* NUL terminate text section */
	text[rem] = '\0';
	/* absolute position of character currently being added to display */
	size_t pos = view->start;
	/* current position into buffer from which to interpret a character */
	char *cur = text;
	/* start from known multibyte state */
	mbstate_t mbstate = { 0 };

	Cell cell = { 0 }, prev_cell = { 0 };

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
			rem = text_bytes_get(view->text, pos, text_size, text);
			text[rem] = '\0';
			cur = text;
			continue;
		} else if (len == 0) {
			/* NUL byte encountered, store it and continue */
			cell = (Cell){ .data = "\x00", .len = 1, .width = 2 };
		} else {
			for (size_t i = 0; i < len; i++)
				cell.data[i] = cur[i];
			cell.data[len] = '\0';
			cell.len = len;
			cell.width = wcwidth(wchar);
			if (cell.width == -1)
				cell.width = 1;
		}

		if (cur[0] == '\r' && rem > 1 && cur[1] == '\n') {
			/* convert views style newline \r\n into a single char with len = 2 */
			cell = (Cell){ .data = "\n", .len = 2, .width = 1 };
		}

		if (cell.width == 0 && prev_cell.len + cell.len < sizeof(cell.len)) {
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

	/* set end of vieviewg region */
	view->end = pos;
	view->lastline = view->line ? view->line : view->bottomline;

	/* clear remaining of line, important to show cursor at end of file */
	if (view->line) {
		for (int x = view->col; x < view->width; x++)
			view->line->cells[x] = cell_blank;
	}

	/* resync position of cursors within visible area */
	for (Cursor *c = view->cursors; c; c = c->next) {
		size_t pos = view_cursors_pos(c);
		if (view_coord_get(view, pos, &c->line, &c->row, &c->col)) {
			c->line->cells[c->col].cursor = true;
			c->line->cells[c->col].cursor_primary = (c == view->cursor);
			if (view->ui && !c->sel) {
				Line *line_match; int col_match;
				size_t pos_match = text_bracket_match_symbol(view->text, pos, "(){}[]\"'`");
				if (pos != pos_match && view_coord_get(view, pos_match, &line_match, NULL, &col_match)) {
					line_match->cells[col_match].selected = true;
				}
			}
		} else if (c == view->cursor) {
			c->line = view->topline;
			c->row = 0;
			c->col = 0;
		}
	}

	view->need_update = true;
}

void view_update(View *view) {
	if (!view->need_update)
		return;

	view_syntax_color(view);

	if (view->colorcolumn > 0) {
		size_t lineno = 0;
		int line_cols = 0; /* Track the number of columns we've passed on each line */
		bool line_cc_set = false; /* Has the colorcolumn attribute been set for this line yet */

		for (Line *l = view->topline; l; l = l->next) {
			if (l->lineno != lineno) {
				line_cols = 0;
				line_cc_set = false;
			}

			if (!line_cc_set) {
				line_cols += view->width;

				/* This screen line contains the cell we want to highlight */
				if (line_cols >= view->colorcolumn) {
					l->cells[(view->colorcolumn - 1) % view->width].style = UI_STYLE_COLOR_COLUMN;
					line_cc_set = true;
				}
			}

			lineno = l->lineno;
		}
	}

	for (Line *l = view->lastline->next; l; l = l->next) {
		strncpy(l->cells[0].data, view->symbols[SYNTAX_SYMBOL_EOF]->symbol, sizeof(l->cells[0].data));
		l->cells[0].style = view->symbols[SYNTAX_SYMBOL_EOF]->style;
		for (int x = 1; x < view->width; x++)
			l->cells[x] = cell_blank;
		l->width = 1;
		l->len = 0;
	}

	for (Selection *s = view->selections; s; s = s->next) {
		Filerange sel = view_selections_get(s);
		if (text_range_valid(&sel)) {
			Line *start_line; int start_col;
			Line *end_line; int end_col;
			view_coord_get(view, sel.start, &start_line, NULL, &start_col);
			view_coord_get(view, sel.end, &end_line, NULL, &end_col);
			if (start_line || end_line) {
				if (!start_line) {
					start_line = view->topline;
					start_col = 0;
				}
				if (!end_line) {
					end_line = view->lastline;
					end_col = end_line->width;
				}
				for (Line *l = start_line; l != end_line->next; l = l->next) {
					int col = (l == start_line) ? start_col : 0;
					int end = (l == end_line) ? end_col : l->width;
					while (col < end) {
						l->cells[col++].selected = true;
					}
				}
			}
		}
	}

	if (view->ui)
		view->ui->draw(view->ui);
	view->need_update = false;
}

bool view_resize(View *view, int width, int height) {
	if (width <= 0)
		width = 1;
	if (height <= 0)
		height = 1;
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
	while (view->selections)
		view_selections_free(view->selections);
	free(view->lines);
	free(view->lexer_name);
	free(view);
}

void view_reload(View *view, Text *text) {
	view->text = text;
	view_selections_clear(view);
	view_cursor_to(view, 0);
}

View *view_new(Text *text, lua_State *lua) {
	if (!text)
		return NULL;
	View *view = calloc(1, sizeof(View));
	if (!view)
		return NULL;
	if (!view_cursors_new(view)) {
		view_free(view);
		return NULL;
	}

	view->text = text;
	view->lua = lua;
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

bool view_viewport_down(View *view, int n) {
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

bool view_viewport_up(View *view, int n) {
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
	while (pos > view->end && view_viewport_down(view, 1));
	view_cursor_to(view, pos);
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
	if (view_viewport_up(view, lines)) {
		if (cursor->line == view->lastline)
			cursor_set(cursor, view->lastline, cursor->col);
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
	if (cursor->line && cursor->line->prev && cursor->line->prev->prev &&
	   cursor->line->lineno != cursor->line->prev->lineno &&
	   cursor->line->prev->lineno != cursor->line->prev->prev->lineno)
		return view_screenline_up(cursor);
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	view_cursors_to(cursor, text_line_up(cursor->view->text, cursor->pos));
	if (cursor->line)
		cursor_set(cursor, cursor->line, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_line_down(Cursor *cursor) {
	if (cursor->line && (!cursor->line->next || cursor->line->next->lineno != cursor->line->lineno))
		return view_screenline_down(cursor);
	int lastcol = cursor->lastcol;
	if (!lastcol)
		lastcol = cursor->col;
	view_cursors_to(cursor, text_line_down(cursor->view->text, cursor->pos));
	if (cursor->line)
		cursor_set(cursor, cursor->line, lastcol);
	cursor->lastcol = lastcol;
	return cursor->pos;
}

size_t view_screenline_up(Cursor *cursor) {
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

const Line *view_lines_get(View *view) {
	return view->topline;
}

void view_scroll_to(View *view, size_t pos) {
	view_cursors_scroll_to(view->cursor, pos);
}

const char *view_syntax_get(View *view) {
	return view->lexer_name;
}

void view_options_set(View *view, enum UiOption options) {
	const int mapping[] = {
		[SYNTAX_SYMBOL_SPACE]    = UI_OPTION_SYMBOL_SPACE,
		[SYNTAX_SYMBOL_TAB]      = UI_OPTION_SYMBOL_TAB,
		[SYNTAX_SYMBOL_TAB_FILL] = UI_OPTION_SYMBOL_TAB_FILL,
		[SYNTAX_SYMBOL_EOL]      = UI_OPTION_SYMBOL_EOL,
		[SYNTAX_SYMBOL_EOF]      = UI_OPTION_SYMBOL_EOF,
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

Cursor *view_cursors_new(View *view) {
	Cursor *c = calloc(1, sizeof(*c));
	if (!c)
		return NULL;

	c->view = view;
	c->next = view->cursors;
	if (view->cursors)
		view->cursors->prev = c;
	view->cursors = c;
	view->cursor = c;
	return c;
}

int view_cursors_count(View *view) {
	int i = 0;
	for (Cursor *c = view_cursors(view); c; c = view_cursors_next(c))
		i++;
	return i;
}

bool view_cursors_multiple(View *view) {
	return view->cursors && view->cursors->next;
}

static void view_cursors_free(Cursor *c) {
	if (!c)
		return;
	register_release(&c->reg);
	if (c->prev)
		c->prev->next = c->next;
	if (c->next)
		c->next->prev = c->prev;
	if (c->view->cursors == c)
		c->view->cursors = c->next;
	if (c->view->cursor == c)
		c->view->cursor = c->next ? c->next : c->prev;
	free(c);
}

void view_cursors_dispose(Cursor *c) {
	if (!c)
		return;
	View *view = c->view;
	if (view->cursors && view->cursors->next) {
		view_selections_free(c->sel);
		view_cursors_free(c);
		view_draw(view);
	}
}

Cursor *view_cursors(View *view) {
	return view->cursors;
}

Cursor *view_cursors_primary_get(View *view) {
	return view->cursor;
}

void view_cursors_primary_set(Cursor *c) {
	if (!c)
		return;
	View *view = c->view;
	view->cursor = c;
	Filerange sel = view_cursors_selection_get(c);
	view_cursors_to(c, view_cursors_pos(c));
	view_cursors_selection_set(c, &sel);
}

Cursor *view_cursors_prev(Cursor *c) {
	return c->prev;
}

Cursor *view_cursors_next(Cursor *c) {
	return c->next;
}

size_t view_cursors_pos(Cursor *c) {
	return text_mark_get(c->view->text, c->mark);
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

Register *view_cursors_register(Cursor *c) {
	return &c->reg;
}

void view_cursors_scroll_to(Cursor *c, size_t pos) {
	View *view = c->view;
	if (view->cursor == c) {
		while (pos < view->start && view_viewport_up(view, 1));
		while (pos > view->end && view_viewport_down(view, 1));
	}
	view_cursors_to(c, pos);
}

void view_cursors_to(Cursor *c, size_t pos) {
	View *view = c->view;
	if (c->view->cursor == c) {
		c->mark = text_mark_set(view->text, pos);

		size_t max = text_size(view->text);
		if (pos == max && view->end < max) {
			/* do not display an empty screen when shoviewg the end of the file */
			view->start = pos;
			view_viewport_up(view, view->height / 2);
		} else {
			/* make sure we redraw changes to the very first character of the window */
			if (view->start == pos)
				view->start_last = 0;
			/* set the start of the viewable region to the start of the line on which
			 * the cursor should be placed. if this line requires more space than
			 * available in the view then simply start displaying text at the new
			 * cursor position */
			for (int i = 0;  i < 2 && (pos <= view->start || pos > view->end); i++) {
				view->start = i == 0 ? text_line_begin(view->text, pos) : pos;
				view_draw(view);
			}
		}
	}

	cursor_to(c, pos);
}

void view_cursors_selection_start(Cursor *c) {
	if (c->sel)
		return;
	size_t pos = view_cursors_pos(c);
	if (pos == EPOS || !(c->sel = view_selections_new(c->view)))
		return;
	Text *txt = c->view->text;
	c->sel->anchor = text_mark_set(txt, pos);
	c->sel->cursor = text_mark_set(txt, text_char_next(txt, pos));
	view_draw(c->view);
}

void view_cursors_selection_restore(Cursor *c) {
	Text *txt = c->view->text;
	if (c->sel)
		return;
	Filerange sel = text_range_new(
		text_mark_get(txt, c->lastsel_anchor),
		text_mark_get(txt, c->lastsel_cursor)
	);
	if (!text_range_valid(&sel))
		return;
	if (!(c->sel = view_selections_new(c->view)))
		return;
	view_selections_set(c->sel, &sel);
	view_cursors_selection_sync(c);
	view_draw(c->view);
}

void view_cursors_selection_stop(Cursor *c) {
	c->sel = NULL;
}

void view_cursors_selection_clear(Cursor *c) {
	view_selections_free(c->sel);
	view_draw(c->view);
}

void view_cursors_selection_swap(Cursor *c) {
	if (!c->sel)
		return;
	view_selections_swap(c->sel);
	view_cursors_selection_sync(c);
}

void view_cursors_selection_sync(Cursor *c) {
	if (!c->sel)
		return;
	Text *txt = c->view->text;
	size_t anchor = text_mark_get(txt, c->sel->anchor);
	size_t cursor = text_mark_get(txt, c->sel->cursor);
	bool right_extending = anchor < cursor;
	if (right_extending)
		cursor = text_char_prev(txt, cursor);
	view_cursors_to(c, cursor);
}

Filerange view_cursors_selection_get(Cursor *c) {
	return view_selections_get(c->sel);
}

void view_cursors_selection_set(Cursor *c, Filerange *r) {
	if (!text_range_valid(r))
		return;
	if (!c->sel)
		c->sel = view_selections_new(c->view);
	if (!c->sel)
		return;

	view_selections_set(c->sel, r);
}

Selection *view_selections_new(View *view) {
	Selection *s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;

	s->view = view;
	s->next = view->selections;
	if (view->selections)
		view->selections->prev = s;
	view->selections = s;
	return s;
}

void view_selections_free(Selection *s) {
	if (!s)
		return;
	if (s->prev)
		s->prev->next = s->next;
	if (s->next)
		s->next->prev = s->prev;
	if (s->view->selections == s)
		s->view->selections = s->next;
	// XXX: add backlink Selection->Cursor?
	for (Cursor *c = s->view->cursors; c; c = c->next) {
		if (c->sel == s) {
			c->lastsel_anchor = s->anchor;
			c->lastsel_cursor = s->cursor;
			c->sel = NULL;
		}
	}
	free(s);
}

void view_selections_clear(View *view) {
	while (view->selections)
		view_selections_free(view->selections);
	view_draw(view);
}

void view_cursors_clear(View *view) {
	for (Cursor *c = view->cursors, *next; c; c = next) {
		next = c->next;
		if (c != view->cursor) {
			view_selections_free(c->sel);
			view_cursors_free(c);
		}
	}
	view_draw(view);
}

void view_selections_swap(Selection *s) {
	Mark temp = s->anchor;
	s->anchor = s->cursor;
	s->cursor = temp;
}

Selection *view_selections(View *view) {
	return view->selections;
}

Selection *view_selections_prev(Selection *s) {
	return s->prev;
}

Selection *view_selections_next(Selection *s) {
	return s->next;
}

Filerange view_selection_get(View *view) {
	return view_selections_get(view->cursor->sel);
}

Filerange view_selections_get(Selection *s) {
	if (!s)
		return text_range_empty();
	Text *txt = s->view->text;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	return text_range_new(anchor, cursor);
}

void view_selections_set(Selection *s, Filerange *r) {
	if (!text_range_valid(r))
		return;
	Text *txt = s->view->text;
	size_t anchor = text_mark_get(txt, s->anchor);
	size_t cursor = text_mark_get(txt, s->cursor);
	bool left_extending = anchor > cursor;
	if (left_extending) {
		s->anchor = text_mark_set(txt, r->end);
		s->cursor = text_mark_set(txt, r->start);
	} else {
		s->anchor = text_mark_set(txt, r->start);
		s->cursor = text_mark_set(txt, r->end);
	}
	view_draw(s->view);
}

Text *view_text(View *view) {
	return view->text;
}
