#ifndef VIS_H
#define VIS_H

#include <signal.h>
#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>

typedef struct Vis Vis;
typedef struct File File;
typedef struct Win Win;

#include "ui.h"
#include "view.h"
#include "register.h"
#include "macro.h"
#include "syntax.h"
#include "ring-buffer.h"
#include "map.h"
#include "text-regex.h"


typedef union {
	bool b;
	int i;
	const char *s;
	void (*w)(View*);    /* generic window commands */
	void (*f)(Vis*); /* generic editor commands */
} Arg;

typedef struct {
	const char *name;
	const char *help;
	const char* (*func)(Vis*, const char *keys, const Arg*);
	/* returns a pointer to the first not consumed character in keys
	 * or NULL if not enough input was available to complete the command */
	const Arg arg;

} KeyAction;

typedef struct {
	const char *key;
	KeyAction *action;
	const char *alias;
} KeyBinding;



Vis *vis_new(Ui*);
void vis_free(Vis*);
void vis_resize(Vis*);
void vis_draw(Vis*);
void vis_update(Vis*);
void vis_suspend(Vis*);

/* load a set of syntax highlighting definitions which will be associated
 * to the underlying window based on the file type loaded.
 *
 * The parameter `syntaxes' has to point to a NULL terminated array.
 */
bool vis_syntax_load(Vis*, Syntax *syntaxes);
void vis_syntax_unload(Vis*);

/* creates a new window, and loads the given file. if filename is NULL
 * an unamed / empty buffer is created. If the given file is already opened
 * in another window, share the underlying text that is changes will be
 * visible in both windows */
bool vis_window_new(Vis*, const char *filename);
/* reload the file currently displayed in the window from disk */
bool vis_window_reload(Win*);
void vis_window_close(Win*);
/* split the given window. changes to the displayed text will be reflected
 * in both windows */
bool vis_window_split(Win*);
/* focus the next / previous window */
void vis_window_next(Vis*);
void vis_window_prev(Vis*);
/* display a user prompt with a certain title and default text */
void vis_prompt_show(Vis*, const char *title, const char *text);
/* hide the user prompt if it is currently shown */
void vis_prompt_hide(Vis*);
/* return the content of the command prompt in a malloc(3)-ed string
 * which the call site has to free. */
char *vis_prompt_get(Vis*);
/* replace the current command line content with the one given */
void vis_prompt_set(Vis*, const char *line);

/* display a message to the user */
void vis_info_show(Vis*, const char *msg, ...);
void vis_info_hide(Vis*);

/* these function operate on the currently focused window but make sure
 * that all windows which show the affected region are redrawn too. */
void vis_insert_key(Vis*, const char *data, size_t len);
void vis_replace_key(Vis*, const char *data, size_t len);
void vis_insert(Vis*, size_t pos, const char *data, size_t len);
void vis_delete(Vis*, size_t pos, size_t len);
void vis_replace(Vis*, size_t pos, const char *data, size_t len);

int vis_run(Vis*, int argc, char *argv[]);
void vis_exit(Vis*, int status);
void vis_die(Vis*, const char *msg, ...);

enum VisMode {
	VIS_MODE_BASIC,
	VIS_MODE_MOVE,
	VIS_MODE_OPERATOR,
	VIS_MODE_OPERATOR_OPTION,
	VIS_MODE_NORMAL,
	VIS_MODE_TEXTOBJ,
	VIS_MODE_VISUAL,
	VIS_MODE_VISUAL_LINE,
	VIS_MODE_READLINE,
	VIS_MODE_PROMPT,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
	VIS_MODE_LAST,
};

void vis_mode_switch(Vis*, enum VisMode);
bool vis_mode_map(Vis*, enum VisMode, const char *name, KeyBinding*);
bool vis_mode_unmap(Vis*, enum VisMode, const char *name); 
bool vis_mode_bindings(Vis*, enum VisMode, KeyBinding **bindings);
const char *vis_mode_status(Vis*);
/* TODO: temporary */
typedef struct Mode Mode;
void vis_mode_set(Vis*, Mode*);

bool vis_action_register(Vis*, KeyAction*);

enum VisOperator {
	OP_DELETE,
	OP_CHANGE,
	OP_YANK,
	OP_PUT,
	OP_SHIFT_RIGHT,
	OP_SHIFT_LEFT,
	OP_JOIN,
	OP_REPEAT_INSERT,
	OP_REPEAT_REPLACE,
	OP_CURSOR_SOL,
	OP_CASE_SWAP,
	/* pseudo operators: keep them at the end to save space in array definition */
	OP_CASE_LOWER,
	OP_CASE_UPPER,
	OP_CURSOR_EOL,
};

/* TODO: overhaul repeatable infrastructure:
 *        - put is not really an operator, but should still be repeatable
 *          and respect count
 "        - review OP_REPEAT_{REPLACE,INSERT}
 */
enum {
	PUT_AFTER,
	PUT_AFTER_END,
	PUT_BEFORE,
	PUT_BEFORE_END,
};

bool vis_operator(Vis*, enum VisOperator);

enum VisMotion {
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
	MOVE_LINE_CHAR_PREV,
	MOVE_LINE_CHAR_NEXT,
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
	MOVE_FUNCTION_START_PREV,
	MOVE_FUNCTION_START_NEXT,
	MOVE_FUNCTION_END_PREV,
	MOVE_FUNCTION_END_NEXT,
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
	MOVE_SEARCH_NEXT,
	MOVE_SEARCH_PREV,
	MOVE_WINDOW_LINE_TOP,
	MOVE_WINDOW_LINE_MIDDLE,
	MOVE_WINDOW_LINE_BOTTOM,
	MOVE_CHANGELIST_NEXT,
	MOVE_CHANGELIST_PREV,
	MOVE_JUMPLIST_NEXT,
	MOVE_JUMPLIST_PREV,
	MOVE_NOP,
	/* pseudo motions: keep them at the end to save space in array definition */
	MOVE_TOTILL_REPEAT,
	MOVE_TOTILL_REVERSE,
	MOVE_SEARCH_FORWARD,
	MOVE_SEARCH_BACKWARD,
};

bool vis_motion(Vis*, enum VisMotion, ...);

int vis_count_get(Vis*);
void vis_count_set(Vis*, int count);

enum VisMotionType {
	VIS_MOTIONTYPE_LINEWISE  = 1 << 0,
	VIS_MOTIONTYPE_CHARWISE  = 1 << 1,
};

void vis_motion_type(Vis *vis, enum VisMotionType);

enum VisTextObject {
	TEXT_OBJ_INNER_WORD,
	TEXT_OBJ_OUTER_WORD,
	TEXT_OBJ_INNER_LONGWORD,
	TEXT_OBJ_OUTER_LONGWORD,
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
	TEXT_OBJ_OUTER_ENTIRE,
	TEXT_OBJ_INNER_ENTIRE,
	TEXT_OBJ_OUTER_FUNCTION,
	TEXT_OBJ_INNER_FUNCTION,
	TEXT_OBJ_OUTER_LINE,
	TEXT_OBJ_INNER_LINE,
};

void vis_textobject(Vis*, enum VisTextObject);

enum VisMacro {
	/* XXX: TEMPORARY */
	VIS_MACRO_LAST_RECORDED = 26,
	VIS_MACRO_INVALID, /* hast to be the last enum member */
};

bool vis_macro_record(Vis*, enum VisMacro);
bool vis_macro_record_stop(Vis*);
bool vis_macro_replay(Vis*, enum VisMacro);

enum VisMark {
	MARK_a,
	MARK_b,
	MARK_c,
	MARK_d,
	MARK_e,
	MARK_f,
	MARK_g,
	MARK_h,
	MARK_i,
	MARK_j,
	MARK_k,
	MARK_l,
	MARK_m,
	MARK_n,
	MARK_o,
	MARK_p,
	MARK_q,
	MARK_r,
	MARK_s,
	MARK_t,
	MARK_u,
	MARK_v,
	MARK_w,
	MARK_x,
	MARK_y,
	MARK_z,
	MARK_SELECTION_START,
	MARK_SELECTION_END,
	VIS_MARK_INVALID,
};

void vis_mark_set(Vis*, enum VisMark mark, size_t pos);

enum VisRegister {
	REG_a, REG_b, REG_c, REG_d, REG_e, REG_f, REG_g, REG_h, REG_i,
	REG_j, REG_k, REG_l, REG_m, REG_n, REG_o, REG_p, REG_q, REG_r,
	REG_s, REG_t, REG_u, REG_v, REG_w, REG_x, REG_y, REG_z,
	REG_DEFAULT,
	VIS_REGISTER_INVALID,
};

void vis_register_set(Vis*, enum VisRegister);
Register *vis_register_get(Vis*, enum VisRegister);

void vis_repeat(Vis*);

/* execute a :-command (call without without leading ':') */
bool vis_cmd(Vis*, const char *cmd);
/* TODO temporary. type is either '/', '?', '+', or ':' */
bool vis_prompt_cmd(Vis *vis, char type, const char *cmd);

const char *vis_key_next(Vis*, const char *keys);
const char *vis_keys(Vis*, const char *input);

const char *vis_expandtab(Vis*);

bool vis_signal_handler(Vis*, int signum, const siginfo_t *siginfo,
	const void *context);

/* TODO: temporary */
typedef struct Operator Operator;
typedef struct Movement Movement;
typedef struct TextObject TextObject;

typedef struct {             /** collects all information until an operator is executed */
	int count;
	enum VisMotionType type;
	const Operator *op;
	const Movement *movement;
	const TextObject *textobj;
	Register *reg;
	enum VisMark mark;
	Arg arg;
} Action;

struct File {
	Text *text;
	const char *name;
	volatile sig_atomic_t truncated;
	bool is_stdin;
	struct stat stat;
	int refcount;
	Mark marks[VIS_MARK_INVALID];
	File *next, *prev;
};

typedef struct {
	time_t state;           /* state of the text, used to invalidate change list */
	size_t index;           /* #number of changes */
	size_t pos;             /* where the current change occured */
} ChangeList;

struct Win {
	Vis *editor;         /* editor instance to which this window belongs */
	UiWin *ui;
	File *file;             /* file being displayed in this window */
	View *view;             /* currently displayed part of underlying text */
	ViewEvent events;
	RingBuffer *jumplist;   /* LRU jump management */
	ChangeList changelist;  /* state for iterating through least recently changes */
	Win *prev, *next;       /* neighbouring windows */
};

struct Vis {
	Ui *ui;
	File *files;
	Win *windows;                     /* list of windows */
	Win *win;                         /* currently active window */
	Syntax *syntaxes;                 /* NULL terminated array of syntax definitions */
	Register registers[VIS_REGISTER_INVALID];     /* register used for copy and paste */
	Macro macros[VIS_MACRO_INVALID];         /* recorded macros */
	Macro *recording, *last_recording;/* currently and least recently recorded macro */
	Win *prompt;                      /* 1-line height window to get user input */
	Win *prompt_window;               /* window which was focused before prompt was shown */
	char prompt_type;                 /* command ':' or search '/','?' prompt */
	Regex *search_pattern;            /* last used search pattern */
	char search_char[8];              /* last used character to search for via 'f', 'F', 't', 'T' */
	int last_totill;                  /* last to/till movement used for ';' and ',' */
	int tabwidth;                     /* how many spaces should be used to display a tab */
	bool expandtab;                   /* whether typed tabs should be converted to spaces */
	bool autoindent;                  /* whether indentation should be copied from previous line on newline */
	Map *cmds;                        /* ":"-commands, used for unique prefix queries */
	Map *options;                     /* ":set"-options */
	Buffer buffer_repeat;             /* holds data to repeat last insertion/replacement */
	Buffer input_queue;               /* holds pending input keys */
	
	Action action;       /* current action which is in progress */
	Action action_prev;  /* last operator action used by the repeat '.' key */
	Mode *mode;          /* currently active mode, used to search for keybindings */
	Mode *mode_prev;     /* previsouly active user mode */
	Mode *mode_before_prompt; /* user mode which was active before entering prompt */
	volatile bool running; /* exit main loop once this becomes false */
	int exit_status;
	volatile sig_atomic_t cancel_filter; /* abort external command */
	volatile sig_atomic_t sigbus;
	sigjmp_buf sigbus_jmpbuf;
	Map *actions;          /* built in special editor keys / commands */
};

#endif
