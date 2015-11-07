#ifndef VIS_CORE_H
#define VIS_CORE_H

#include "vis.h"
#include "text.h"
#include "text-regex.h"
#include "map.h"
#include "ring-buffer.h"
#include "macro.h"

/* a mode contains a set of key bindings which are currently valid.
 *
 * each mode can specify one parent mode which is consultated if a given key
 * is not found in the current mode. hence the modes form a tree which is
 * searched from the current mode up towards the root mode until a valid binding
 * is found.
 *
 * if no binding is found, mode->input(...) is called and the user entered
 * keys are passed as argument. this is used to change the document content.
 */
typedef struct Mode Mode;
struct Mode {
	Mode *parent;                       /* if no match is found in this mode, search will continue there */
	Map *bindings;                      
	const char *name;                   /* descriptive, user facing name of the mode */
	const char *status;                 /* name displayed in the window status bar */
	const char *help;                   /* short description used by :help */
	bool isuser;                        /* whether this is a user or internal mode */
	void (*enter)(Vis*, Mode *old);           /* called right before the mode becomes active */
	void (*leave)(Vis*, Mode *new);           /* called right before the mode becomes inactive */
	void (*input)(Vis*, const char*, size_t); /* called whenever a key is not found in this mode and all its parent modes */
	void (*idle)(Vis*);                 /* called whenever a certain idle time i.e. without any user input elapsed */
	time_t idle_timeout;                /* idle time in seconds after which the registered function will be called */
	bool visual;                        /* whether text selection is possible in this mode */
};



typedef struct {
	int count;        /* how many times should the command be executed? */
	Register *reg;    /* always non-NULL, set to a default register */
	Filerange range;  /* which part of the file should be affected by the operator */
	size_t pos;       /* at which byte from the start of the file should the operation start? */
	size_t newpos;    /* new position after motion or EPOS if none given */
	bool linewise;    /* should the changes always affect whole lines? */
	const Arg *arg;   /* arbitrary arguments */
} OperatorContext;

typedef struct {
	size_t (*func)(Vis*, Text*, OperatorContext*); /* operator logic, returns new cursor position */
} Operator;

typedef struct {
	/* TODO: merge types / use union to save space */
	size_t (*cur)(Cursor*);            /* a movement based on current window content from view.h */
	size_t (*txt)(Text*, size_t pos); /* a movement form text-motions.h */
	size_t (*file)(Vis*, File*, size_t pos);
	size_t (*vis)(Vis*, Text*, size_t pos);
	size_t (*view)(Vis*, View*);
	size_t (*win)(Vis*, Win*, size_t pos);
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
	enum VisMotionType type;
	const Operator *op;
	const Movement *movement;
	const TextObject *textobj;
	const Macro *macro;
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
	Macro *macro_operator;
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
	Buffer *keys;          /* if non-NULL we are currently handling keys from this buffer,
	                        * points to either the input_queue or a macro */
};

/* TODO: make part of Vis struct? enable dynamic modes? */
extern Mode vis_modes[VIS_MODE_LAST];

extern Movement moves[MOVE_INVALID];

extern Operator ops[OP_INVALID];

const char *expandtab(Vis *vis);

void macro_operator_stop(Vis *vis);
void macro_operator_record(Vis *vis);

void action_reset(Action*);

void mode_set(Vis *vis, Mode *new_mode);
Mode *mode_get(Vis *vis, enum VisMode mode);

#endif