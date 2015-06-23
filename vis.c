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
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "ui-curses.h"
#include "editor.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"
#include "map.h"

/** global variables */
static Editor *vis;         /* global editor instance, keeps track of all windows etc. */

/** operators */
static size_t op_change(OperatorContext *c);
static size_t op_yank(OperatorContext *c);
static size_t op_put(OperatorContext *c);
static size_t op_delete(OperatorContext *c);
static size_t op_shift_right(OperatorContext *c);
static size_t op_shift_left(OperatorContext *c);
static size_t op_case_change(OperatorContext *c);
static size_t op_join(OperatorContext *c);
static size_t op_repeat_insert(OperatorContext *c);
static size_t op_repeat_replace(OperatorContext *c);

/* these can be passed as int argument to operator(&(const Arg){ .i = OP_*}) */
enum {
	OP_DELETE,
	OP_CHANGE,
	OP_YANK,
	OP_PUT,
	OP_SHIFT_RIGHT,
	OP_SHIFT_LEFT,
	OP_CASE_CHANGE,
	OP_JOIN,
	OP_REPEAT_INSERT,
	OP_REPEAT_REPLACE,
};

static Operator ops[] = {
	[OP_DELETE]      = { op_delete      },
	[OP_CHANGE]      = { op_change      },
	[OP_YANK]        = { op_yank        },
	[OP_PUT]         = { op_put         },
	[OP_SHIFT_RIGHT] = { op_shift_right },
	[OP_SHIFT_LEFT]  = { op_shift_left  },
	[OP_CASE_CHANGE] = { op_case_change },
	[OP_JOIN]          = { op_join          },
	[OP_REPEAT_INSERT]  = { op_repeat_insert  },
	[OP_REPEAT_REPLACE] = { op_repeat_replace },
};

#define PAGE      INT_MAX
#define PAGE_HALF (INT_MAX-1)

/* these can be passed as int argument to movement(&(const Arg){ .i = MOVE_* }) */
enum {
	MOVE_LINE_DOWN,
	MOVE_LINE_UP,
	MOVE_SCREEN_LINE_UP,
	MOVE_SCREEN_LINE_DOWN,
	MOVE_SCREEN_LINE_BEGIN,
	MOVE_SCREEN_LINE_MIDDLE,
	MOVE_SCREEN_LINE_END,
	MOVE_LINE_PREV,
	MOVE_LINE_BEGIN,
	MOVE_LINE_START,
	MOVE_LINE_FINISH,
	MOVE_LINE_LASTCHAR,
	MOVE_LINE_END,
	MOVE_LINE_NEXT,
	MOVE_LINE,
	MOVE_COLUMN,
	MOVE_CHAR_PREV,
	MOVE_CHAR_NEXT,
	MOVE_WORD_START_NEXT,
	MOVE_WORD_END_PREV,
	MOVE_WORD_END_NEXT,
	MOVE_WORD_START_PREV,
	MOVE_LONGWORD_START_PREV,
	MOVE_LONGWORD_START_NEXT,
	MOVE_LONGWORD_END_PREV,
	MOVE_LONGWORD_END_NEXT,
	MOVE_SENTENCE_PREV,
	MOVE_SENTENCE_NEXT,
	MOVE_PARAGRAPH_PREV,
	MOVE_PARAGRAPH_NEXT,
	MOVE_BRACKET_MATCH,
	MOVE_LEFT_TO,
	MOVE_RIGHT_TO,
	MOVE_LEFT_TILL,
	MOVE_RIGHT_TILL,
	MOVE_FILE_BEGIN,
	MOVE_FILE_END,
	MOVE_MARK,
	MOVE_MARK_LINE,
	MOVE_SEARCH_WORD_FORWARD,
	MOVE_SEARCH_WORD_BACKWARD,
	MOVE_SEARCH_FORWARD,
	MOVE_SEARCH_BACKWARD,
	MOVE_WINDOW_LINE_TOP,
	MOVE_WINDOW_LINE_MIDDLE,
	MOVE_WINDOW_LINE_BOTTOM,
};

/** movements which can be used besides the one in text-motions.h and view.h */

/* search in forward direction for the word under the cursor */
static size_t search_word_forward(Text *txt, size_t pos);
/* search in backward direction for the word under the cursor */
static size_t search_word_backward(Text *txt, size_t pos);
/* search again for the last used search pattern */
static size_t search_forward(Text *txt, size_t pos);
static size_t search_backward(Text *txt, size_t pos);
/* goto action.mark */
static size_t mark_goto(File *txt, size_t pos);
/* goto first non-blank char on line pointed by action.mark */
static size_t mark_line_goto(File *txt, size_t pos);
/* goto to next occurence of action.key to the right */
static size_t to(Text *txt, size_t pos);
/* goto to position before next occurence of action.key to the right */
static size_t till(Text *txt, size_t pos);
/* goto to next occurence of action.key to the left */
static size_t to_left(Text *txt, size_t pos);
/* goto to position after next occurence of action.key to the left */
static size_t till_left(Text *txt, size_t pos);
/* goto line number action.count */
static size_t line(Text *txt, size_t pos);
/* goto to byte action.count on current line */
static size_t column(Text *txt, size_t pos);
/* goto the action.count-th line from top of the focused window */
static size_t view_lines_top(const Arg *arg);
/* goto the start of middle line of the focused window */
static size_t view_lines_middle(const Arg *arg);
/* goto the action.count-th line from bottom of the focused window */
static size_t view_lines_bottom(const Arg *arg);

static Movement moves[] = {
	[MOVE_LINE_UP]             = { .view = view_line_up                                       },
	[MOVE_LINE_DOWN]           = { .view = view_line_down                                     },
	[MOVE_SCREEN_LINE_UP]      = { .view = view_screenline_up                                 },
	[MOVE_SCREEN_LINE_DOWN]    = { .view = view_screenline_down                               },
	[MOVE_SCREEN_LINE_BEGIN]   = { .view = view_screenline_begin,  .type = CHARWISE           },
	[MOVE_SCREEN_LINE_MIDDLE]  = { .view = view_screenline_middle, .type = CHARWISE           },
	[MOVE_SCREEN_LINE_END]     = { .view = view_screenline_end,    .type = CHARWISE|INCLUSIVE },
	[MOVE_LINE_PREV]           = { .txt = text_line_prev,           .type = LINEWISE           },
	[MOVE_LINE_BEGIN]          = { .txt = text_line_begin,          .type = LINEWISE           },
	[MOVE_LINE_START]          = { .txt = text_line_start,          .type = LINEWISE           },
	[MOVE_LINE_FINISH]         = { .txt = text_line_finish,         .type = LINEWISE|INCLUSIVE },
	[MOVE_LINE_LASTCHAR]       = { .txt = text_line_lastchar,       .type = LINEWISE|INCLUSIVE },
	[MOVE_LINE_END]            = { .txt = text_line_end,            .type = LINEWISE           },
	[MOVE_LINE_NEXT]           = { .txt = text_line_next,           .type = LINEWISE           },
	[MOVE_LINE]                = { .txt = line,                     .type = LINEWISE|IDEMPOTENT|JUMP},
	[MOVE_COLUMN]              = { .txt = column,                   .type = CHARWISE|IDEMPOTENT},
	[MOVE_CHAR_PREV]           = { .view = view_char_prev                                     },
	[MOVE_CHAR_NEXT]           = { .view = view_char_next                                     },
	[MOVE_WORD_START_PREV]     = { .txt = text_word_start_prev,     .type = CHARWISE           },
	[MOVE_WORD_START_NEXT]     = { .txt = text_word_start_next,     .type = CHARWISE           },
	[MOVE_WORD_END_PREV]       = { .txt = text_word_end_prev,       .type = CHARWISE|INCLUSIVE },
	[MOVE_WORD_END_NEXT]       = { .txt = text_word_end_next,       .type = CHARWISE|INCLUSIVE },
	[MOVE_LONGWORD_START_PREV] = { .txt = text_longword_start_prev, .type = CHARWISE           },
	[MOVE_LONGWORD_START_NEXT] = { .txt = text_longword_start_next, .type = CHARWISE           },
	[MOVE_LONGWORD_END_PREV]   = { .txt = text_longword_end_prev,   .type = CHARWISE|INCLUSIVE },
	[MOVE_LONGWORD_END_NEXT]   = { .txt = text_longword_end_next,   .type = CHARWISE|INCLUSIVE },
	[MOVE_SENTENCE_PREV]       = { .txt = text_sentence_prev,       .type = LINEWISE           },
	[MOVE_SENTENCE_NEXT]       = { .txt = text_sentence_next,       .type = LINEWISE           },
	[MOVE_PARAGRAPH_PREV]      = { .txt = text_paragraph_prev,      .type = LINEWISE|JUMP      },
	[MOVE_PARAGRAPH_NEXT]      = { .txt = text_paragraph_next,      .type = LINEWISE|JUMP      },
	[MOVE_BRACKET_MATCH]       = { .txt = text_bracket_match,       .type = LINEWISE|INCLUSIVE|JUMP },
	[MOVE_FILE_BEGIN]          = { .txt = text_begin,               .type = LINEWISE|JUMP      },
	[MOVE_FILE_END]            = { .txt = text_end,                 .type = LINEWISE|JUMP      },
	[MOVE_LEFT_TO]             = { .txt = to_left,                  .type = LINEWISE           },
	[MOVE_RIGHT_TO]            = { .txt = to,                       .type = LINEWISE|INCLUSIVE },
	[MOVE_LEFT_TILL]           = { .txt = till_left,                .type = LINEWISE           },
	[MOVE_RIGHT_TILL]          = { .txt = till,                     .type = LINEWISE|INCLUSIVE },
	[MOVE_MARK]                = { .file = mark_goto,             .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_MARK_LINE]           = { .file = mark_line_goto,        .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_SEARCH_WORD_FORWARD] = { .txt = search_word_forward,      .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_WORD_BACKWARD]= { .txt = search_word_backward,     .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_FORWARD]      = { .txt = search_forward,           .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_BACKWARD]     = { .txt = search_backward,          .type = LINEWISE|JUMP      },
	[MOVE_WINDOW_LINE_TOP]     = { .cmd = view_lines_top,         .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_WINDOW_LINE_MIDDLE]  = { .cmd = view_lines_middle,      .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_WINDOW_LINE_BOTTOM]  = { .cmd = view_lines_bottom,      .type = LINEWISE|JUMP|IDEMPOTENT },
};

/* these can be passed as int argument to textobj(&(const Arg){ .i = TEXT_OBJ_* }) */
enum {
	TEXT_OBJ_INNER_WORD,
	TEXT_OBJ_OUTER_WORD,
	TEXT_OBJ_INNER_LONGWORD,
	TEXT_OBJ_OUTER_LONGWORD,
	TEXT_OBJ_LINE_UP,
	TEXT_OBJ_LINE_DOWN,
	TEXT_OBJ_SENTENCE,
	TEXT_OBJ_PARAGRAPH,
	TEXT_OBJ_OUTER_SQUARE_BRACKET,
	TEXT_OBJ_INNER_SQUARE_BRACKET,
	TEXT_OBJ_OUTER_CURLY_BRACKET,
	TEXT_OBJ_INNER_CURLY_BRACKET,
	TEXT_OBJ_OUTER_ANGLE_BRACKET,
	TEXT_OBJ_INNER_ANGLE_BRACKET,
	TEXT_OBJ_OUTER_PARANTHESE,
	TEXT_OBJ_INNER_PARANTHESE,
	TEXT_OBJ_OUTER_QUOTE,
	TEXT_OBJ_INNER_QUOTE,
	TEXT_OBJ_OUTER_SINGLE_QUOTE,
	TEXT_OBJ_INNER_SINGLE_QUOTE,
	TEXT_OBJ_OUTER_BACKTICK,
	TEXT_OBJ_INNER_BACKTICK,
};

static TextObject textobjs[] = {
	[TEXT_OBJ_INNER_WORD]           = { text_object_word                  },
	[TEXT_OBJ_OUTER_WORD]           = { text_object_word_outer            },
	[TEXT_OBJ_INNER_LONGWORD]       = { text_object_longword              },
	[TEXT_OBJ_OUTER_LONGWORD]       = { text_object_longword_outer        },
	[TEXT_OBJ_LINE_UP]              = { text_object_line                  },
	[TEXT_OBJ_LINE_DOWN]            = { text_object_line                  },
	[TEXT_OBJ_SENTENCE]             = { text_object_sentence              },
	[TEXT_OBJ_PARAGRAPH]            = { text_object_paragraph             },
	[TEXT_OBJ_OUTER_SQUARE_BRACKET] = { text_object_square_bracket, OUTER },
	[TEXT_OBJ_INNER_SQUARE_BRACKET] = { text_object_square_bracket, INNER },
	[TEXT_OBJ_OUTER_CURLY_BRACKET]  = { text_object_curly_bracket,  OUTER },
	[TEXT_OBJ_INNER_CURLY_BRACKET]  = { text_object_curly_bracket,  INNER },
	[TEXT_OBJ_OUTER_ANGLE_BRACKET]  = { text_object_angle_bracket,  OUTER },
	[TEXT_OBJ_INNER_ANGLE_BRACKET]  = { text_object_angle_bracket,  INNER },
	[TEXT_OBJ_OUTER_PARANTHESE]     = { text_object_paranthese,     OUTER },
	[TEXT_OBJ_INNER_PARANTHESE]     = { text_object_paranthese,     INNER },
	[TEXT_OBJ_OUTER_QUOTE]          = { text_object_quote,          OUTER },
	[TEXT_OBJ_INNER_QUOTE]          = { text_object_quote,          INNER },
	[TEXT_OBJ_OUTER_SINGLE_QUOTE]   = { text_object_single_quote,   OUTER },
	[TEXT_OBJ_INNER_SINGLE_QUOTE]   = { text_object_single_quote,   INNER },
	[TEXT_OBJ_OUTER_BACKTICK]       = { text_object_backtick,       OUTER },
	[TEXT_OBJ_INNER_BACKTICK]       = { text_object_backtick,       INNER },
};

/* if some movements are forced to be linewise, they are translated to text objects */
static TextObject *moves_linewise[] = {
	[MOVE_LINE_UP]          = &textobjs[TEXT_OBJ_LINE_UP],
	[MOVE_LINE_DOWN]        = &textobjs[TEXT_OBJ_LINE_DOWN],
	[MOVE_SCREEN_LINE_UP]   = &textobjs[TEXT_OBJ_LINE_UP],
	[MOVE_SCREEN_LINE_DOWN] = &textobjs[TEXT_OBJ_LINE_DOWN],
};

/** functions to be called from keybindings */
/* navigate jump list either in forward (arg->i>0) or backward (arg->i<0) direction */
static void jumplist(const Arg *arg);
/* navigate change list either in forward (arg->i>0) or backward (arg->i<0) direction */
static void changelist(const Arg *arg);
static void macro_record(const Arg *arg);
static void macro_replay(const Arg *arg);
/* temporarily suspend the editor and return to the shell, type 'fg' to get back */
static void suspend(const Arg *arg);
/* switch to mode indicated by arg->i */
static void switchmode(const Arg *arg);
/* set mark indicated by arg->i to current cursor position */
static void mark_set(const Arg *arg);
/* insert arg->s at the current cursor position */
static void insert(const Arg *arg);
/* insert a tab or the needed amount of spaces at the current cursor position */
static void insert_tab(const Arg *arg);
/* inserts a newline (either \n or \r\n depending on file type) */
static void insert_newline(const Arg *arg);
/* put register content either before (if arg->i < 0) or after (if arg->i > 0)
 * current cursor position */
static void put(const Arg *arg);
/* add a new line either before or after the one where the cursor currently is */
static void openline(const Arg *arg);
/* join lines from current cursor position to movement indicated by arg */
static void join(const Arg *arg);
/* execute arg->s as if it was typed on command prompt */
static void cmd(const Arg *arg);
/* perform last action i.e. action_prev again */
static void repeat(const Arg *arg);
/* repeat last to/till movement */
static void totill_repeat(const Arg *arg);
/* repeat last to/till movement but in opposite direction */
static void totill_reverse(const Arg *arg);
/* replace character at cursor with one read form keyboard */
static void replace(const Arg *arg);
/* adjust action.count by arg->i */
static void count(const Arg *arg);
/* move to the action.count-th line or if not given either to the first (arg->i < 0)
 *  or last (arg->i > 0) line of file */
static void gotoline(const Arg *arg);
/* force operator to linewise (if arg->b is set) */
static void linewise(const Arg *arg);
/* make the current action use the operator indicated by arg->i */
static void operator(const Arg *arg);
/* execute operator twice useful for synonyms (e.g. 'cc') */
static void operator_twice(const Arg *arg);
/* change case of a file range to upper (arg->i > 0) or lowercase (arg->i < 0) */
static void changecase(const Arg *arg);
/* blocks to read a key and performs movement indicated by arg->i which
 * should be one of MOVE_{RIGHT,LEFT}_{TO,TILL} */
static void movement_key(const Arg *arg);
/* perform the movement as indicated by arg->i */
static void movement(const Arg *arg);
/* let the current operator affect the range indicated by the text object arg->i */
static void textobj(const Arg *arg);
/* move to the other end of selected text */
static void selection_end(const Arg *arg);
/* use register indicated by arg->i for the current operator */
static void reg(const Arg *arg);
/* perform a movement to mark arg->i */
static void mark(const Arg *arg);
/* perform a movement to the first non-blank on the line pointed by mark arg->i */
static void mark_line(const Arg *arg);
/* {un,re}do last action, redraw window */
static void undo(const Arg *arg);
static void redo(const Arg *arg);
/* earlier, later action chronologically, redraw window */
static void earlier(const Arg *arg);
static void later(const Arg *arg);
/* either part of multiplier or a movement to begin of line */
static void zero(const Arg *arg);
/* hange/delete from the current cursor position to the end of
 * movement as indicated by arg->i */
static void change(const Arg *arg);
static void delete(const Arg *arg);
/* perform movement according to arg->i, then switch to insert mode */
static void insertmode(const Arg *arg);
/* insert register content indicated by arg->i at current cursor position */
static void insert_register(const Arg *arg);
/* show a user prompt to get input with title arg->s */
static void prompt_search(const Arg *arg);
static void prompt_cmd(const Arg *arg);
/* evaluate user input at prompt, perform search or execute a command */
static void prompt_enter(const Arg *arg);
/* cycle through past user inputs */
static void prompt_up(const Arg *arg);
static void prompt_down(const Arg *arg);
/* exit command mode if the last char is deleted */
static void prompt_backspace(const Arg *arg);
/* blocks to read 3 consecutive digits and inserts the corresponding byte value */
static void insert_verbatim(const Arg *arg);
/* scroll window content according to arg->i which can be either PAGE, PAGE_HALF,
 * or an arbitrary number of lines. a multiplier overrides what is given in arg->i.
 * negative values scroll back, positive forward. */
static void wscroll(const Arg *arg);
/* similar to scroll, but do only move window content not cursor position */
static void wslide(const Arg *arg);
/* call editor function as indicated by arg->f */
static void call(const Arg *arg);
/* call window function as indicated by arg->w */
static void window(const Arg *arg);
/* quit editor, discard all changes */
static void quit(const Arg *arg);

/** commands to enter at the ':'-prompt */
/* set various runtime options */
static bool cmd_set(Filerange*, enum CmdOpt, const char *argv[]);
/* for each argument create a new window and open the corresponding file */
static bool cmd_open(Filerange*, enum CmdOpt, const char *argv[]);
/* close current window (discard modifications if forced ) and open argv[1],
 * if no argv[1] is given re-read to current file from disk */
static bool cmd_edit(Filerange*, enum CmdOpt, const char *argv[]);
/* close the current window, discard modifications if forced */
static bool cmd_quit(Filerange*, enum CmdOpt, const char *argv[]);
/* close all windows which show current file, discard modifications if forced  */
static bool cmd_bdelete(Filerange*, enum CmdOpt, const char *argv[]);
/* close all windows, exit editor, discard modifications if forced */
static bool cmd_qall(Filerange*, enum CmdOpt, const char *argv[]);
/* for each argument try to insert the file content at current cursor postion */
static bool cmd_read(Filerange*, enum CmdOpt, const char *argv[]);
static bool cmd_substitute(Filerange*, enum CmdOpt, const char *argv[]);
/* if no argument are given, split the current window horizontally,
 * otherwise open the file */
static bool cmd_split(Filerange*, enum CmdOpt, const char *argv[]);
/* if no argument are given, split the current window vertically,
 * otherwise open the file */
static bool cmd_vsplit(Filerange*, enum CmdOpt, const char *argv[]);
/* create a new empty window and arrange all windows either horizontally or vertically */
static bool cmd_new(Filerange*, enum CmdOpt, const char *argv[]);
static bool cmd_vnew(Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window and close it */
static bool cmd_wq(Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window if it was changed, then close the window */
static bool cmd_xit(Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window to the name given.
 * do not change internal filname association. further :w commands
 * without arguments will still write to the old filename */
static bool cmd_write(Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window to the name given,
 * associate the new name with the buffer. further :w commands
 * without arguments will write to the new filename */
static bool cmd_saveas(Filerange*, enum CmdOpt, const char *argv[]);
/* filter range through external program argv[1] */
static bool cmd_filter(Filerange*, enum CmdOpt, const char *argv[]);
/* switch to the previous saved state of the text, chronologically */
static bool cmd_earlier(Filerange*, enum CmdOpt, const char *argv[]);
/* switch to the next saved state of the text, chronologically */
static bool cmd_later(Filerange*, enum CmdOpt, const char *argv[]);

static void action_reset(Action *a);
static void switchmode_to(Mode *new_mode);
static bool vis_window_new(const char *file);
static bool vis_window_split(Win *win);

#include "config.h"

static Key getkey(void);
static void keypress(Key *key);
static void action_do(Action *a);
static bool exec_command(char type, const char *cmdline);

/** operator implementations of type: void (*op)(OperatorContext*) */

static size_t op_delete(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t len = c->range.end - c->range.start;
	c->reg->linewise = c->linewise;
	register_put(c->reg, txt, &c->range);
	text_delete(txt, c->range.start, len);
	return c->range.start;
}

static size_t op_change(OperatorContext *c) {
	size_t pos = op_delete(c);
	switchmode(&(const Arg){ .i = VIS_MODE_INSERT });
	return pos;
}

static size_t op_yank(OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put(c->reg, vis->win->file->text, &c->range);
	return c->pos;
}

static size_t op_put(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t pos = c->pos;
	if (c->arg->i > 0) {
		if (c->reg->linewise)
			pos = text_line_next(txt, pos);
		else
			pos = text_char_next(txt, pos);
	} else {
		if (c->reg->linewise)
			pos = text_line_begin(txt, pos);
	}
	text_insert(txt, pos, c->reg->data, c->reg->len);
	if (c->reg->linewise)
		return text_line_start(txt, pos);
	else
		return pos + c->reg->len;
}

static const char *expand_tab(void) {
	static char spaces[9];
	int tabwidth = editor_tabwidth_get(vis);
	tabwidth = MIN(tabwidth, LENGTH(spaces) - 1);
	for (int i = 0; i < tabwidth; i++)
		spaces[i] = ' ';
	spaces[tabwidth] = '\0';
	return vis->expandtab ? spaces : "\t";
}

static size_t op_shift_right(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	const char *tab = expand_tab();
	size_t tablen = strlen(tab);

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		prev_pos = pos = text_line_begin(txt, pos);
		text_insert(txt, pos, tab, tablen);
		pos = text_line_prev(txt, pos);
	}  while (pos >= c->range.start && pos != prev_pos);

	return c->pos + tablen;
}

static size_t op_shift_left(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	size_t tabwidth = editor_tabwidth_get(vis), tablen;

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		char c;
		size_t len = 0;
		prev_pos = pos = text_line_begin(txt, pos);
		Iterator it = text_iterator_get(txt, pos);
		if (text_iterator_byte_get(&it, &c) && c == '\t') {
			len = 1;
		} else {
			for (len = 0; text_iterator_byte_get(&it, &c) && c == ' '; len++)
				text_iterator_byte_next(&it, NULL);
		}
		tablen = MIN(len, tabwidth);
		text_delete(txt, pos, tablen);
		pos = text_line_prev(txt, pos);
	}  while (pos >= c->range.start && pos != prev_pos);

	return c->pos - tablen;
}

static size_t op_case_change(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t len = c->range.end - c->range.start;
	char *buf = malloc(len);
	if (!buf)
		return c->pos;
	len = text_bytes_get(txt, c->range.start, len, buf);
	size_t rem = len;
	for (char *cur = buf; rem > 0; cur++, rem--) {
		int ch = (unsigned char)*cur;
		if (isascii(ch)) {
			if (c->arg->i == 0)
				*cur = islower(ch) ? toupper(ch) : tolower(ch);
			else if (c->arg->i > 0)
				*cur = toupper(ch);
			else
				*cur = tolower(ch);
		}
	}

	text_delete(txt, c->range.start, len);
	text_insert(txt, c->range.start, buf, len);
	free(buf);
	return c->pos;
}

static size_t op_join(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	Filerange sel = view_selection_get(vis->win->view);
	/* if a selection ends at the begin of a line, skip line break */
	if (pos == c->range.end && text_range_valid(&sel))
		pos = text_line_prev(txt, pos);

	do {
		prev_pos = pos;
		size_t end = text_line_start(txt, pos);
		pos = text_char_next(txt, text_line_finish(txt, text_line_prev(txt, end)));
		if (pos >= c->range.start && end > pos) {
			text_delete(txt, pos, end - pos);
			text_insert(txt, pos, " ", 1);
		} else {
			break;
		}
	} while (pos != prev_pos);

	return c->range.start;
}

static size_t op_repeat_insert(OperatorContext *c) {
	size_t len = vis->buffer_repeat.len;
	if (!len)
		return c->pos;
	text_insert(vis->win->file->text, c->pos, vis->buffer_repeat.data, len);
	return c->pos + len;
}

static size_t op_repeat_replace(OperatorContext *c) {
	Text *txt = vis->win->file->text;
	size_t chars = 0, len = vis->buffer_repeat.len;
	if (!len)
		return c->pos;
	const char *data = vis->buffer_repeat.data;
	for (size_t i = 0; i < len; i++) {
		if (ISUTF8(data[i]))
			chars++;
	}

	Iterator it = text_iterator_get(txt, c->pos);
	while (chars-- > 0)
		text_iterator_char_next(&it, NULL);
	text_delete(txt, c->pos, it.pos - c->pos);
	return op_repeat_insert(c);
}

/** movement implementations of type: size_t (*move)(const Arg*) */

static char *get_word_at(Text *txt, size_t pos) {
	Filerange word = text_object_word(txt, pos);
	if (!text_range_valid(&word))
		return NULL;
	size_t len = word.end - word.start;
	char *buf = malloc(len+1);
	if (!buf)
		return NULL;
	len = text_bytes_get(txt, word.start, len, buf);
	buf[len] = '\0';
	return buf;
}

static size_t search_word_forward(Text *txt, size_t pos) {
	char *word = get_word_at(txt, pos);
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_forward(txt, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_word_backward(Text *txt, size_t pos) {
	char *word = get_word_at(txt, pos);
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_backward(txt, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_forward(Text *txt, size_t pos) {
	return text_search_forward(txt, pos, vis->search_pattern);
}

static size_t search_backward(Text *txt, size_t pos) {
	return text_search_backward(txt, pos, vis->search_pattern);
}

static void mark_set(const Arg *arg) {
	size_t pos = view_cursor_get(vis->win->view);
	vis->win->file->marks[arg->i] = text_mark_set(vis->win->file->text, pos);
}

static size_t mark_goto(File *txt, size_t pos) {
	return text_mark_get(txt->text, txt->marks[vis->action.mark]);
}

static size_t mark_line_goto(File *txt, size_t pos) {
	return text_line_start(txt->text, mark_goto(txt, pos));
}

static size_t to(Text *txt, size_t pos) {
	char c;
	size_t hit = text_find_next(txt, pos+1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till(Text *txt, size_t pos) {
	size_t hit = to(txt, pos);
	if (hit != pos)
		return text_char_prev(txt, hit);
	return pos;
}

static size_t to_left(Text *txt, size_t pos) {
	char c;
	if (pos == 0)
		return pos;
	size_t hit = text_find_prev(txt, pos-1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till_left(Text *txt, size_t pos) {
	size_t hit = to_left(txt, pos);
	if (hit != pos)
		return text_char_next(txt, hit);
	return pos;
}

static size_t line(Text *txt, size_t pos) {
	return text_pos_by_lineno(txt, vis->action.count);
}

static size_t column(Text *txt, size_t pos) {
	return text_line_offset(txt, pos, vis->action.count);
}

static size_t view_lines_top(const Arg *arg) {
	return view_screenline_goto(vis->win->view, vis->action.count);
}

static size_t view_lines_middle(const Arg *arg) {
	int h = view_height_get(vis->win->view);
	return view_screenline_goto(vis->win->view, h/2);
}

static size_t view_lines_bottom(const Arg *arg) {
	int h = view_height_get(vis->win->view);
	return view_screenline_goto(vis->win->view, h - vis->action.count);
}

/** key bindings functions of type: void (*func)(const Arg*) */

static void jumplist(const Arg *arg) {
	size_t pos;
	if (arg->i > 0)
		pos = editor_window_jumplist_next(vis->win);
	else
		pos = editor_window_jumplist_prev(vis->win);
	if (pos != EPOS)
		view_cursor_to(vis->win->view, pos);
}

static void changelist(const Arg *arg) {
	size_t pos;
	if (arg->i > 0)
		pos = editor_window_changelist_next(vis->win);
	else
		pos = editor_window_changelist_prev(vis->win);
	if (pos != EPOS)
		view_cursor_to(vis->win->view, pos);
}

static Macro *key2macro(const Arg *arg) {
	if (arg->i)
		return &vis->macros[arg->i];
	Key key = getkey();
	if (key.str[0] >= 'a' && key.str[0] <= 'z')
		return &vis->macros[key.str[0] - 'a'];
	if (key.str[0] == '@')
		return vis->last_recording;
	return NULL;
}

static void macro_record(const Arg *arg) {
	if (vis->recording) {
		/* hack to remove last recorded key, otherwise upon replay
		 * we would start another recording */
		vis->recording->len -= sizeof(Key);
		vis->last_recording = vis->recording;
		vis->recording = NULL;
	} else {
		vis->recording = key2macro(arg);
		if (vis->recording)
			macro_reset(vis->recording);
	}
	editor_draw(vis);
}

static void macro_replay(const Arg *arg) {
	Macro *macro = key2macro(arg);
	if (!macro || macro == vis->recording)
		return;
	for (size_t i = 0; i < macro->len; i += sizeof(Key))
		keypress((Key*)(macro->data + i));
}

static void suspend(const Arg *arg) {
	editor_suspend(vis);
}

static void repeat(const Arg *arg) {
	int count = vis->action.count;
	vis->action = vis->action_prev;
	if (count)
		vis->action.count = count;
	action_do(&vis->action);
}

static void totill_repeat(const Arg *arg) {
	if (!vis->last_totill)
		return;
	movement(&(const Arg){ .i = vis->last_totill });
}

static void totill_reverse(const Arg *arg) {
	int type = vis->last_totill;
	switch (type) {
	case MOVE_RIGHT_TO:
		type = MOVE_LEFT_TO;
		break;
	case MOVE_LEFT_TO:
		type = MOVE_RIGHT_TO;
		break;
	case MOVE_RIGHT_TILL:
		type = MOVE_LEFT_TILL;
		break;
	case MOVE_LEFT_TILL:
		type = MOVE_RIGHT_TILL;
		break;
	default:
		return;
	}
	movement(&(const Arg){ .i = type });
}

static void replace(const Arg *arg) {
	Key k = getkey();
	if (!k.str[0])
		return;
	size_t pos = view_cursor_get(vis->win->view);
	action_reset(&vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_REPLACE];
	buffer_put(&vis->buffer_repeat, k.str, strlen(k.str));
	editor_delete_key(vis);
	editor_insert_key(vis, k.str, strlen(k.str));
	text_snapshot(vis->win->file->text);
	view_cursor_to(vis->win->view, pos);
}

static void count(const Arg *arg) {
	vis->action.count = vis->action.count * 10 + arg->i;
}

static void gotoline(const Arg *arg) {
	if (vis->action.count)
		movement(&(const Arg){ .i = MOVE_LINE });
	else if (arg->i < 0)
		movement(&(const Arg){ .i = MOVE_FILE_BEGIN });
	else
		movement(&(const Arg){ .i = MOVE_FILE_END });
}

static void linewise(const Arg *arg) {
	vis->action.linewise = arg->b;
}

static void operator(const Arg *arg) {
	Operator *op = &ops[arg->i];
	if (vis->mode->visual) {
		vis->action.op = op;
		action_do(&vis->action);
		return;
	}
	/* switch to operator mode inorder to make operator options and
	 * text-object available */
	switchmode(&(const Arg){ .i = VIS_MODE_OPERATOR });
	if (vis->action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		vis->action.linewise = true;
		vis->action.textobj = moves_linewise[MOVE_SCREEN_LINE_DOWN];
		action_do(&vis->action);
	} else {
		vis->action.op = op;
	}
}

static void operator_twice(const Arg *arg) {
	operator(arg);
	operator(arg);
}

static void changecase(const Arg *arg) {
	vis->action.arg = *arg;
	operator(&(const Arg){ .i = OP_CASE_CHANGE });
}

static void movement_key(const Arg *arg) {
	Key k = getkey();
	if (!k.str[0]) {
		action_reset(&vis->action);
		return;
	}
	strncpy(vis->search_char, k.str, sizeof(vis->search_char));
	vis->last_totill = arg->i;
	vis->action.movement = &moves[arg->i];
	action_do(&vis->action);
}

static void movement(const Arg *arg) {
	if (vis->action.linewise && arg->i < LENGTH(moves_linewise))
		vis->action.textobj = moves_linewise[arg->i];
	else
		vis->action.movement = &moves[arg->i];

	if (vis->action.op == &ops[OP_CHANGE]) {
		if (vis->action.movement == &moves[MOVE_WORD_START_NEXT])
			vis->action.movement = &moves[MOVE_WORD_END_NEXT];
		else if (vis->action.movement == &moves[MOVE_LONGWORD_START_NEXT])
			vis->action.movement = &moves[MOVE_LONGWORD_END_NEXT];
	}

	action_do(&vis->action);
}

static void textobj(const Arg *arg) {
	vis->action.textobj = &textobjs[arg->i];
	action_do(&vis->action);
}

static void selection_end(const Arg *arg) {
	size_t pos = view_cursor_get(vis->win->view);
	Filerange sel = view_selection_get(vis->win->view);
	if (pos == sel.start) {
		pos = text_char_prev(vis->win->file->text, sel.end);
	} else {
		pos = sel.start;
		sel.start = text_char_prev(vis->win->file->text, sel.end);
		sel.end = pos;
	}
	view_selection_set(vis->win->view, &sel);
	view_cursor_to(vis->win->view, pos);
}

static void reg(const Arg *arg) {
	vis->action.reg = &vis->registers[arg->i];
}

static void mark(const Arg *arg) {
	vis->action.mark = arg->i;
	vis->action.movement = &moves[MOVE_MARK];
	action_do(&vis->action);
}

static void mark_line(const Arg *arg) {
	vis->action.mark = arg->i;
	vis->action.movement = &moves[MOVE_MARK_LINE];
	action_do(&vis->action);
}

static void undo(const Arg *arg) {
	size_t pos = text_undo(vis->win->file->text);
	if (pos != EPOS) {
		view_cursor_to(vis->win->view, pos);
		/* redraw all windows in case some display the same file */
		editor_draw(vis);
	}
}

static void redo(const Arg *arg) {
	size_t pos = text_redo(vis->win->file->text);
	if (pos != EPOS) {
		view_cursor_to(vis->win->view, pos);
		/* redraw all windows in case some display the same file */
		editor_draw(vis);
	}
}

static void earlier(const Arg *arg) {
	size_t pos = text_earlier(vis->win->file->text);
	if (pos != EPOS) {
		view_cursor_to(vis->win->view, pos);
		/* redraw all windows in case some display the same file */
		editor_draw(vis);
	}
}

static void later(const Arg *arg) {
	size_t pos = text_later(vis->win->file->text);
	if (pos != EPOS) {
		view_cursor_to(vis->win->view, pos);
		/* redraw all windows in case some display the same file */
		editor_draw(vis);
	}
}

static void zero(const Arg *arg) {
	if (vis->action.count == 0)
		movement(&(const Arg){ .i = MOVE_LINE_BEGIN });
	else
		count(&(const Arg){ .i = 0 });
}

static void insertmode(const Arg *arg) {
	movement(arg);
	switchmode(&(const Arg){ .i = VIS_MODE_INSERT });
}

static void change(const Arg *arg) {
	operator(&(const Arg){ .i = OP_CHANGE });
	movement(arg);
}

static void delete(const Arg *arg) {
	operator(&(const Arg){ .i = OP_DELETE });
	movement(arg);
}

static void insert_register(const Arg *arg) {
	Register *reg = &vis->registers[arg->i];
	int pos = view_cursor_get(vis->win->view);
	editor_insert(vis, pos, reg->data, reg->len);
	view_cursor_to(vis->win->view, pos + reg->len);
}

static void prompt_search(const Arg *arg) {
	editor_prompt_show(vis, arg->s, "");
	switchmode(&(const Arg){ .i = VIS_MODE_PROMPT });
}

static void prompt_cmd(const Arg *arg) {
	editor_prompt_show(vis, ":", arg->s);
	switchmode(&(const Arg){ .i = VIS_MODE_PROMPT });
}

static void prompt_enter(const Arg *arg) {
	char *s = editor_prompt_get(vis);
	/* it is important to switch back to the previous mode, which hides
	 * the prompt and more importantly resets vis->win to the currently
	 * focused editor window *before* anything is executed which depends
	 * on vis->win.
	 */
	switchmode_to(vis->mode_before_prompt);
	if (s && *s && exec_command(vis->prompt_type, s) && vis->running)
		switchmode(&(const Arg){ .i = VIS_MODE_NORMAL });
	free(s);
	editor_draw(vis);
}

static void prompt_up(const Arg *arg) {

}

static void prompt_down(const Arg *arg) {

}

static void prompt_backspace(const Arg *arg) {
	char *cmd = editor_prompt_get(vis);
	if (!cmd || !*cmd)
		prompt_enter(NULL);
	else
		view_backspace_key(vis->win->view);
	free(cmd);
}

static void insert_verbatim(const Arg *arg) {
	int value = 0;
	for (int i = 0; i < 3; i++) {
		Key k = getkey();
		if (k.str[0] < '0' || k.str[0] > '9')
			return;
		value = value * 10 + k.str[0] - '0';
	}
	char v = value;
	editor_insert(vis, view_cursor_get(vis->win->view), &v, 1);
}

static void quit(const Arg *arg) {
	vis->running = false;
}

static void cmd(const Arg *arg) {
	exec_command(':', arg->s);
}

static int argi2lines(const Arg *arg) {
	switch (arg->i) {
	case -PAGE:
	case +PAGE:
		return view_height_get(vis->win->view);
	case -PAGE_HALF:
	case +PAGE_HALF:
		return view_height_get(vis->win->view)/2;
	default:
		if (vis->action.count > 0)
			return vis->action.count;
		return arg->i < 0 ? -arg->i : arg->i;
	}
}

static void wscroll(const Arg *arg) {
	if (arg->i >= 0)
		view_scroll_down(vis->win->view, argi2lines(arg));
	else
		view_scroll_up(vis->win->view, argi2lines(arg));
}

static void wslide(const Arg *arg) {
	if (arg->i >= 0)
		view_slide_down(vis->win->view, argi2lines(arg));
	else
		view_slide_up(vis->win->view, argi2lines(arg));
}

static void call(const Arg *arg) {
	arg->f(vis);
}

static void window(const Arg *arg) {
	arg->w(vis->win->view);
}

static void insert(const Arg *arg) {
	editor_insert_key(vis, arg->s, arg->s ? strlen(arg->s) : 0);
}

static void insert_tab(const Arg *arg) {
	insert(&(const Arg){ .s = expand_tab() });
}

static void copy_indent_from_previous_line(View *view, Text *text) {
	size_t pos = view_cursor_get(view);
	size_t prev_line = text_line_prev(text, pos);
	if (pos == prev_line)
		return;
	size_t begin = text_line_begin(text, prev_line);
	size_t start = text_line_start(text, begin);
	size_t len = start-begin;
	char *buf = malloc(len);
	if (!buf)
		return;
	len = text_bytes_get(text, begin, len, buf);
	editor_insert_key(vis, buf, len);
	free(buf);
}

static void insert_newline(const Arg *arg) {
	const char *nl;
	switch (text_newline_type(vis->win->file->text)) {
	case TEXT_NEWLINE_CRNL:
		nl = "\r\n";
		break;
	default:
		nl = "\n";
		break;
	}

	insert(&(const Arg){ .s = nl });

	if (vis->autoindent)
		copy_indent_from_previous_line(vis->win->view, vis->win->file->text);
}

static void put(const Arg *arg) {
	vis->action.arg = *arg;
	operator(&(const Arg){ .i = OP_PUT });
	action_do(&vis->action);
}

static void openline(const Arg *arg) {
	if (arg->i == MOVE_LINE_NEXT) {
		movement(&(const Arg){ .i = MOVE_LINE_END });
		insert_newline(NULL);
	} else {
		movement(&(const Arg){ .i = MOVE_LINE_BEGIN });
		insert_newline(NULL);
		movement(&(const Arg){ .i = MOVE_LINE_PREV });
	}
	switchmode(&(const Arg){ .i = VIS_MODE_INSERT });
}

static void join(const Arg *arg) {
	operator(&(const Arg){ .i = OP_JOIN });
	movement(arg);
}

static void switchmode(const Arg *arg) {
	switchmode_to(&vis_modes[arg->i]);
}

/** action processing: execut the operator / movement / text object */

static void action_do(Action *a) {
	Text *txt = vis->win->file->text;
	View *view = vis->win->view;
	size_t pos = view_cursor_get(view);
	int count = MAX(1, a->count);
	OperatorContext c = {
		.count = a->count,
		.pos = pos,
		.range = text_range_empty(),
		.reg = a->reg ? a->reg : &vis->registers[REG_DEFAULT],
		.linewise = a->linewise,
		.arg = &a->arg,
	};

	if (a->movement) {
		size_t start = pos;
		for (int i = 0; i < count; i++) {
			if (a->movement->txt)
				pos = a->movement->txt(txt, pos);
			else if (a->movement->view)
				pos = a->movement->view(view);
			else if (a->movement->file)
				pos = a->movement->file(vis->win->file, pos);
			else
				pos = a->movement->cmd(&a->arg);
			if (pos == EPOS || a->movement->type & IDEMPOTENT)
				break;
		}

		if (pos == EPOS) {
			c.range.start = start;
			c.range.end = start;
			pos = start;
		} else {
			c.range.start = MIN(start, pos);
			c.range.end = MAX(start, pos);
		}

		if (!a->op) {
			if (a->movement->type & CHARWISE)
				view_scroll_to(view, pos);
			else
				view_cursor_to(view, pos);
			if (a->movement->type & JUMP)
				editor_window_jumplist_add(vis->win, pos);
			else
				editor_window_jumplist_invalidate(vis->win);
		} else if (a->movement->type & INCLUSIVE) {
			Iterator it = text_iterator_get(txt, c.range.end);
			text_iterator_char_next(&it, NULL);
			c.range.end = it.pos;
		}
	} else if (a->textobj) {
		if (vis->mode->visual)
			c.range = view_selection_get(view);
		else
			c.range.start = c.range.end = pos;
		for (int i = 0; i < count; i++) {
			Filerange r = a->textobj->range(txt, pos);
			if (!text_range_valid(&r))
				break;
			if (a->textobj->type == OUTER) {
				r.start--;
				r.end++;
			}

			c.range = text_range_union(&c.range, &r);

			if (i < count - 1) {
				if (a->textobj == &textobjs[TEXT_OBJ_LINE_UP]) {
					pos = c.range.start - 1;
				} else {
					pos = c.range.end + 1;
				}
			}
		}

		if (vis->mode->visual) {
			view_selection_set(view, &c.range);
			pos = c.range.end;
			view_cursor_to(view, pos);
		}
	} else if (vis->mode->visual) {
		c.range = view_selection_get(view);
		if (!text_range_valid(&c.range))
			c.range.start = c.range.end = pos;
	}

	if (vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE] && (a->movement || a->textobj)) {
		Filerange sel = view_selection_get(view);
		sel.end = text_char_prev(txt, sel.end);
		size_t start = text_line_begin(txt, sel.start);
		size_t end = text_line_end(txt, sel.end);
		if (sel.start == pos) { /* extend selection upwards */
			sel.end = start;
			sel.start = end;
		} else { /* extend selection downwards */
			sel.start = start;
			sel.end = end;
		}
		view_selection_set(view, &sel);
		c.range = sel;
	}

	if (a->op) {
		view_cursor_to(view, a->op->func(&c));
		editor_draw(vis);

		if (vis->mode == &vis_modes[VIS_MODE_OPERATOR])
			switchmode_to(vis->mode_prev);
		else if (vis->mode->visual)
			switchmode(&(const Arg){ .i = VIS_MODE_NORMAL });
		text_snapshot(txt);
	}

	if (a != &vis->action_prev) {
		if (a->op)
			vis->action_prev = *a;
		action_reset(a);
	}
}

static void action_reset(Action *a) {
	a->count = 0;
	a->linewise = false;
	a->op = NULL;
	a->movement = NULL;
	a->textobj = NULL;
	a->reg = NULL;
}

static void switchmode_to(Mode *new_mode) {
	if (vis->mode == new_mode)
		return;
	if (vis->mode->leave)
		vis->mode->leave(new_mode);
	if (vis->mode->isuser)
		vis->mode_prev = vis->mode;
	vis->mode = new_mode;
	if (new_mode == config->mode || (new_mode->name && new_mode->name[0] == '-'))
		vis->win->ui->draw_status(vis->win->ui);
	if (new_mode->enter)
		new_mode->enter(vis->mode_prev);
}

/** ':'-command implementations */

/* parse human-readable boolean value in s. If successful, store the result in
 * outval and return true. Else return false and leave outval alone. */
static bool parse_bool(const char *s, bool *outval) {
	for (const char **t = (const char*[]){"1", "true", "yes", "on", NULL}; *t; t++) {
		if (!strcasecmp(s, *t)) {
			*outval = true;
			return true;
		}
	}
	for (const char **f = (const char*[]){"0", "false", "no", "off", NULL}; *f; f++) {
		if (!strcasecmp(s, *f)) {
			*outval = false;
			return true;
		}
	}
	return false;
}

static bool cmd_set(Filerange *range, enum CmdOpt cmdopt, const char *argv[]) {

	typedef struct {
		const char *names[3];
		enum {
			OPTION_TYPE_STRING,
			OPTION_TYPE_BOOL,
			OPTION_TYPE_NUMBER,
		} type;
		bool optional;
		int index;
	} OptionDef;

	enum {
		OPTION_AUTOINDENT,
		OPTION_EXPANDTAB,
		OPTION_TABWIDTH,
		OPTION_SYNTAX,
		OPTION_NUMBER,
		OPTION_NUMBER_RELATIVE,
	};

	/* definitions have to be in the same order as the enum above */
	static OptionDef options[] = {
		[OPTION_AUTOINDENT]      = { { "autoindent", "ai"       }, OPTION_TYPE_BOOL   },
		[OPTION_EXPANDTAB]       = { { "expandtab", "et"        }, OPTION_TYPE_BOOL   },
		[OPTION_TABWIDTH]        = { { "tabwidth", "tw"         }, OPTION_TYPE_NUMBER },
		[OPTION_SYNTAX]          = { { "syntax"                 }, OPTION_TYPE_STRING, true },
		[OPTION_NUMBER]          = { { "numbers", "nu"          }, OPTION_TYPE_BOOL   },
		[OPTION_NUMBER_RELATIVE] = { { "relativenumbers", "rnu" }, OPTION_TYPE_BOOL   },
	};

	if (!vis->options) {
		if (!(vis->options = map_new()))
			return false;
		for (int i = 0; i < LENGTH(options); i++) {
			options[i].index = i;
			for (const char **name = options[i].names; *name; name++) {
				if (!map_put(vis->options, *name, &options[i]))
					return false;
			}
		}
	}

	if (!argv[1]) {
		editor_info_show(vis, "Expecting: set option [value]");
		return false;
	}

	Arg arg;
	bool invert = false;
	OptionDef *opt = NULL;

	if (!strncasecmp(argv[1], "no", 2)) {
		opt = map_closest(vis->options, argv[1]+2);
		if (opt && opt->type == OPTION_TYPE_BOOL)
			invert = true;
		else
			opt = NULL;
	}

	if (!opt)
		opt = map_closest(vis->options, argv[1]);
	if (!opt) {
		editor_info_show(vis, "Unknown option: `%s'", argv[1]);
		return false;
	}

	switch (opt->type) {
	case OPTION_TYPE_STRING:
		if (!opt->optional && !argv[2]) {
			editor_info_show(vis, "Expecting string option value");
			return false;
		}
		break;
	case OPTION_TYPE_BOOL:
		if (!argv[2]) {
			arg.b = true;
		} else if (!parse_bool(argv[2], &arg.b)) {
			editor_info_show(vis, "Expecting boolean option value not: `%s'", argv[2]);
			return false;
		}
		if (invert)
			arg.b = !arg.b;
		break;
	case OPTION_TYPE_NUMBER:
		if (!argv[2]) {
			editor_info_show(vis, "Expecting number");
			return false;
		}
		/* TODO: error checking? long type */
		arg.i = strtoul(argv[2], NULL, 10);
		break;
	}

	switch (opt->index) {
	case OPTION_EXPANDTAB:
		vis->expandtab = arg.b;
		break;
	case OPTION_AUTOINDENT:
		vis->autoindent = arg.b;
		break;
	case OPTION_TABWIDTH:
		editor_tabwidth_set(vis, arg.i);
		break;
	case OPTION_SYNTAX:
		if (!argv[2]) {
			Syntax *syntax = view_syntax_get(vis->win->view);
			if (syntax)
				editor_info_show(vis, "Syntax definition in use: `%s'", syntax->name);
			else
				editor_info_show(vis, "No syntax definition in use");
			return true;
		}

		for (Syntax *syntax = syntaxes; syntax && syntax->name; syntax++) {
			if (!strcasecmp(syntax->name, argv[2])) {
				view_syntax_set(vis->win->view, syntax);
				return true;
			}
		}

		if (parse_bool(argv[2], &arg.b) && !arg.b)
			view_syntax_set(vis->win->view, NULL);
		else
			editor_info_show(vis, "Unknown syntax definition: `%s'", argv[2]);
		break;
	case OPTION_NUMBER:
		editor_window_options(vis->win, arg.b ? UI_OPTION_LINE_NUMBERS_ABSOLUTE :
			UI_OPTION_LINE_NUMBERS_NONE);
		break;
	case OPTION_NUMBER_RELATIVE:
		editor_window_options(vis->win, arg.b ? UI_OPTION_LINE_NUMBERS_RELATIVE :
			UI_OPTION_LINE_NUMBERS_NONE);
		break;
	}

	return true;
}

static bool cmd_open(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (!argv[1])
		return vis_window_new(NULL);
	for (const char **file = &argv[1]; *file; file++) {
		if (!vis_window_new(*file)) {
			errno = 0;
			editor_info_show(vis, "Can't open `%s' %s", *file,
			                 errno ? strerror(errno) : "");
			return false;
		}
	}
	return true;
}

static bool is_view_closeable(Win *win) {
	if (!text_modified(win->file->text))
		return true;
	return win->file->refcount > 1;
}

static void info_unsaved_changes(void) {
	editor_info_show(vis, "No write since last change (add ! to override)");
}

static bool cmd_edit(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	Win *oldwin = vis->win;
	if (!(opt & CMD_OPT_FORCE) && !is_view_closeable(oldwin)) {
		info_unsaved_changes();
		return false;
	}
	if (!argv[1])
		return editor_window_reload(oldwin);
	if (!vis_window_new(argv[1]))
		return false;
	editor_window_close(oldwin);
	return true;
}

static bool cmd_quit(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (!(opt & CMD_OPT_FORCE) && !is_view_closeable(vis->win)) {
		info_unsaved_changes();
		return false;
	}
	editor_window_close(vis->win);
	if (!vis->windows)
		quit(NULL);
	return true;
}

static bool cmd_xit(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (text_modified(vis->win->file->text) && !cmd_write(range, opt, argv)) {
		if (!(opt & CMD_OPT_FORCE))
			return false;
	}
	return cmd_quit(range, opt, argv);
}

static bool cmd_bdelete(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	Text *txt = vis->win->file->text;
	if (text_modified(txt) && !(opt & CMD_OPT_FORCE)) {
		info_unsaved_changes();
		return false;
	}
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (win->file->text == txt)
			editor_window_close(win);
	}
	if (!vis->windows)
		quit(NULL);
	return true;
}

static bool cmd_qall(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (!text_modified(vis->win->file->text) || (opt & CMD_OPT_FORCE))
			editor_window_close(win);
	}
	if (!vis->windows)
		quit(NULL);
	else
		info_unsaved_changes();
	return vis->windows == NULL;
}

static bool cmd_read(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	char cmd[255];

	if (!argv[1]) {
		editor_info_show(vis, "Filename or command expected");
		return false;
	}

	bool iscmd = (opt & CMD_OPT_FORCE) || argv[1][0] == '!';
	const char *arg = argv[1]+(argv[1][0] == '!');
	snprintf(cmd, sizeof cmd, "%s%s", iscmd ? "" : "cat ", arg);

	size_t pos = view_cursor_get(vis->win->view);
	if (!text_range_valid(range))
		range = &(Filerange){ .start = pos, .end = pos };
	Filerange delete = *range;
	range->start = range->end;

	bool ret = cmd_filter(range, opt, (const char*[]){ argv[0], "sh", "-c", cmd, NULL});
	if (ret)
		text_delete(vis->win->file->text, delete.start, delete.end - delete.start);
	return ret;
}

static bool cmd_substitute(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	char pattern[255];
	if (!text_range_valid(range))
		range = &(Filerange){ .start = 0, .end = text_size(vis->win->file->text) };
	snprintf(pattern, sizeof pattern, "s%s", argv[1]);
	return cmd_filter(range, opt, (const char*[]){ argv[0], "sed", pattern, NULL});
}

static bool openfiles(const char **files) {
	for (; *files; files++) {
		errno = 0;
		if (!vis_window_new(*files)) {
			editor_info_show(vis, "Could not open `%s' %s", *files,
			                 errno ? strerror(errno) : "");
			return false;
		}
	}
	return true;
}

static bool cmd_split(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	editor_windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	if (!argv[1])
		return vis_window_split(vis->win);
	return openfiles(&argv[1]);
}

static bool cmd_vsplit(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	editor_windows_arrange(vis, UI_LAYOUT_VERTICAL);
	if (!argv[1])
		return vis_window_split(vis->win);
	return openfiles(&argv[1]);
}

static bool cmd_new(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	editor_windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	return vis_window_new(NULL);
}

static bool cmd_vnew(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	editor_windows_arrange(vis, UI_LAYOUT_VERTICAL);
	return vis_window_new(NULL);
}

static bool cmd_wq(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (cmd_write(range, opt, argv))
		return cmd_quit(range, opt, argv);
	return false;
}

static bool cmd_write(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	Text *text = vis->win->file->text;
	if (!text_range_valid(range))
		range = &(Filerange){ .start = 0, .end = text_size(text) };
	if (!argv[1])
		argv[1] = text_filename_get(text);
	if (!argv[1]) {
		if (text_fd_get(text) == STDIN_FILENO) {
			if (strchr(argv[0], 'q'))
				return text_range_write(text, range, STDOUT_FILENO) >= 0;
			editor_info_show(vis, "No filename given, use 'wq' to write to stdout");
			return false;
		}
		editor_info_show(vis, "Filename expected");
		return false;
	}
	for (const char **file = &argv[1]; *file; file++) {
		if (!text_range_save(text, range, *file)) {
			editor_info_show(vis, "Can't write `%s'", *file);
			return false;
		}
	}
	return true;
}

static bool cmd_saveas(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (cmd_write(range, opt, argv)) {
		text_filename_set(vis->win->file->text, argv[1]);
		return true;
	}
	return false;
}

static void cancel_filter(int sig) {
	vis->cancel_filter = true;
}

static bool cmd_filter(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	/* if an invalid range was given, stdin (i.e. key board input) is passed
	 * through the external command. */
	Text *text = vis->win->file->text;
	View *view = vis->win->view;
	int pin[2], pout[2], perr[2], status = -1;
	bool interactive = !text_range_valid(range);
	size_t pos = view_cursor_get(view);

	if (pipe(pin) == -1)
		return false;
	if (pipe(pout) == -1) {
		close(pin[0]);
		close(pin[1]);
		return false;
	}

	if (pipe(perr) == -1) {
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		return false;
	}

	reset_shell_mode();
	pid_t pid = fork();

	if (pid == -1) {
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		close(perr[0]);
		close(perr[1]);
		editor_info_show(vis, "fork failure: %s", strerror(errno));
		return false;
	} else if (pid == 0) { /* child i.e filter */
		if (!interactive)
			dup2(pin[0], STDIN_FILENO);
		close(pin[0]);
		close(pin[1]);
		dup2(pout[1], STDOUT_FILENO);
		close(pout[1]);
		close(pout[0]);
		if (!interactive)
			dup2(perr[1], STDERR_FILENO);
		close(perr[0]);
		close(perr[1]);
		if (!argv[2])
			execl("/bin/sh", "sh", "-c", argv[1], NULL);
		else
			execvp(argv[1], (char**)argv+1);
		editor_info_show(vis, "exec failure: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* set up a signal handler to cancel the filter via CTRL-C */
	struct sigaction sa, oldsa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = cancel_filter;

	bool restore_signals = sigaction(SIGINT, &sa, &oldsa) == 0;
	vis->cancel_filter = false;

	close(pin[0]);
	close(pout[1]);
	close(perr[1]);

	fcntl(pout[0], F_SETFL, O_NONBLOCK);
	fcntl(perr[0], F_SETFL, O_NONBLOCK);

	if (interactive)
		range = &(Filerange){ .start = pos, .end = pos };

	/* ranges which are written to the filter and read back in */
	Filerange rout = *range;
	Filerange rin = (Filerange){ .start = range->end, .end = range->end };

	/* The general idea is the following:
	 *
	 *  1) take a snapshot
	 *  2) write [range.start, range.end] to exteneral command
	 *  3) read the output of the external command and insert it after the range
	 *  4) depending on the exit status of the external command
	 *     - on success: delete original range
	 *     - on failure: revert to previous snapshot
	 *
	 *  2) and 3) happend in small junks
	 */

	text_snapshot(text);

	fd_set rfds, wfds;
	Buffer errmsg;
	buffer_init(&errmsg);

	do {
		if (vis->cancel_filter) {
			kill(-pid, SIGTERM);
			editor_info_show(vis, "Command cancelled");
			break;
		}

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if (pin[1] != -1)
			FD_SET(pin[1], &wfds);
		if (pout[0] != -1)
			FD_SET(pout[0], &rfds);
		if (perr[0] != -1)
			FD_SET(perr[0], &rfds);

		if (select(FD_SETSIZE, &rfds, &wfds, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			editor_info_show(vis, "Select failure");
			break;
		}

		if (FD_ISSET(pin[1], &wfds)) {
			Filerange junk = *range;
			if (junk.end > junk.start + PIPE_BUF)
				junk.end = junk.start + PIPE_BUF;
			ssize_t len = text_range_write(text, &junk, pin[1]);
			if (len > 0) {
				range->start += len;
				if (text_range_size(range) == 0) {
					close(pout[1]);
					pout[1] = -1;
				}
			} else {
				close(pin[1]);
				pin[1] = -1;
				if (len == -1)
					editor_info_show(vis, "Error writing to external command");
			}
		}

		if (FD_ISSET(pout[0], &rfds)) {
			char buf[BUFSIZ];
			ssize_t len = read(pout[0], buf, sizeof buf);
			if (len > 0) {
				text_insert(text, rin.end, buf, len);
				rin.end += len;
			} else if (len == 0) {
				close(pout[0]);
				pout[0] = -1;
			} else if (errno != EINTR && errno != EWOULDBLOCK) {
				editor_info_show(vis, "Error reading from filter stdout");
				close(pout[0]);
				pout[0] = -1;
			}
		}

		if (FD_ISSET(perr[0], &rfds)) {
			char buf[BUFSIZ];
			ssize_t len = read(perr[0], buf, sizeof buf);
			if (len > 0) {
				buffer_append(&errmsg, buf, len);
			} else if (len == 0) {
				close(perr[0]);
				perr[0] = -1;
			} else if (errno != EINTR && errno != EWOULDBLOCK) {
				editor_info_show(vis, "Error reading from filter stderr");
				close(pout[0]);
				pout[0] = -1;
			}
		}

	} while (pin[1] != -1 || pout[0] != -1 || perr[0] != -1);

	if (pin[1] != -1)
		close(pin[1]);
	if (pout[0] != -1)
		close(pout[0]);
	if (perr[0] != -1)
		close(perr[0]);

	if (waitpid(pid, &status, 0) == pid && status == 0) {
		text_delete(text, rout.start, rout.end - rout.start);
		text_snapshot(text);
	} else {
		/* make sure we have somehting to undo */
		text_insert(text, pos, " ", 1);
		text_undo(text);
	}

	view_cursor_to(view, rout.start);

	if (!vis->cancel_filter) {
		if (status == 0)
			editor_info_show(vis, "Command succeded");
		else if (errmsg.len > 0)
			editor_info_show(vis, "Command failed: %s", errmsg.data);
		else
			editor_info_show(vis, "Command failed");
	}

	if (restore_signals)
		sigaction(SIGTERM, &oldsa, NULL);

	reset_prog_mode();
	wclear(stdscr);
	return status == 0;
}

static bool cmd_earlier(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (argv[1])
		return false;
	//TODO eventually support time arguments
	text_earlier(vis->win->file->text);
	return true;
}

static bool cmd_later(Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (argv[1])
		return false;
	//TODO eventually support time arguments
	text_later(vis->win->file->text);
	return true;
}

static Filepos parse_pos(char **cmd) {
	size_t pos = EPOS;
	View *view = vis->win->view;
	Text *txt = vis->win->file->text;
	Mark *marks = vis->win->file->marks;
	switch (**cmd) {
	case '.':
		pos = text_line_begin(txt, view_cursor_get(view));
		(*cmd)++;
		break;
	case '$':
		pos = text_size(txt);
		(*cmd)++;
		break;
	case '\'':
		(*cmd)++;
		if ('a' <= **cmd && **cmd <= 'z')
			pos = text_mark_get(txt, marks[**cmd - 'a']);
		else if (**cmd == '<')
			pos = text_mark_get(txt, marks[MARK_SELECTION_START]);
		else if (**cmd == '>')
			pos = text_mark_get(txt, marks[MARK_SELECTION_END]);
		(*cmd)++;
		break;
	case '/':
		(*cmd)++;
		char *pattern_end = strchr(*cmd, '/');
		if (!pattern_end)
			return EPOS;
		*pattern_end++ = '\0';
		Regex *regex = text_regex_new();
		if (!regex)
			return EPOS;
		if (!text_regex_compile(regex, *cmd, 0)) {
			*cmd = pattern_end;
			pos = text_search_forward(txt, view_cursor_get(view), regex);
		}
		text_regex_free(regex);
		break;
	case '+':
	case '-':
	{
		CursorPos curspos = view_cursor_getpos(view);
		long long line = curspos.line + strtoll(*cmd, cmd, 10);
		if (line < 0)
			line = 0;
		pos = text_pos_by_lineno(txt, line);
		break;
	}
	default:
		if ('0' <= **cmd && **cmd <= '9')
			pos = text_pos_by_lineno(txt, strtoul(*cmd, cmd, 10));
		break;
	}

	return pos;
}

static Filerange parse_range(char **cmd) {
	Text *txt = vis->win->file->text;
	Filerange r = text_range_empty();
	Mark *marks = vis->win->file->marks;
	char start = **cmd;
	switch (**cmd) {
	case '%':
		r.start = 0;
		r.end = text_size(txt);
		(*cmd)++;
		break;
	case '*':
		r.start = text_mark_get(txt, marks[MARK_SELECTION_START]);
		r.end = text_mark_get(txt, marks[MARK_SELECTION_END]);
		(*cmd)++;
		break;
	default:
		r.start = parse_pos(cmd);
		if (**cmd != ',') {
			if (start == '.')
				r.end = text_line_next(txt, r.start);
			return r;
		}
		(*cmd)++;
		r.end = parse_pos(cmd);
		break;
	}
	return r;
}

static Command *lookup_cmd(const char *name) {
	if (!vis->cmds) {
		if (!(vis->cmds = map_new()))
			return NULL;
	
		for (Command *cmd = cmds; cmd && cmd->name[0]; cmd++) {
			for (const char **name = cmd->name; *name; name++)
				map_put(vis->cmds, *name, cmd);
		}
	}
	return map_closest(vis->cmds, name);
}

static bool exec_cmdline_command(const char *cmdline) {
	enum CmdOpt opt = CMD_OPT_NONE;
	size_t len = strlen(cmdline);
	char *line = malloc(len+2);
	if (!line)
		return false;
	line = strncpy(line, cmdline, len+1);
	char *name = line;

	Filerange range = parse_range(&name);
	if (!text_range_valid(&range)) {
		/* if only one position was given, jump to it */
		if (range.start != EPOS && !*name) {
			view_cursor_to(vis->win->view, range.start);
			free(line);
			return true;
		}

		if (name != line) {
			editor_info_show(vis, "Invalid range\n");
			free(line);
			return false;
		}
	}
	/* skip leading white space */
	while (*name == ' ')
		name++;
	char *param = name;
	while (*param && isalpha(*param))
		param++;

	if (*param == '!') {
		if (param != name) {
			opt |= CMD_OPT_FORCE;
			*param = ' ';
		} else {
			param++;
		}
	}

	memmove(param+1, param, strlen(param)+1);
	*param++ = '\0'; /* separate command name from parameters */

	Command *cmd = lookup_cmd(name);
	if (!cmd) {
		editor_info_show(vis, "Not an editor command");
		free(line);
		return false;
	}

	char *s = param;
	const char *argv[32] = { name };
	for (int i = 1; i < LENGTH(argv); i++) {
		while (s && *s && *s == ' ')
			s++;
		if (s && !*s)
			s = NULL;
		argv[i] = s;
		if (!(cmd->opt & CMD_OPT_ARGS)) {
			/* remove trailing spaces */
			if (s) {
				while (*s) s++;
				while (*(--s) == ' ') *s = '\0';
			}
			s = NULL;
		}
		if (s && (s = strchr(s, ' ')))
			*s++ = '\0';
		/* strip out a single '!' argument to make ":q !" work */
		if (argv[i] && !strcmp(argv[i], "!")) {
			opt |= CMD_OPT_FORCE;
			i--;
		}
	}

	cmd->cmd(&range, opt, argv);
	free(line);
	return true;
}

static bool exec_command(char type, const char *cmd) {
	if (!cmd || !cmd[0])
		return true;
	switch (type) {
	case '/':
	case '?':
		if (text_regex_compile(vis->search_pattern, cmd, REG_EXTENDED)) {
			action_reset(&vis->action);
			return false;
		}
		movement(&(const Arg){ .i =
			type == '/' ? MOVE_SEARCH_FORWARD : MOVE_SEARCH_BACKWARD });
		return true;
	case '+':
	case ':':
		if (exec_cmdline_command(cmd))
			return true;
	}
	return false;
}

static void settings_apply(const char **settings) {
	for (const char **opt = settings; opt && *opt; opt++)
		exec_cmdline_command(*opt);
}

static bool vis_window_new(const char *file) {
	if (!editor_window_new(vis, file))
		return false;
	Syntax *s = view_syntax_get(vis->win->view);
	if (s)
		settings_apply(s->settings);
	return true;
}

static bool vis_window_new_fd(int fd) {
	if (!editor_window_new_fd(vis, fd))
		return false;
	Syntax *s = view_syntax_get(vis->win->view);
	if (s)
		settings_apply(s->settings);
	return true;
}

static bool vis_window_split(Win *win) {
	if (!editor_window_split(win))
		return false;
	Syntax *s = view_syntax_get(vis->win->view);
	if (s)
		settings_apply(s->settings);
	return true;
}

static void die(const char *errstr, ...) {
	va_list ap;
	editor_free(vis);
	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static bool keymatch(Key *key0, Key *key1) {
	return (key0->str[0] && memcmp(key0->str, key1->str, sizeof(key1->str)) == 0) ||
	       (key0->code && key0->code == key1->code);
}

static bool keyvalid(Key *k) {
	return k && (k->str[0] || k->code);
}

static KeyBinding *keybinding(Mode *mode, KeyCombo keys) {
	int combolen = 0;
	while (combolen < MAX_KEYS && keyvalid(&keys[combolen]))
		combolen++;
	for (; mode; mode = mode->parent) {
		if (mode->common_prefix && !keymatch(&keys[0], &mode->bindings->key[0]))
			continue;
		for (KeyBinding *kb = mode->bindings; kb && keyvalid(&kb->key[0]); kb++) {
			for (int k = 0; k < combolen; k++) {
				if (!keymatch(&keys[k], &kb->key[k]))
					break;
				if (k == combolen - 1)
					return kb;
			}
		}
		if (mode->unknown && !mode->unknown(keys))
			break;
	}
	return NULL;
}

static void keypress(Key *key) {
	static KeyCombo keys;
	static int keylen;


	keys[keylen++] = *key;
	KeyBinding *action = keybinding(vis->mode, keys);

	if (action) {
		int combolen = 0;
		while (combolen < MAX_KEYS && keyvalid(&action->key[combolen]))
			combolen++;
		if (keylen < combolen)
			return; /* combo not yet complete */
		/* need to reset state before calling action->func in case
		 * it will call us (=keypress) again as e.g. macro_replay */
		keylen = 0;
		memset(keys, 0, sizeof(keys));
		if (action->func)
			action->func(&action->arg);
	} else if (keylen == 1 && key->code == 0 && vis->mode->input) {
		vis->mode->input(key->str, strlen(key->str));
	}

	keylen = 0;
	memset(keys, 0, sizeof(keys));
}

static Key getkey(void) {
	Key key = { .str = "", .code = 0 };
	int keycode = getch(), cur = 0;
	if (keycode == ERR)
		return key;

	if (keycode >= KEY_MIN) {
		key.code = keycode;
	} else {
		key.str[cur++] = keycode;
		int len = 1;
		unsigned char keychar = keycode;
		if (ISASCII(keychar)) len = 1;
		else if (keychar == 0x1B || keychar >= 0xFC) len = 6;
		else if (keychar >= 0xF8) len = 5;
		else if (keychar >= 0xF0) len = 4;
		else if (keychar >= 0xE0) len = 3;
		else if (keychar >= 0xC0) len = 2;
		len = MIN(len, LENGTH(key.str));

		if (cur < len) {
			nodelay(stdscr, TRUE);
			for (int t; cur < len && (t = getch()) != ERR; cur++)
				key.str[cur] = t;
			nodelay(stdscr, FALSE);
		}
	}

	if (config->keypress && !config->keypress(&key))
		return (Key){ .str = "", .code = 0 };

	return key;
}

static void mainloop() {
	struct timespec idle = { .tv_nsec = 0 }, *timeout = NULL;
	sigset_t emptyset, blockset;
	sigemptyset(&emptyset);
	sigemptyset(&blockset);
	sigaddset(&blockset, SIGWINCH);
	sigprocmask(SIG_BLOCK, &blockset, NULL);
	signal(SIGPIPE, SIG_IGN);
	editor_draw(vis);
	vis->running = true;

	while (vis->running) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		editor_update(vis);
		idle.tv_sec = vis->mode->idle_timeout;
		int r = pselect(1, &fds, NULL, NULL, timeout, &emptyset);
		if (r == -1 && errno == EINTR)
			continue;

		if (r < 0) {
			/* TODO save all pending changes to a ~suffixed file */
			die("Error in mainloop: %s\n", strerror(errno));
		}

		if (!FD_ISSET(STDIN_FILENO, &fds)) {
			if (vis->mode->idle)
				vis->mode->idle();
			timeout = NULL;
			continue;
		}

		Key key = getkey();
		keypress(&key);

		if (vis->mode->idle)
			timeout = &idle;
	}
}


int main(int argc, char *argv[]) {
	/* decide which key configuration to use based on argv[0] */
	char *arg0 = argv[0];
	while (*arg0 && (*arg0 == '.' || *arg0 == '/'))
		arg0++;
	for (int i = 0; i < LENGTH(editors); i++) {
		if (strcmp(editors[i].name, arg0) == 0) {
			config = &editors[i];
			break;
		}
	}

	if (!(vis = editor_new(ui_curses_new())))
		die("Could not allocate editor core\n");

	vis->mode_prev = vis->mode = config->mode;

	if (!editor_syntax_load(vis, syntaxes, colors))
		die("Could not load syntax highlighting definitions\n");

	char *cmd = NULL;
	bool end_of_options = false;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && !end_of_options) {
			switch (argv[i][1]) {
			case '-':
				end_of_options = true;
				break;
			case 'v':
				die("vis %s, compiled " __DATE__ " " __TIME__ "\n", VERSION);
				break;
			case '\0':
				break;
			default:
				die("Unknown command option: %s\n", argv[i]);
				break;
			}
		} else if (argv[i][0] == '+') {
			cmd = argv[i] + (argv[i][1] == '/' || argv[i][1] == '?');
		} else if (!vis_window_new(argv[i])) {
			die("Can not load `%s': %s\n", argv[i], strerror(errno));
		} else if (cmd) {
			exec_command(cmd[0], cmd+1);
			cmd = NULL;
		}
	}

	if (!vis->windows) {
		if (!strcmp(argv[argc-1], "-") || !isatty(STDIN_FILENO)) {
			if (!vis_window_new_fd(STDIN_FILENO))
				die("Can not read from stdin\n");
			int fd = open("/dev/tty", O_RDONLY);
			if (fd == -1)
				die("Can not reopen stdin\n");
			dup2(fd, STDIN_FILENO);
			close(fd);
		} else if (!vis_window_new(NULL)) {
			die("Can not create empty buffer\n");
		}
		if (cmd)
			exec_command(cmd[0], cmd+1);
	}

	settings_apply(settings);
	mainloop();
	editor_free(vis);
	return 0;
}
