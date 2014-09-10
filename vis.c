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
#define _POSIX_SOURCE
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
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
	size_t i;
	const char *s;
	size_t (*m)(Win*);  /* cursor movement based on window content */
	void (*f)(Editor*); /* generic editor commands */
} Arg;

typedef struct {
	char str[6]; /* UTF8 character or terminal escape code */
	int code;    /* curses KEY_* constant */
} Key;

typedef struct {
	Key key[2];
	void (*func)(const Arg *arg);
	const Arg arg;
} KeyBinding;

typedef struct Mode Mode;
struct Mode {
	Mode *parent;                       /* if no match is found in this mode, search will continue there */
	KeyBinding *bindings;               /* NULL terminated array of keybindings for this mode */
	const char *name;                   /* descriptive, user facing name of the mode */
	bool common_prefix;                 /* whether the first key in this mode is always the same */
	void (*enter)(Mode *old);           /* called right before the mode becomes active */
	void (*leave)(Mode *new);           /* called right before the mode becomes inactive */
	bool (*unknown)(Key*, Key*);        /* called whenever a key is not found in this mode,
	                                       the return value determines whether parent modes will be searched */
	void (*input)(const char*, size_t); /* called whenever a key is not found in this mode and all its parent modes */
	void (*idle)(void);                 /* called whenever a certain idle time i.e. without any user input elapsed */
};

typedef struct {
	char *name;                    /* is used to match against argv[0] to enable this config */
	Mode *mode;                    /* default mode in which the editor should start in */
	void (*statusbar)(EditorWin*); /* routine which is called whenever the cursor is moved within a window */
} Config;

typedef struct {
	int count;        /* how many times should the command be executed? */
	Register *reg;    /* always non-NULL, set to a default register */
	Filerange range;  /* which part of the file should be affected by the operator */
	size_t pos;       /* at which byte from the start of the file should the operation start? */
	bool linewise;    /* should the changes always affect whole lines? */
} OperatorContext;

typedef struct {
	void (*func)(OperatorContext*); /* function implementing the operator logic */
	bool selfcontained;             /* is this operator followed by movements/text-objects */
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
	Mark mark;
	Key key;
	Arg arg;
} Action;

typedef struct {                         /* command definitions for the ':'-prompt */
	const char *name;                /* regular expression pattern to match command */
	bool (*cmd)(const char *argv[]); /* command logic called with a NULL terminated array
	                                  * of arguments. argv[0] will be the command name,
	                                  * as matched by the regex. */
	regex_t regex;                   /* compiled form of the pattern in 'name' */
} Command;

/** global variables */
static bool running = true; /* exit main loop once this becomes false */
static Editor *vis;         /* global editor instance, keeps track of all windows etc. */
static Mode *mode;          /* currently active mode, used to search for keybindings */
static Mode *mode_prev;     /* mode which was active previously */
static Action action;       /* current action which is in progress */
static Action action_prev;  /* last operator action used by the repeat '.' key */

/** operators */
static void op_change(OperatorContext *c);
static void op_yank(OperatorContext *c);
static void op_paste(OperatorContext *c);
static void op_delete(OperatorContext *c);

/* these can be passed as int argument to operator(&(const Arg){ .i = OP_*}) */
enum {
	OP_DELETE,
	OP_CHANGE,
	OP_YANK,
	OP_PASTE,
};

static Operator ops[] = {
	[OP_DELETE] = { op_delete, false },
	[OP_CHANGE] = { op_change, false },
	[OP_YANK]   = { op_yank,   false },
	[OP_PASTE]  = { op_paste,  true  },
};

/* these can be passed as int argument to movement(&(const Arg){ .i = MOVE_* }) */
enum {
	MOVE_LINE_UP,
	MOVE_LINE_DOWN,
	MOVE_LINE_BEGIN,
	MOVE_LINE_START,
	MOVE_LINE_FINISH,
	MOVE_LINE_END,
	MOVE_LINE,
	MOVE_COLUMN,
	MOVE_CHAR_PREV,
	MOVE_CHAR_NEXT,
	MOVE_WORD_START_PREV,
	MOVE_WORD_START_NEXT,
	MOVE_WORD_END_PREV,
	MOVE_WORD_END_NEXT,
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
	MOVE_SEARCH_FORWARD,
	MOVE_SEARCH_BACKWARD,
};

/** movements which can be used besides the one in text-motions.h and window.h */
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
/* goto line number action.count, or if zero to end of file */
static size_t line(const Arg *arg);
/* goto to byte action.count on current line */
static size_t column(const Arg *arg);

static Movement moves[] = {
	[MOVE_LINE_UP]         = { .win = window_line_up                                   },
	[MOVE_LINE_DOWN]       = { .win = window_line_down                                 },
	[MOVE_LINE_BEGIN]      = { .txt = text_line_begin,      .type = LINEWISE           },
	[MOVE_LINE_START]      = { .txt = text_line_start,      .type = LINEWISE           },
	[MOVE_LINE_FINISH]     = { .txt = text_line_finish,     .type = LINEWISE           },
	[MOVE_LINE_END]        = { .txt = text_line_end,        .type = LINEWISE           },
	[MOVE_LINE]            = { .cmd = line,                 .type = LINEWISE           },
	[MOVE_COLUMN]          = { .cmd = column,               .type = CHARWISE           },
	[MOVE_CHAR_PREV]       = { .win = window_char_prev                                 },
	[MOVE_CHAR_NEXT]       = { .win = window_char_next                                 },
	[MOVE_WORD_START_PREV] = { .txt = text_word_start_prev, .type = CHARWISE           },
	[MOVE_WORD_START_NEXT] = { .txt = text_word_start_next, .type = CHARWISE           },
	[MOVE_WORD_END_PREV]   = { .txt = text_word_end_prev,   .type = CHARWISE|INCLUSIVE },
	[MOVE_WORD_END_NEXT]   = { .txt = text_word_end_next,   .type = CHARWISE|INCLUSIVE },
	[MOVE_SENTENCE_PREV]   = { .txt = text_sentence_prev,   .type = LINEWISE           },
	[MOVE_SENTENCE_NEXT]   = { .txt = text_sentence_next,   .type = LINEWISE           },
	[MOVE_PARAGRAPH_PREV]  = { .txt = text_paragraph_prev,  .type = LINEWISE           },
	[MOVE_PARAGRAPH_NEXT]  = { .txt = text_paragraph_next,  .type = LINEWISE           },
	[MOVE_BRACKET_MATCH]   = { .txt = text_bracket_match,   .type = LINEWISE|INCLUSIVE },
	[MOVE_FILE_BEGIN]      = { .txt = text_begin,           .type = LINEWISE           },
	[MOVE_FILE_END]        = { .txt = text_end,             .type = LINEWISE           },
	[MOVE_LEFT_TO]         = { .cmd = to_left,              .type = LINEWISE           },
	[MOVE_RIGHT_TO]        = { .cmd = to,                   .type = LINEWISE           },
	[MOVE_LEFT_TILL]       = { .cmd = till_left,            .type = LINEWISE           },
	[MOVE_RIGHT_TILL]      = { .cmd = till,                 .type = LINEWISE           },
	[MOVE_MARK]            = { .cmd = mark_goto,            .type = LINEWISE           },
	[MOVE_MARK_LINE]       = { .cmd = mark_line_goto,       .type = LINEWISE           },
	[MOVE_SEARCH_FORWARD]  = { .cmd = search_forward,       .type = LINEWISE           },
	[MOVE_SEARCH_BACKWARD] = { .cmd = search_backward,      .type = LINEWISE           },
};

/* these can be passed as int argument to textobj(&(const Arg){ .i = TEXT_OBJ_* }) */
enum {
	TEXT_OBJ_WORD,
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
	[TEXT_OBJ_WORD]                 = { text_object_word                  },
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
	[MOVE_LINE_UP]   = &textobjs[TEXT_OBJ_LINE_UP],
	[MOVE_LINE_DOWN] = &textobjs[TEXT_OBJ_LINE_DOWN],
};

/** functions to be called from keybindings */
/* switch to mode indicated by arg->i */
static void switchmode(const Arg *arg);
/* set mark indicated by arg->i to current cursor position */
static void mark_set(const Arg *arg);
/* insert arg->s at the current cursor position */
static void insert(const Arg *arg);
/* insert a tab or the needed amount of spaces at the current cursor position */
static void insert_tab(const Arg *arg);
/* inserts a newline (either \n or \n\r depending on file type) */
static void insert_newline(const Arg *arg);
/* split current window horizontally (default) or vertically (if arg->b is set) */
static void split(const Arg *arg);
/* perform last action i.e. action_prev again */
static void repeat(const Arg *arg);
/* replace character at cursor with one read form keyboard */
static void replace(const Arg *arg);
/* adjust action.count by arg->i */
static void count(const Arg *arg);
/* force operator to linewise (if arg->b is set) */
static void linewise(const Arg *arg);
/* make the current action use the operator indicated by arg->i */
static void operator(const Arg *arg);
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
/* delete from the current cursor position to the start of the previous word */
static void delete_word(const Arg *arg);
/* insert register content indicated by arg->i at current cursor position */
static void insert_register(const Arg *arg);
/* show a user prompt to get input with title arg->s */
static void prompt(const Arg *arg);
/* evaluate user input at prompt, perform search or execute a command */
static void prompt_enter(const Arg *arg);
/* cycle through past user inputs */
static void prompt_up(const Arg *arg);
static void prompt_down(const Arg *arg);
/* blocks to read 3 consecutive digits and inserts the corresponding byte value */
static void insert_verbatim(const Arg *arg);
/* cursor movement based on the current window content as indicated by arg->m
 * which should point to a function from window.h */
static void cursor(const Arg *arg);
/* call editor function as indicated by arg->f */
static void call(const Arg *arg);
static void quit(const Arg *arg);

/** commands to enter at the ':'-prompt */
/* goto line indicated by argv[0] */
static bool cmd_gotoline(const char *argv[]);
/* for each argument create a new window and open the corresponding file */
static bool cmd_open(const char *argv[]);
/* close the current window, if argv[0] contains a '!' discard modifications */
static bool cmd_quit(const char *argv[]);
/* for each argument try to insert the file content at current cursor postion */
static bool cmd_read(const char *argv[]);
static bool cmd_substitute(const char *argv[]);
/* if no argument are given, split the current window horizontally,
 * otherwise open the file */
static bool cmd_split(const char *argv[]);
/* if no argument are given, split the current window vertically,
 * otherwise open the file */
static bool cmd_vsplit(const char *argv[]);
/* save the file displayed in the current window and close it */
static bool cmd_wq(const char *argv[]);
/* save the file displayed in the current window to the name given */
static bool cmd_write(const char *argv[]);

static void action_reset(Action *a);
static void switchmode_to(Mode *new_mode);

#include "config.h"

static Key getkey(void);
static void action_do(Action *a);
static bool exec_command(char *cmdline);

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

static void op_paste(OperatorContext *c) {
	size_t pos = window_cursor_get(vis->win->win);
	if (c->reg->linewise)
		pos = text_line_next(vis->win->text, pos);
	editor_insert(vis, pos, c->reg->data, c->reg->len);
	window_cursor_to(vis->win->win, pos + c->reg->len);
}

/** movement implementations of type: size_t (*move)(const Arg*) */

static size_t search_forward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	return text_search_forward(vis->win->text, pos, vis->search_pattern);
}

static size_t search_backward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	return text_search_backward(vis->win->text, pos, vis->search_pattern);
}

static void mark_set(const Arg *arg) {
	text_mark_set(vis->win->text, arg->i, window_cursor_get(vis->win->win));
}

static size_t mark_goto(const Arg *arg) {
	return text_mark_get(vis->win->text, action.mark);
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
	if (action.count == 0)
		return text_size(vis->win->text);
	size_t pos = text_pos_by_lineno(vis->win->text, action.count);
	action.count = 0;
	return pos;
}

static size_t column(const Arg *arg) {
	char c;
	EditorWin *win = vis->win;
	size_t pos = window_cursor_get(win->win);
	Iterator it = text_iterator_get(win->text, text_line_begin(win->text, pos));
	while (action.count-- > 0 && text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	action.count = 0;
	return it.pos;
}

/** key bindings functions of type: void (*func)(const Arg*) */

static void repeat(const Arg *arg) {
	action = action_prev;
	action_do(&action);
}

static void replace(const Arg *arg) {
	Key k = getkey();
	if (!k.str[0])
		return;
	editor_replace_key(vis, k.str, strlen(k.str));
}

static void count(const Arg *arg) {
	action.count = action.count * 10 + arg->i;
}

static void linewise(const Arg *arg) {
	action.linewise = arg->b;
}

static void operator(const Arg *arg) {
	Operator *op = &ops[arg->i];
	if (mode == &vis_modes[VIS_MODE_VISUAL]) {
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
		action.textobj = moves_linewise[MOVE_LINE_DOWN];
		action_do(&action);
	} else {
		action.op = op;
		if (op->selfcontained)
			action_do(&action);
	}
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
	if (text_undo(vis->win->text))
		editor_draw(vis);
}

static void redo(const Arg *arg) {
	if (text_redo(vis->win->text))
		editor_draw(vis);
}

static void zero(const Arg *arg) {
	if (action.count == 0)
		movement(&(const Arg){ .i = MOVE_LINE_BEGIN });
	else
		count(&(const Arg){ .i = 0 });
}

static void delete_word(const Arg *arg) {
	operator(&(const Arg){ .i = OP_DELETE });
	movement(&(const Arg){ .i = MOVE_WORD_START_PREV });
}

static void insert_register(const Arg *arg) {
	Register *reg = &vis->registers[arg->i];
	editor_insert(vis, window_cursor_get(vis->win->win), reg->data, reg->len);
}

static void prompt(const Arg *arg) {
	editor_prompt_show(vis, arg->s);
	switchmode(&(const Arg){ .i = VIS_MODE_PROMPT });
}

static void prompt_enter(const Arg *arg) {
	char *s = editor_prompt_get(vis);
	/* it is important to switch to normal mode, which hides the prompt and
	 * more importantly resets vis->win to the currently focused editor
	 * window *before* anything is executed which depends on vis->win.
	 */
	switchmode(&(const Arg){ .i = VIS_MODE_NORMAL });
	switch (vis->prompt->title[0]) {
	case '/':
	case '?':
		if (text_regex_compile(vis->search_pattern, s, REG_EXTENDED)) {
			action_reset(&action);
		} else {
			movement(&(const Arg){ .i = vis->prompt->title[0] == '/' ?
				MOVE_SEARCH_FORWARD : MOVE_SEARCH_BACKWARD });
		}
		break;
	case ':':
		exec_command(s);
		break;
	}
	free(s);
	editor_draw(vis);
}

static void prompt_up(const Arg *arg) {

}

static void prompt_down(const Arg *arg) {

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

static void split(const Arg *arg) {
	if (arg->b)
		editor_window_vsplit(vis, arg->s);
	else
		editor_window_split(vis, arg->s);
}

static void cursor(const Arg *arg) {
	arg->m(vis->win->win);
}

static void call(const Arg *arg) {
	arg->f(vis);
}

static void insert(const Arg *arg) {
	editor_insert_key(vis, arg->s, arg->s ? strlen(arg->s) : 0);
}

static void insert_tab(const Arg *arg) {
	insert(&(const Arg){ .s = "\t" });
}

static void insert_newline(const Arg *arg) {
	// TODO determine file type to insert \n\r or \n
	insert(&(const Arg){ .s = "\n" });
}

static void switchmode(const Arg *arg) {
	switchmode_to(&vis_modes[arg->i]);
}

/** action processing: execut the operator / movement / text object */

static void action_do(Action *a) {
	Text *txt = vis->win->text;
	Win *win = vis->win->win;
	size_t pos = window_cursor_get(win);
	if (a->count == 0)
		a->count = 1;
	OperatorContext c = {
		.count = a->count,
		.pos = pos,
		.reg = a->reg ? a->reg : &vis->registers[REG_DEFAULT],
		.linewise = a->linewise,
	};

	if (a->movement) {
		size_t start = pos;
		for (int i = 0; i < a->count; i++) {
			if (a->movement->txt)
				pos = a->movement->txt(txt, pos);
			else if (a->movement->win)
				pos = a->movement->win(win);
			else
				pos = a->movement->cmd(&a->arg);
			if (pos == (size_t)-1)
				break;
		}

		if (pos == (size_t)-1) {
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
		} else if (a->movement->type & INCLUSIVE) {
			Iterator it = text_iterator_get(txt, c.range.end);
			text_iterator_char_next(&it, NULL);
			c.range.end = it.pos;
		}
	} else if (a->textobj) {
		Filerange r;
		c.range.start = c.range.end = pos;
		for (int i = 0; i < a->count; i++) {
			r = a->textobj->range(txt, pos);
			// TODO range_valid?
			if (r.start == (size_t)-1 || r.end == (size_t)-1)
				break;
			if (a->textobj->type == OUTER) {
				r.start--;
				r.end++;
			}
			// TODO c.range = range_union(&c.range, &r);
			c.range.start = MIN(c.range.start, r.start);
			c.range.end = MAX(c.range.end, r.end);
			if (i < a->count - 1) {
				if (a->textobj == &textobjs[TEXT_OBJ_LINE_UP]) {
					pos = c.range.start - 1;
				} else {
					pos = c.range.end + 1;
				}
			}
		}
	} else if (mode == &vis_modes[VIS_MODE_VISUAL]) {
		c.range = window_selection_get(win);
		if (c.range.start == (size_t)-1 || c.range.end == (size_t)-1)
			c.range.start = c.range.end = pos;
	}

	if (a->op) {
		a->op->func(&c);
		if (mode == &vis_modes[VIS_MODE_OPERATOR])
			switchmode_to(mode_prev);
		else if (mode == &vis_modes[VIS_MODE_VISUAL])
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
	mode_prev = mode;
	//fprintf(stderr, "%s -> %s\n", mode_prev->name, new_mode->name);
	mode = new_mode;
	if (mode->enter)
		mode->enter(mode_prev);
	// TODO display mode name somewhere?

}

/** ':'-command implementations */

static bool cmd_gotoline(const char *argv[]) {
	action.count = strtoul(argv[0], NULL, 10);
	movement(&(const Arg){ .i = MOVE_LINE });
	return true;
}

static bool cmd_open(const char *argv[]) {
	for (const char **file = &argv[1]; *file; file++)
		editor_window_new(vis, *file);
	return true;
}

static bool cmd_quit(const char *argv[]) {
	bool force = strchr(argv[0], '!') != NULL;
	for (EditorWin *win = vis->windows; !force && win; win = win->next) {
		if (win != vis->win && win->text == vis->win->text)
			force = true;
	}
	if (!force && text_modified(vis->win->text))
		return false;
	editor_window_close(vis);
	if (!vis->windows)
		running = false;
	return true;
}

static bool cmd_read(const char *argv[]) {
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

		text_insert_raw(vis->win->text, pos, data, info.st_size);
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

static bool cmd_substitute(const char *argv[]) {
	// TODO
	return true;
}

static bool cmd_split(const char *argv[]) {
	editor_window_split(vis, argv[1]);
	for (const char **file = &argv[2]; *file; file++)
		editor_window_split(vis, *file);
	return true;
}

static bool cmd_vsplit(const char *argv[]) {
	editor_window_vsplit(vis, argv[1]);
	for (const char **file = &argv[2]; *file; file++)
		editor_window_vsplit(vis, *file);
	return true;
}

static bool cmd_wq(const char *argv[]) {
	if (cmd_write(argv))
		return cmd_quit(argv);
	return false;
}

static bool cmd_write(const char *argv[]) {
	Text *text = vis->win->text;
	if (!argv[1])
		argv[1] = text_filename(text);
	for (const char **file = &argv[1]; *file; file++) {
		if (text_save(text, *file))
			return false;
	}
	return true;
}

static bool exec_command(char *cmdline) {
	static bool init = false;
	if (!init) {
		/* compile the regexes on first inovaction */
		for (Command *c = cmds; c->name; c++)
			regcomp(&c->regex, c->name, REG_EXTENDED);
		init = true;
	}

	Command *cmd = NULL;
	for (Command *c = cmds; c->name; c++) {
		if (!regexec(&c->regex, cmdline, 0, NULL, 0)) {
			cmd = c;
			break;
		}
	}

	if (!cmd)
		return false;

	const char *argv[32] = { cmdline };
	char *s = cmdline;
	for (int i = 1; i < LENGTH(argv); i++) {
		if (s) {
			if ((s = strchr(s, ' ')))
				*s++ = '\0';
		}
		while (s && *s && *s == ' ')
			s++;
		argv[i] = s ? s : NULL;
	}

	cmd->cmd(argv);
	return true;
}

/* default editor configuration to use */
static Config *config = &editors[0];


typedef struct Screen Screen;
static struct Screen {
	int w, h;
	bool need_resize;
} screen = { .need_resize = true };

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
	initscr();
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
}

static void cleanup() {
	endwin();
	//delscreen(set_term(NULL));
}

static bool keymatch(Key *key0, Key *key1) {
	return (key0->str[0] && memcmp(key0->str, key1->str, sizeof(key1->str)) == 0) ||
	       (key0->code && key0->code == key1->code);
}

static KeyBinding *keybinding(Mode *mode, Key *key0, Key *key1) {
	for (; mode; mode = mode->parent) {
		if (mode->common_prefix && !keymatch(key0, &mode->bindings->key[0]))
			continue;
		for (KeyBinding *kb = mode->bindings; kb && (kb->key[0].code || kb->key[0].str[0]); kb++) {
			if (keymatch(key0, &kb->key[0]) && (!key1 || keymatch(key1, &kb->key[1])))
				return kb;
		}
		if (mode->unknown && !mode->unknown(key0, key1))
			break;
	}
	return NULL;
}

static Key getkey(void) {
	Key key = { .str = "\0\0\0\0\0\0", .code = 0 };
	int keycode = getch(), len = 0;
	if (keycode == ERR)
		return key;

	if (keycode >= KEY_MIN) {
		key.code = keycode;
	} else {
		char keychar = keycode;
		key.str[len++] = keychar;

		if (!ISASCII(keychar) || keychar == '\e') {
			nodelay(stdscr, TRUE);
			for (int t; len < LENGTH(key.str) && (t = getch()) != ERR; len++)
				key.str[len] = t;
			nodelay(stdscr, FALSE);
		}
	}

	return key;
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
		return 1;
	if (!editor_syntax_load(vis, syntaxes, colors))
		return 1;
	editor_statusbar_set(vis, config->statusbar);

	if (!editor_window_new(vis, argc > 1 ? argv[1] : NULL))
		return 1;
	for (int i = 2; i < argc; i++) {
		if (!editor_window_new(vis, argv[i]))
			return 1;
	}

	struct timeval idle = { .tv_usec = 0 }, *timeout = NULL;
	Key key, key_prev, *key_mod = NULL;

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
		idle.tv_sec = 3;
		int r = select(1, &fds, NULL, NULL, timeout);
		if (r == -1 && errno == EINTR)
			continue;

		if (r < 0) {
			perror("select()");
			exit(EXIT_FAILURE);
		}

		if (!FD_ISSET(STDIN_FILENO, &fds)) {
			if (mode->idle)
				mode->idle();
			timeout = NULL;
			continue;
		}

		key = getkey();
		KeyBinding *action = keybinding(mode, key_mod ? key_mod : &key, key_mod ? &key : NULL);

		if (!action && key_mod) {
			/* second char of a combination was invalid, search again without the prefix */
			action = keybinding(mode, &key, NULL);
			key_mod = NULL;
		}
		if (action) {
			/* check if it is the first part of a combination */
			if (!key_mod && (action->key[1].code || action->key[1].str[0])) {
				key_prev = key;
				key_mod = &key_prev;
				continue;
			}
			action->func(&action->arg);
			key_mod = NULL;
			continue;
		}

		if (key.code)
			continue;

		if (mode->input)
			mode->input(key.str, strlen(key.str));
		if (mode->idle)
			timeout = &idle;
	}

	editor_free(vis);
	cleanup();
	return 0;
}
