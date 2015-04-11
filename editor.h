#ifndef EDITOR_H
#define EDITOR_H

#include <curses.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct Editor Editor;
typedef struct EditorWin EditorWin;

#include "ui.h"
#include "window.h"
#include "register.h"
#include "macro.h"
#include "syntax.h"
#include "ring-buffer.h"
#include "map.h"



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
	const Operator *op;
	const Movement *movement;
	const TextObject *textobj;
	Register *reg;
	int mark;
	Key key;
	Arg arg;
} Action;

enum CmdOpt {          /* option flags for command definitions */
	CMD_OPT_NONE,  /* no option (default value) */
	CMD_OPT_FORCE, /* whether the command can be forced by appending '!' */
	CMD_OPT_ARGS,  /* whether the command line should be parsed in to space
	                * separated arguments to placed into argv, otherwise argv[1]
	                * will contain the  remaining command line unmodified */
};

typedef struct {             /* command definitions for the ':'-prompt */
	const char *name[3]; /* name and optional alias for the command */
	/* command logic called with a NULL terminated array of arguments.
	 * argv[0] will be the command name */
	bool (*cmd)(Filerange*, enum CmdOpt opt, const char *argv[]);
	enum CmdOpt opt;     /* command option flags */
} Command;

enum Reg {
	REG_a,
	REG_b,
	REG_c,
	REG_d,
	REG_e,
	REG_f,
	REG_g,
	REG_h,
	REG_i,
	REG_j,
	REG_k,
	REG_l,
	REG_m,
	REG_n,
	REG_o,
	REG_p,
	REG_q,
	REG_r,
	REG_s,
	REG_t,
	REG_u,
	REG_v,
	REG_w,
	REG_x,
	REG_y,
	REG_z,
	REG_DEFAULT,
	REG_LAST,
};

enum Mark {
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
	MARK_LAST,
};


typedef struct VisText VisText;

struct VisText {
	Text *data;
	int refcount;
	Mark marks[MARK_LAST];
	VisText *next, *prev;
};

typedef struct {
	size_t index;           /* #number of changes */
	size_t pos;             /* where the current change occured */
} ChangeList;

struct EditorWin {
	Editor *editor;         /* editor instance to which this window belongs */
	UiWin *ui;
	VisText *text;             /* underlying text management */
	Win *win;               /* window for the text area  */
	ViewEvent events;
	RingBuffer *jumplist;   /* LRU jump management */
	ChangeList changelist;  /* state for iterating through least recently changes */
	EditorWin *prev, *next; /* neighbouring windows */
};

struct Editor {
	Ui *ui;
	VisText *texts;
	EditorWin *windows;               /* list of windows */
	EditorWin *win;                   /* currently active window */
	Syntax *syntaxes;                 /* NULL terminated array of syntax definitions */
	Register registers[REG_LAST];     /* register used for copy and paste */
	Macro macros[26];                 /* recorded macros */
	Macro *recording, *last_recording;/* currently and least recently recorded macro */
	EditorWin *prompt;                /* 1-line height window to get user input */
	EditorWin *prompt_window;         /* window which was focused before prompt was shown */
	char prompt_type;                 /* command ':' or search '/','?' prompt */
	Regex *search_pattern;            /* last used search pattern */
	int tabwidth;                     /* how many spaces should be used to display a tab */
	bool expandtab;                   /* whether typed tabs should be converted to spaces */
	bool autoindent;                  /* whether indentation should be copied from previous line on newline */
	Map *cmds;                        /* ":"-commands, used for unique prefix queries */
	Map *options;                     /* ":set"-options */
	Buffer buffer_repeat;             /* holds data to repeat last insertion/replacement */
	
	Action action;       /* current action which is in progress */
	Action action_prev;  /* last operator action used by the repeat '.' key */
	Mode *mode;          /* currently active mode, used to search for keybindings */
	Mode *mode_prev;     /* previsouly active user mode */
	Mode *mode_before_prompt; /* user mode which was active before entering prompt */
	volatile bool running; /* exit main loop once this becomes false */
};

Editor *editor_new(Ui*);
void editor_free(Editor*);
void editor_resize(Editor*);
void editor_draw(Editor*);
void editor_update(Editor*);
void editor_suspend(Editor*);

/* these function operate on the currently focused window but make sure
 * that all windows which show the affected region are redrawn too. */
void editor_insert_key(Editor*, const char *c, size_t len);
void editor_replace_key(Editor*, const char *c, size_t len);
void editor_backspace_key(Editor*);
void editor_delete_key(Editor*);
void editor_insert(Editor*, size_t pos, const char *data, size_t len);
void editor_delete(Editor*, size_t pos, size_t len);

/* set tabwidth (must be in range [1, 8], affects all windows */
void editor_tabwidth_set(Editor*, int tabwidth);
int editor_tabwidth_get(Editor*);

/* load a set of syntax highlighting definitions which will be associated
 * to the underlying window based on the file type loaded.
 *
 * both *syntaxes and *colors must point to a NULL terminated arrays.
 */
bool editor_syntax_load(Editor*, Syntax *syntaxes, Color *colors);
void editor_syntax_unload(Editor*);

/* creates a new window, and loads the given file. if filename is NULL
 * an unamed / empty buffer is created. If the given file is already opened
 * in another window, share the underlying text that is changes will be
 * visible in both windows */
bool editor_window_new(Editor*, const char *filename);
bool editor_window_new_fd(Editor*, int fd);
/* reload the file currently displayed in the window from disk */
bool editor_window_reload(EditorWin*);
void editor_window_close(EditorWin*);
/* split the given window. changes to the displayed text will be reflected
 * in both windows */
bool editor_window_split(EditorWin*);
/* focus the next / previous window */
void editor_window_next(Editor*);
void editor_window_prev(Editor*);

void editor_window_jumplist_add(EditorWin*, size_t pos);
size_t editor_window_jumplist_prev(EditorWin*);
size_t editor_window_jumplist_next(EditorWin*);
void editor_window_jumplist_invalidate(EditorWin*);

size_t editor_window_changelist_prev(EditorWin*);
size_t editor_window_changelist_next(EditorWin*);
/* rearrange all windows either vertically or horizontally */
void editor_windows_arrange(Editor*, enum UiLayout);
/* display a user prompt with a certain title and default text */
void editor_prompt_show(Editor*, const char *title, const char *text);
/* hide the user prompt if it is currently shown */
void editor_prompt_hide(Editor*);
/* return the content of the command prompt in a malloc(3)-ed string
 * which the call site has to free. */
char *editor_prompt_get(Editor*);
/* replace the current command line content with the one given */
void editor_prompt_set(Editor*, const char *line);

/* display a message to the user */
void editor_info_show(Editor*, const char *msg, ...);
void editor_info_hide(Editor*);
void editor_window_options(EditorWin*, enum UiOption options);

/* look up a curses color pair for the given combination of fore and
 * background color */
short editor_color_get(short fg, short bg);

#endif
