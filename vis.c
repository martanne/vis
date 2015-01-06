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
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "editor.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"

#ifdef PDCURSES
int ESCDELAY;
#endif
#ifndef NCURSES_REENTRANT
# define set_escdelay(d) (ESCDELAY = (d))
#endif

typedef union {
	bool b;
	int i;
	const char *s;
	void (*w)(Win*);    /* generic window commands */
	void (*f)(Editor*); /* generic editor commands */
} Arg;

typedef struct {
	char str[6]; /* UTF8 character or terminal escape code */
	int code;    /* curses KEY_* constant */
} Key;

#define MAX_KEYS 2
typedef Key KeyCombo[MAX_KEYS];

typedef struct {
	KeyCombo key;
	void (*func)(const Arg *arg);
	const Arg arg;
} KeyBinding;

typedef struct Mode Mode;
struct Mode {
	Mode *parent;                       /* if no match is found in this mode, search will continue there */
	KeyBinding *bindings;               /* NULL terminated array of keybindings for this mode */
	const char *name;                   /* descriptive, user facing name of the mode */
	bool isuser;                        /* whether this is a user or internal mode */
	bool common_prefix;                 /* whether the first key in this mode is always the same */
	void (*enter)(Mode *old);           /* called right before the mode becomes active */
	void (*leave)(Mode *new);           /* called right before the mode becomes inactive */
	bool (*unknown)(KeyCombo);          /* called whenever a key combination is not found in this mode,
	                                       the return value determines whether parent modes will be searched */
	void (*input)(const char*, size_t); /* called whenever a key is not found in this mode and all its parent modes */
	void (*idle)(void);                 /* called whenever a certain idle time i.e. without any user input elapsed */
	time_t idle_timeout;                /* idle time in seconds after which the registered function will be called */
	bool visual;                        /* whether text selection is possible in this mode */
};

typedef struct {
	char *name;                    /* is used to match against argv[0] to enable this config */
	Mode *mode;                    /* default mode in which the editor should start in */
	void (*statusbar)(EditorWin*); /* routine which is called whenever the cursor is moved within a window */
	bool (*keypress)(Key*);        /* called before any other keybindings are checked,
	                                * return value decides whether key should be ignored */
} Config;

typedef struct {
	int count;        /* how many times should the command be executed? */
	Register *reg;    /* always non-NULL, set to a default register */
	Filerange range;  /* which part of the file should be affected by the operator */
	size_t pos;       /* at which byte from the start of the file should the operation start? */
	bool linewise;    /* should the changes always affect whole lines? */
	const Arg *arg;   /* arbitrary arguments */
} OperatorContext;

typedef struct {
	void (*func)(OperatorContext*); /* function implementing the operator logic */
} Operator;

typedef struct {
	size_t (*cmd)(const Arg*);        /* a custom movement based on user input from vis.c */
	size_t (*win)(Win*);              /* a movement based on current window content from window.h */
	size_t (*txt)(Text*, size_t pos); /* a movement form text-motions.h */
	enum {
		LINEWISE  = 1 << 0,
		CHARWISE  = 1 << 1,
		INCLUSIVE = 1 << 2,
		EXCLUSIVE = 1 << 3,
		IDEMPOTENT = 1 << 4,
		JUMP = 1 << 5,
	} type;
	int count;
} Movement;

typedef struct {
	Filerange (*range)(Text*, size_t pos); /* a text object from text-objects.h */
	enum {
		INNER,
		OUTER,
	} type;
} TextObject;

typedef struct {             /** collects all information until an operator is executed */
	int count;
	bool linewise;
	Operator *op;
	Movement *movement;
	TextObject *textobj;
	Register *reg;
	MarkIntern mark;
	Key key;
	Arg arg;
} Action;

typedef struct {                         /* command definitions for the ':'-prompt */
	const char *name;                /* regular expression pattern to match command */
	bool (*cmd)(Filerange*, const char *argv[]);
	                                 /* command logic called with a NULL terminated array
	                                  * of arguments. argv[0] will be the command name,
	                                  * as matched by the regex. */
	bool args;                       /* whether argv should be populated with words
	                                  * separated by spaces. if false, argv[1] will
	                                  * contain the remaining command line unmodified */
	regex_t regex;                   /* compiled form of the pattern in 'name' */
} Command;

/** global variables */
static volatile bool running = true; /* exit main loop once this becomes false */
static Editor *vis;         /* global editor instance, keeps track of all windows etc. */
static Mode *mode;          /* currently active mode, used to search for keybindings */
static Mode *mode_prev;     /* previsouly active user mode */
static Mode *mode_before_prompt; /* user mode which was active before entering prompt */
static Action action;       /* current action which is in progress */
static Action action_prev;  /* last operator action used by the repeat '.' key */
static Buffer buffer_repeat;/* repeat last modification i.e. insertion/replacement */

/** operators */
static void op_change(OperatorContext *c);
static void op_yank(OperatorContext *c);
static void op_put(OperatorContext *c);
static void op_delete(OperatorContext *c);
static void op_shift_right(OperatorContext *c);
static void op_shift_left(OperatorContext *c);
static void op_case_change(OperatorContext *c);
static void op_join(OperatorContext *c);
static void op_repeat_insert(OperatorContext *c);
static void op_repeat_replace(OperatorContext *c);

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

/** movements which can be used besides the one in text-motions.h and window.h */

/* search in forward direction for the word under the cursor */
static size_t search_word_forward(const Arg *arg);
/* search in backward direction for the word under the cursor */
static size_t search_word_backward(const Arg *arg);
/* search again for the last used search pattern */
static size_t search_forward(const Arg *arg);
static size_t search_backward(const Arg *arg);
/* goto action.mark */
static size_t mark_goto(const Arg *arg);
/* goto first non-blank char on line pointed by action.mark */
static size_t mark_line_goto(const Arg *arg);
/* goto to next occurence of action.key to the right */
static size_t to(const Arg *arg);
/* goto to position before next occurence of action.key to the right */
static size_t till(const Arg *arg);
/* goto to next occurence of action.key to the left */
static size_t to_left(const Arg *arg);
/* goto to position after next occurence of action.key to the left */
static size_t till_left(const Arg *arg);
/* goto line number action.count */
static size_t line(const Arg *arg);
/* goto to byte action.count on current line */
static size_t column(const Arg *arg);
/* goto the action.count-th line from top of the focused window */
static size_t window_lines_top(const Arg *arg);
/* goto the start of middle line of the focused window */
static size_t window_lines_middle(const Arg *arg);
/* goto the action.count-th line from bottom of the focused window */
static size_t window_lines_bottom(const Arg *arg);

static Movement moves[] = {
	[MOVE_SCREEN_LINE_UP]      = { .win = window_line_up                                       },
	[MOVE_SCREEN_LINE_DOWN]    = { .win = window_line_down                                     },
	[MOVE_SCREEN_LINE_BEGIN]   = { .win = window_line_begin,        .type = CHARWISE           },
	[MOVE_SCREEN_LINE_MIDDLE]  = { .win = window_line_middle,       .type = CHARWISE           },
	[MOVE_SCREEN_LINE_END]     = { .win = window_line_end,          .type = CHARWISE|INCLUSIVE },
	[MOVE_LINE_PREV]           = { .txt = text_line_prev,           .type = LINEWISE           },
	[MOVE_LINE_BEGIN]          = { .txt = text_line_begin,          .type = LINEWISE           },
	[MOVE_LINE_START]          = { .txt = text_line_start,          .type = LINEWISE           },
	[MOVE_LINE_FINISH]         = { .txt = text_line_finish,         .type = LINEWISE|INCLUSIVE },
	[MOVE_LINE_LASTCHAR]       = { .txt = text_line_lastchar,       .type = LINEWISE|INCLUSIVE },
	[MOVE_LINE_END]            = { .txt = text_line_end,            .type = LINEWISE           },
	[MOVE_LINE_NEXT]           = { .txt = text_line_next,           .type = LINEWISE           },
	[MOVE_LINE]                = { .cmd = line,                     .type = LINEWISE|IDEMPOTENT|JUMP},
	[MOVE_COLUMN]              = { .cmd = column,                   .type = CHARWISE|IDEMPOTENT},
	[MOVE_CHAR_PREV]           = { .win = window_char_prev                                     },
	[MOVE_CHAR_NEXT]           = { .win = window_char_next                                     },
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
	[MOVE_LEFT_TO]             = { .cmd = to_left,                  .type = LINEWISE           },
	[MOVE_RIGHT_TO]            = { .cmd = to,                       .type = LINEWISE|INCLUSIVE },
	[MOVE_LEFT_TILL]           = { .cmd = till_left,                .type = LINEWISE           },
	[MOVE_RIGHT_TILL]          = { .cmd = till,                     .type = LINEWISE|INCLUSIVE },
	[MOVE_MARK]                = { .cmd = mark_goto,                .type = LINEWISE|JUMP      },
	[MOVE_MARK_LINE]           = { .cmd = mark_line_goto,           .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_WORD_FORWARD] = { .cmd = search_word_forward,      .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_WORD_BACKWARD]= { .cmd = search_word_backward,     .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_FORWARD]      = { .cmd = search_forward,           .type = LINEWISE|JUMP      },
	[MOVE_SEARCH_BACKWARD]     = { .cmd = search_backward,          .type = LINEWISE|JUMP      },
	[MOVE_WINDOW_LINE_TOP]     = { .cmd = window_lines_top,         .type = LINEWISE|JUMP      },
	[MOVE_WINDOW_LINE_MIDDLE]  = { .cmd = window_lines_middle,      .type = LINEWISE|JUMP      },
	[MOVE_WINDOW_LINE_BOTTOM]  = { .cmd = window_lines_bottom,      .type = LINEWISE|JUMP      },
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
/* use register indicated by arg->i for the current operator */
static void reg(const Arg *arg);
/* perform a movement to mark arg->i */
static void mark(const Arg *arg);
/* perform a movement to the first non-blank on the line pointed by mark arg->i */
static void mark_line(const Arg *arg);
/* {un,re}do last action, redraw window */
static void undo(const Arg *arg);
static void redo(const Arg *arg);
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
static bool cmd_set(Filerange*, const char *argv[]);
/* for each argument create a new window and open the corresponding file */
static bool cmd_open(Filerange*, const char *argv[]);
/* close current window (discard modifications if argv[0] contains '!')
 * and open argv[1], if no argv[1] is given re-read to current file from disk */
static bool cmd_edit(Filerange*, const char *argv[]);
/* close the current window, if argv[0] contains a '!' discard modifications */
static bool cmd_quit(Filerange*, const char *argv[]);
/* close all windows which show current file. if argv[0] contains a '!' discard modifications */
static bool cmd_bdelete(Filerange*, const char *argv[]);
/* close all windows, exit editor, if argv[0] contains a '!' discard modifications */
static bool cmd_qall(Filerange*, const char *argv[]);
/* for each argument try to insert the file content at current cursor postion */
static bool cmd_read(Filerange*, const char *argv[]);
static bool cmd_substitute(Filerange*, const char *argv[]);
/* if no argument are given, split the current window horizontally,
 * otherwise open the file */
static bool cmd_split(Filerange*, const char *argv[]);
/* if no argument are given, split the current window vertically,
 * otherwise open the file */
static bool cmd_vsplit(Filerange*, const char *argv[]);
/* create a new empty window and arrange all windows either horizontally or vertically */
static bool cmd_new(Filerange*, const char *argv[]);
static bool cmd_vnew(Filerange*, const char *argv[]);
/* save the file displayed in the current window and close it */
static bool cmd_wq(Filerange*, const char *argv[]);
/* save the file displayed in the current window if it was changed, then close the window */
static bool cmd_xit(Filerange*, const char *argv[]);
/* save the file displayed in the current window to the name given.
 * do not change internal filname association. further :w commands
 * without arguments will still write to the old filename */
static bool cmd_write(Filerange*, const char *argv[]);
/* save the file displayed in the current window to the name given,
 * associate the new name with the buffer. further :w commands
 * without arguments will write to the new filename */
static bool cmd_saveas(Filerange*, const char *argv[]);

static void action_reset(Action *a);
static void switchmode_to(Mode *new_mode);
static bool vis_window_new(const char *file);
static bool vis_window_split(EditorWin *win);

#include "config.h"

static Key getkey(void);
static void keypress(Key *key);
static void action_do(Action *a);
static bool exec_command(char type, char *cmdline);

/** operator implementations of type: void (*op)(OperatorContext*) */

static void op_delete(OperatorContext *c) {
	size_t len = c->range.end - c->range.start;
	c->reg->linewise = c->linewise;
	register_put(c->reg, vis->win->text, &c->range);
	editor_delete(vis, c->range.start, len);
	window_cursor_to(vis->win->win, c->range.start);
}

static void op_change(OperatorContext *c) {
	op_delete(c);
	switchmode(&(const Arg){ .i = VIS_MODE_INSERT });
}

static void op_yank(OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put(c->reg, vis->win->text, &c->range);
}

static void op_put(OperatorContext *c) {
	Text *txt = vis->win->text;
	size_t pos = window_cursor_get(vis->win->win);
	if (c->arg->i > 0) {
		if (c->reg->linewise)
			pos = text_line_next(txt, pos);
		else
			pos = text_char_next(txt, pos);
	} else {
		if (c->reg->linewise)
			pos = text_line_begin(txt, pos);
	}
	editor_insert(vis, pos, c->reg->data, c->reg->len);
	if (c->reg->linewise)
		window_cursor_to(vis->win->win, text_line_start(txt, pos));
	else
		window_cursor_to(vis->win->win, pos + c->reg->len);
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

static void op_shift_right(OperatorContext *c) {
	Text *txt = vis->win->text;
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
	window_cursor_to(vis->win->win, c->pos + tablen);
	editor_draw(vis);
}

static void op_shift_left(OperatorContext *c) {
	Text *txt = vis->win->text;
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
	window_cursor_to(vis->win->win, c->pos - tablen);
	editor_draw(vis);
}

static void op_case_change(OperatorContext *c) {
	size_t len = c->range.end - c->range.start;
	char *buf = malloc(len);
	if (!buf)
		return;
	len = text_bytes_get(vis->win->text, c->range.start, len, buf);
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

	text_delete(vis->win->text, c->range.start, len);
	text_insert(vis->win->text, c->range.start, buf, len);
	editor_draw(vis);
	free(buf);
}

static void op_join(OperatorContext *c) {
	Text *txt = vis->win->text;
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	Filerange sel = window_selection_get(vis->win->win);
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

	window_cursor_to(vis->win->win, c->range.start);
	editor_draw(vis);
}

static void op_repeat_insert(OperatorContext *c) {
	if (!buffer_repeat.len)
		return;
	editor_insert(vis, c->pos, buffer_repeat.data, buffer_repeat.len);
	window_cursor_to(vis->win->win, c->pos + buffer_repeat.len);
}

static void op_repeat_replace(OperatorContext *c) {
	if (!buffer_repeat.len)
		return;

	size_t chars = 0;
	for (size_t i = 0; i < buffer_repeat.len; i++) {
		if (ISUTF8(buffer_repeat.data[i]))
			chars++;
	}

	Iterator it = text_iterator_get(vis->win->text, c->pos);
	while (chars-- > 0)
		text_iterator_char_next(&it, NULL);
	editor_delete(vis, c->pos, it.pos - c->pos);
	op_repeat_insert(c);
}

/** movement implementations of type: size_t (*move)(const Arg*) */

static char *get_word_under_cursor() {
	Filerange word = text_object_word(vis->win->text, window_cursor_get(vis->win->win));
	if (!text_range_valid(&word))
		return NULL;
	size_t len = word.end - word.start;
	char *buf = malloc(len+1);
	if (!buf)
		return NULL;
	len = text_bytes_get(vis->win->text, word.start, len, buf);
	buf[len] = '\0';
	return buf;
}

static size_t search_word_forward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	char *word = get_word_under_cursor();
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_forward(vis->win->text, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_word_backward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	char *word = get_word_under_cursor();
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_backward(vis->win->text, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_forward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	return text_search_forward(vis->win->text, pos, vis->search_pattern);
}

static size_t search_backward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	return text_search_backward(vis->win->text, pos, vis->search_pattern);
}

static void mark_set(const Arg *arg) {
	text_mark_intern_set(vis->win->text, arg->i, window_cursor_get(vis->win->win));
}

static size_t mark_goto(const Arg *arg) {
	return text_mark_intern_get(vis->win->text, action.mark);
}

static size_t mark_line_goto(const Arg *arg) {
	return text_line_start(vis->win->text, mark_goto(arg));
}

static size_t to(const Arg *arg) {
	return text_find_char_next(vis->win->text, window_cursor_get(vis->win->win) + 1,
		action.key.str, strlen(action.key.str));
}

static size_t till(const Arg *arg) {
	return text_char_prev(vis->win->text, to(arg));
}

static size_t to_left(const Arg *arg) {
	return text_find_char_prev(vis->win->text, window_cursor_get(vis->win->win) - 1,
		action.key.str, strlen(action.key.str));
}

static size_t till_left(const Arg *arg) {
	return text_char_next(vis->win->text, to_left(arg));
}

static size_t line(const Arg *arg) {
	return text_pos_by_lineno(vis->win->text, action.count);
}

static size_t column(const Arg *arg) {
	char c;
	EditorWin *win = vis->win;
	size_t pos = window_cursor_get(win->win);
	Iterator it = text_iterator_get(win->text, text_line_begin(win->text, pos));
	int count = action.count;
	while (count > 0 && text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

static size_t window_lines_top(const Arg *arg) {
	return window_line_goto(vis->win->win, action.count);
}

static size_t window_lines_middle(const Arg *arg) {
	return window_line_goto(vis->win->win, vis->win->height / 2);
}

static size_t window_lines_bottom(const Arg *arg) {
	return window_line_goto(vis->win->win, vis->win->height - action.count);
}

/** key bindings functions of type: void (*func)(const Arg*) */

static void jumplist(const Arg *arg) {
	size_t pos;
	if (arg->i > 0)
		pos = editor_window_jumplist_next(vis->win);
	else
		pos = editor_window_jumplist_prev(vis->win);
	if (pos != EPOS)
		window_cursor_to(vis->win->win, pos);
}

static void changelist(const Arg *arg) {
	size_t pos;
	if (arg->i > 0)
		pos = editor_window_changelist_next(vis->win);
	else
		pos = editor_window_changelist_prev(vis->win);
	if (pos != EPOS)
		window_cursor_to(vis->win->win, pos);
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
	endwin();
	raise(SIGSTOP);
}

static void repeat(const Arg *arg) {
	action = action_prev;
	action_do(&action);
}

static void replace(const Arg *arg) {
	Key k = getkey();
	if (!k.str[0])
		return;
	size_t pos = window_cursor_get(vis->win->win);
	action_reset(&action_prev);
	action_prev.op = &ops[OP_REPEAT_REPLACE];
	buffer_put(&buffer_repeat, k.str, strlen(k.str));
	editor_delete_key(vis);
	editor_insert_key(vis, k.str, strlen(k.str));
	window_cursor_to(vis->win->win, pos);
}

static void count(const Arg *arg) {
	action.count = action.count * 10 + arg->i;
}

static void gotoline(const Arg *arg) {
	if (action.count)
		movement(&(const Arg){ .i = MOVE_LINE });
	else if (arg->i < 0)
		movement(&(const Arg){ .i = MOVE_FILE_BEGIN });
	else
		movement(&(const Arg){ .i = MOVE_FILE_END });
}

static void linewise(const Arg *arg) {
	action.linewise = arg->b;
}

static void operator(const Arg *arg) {
	Operator *op = &ops[arg->i];
	if (mode->visual) {
		action.op = op;
		action_do(&action);
		return;
	}
	/* switch to operator mode inorder to make operator options and
	 * text-object available */
	switchmode(&(const Arg){ .i = VIS_MODE_OPERATOR });
	if (action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		action.linewise = true;
		action.textobj = moves_linewise[MOVE_SCREEN_LINE_DOWN];
		action_do(&action);
	} else {
		action.op = op;
	}
}

static void operator_twice(const Arg *arg) {
	operator(arg);
	operator(arg);
}

static void changecase(const Arg *arg) {
	action.arg = *arg;
	operator(&(const Arg){ .i = OP_CASE_CHANGE });
}

static void movement_key(const Arg *arg) {
	Key k = getkey();
	if (!k.str[0]) {
		action_reset(&action);
		return;
	}
	action.key = k;
	action.movement = &moves[arg->i];
	action_do(&action);
}

static void movement(const Arg *arg) {
	if (action.linewise && arg->i < LENGTH(moves_linewise))
		action.textobj = moves_linewise[arg->i];
	else
		action.movement = &moves[arg->i];
	action_do(&action);
}

static void textobj(const Arg *arg) {
	action.textobj = &textobjs[arg->i];
	action_do(&action);
}

static void reg(const Arg *arg) {
	action.reg = &vis->registers[arg->i];
}

static void mark(const Arg *arg) {
	action.mark = arg->i;
	action.movement = &moves[MOVE_MARK];
	action_do(&action);
}

static void mark_line(const Arg *arg) {
	action.mark = arg->i;
	action.movement = &moves[MOVE_MARK_LINE];
	action_do(&action);
}

static void undo(const Arg *arg) {
	size_t pos = text_undo(vis->win->text);
	if (pos != EPOS) {
		window_cursor_to(vis->win->win, pos);
		/* redraw all windows in case some display the same file */
		editor_draw(vis);
	}
}

static void redo(const Arg *arg) {
	size_t pos = text_redo(vis->win->text);
	if (pos != EPOS) {
		window_cursor_to(vis->win->win, pos);
		/* redraw all windows in case some display the same file */
		editor_draw(vis);
	}
}

static void zero(const Arg *arg) {
	if (action.count == 0)
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
	int pos = window_cursor_get(vis->win->win);
	editor_insert(vis, pos, reg->data, reg->len);
	window_cursor_to(vis->win->win, pos + reg->len);
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
	switchmode_to(mode_before_prompt);
	if (s && *s && exec_command(vis->prompt->title[0], s) && running)
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
		window_backspace_key(vis->win->win);
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
	editor_insert(vis, window_cursor_get(vis->win->win), &v, 1);
}

static void quit(const Arg *arg) {
	running = false;
}

static void cmd(const Arg *arg) {
	/* casting to char* is only save if arg->s contains no arguments */
	exec_command(':', (char*)arg->s);
}

static int argi2lines(const Arg *arg) {
	switch (arg->i) {
	case -PAGE:
	case +PAGE:
		return vis->win->height-1;
	case -PAGE_HALF:
	case +PAGE_HALF:
		return vis->win->height/2;
	default:
		if (action.count > 0)
			return action.count;
		return arg->i < 0 ? -arg->i : arg->i;
	}
}

static void wscroll(const Arg *arg) {
	if (arg->i >= 0)
		window_scroll_down(vis->win->win, argi2lines(arg));
	else
		window_scroll_up(vis->win->win, argi2lines(arg));
}

static void wslide(const Arg *arg) {
	if (arg->i >= 0)
		window_slide_down(vis->win->win, argi2lines(arg));
	else
		window_slide_up(vis->win->win, argi2lines(arg));
}

static void call(const Arg *arg) {
	arg->f(vis);
}

static void window(const Arg *arg) {
	arg->w(vis->win->win);
}

static void insert(const Arg *arg) {
	editor_insert_key(vis, arg->s, arg->s ? strlen(arg->s) : 0);
}

static void insert_tab(const Arg *arg) {
	insert(&(const Arg){ .s = expand_tab() });
}

static void copy_indent_from_previous_line(Win *win, Text *text) {
	size_t pos = window_cursor_get(win);
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
	insert(&(const Arg){ .s =
	       text_newlines_crnl(vis->win->text) ? "\r\n" : "\n" });

	if (vis->autoindent)
		copy_indent_from_previous_line(vis->win->win, vis->win->text);
}

static void put(const Arg *arg) {
	action.arg = *arg;
	operator(&(const Arg){ .i = OP_PUT });
	action_do(&action);
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
	Text *txt = vis->win->text;
	Win *win = vis->win->win;
	size_t pos = window_cursor_get(win);
	int count = MAX(1, a->count);
	OperatorContext c = {
		.count = a->count,
		.pos = pos,
		.reg = a->reg ? a->reg : &vis->registers[REG_DEFAULT],
		.linewise = a->linewise,
		.arg = &a->arg,
	};

	if (a->movement) {
		size_t start = pos;
		for (int i = 0; i < count; i++) {
			if (a->movement->txt)
				pos = a->movement->txt(txt, pos);
			else if (a->movement->win)
				pos = a->movement->win(win);
			else
				pos = a->movement->cmd(&a->arg);
			if (pos == EPOS || a->movement->type & IDEMPOTENT)
				break;
		}

		if (pos == EPOS) {
			c.range.start = start;
			c.range.end = start;
		} else {
			c.range.start = MIN(start, pos);
			c.range.end = MAX(start, pos);
		}

		if (!a->op) {
			if (a->movement->type & CHARWISE)
				window_scroll_to(win, pos);
			else
				window_cursor_to(win, pos);
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
		if (mode->visual)
			c.range = window_selection_get(win);
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

		if (mode->visual) {
			window_selection_set(win, &c.range);
			pos = c.range.end;
			window_cursor_to(win, pos);
		}
	} else if (mode->visual) {
		c.range = window_selection_get(win);
		if (!text_range_valid(&c.range))
			c.range.start = c.range.end = pos;
	}

	if (mode == &vis_modes[VIS_MODE_VISUAL_LINE] && (a->movement || a->textobj)) {
		Filerange sel = window_selection_get(win);
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
		window_selection_set(win, &sel);
		c.range = sel;
	}

	if (a->op) {
		a->op->func(&c);
		if (mode == &vis_modes[VIS_MODE_OPERATOR])
			switchmode_to(mode_prev);
		else if (mode->visual)
			switchmode(&(const Arg){ .i = VIS_MODE_NORMAL });
		text_snapshot(txt);
	}

	if (a != &action_prev) {
		if (a->op)
			action_prev = *a;
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
	if (mode == new_mode)
		return;
	if (mode->leave)
		mode->leave(new_mode);
	if (mode->isuser)
		mode_prev = mode;
	mode = new_mode;
	if (mode == config->mode || (mode->name && mode->name[0] == '-'))
		statusbar(vis->win);
	if (mode->enter)
		mode->enter(mode_prev);
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

static bool cmd_set(Filerange *range, const char *argv[]) {

	typedef struct {
		const char *name;
		enum {
			OPTION_TYPE_STRING,
			OPTION_TYPE_BOOL,
			OPTION_TYPE_NUMBER,
		} type;
		regex_t regex;
	} OptionDef;

	enum {
		OPTION_AUTOINDENT,
		OPTION_EXPANDTAB,
		OPTION_TABWIDTH,
		OPTION_SYNTAX,
		OPTION_NUMBER,
	};

	static OptionDef options[] = {
		[OPTION_AUTOINDENT] = { "^(autoindent|ai)$", OPTION_TYPE_BOOL   },
		[OPTION_EXPANDTAB]  = { "^(expandtab|et)$",  OPTION_TYPE_BOOL   },
		[OPTION_TABWIDTH]   = { "^(tabwidth|tw)$",   OPTION_TYPE_NUMBER },
		[OPTION_SYNTAX]     = { "^syntax$",          OPTION_TYPE_STRING },
		[OPTION_NUMBER]     = { "^(numbers?|nu)$",   OPTION_TYPE_BOOL   },
	};

	static bool init = false;

	if (!init) {
		for (int i = 0; i < LENGTH(options); i++)
			regcomp(&options[i].regex, options[i].name, REG_EXTENDED|REG_ICASE);
		init = true;
	}

	if (!argv[1]) {
		editor_info_show(vis, "Expecting: set option [value]");
		return false;
	}

	int opt = -1; Arg arg; bool invert = false;

	for (int i = 0; i < LENGTH(options); i++) {
		if (!regexec(&options[i].regex, argv[1], 0, NULL, 0)) {
			opt = i;
			break;
		}
		if (options[i].type == OPTION_TYPE_BOOL && !strncasecmp(argv[1], "no", 2) &&
		    !regexec(&options[i].regex, argv[1]+2, 0, NULL, 0)) {
			opt = i;
			invert = true;
			break;
		}
	}

	if (opt == -1) {
		editor_info_show(vis, "Unknown option: `%s'", argv[1]);
		return false;
	}

	switch (options[opt].type) {
	case OPTION_TYPE_STRING:
		if (!argv[2]) {
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

	switch (opt) {
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
		for (Syntax *syntax = syntaxes; syntax && syntax->name; syntax++) {
			if (!strcasecmp(syntax->name, argv[2])) {
				window_syntax_set(vis->win->win, syntax);
				return true;
			}
		}

		if (parse_bool(argv[2], &arg.b) && !arg.b)
			window_syntax_set(vis->win->win, NULL);
		else
			editor_info_show(vis, "Unknown syntax definition: `%s'", argv[2]);
		break;
	case OPTION_NUMBER:
		window_line_numbers_show(vis->win->win, arg.b);
		break;
	}

	return true;
}

static bool cmd_open(Filerange *range, const char *argv[]) {
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

static bool is_window_closeable(EditorWin *win) {
	if (!text_modified(win->text))
		return true;
	for (EditorWin *w = vis->windows; w; w = w->next) {
		if (w != win && w->text == win->text)
			return true;
	}
	return false;
}

static void info_unsaved_changes(void) {
	editor_info_show(vis, "No write since last change (add ! to override)");
}

static bool cmd_edit(Filerange *range, const char *argv[]) {
	EditorWin *oldwin = vis->win;
	bool force = strchr(argv[0], '!') != NULL;
	if (!force && !is_window_closeable(oldwin)) {
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

static bool cmd_quit(Filerange *range, const char *argv[]) {
	bool force = strchr(argv[0], '!') != NULL;
	if (!force && !is_window_closeable(vis->win)) {
		info_unsaved_changes();
		return false;
	}
	editor_window_close(vis->win);
	if (!vis->windows)
		quit(NULL);
	return true;
}

static bool cmd_xit(Filerange *range, const char *argv[]) {
	if (text_modified(vis->win->text) && !cmd_write(range, argv)) {
		bool force = strchr(argv[0], '!') != NULL;
		if (!force)
			return false;
	}
	return cmd_quit(range, argv);
}

static bool cmd_bdelete(Filerange *range, const char *argv[]) {
	bool force = strchr(argv[0], '!') != NULL;
	Text *txt = vis->win->text;
	if (text_modified(txt) && !force) {
		info_unsaved_changes();
		return false;
	}
	for (EditorWin *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (win->text == txt)
			editor_window_close(win);
	}
	if (!vis->windows)
		quit(NULL);
	return true;
}

static bool cmd_qall(Filerange *range, const char *argv[]) {
	bool force = strchr(argv[0], '!') != NULL;
	for (EditorWin *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (!text_modified(vis->win->text) || force)
			editor_window_close(win);
	}
	if (!vis->windows)
		quit(NULL);
	else
		info_unsaved_changes();
	return vis->windows == NULL;
}

static bool cmd_read(Filerange *range, const char *argv[]) {
	size_t pos = window_cursor_get(vis->win->win);
	for (const char **file = &argv[1]; *file; file++) {
		int fd = open(*file, O_RDONLY);
		char *data = NULL;
		struct stat info;
		if (fd == -1)
			goto err;
		if (fstat(fd, &info) == -1)
			goto err;
		if (!S_ISREG(info.st_mode))
			goto err;
		// XXX: use lseek(fd, 0, SEEK_END); instead?
		data = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (data == MAP_FAILED)
			goto err;

		text_insert(vis->win->text, pos, data, info.st_size);
		pos += info.st_size;
	err:
		if (fd > 2)
			close(fd);
		if (data && data != MAP_FAILED)
			munmap(data, info.st_size);
	}
	editor_draw(vis);
	return true;
}

static bool cmd_substitute(Filerange *range, const char *argv[]) {
	// TODO
	return true;
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

static bool cmd_split(Filerange *range, const char *argv[]) {
	editor_windows_arrange_horizontal(vis);
	if (!argv[1])
		return vis_window_split(vis->win);
	return openfiles(&argv[1]);
}

static bool cmd_vsplit(Filerange *range, const char *argv[]) {
	editor_windows_arrange_vertical(vis);
	if (!argv[1])
		return editor_window_split(vis->win);
	return openfiles(&argv[1]);
}

static bool cmd_new(Filerange *range, const char *argv[]) {
	editor_windows_arrange_horizontal(vis);
	return vis_window_new(NULL);
}

static bool cmd_vnew(Filerange *range, const char *argv[]) {
	editor_windows_arrange_vertical(vis);
	return vis_window_new(NULL);
}

static bool cmd_wq(Filerange *range, const char *argv[]) {
	if (cmd_write(range, argv))
		return cmd_quit(range, argv);
	return false;
}

static bool cmd_write(Filerange *range, const char *argv[]) {
	Text *text = vis->win->text;
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

static bool cmd_saveas(Filerange *range, const char *argv[]) {
	if (cmd_write(range, argv)) {
		text_filename_set(vis->win->text, argv[1]);
		return true;
	}
	return false;
}

static Filepos parse_pos(char **cmd) {
	size_t pos = EPOS;
	Text *txt = vis->win->text;
	Win *win = vis->win->win;
	switch (**cmd) {
	case '.':
		pos = text_line_begin(txt, window_cursor_get(win));
		(*cmd)++;
		break;
	case '$':
		pos = text_size(txt);
		(*cmd)++;
		break;
	case '\'':
		(*cmd)++;
		if ('a' <= **cmd && **cmd <= 'z')
			pos = text_mark_intern_get(txt, **cmd - 'a');
		else if (**cmd == '<')
			pos = text_mark_intern_get(txt, MARK_SELECTION_START);
		else if (**cmd == '>')
			pos = text_mark_intern_get(txt, MARK_SELECTION_END);
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
			pos = text_search_forward(txt, window_cursor_get(win), regex);
		}
		text_regex_free(regex);
		break;
	case '+':
	case '-':
	{
		CursorPos curspos = window_cursor_getpos(win);
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
	Text *txt = vis->win->text;
	Filerange r = text_range_empty();
	switch (**cmd) {
	case '%':
		r.start = 0;
		r.end = text_size(txt);
		(*cmd)++;
		break;
	case '*':
		r.start = text_mark_intern_get(txt, MARK_SELECTION_START);
		r.end = text_mark_intern_get(txt, MARK_SELECTION_END);
		(*cmd)++;
		break;
	default:
		r.start = parse_pos(cmd);
		if (**cmd != ',')
			return r;
		(*cmd)++;
		r.end = parse_pos(cmd);
		break;
	}
	return r;
}

static bool exec_cmdline_command(char *cmdline) {
	static bool init = false;
	if (!init) {
		/* compile the regexes on first inovaction */
		for (Command *c = cmds; c->name; c++)
			regcomp(&c->regex, c->name, REG_EXTENDED);
		init = true;
	}

	char *cmdstart = cmdline;
	Filerange range = parse_range(&cmdstart);
	if (!text_range_valid(&range)) {
		/* if only one position was given, jump to it */
		if (range.start != EPOS && !*cmdstart) {
			window_cursor_to(vis->win->win, range.start);
			return true;
		}

		if (cmdstart != cmdline) {
			editor_info_show(vis, "Invalid range\n");
			return false;
		}
		range = (Filerange){ .start = 0, .end = text_size(vis->win->text) };
	}
	while (*cmdstart == ' ') cmdstart++;
	char *cmdend = strchr(cmdstart, ' ');
	/* regex should only apply to command name */
	if (cmdend)
		*cmdend++ = '\0';

	Command *cmd = NULL;
	for (Command *c = cmds; c->name; c++) {
		if (!regexec(&c->regex, cmdstart, 0, NULL, 0)) {
			cmd = c;
			break;
		}
	}

	if (!cmd) {
		editor_info_show(vis, "Not an editor command");
		return false;
	}

	char *s = cmdend;
	const char *argv[32] = { cmdline };
	for (int i = 1; i < LENGTH(argv); i++) {
		while (s && *s && *s == ' ')
			s++;
		if (s && !*s)
			s = NULL;
		argv[i] = s;
		if (!cmd->args) {
			/* remove trailing spaces */
			if (s) {
				while (*s) s++;
				while (*(--s) == ' ') *s = '\0';
			}
			s = NULL;
		}
		if (s && (s = strchr(s, ' ')))
			*s++ = '\0';
	}

	cmd->cmd(&range, argv);
	return true;
}

static bool exec_command(char type, char *cmd) {
	if (!cmd || !cmd[0])
		return true;
	switch (type) {
	case '/':
	case '?':
		if (text_regex_compile(vis->search_pattern, cmd, REG_EXTENDED)) {
			action_reset(&action);
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
	for (const char **opt = settings; opt && *opt; opt++) {
		char *tmp = strdup(*opt);
		if (tmp)
			exec_cmdline_command(tmp);
		free(tmp);
	}
}

static bool vis_window_new(const char *file) {
	if (!editor_window_new(vis, file))
		return false;
	Syntax *s = window_syntax_get(vis->win->win);
	if (s)
		settings_apply(s->settings);
	return true;
}

static bool vis_window_new_fd(int fd) {
	if (!editor_window_new_fd(vis, fd))
		return false;
	Syntax *s = window_syntax_get(vis->win->win);
	if (s)
		settings_apply(s->settings);
	return true;
}

static bool vis_window_split(EditorWin *win) {
	if (!editor_window_split(win))
		return false;
	Syntax *s = window_syntax_get(vis->win->win);
	if (s)
		settings_apply(s->settings);
	return true;
}

typedef struct Screen Screen;
static struct Screen {
	int w, h;
	bool need_resize;
} screen = { .need_resize = true };

static void die(const char *errstr, ...) {
	va_list ap;
	endwin();
	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static void sigwinch_handler(int sig) {
	screen.need_resize = true;
}

static void resize_screen(Screen *screen) {
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) == -1) {
		getmaxyx(stdscr, screen->h, screen->w);
	} else {
		screen->w = ws.ws_col;
		screen->h = ws.ws_row;
	}

	resizeterm(screen->h, screen->w);
	wresize(stdscr, screen->h, screen->w);
	screen->need_resize = false;
}

static void setup() {
	setlocale(LC_CTYPE, "");
	if (!getenv("ESCDELAY"))
		set_escdelay(50);
	char *term = getenv("TERM");
	if (!term)
		term = DEFAULT_TERM;
	if (!newterm(term, stderr, stdin))
		die("Can not initialize terminal\n");
	start_color();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	resize_screen(&screen);
	/* needed because we use getch() which implicitly calls refresh() which
	   would clear the screen (overwrite it with an empty / unused stdscr */
	refresh();

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);
	sigaction(SIGCONT, &sa, NULL);
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
	KeyBinding *action = keybinding(mode, keys);

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
	} else if (keylen == 1 && key->code == 0 && mode->input) {
		mode->input(key->str, strlen(key->str));
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
	editor_draw(vis);

	while (running) {
		if (screen.need_resize) {
			resize_screen(&screen);
			editor_resize(vis, screen.w, screen.h);
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		editor_update(vis);
		doupdate();
		idle.tv_sec = mode->idle_timeout;
		int r = pselect(1, &fds, NULL, NULL, timeout, &emptyset);
		if (r == -1 && errno == EINTR)
			continue;

		if (r < 0) {
			/* TODO save all pending changes to a ~suffixed file */
			die("Error in mainloop: %s\n", strerror(errno));
		}

		if (!FD_ISSET(STDIN_FILENO, &fds)) {
			if (mode->idle)
				mode->idle();
			timeout = NULL;
			continue;
		}

		Key key = getkey();
		keypress(&key);

		if (mode->idle)
			timeout = &idle;
	}
}

int main(int argc, char *argv[]) {
	/* decide which key configuration to use based on argv[0] */
	char *arg0 = argv[0];
	while (*arg0 && (*arg0 == '.' || *arg0 == '/'))
		arg0++;
	for (int i = 0; i < LENGTH(editors); i++) {
		if (editors[i].name[0] == arg0[0]) {
			config = &editors[i];
			break;
		}
	}

	mode_prev = mode = config->mode;
	setup();

	if (!(vis = editor_new(screen.w, screen.h)))
		die("Could not allocate editor core\n");
	if (!editor_syntax_load(vis, syntaxes, colors))
		die("Could not load syntax highlighting definitions\n");
	editor_statusbar_set(vis, config->statusbar);

	char *cmd = NULL;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			/* handle command line arguments */
			default:
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
	endwin();
	return 0;
}
