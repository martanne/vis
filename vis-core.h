#ifndef VIS_CORE_H
#define VIS_CORE_H

#include <setjmp.h>
#include "vis.h"
#include "sam.h"
#include "vis-lua.h"
#include "text.h"
#include "text-util.h"
#include "map.h"
#include "array.h"
#include "buffer.h"
#include "util.h"

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
	enum VisMode id;
	Mode *parent;                       /* if no match is found in this mode, search will continue there */
	Map *bindings;
	const char *name;                   /* descriptive, user facing name of the mode */
	const char *status;                 /* name displayed in the window status bar */
	const char *help;                   /* short description used by :help */
	void (*enter)(Vis*, Mode *old);           /* called right before the mode becomes active */
	void (*leave)(Vis*, Mode *new);           /* called right before the mode becomes inactive */
	void (*input)(Vis*, const char*, size_t); /* called whenever a key is not found in this mode and all its parent modes */
	void (*idle)(Vis*);                 /* called whenever a certain idle time i.e. without any user input elapsed */
	time_t idle_timeout;                /* idle time in seconds after which the registered function will be called */
	bool visual;                        /* whether text selection is possible in this mode */
};

typedef struct {
	Array values;
	bool linewise; /* place register content on a new line when inserting? */
	bool append;
	enum {
		REGISTER_NORMAL,
		REGISTER_NUMBER,
		REGISTER_BLACKHOLE,
		REGISTER_CLIPBOARD,
	} type;
} Register;

struct OperatorContext {
	int count;        /* how many times should the command be executed? */
	Register *reg;    /* always non-NULL, set to a default register */
	size_t reg_slot;  /* register slot to use */
	Filerange range;  /* which part of the file should be affected by the operator */
	size_t pos;       /* at which byte from the start of the file should the operation start? */
	size_t newpos;    /* new position after motion or EPOS if none given */
	bool linewise;    /* should the changes always affect whole lines? */
	const Arg *arg;   /* arbitrary arguments */
	void *context;    /* used by user-registered operators */
};

typedef struct {
	/* operator logic, returns new cursor position, if EPOS is
	 * the cursor is disposed (except if it is the primary one) */
	VisOperatorFunction *func;
	void *context;
} Operator;

typedef struct { /* Motion implementation, takes a cursor postion and returns a new one */
	/* TODO: merge types / use union to save space */
	size_t (*cur)(Selection*);
	size_t (*txt)(Text*, size_t pos);
	size_t (*file)(Vis*, File*, Selection*);
	size_t (*vis)(Vis*, Text*, size_t pos);
	size_t (*view)(Vis*, View*);
	size_t (*win)(Vis*, Win*, size_t pos);
	size_t (*user)(Vis*, Win*, void*, size_t pos);
	enum {
		LINEWISE  = VIS_MOTIONTYPE_LINEWISE,  /* should the covered range be extended to whole lines? */
		CHARWISE  = VIS_MOTIONTYPE_CHARWISE,  /* scrolls window content until position is visible */
		INCLUSIVE = 1 << 2,  /* should new position be included in operator range? */
		LINEWISE_INCLUSIVE = 1 << 3,  /* inclusive, but only if motion is linewise? */
		IDEMPOTENT = 1 << 4, /* does the returned postion remain the same if called multiple times? */
		JUMP = 1 << 5, /* should the resulting position of the motion be recorded in the jump list? */
		COUNT_EXACT = 1 << 6, /* fail (keep initial position) if count can not be satisfied exactly */
	} type;
	void *data;
} Movement;

typedef struct {
	/* gets a cursor position and returns a file range (or text_range_empty())
	 * representing the text object containing the position. */
	Filerange (*txt)(Text*, size_t pos);
	Filerange (*vis)(Vis*, Text*, size_t pos);
	Filerange (*user)(Vis*, Win*, void *data, size_t pos);
	enum {
		TEXTOBJECT_DELIMITED_INNER = 1 << 0, /* single byte delimited, inner variant */
		TEXTOBJECT_DELIMITED_OUTER = 1 << 1, /* single byte delimited, outer variant */
		TEXTOBJECT_NON_CONTIGUOUS  = 1 << 2, /* multiple applications yield a split range */
		TEXTOBJECT_EXTEND_FORWARD  = 1 << 3, /* multiple applications extend towards the end of file (default) */
		TEXTOBJECT_EXTEND_BACKWARD = 1 << 4, /* multiple applications extend towards the begin of file */
	} type;
	void *data;
} TextObject;

/* a macro is just a sequence of symbolic keys as received from ui->getkey */
typedef Buffer Macro;
#define macro_init buffer_init
#define macro_release buffer_release
#define macro_reset buffer_clear
#define macro_append buffer_append0

typedef struct {             /** collects all information until an operator is executed */
	int count;
	enum VisMode mode;
	enum VisMotionType type;
	const Operator *op;
	const Movement *movement;
	const TextObject *textobj;
	const Macro *macro;
	Register *reg;
	enum VisMark mark;
	Arg arg;
} Action;

typedef struct Change Change;
typedef struct {
	Change *changes;      /* all changes in monotonically increasing file position */
	Change *latest;       /* most recent change */
	enum SamError error;  /* non-zero in case something went wrong */
} Transcript;

typedef struct {
	Array prev;
	Array next;
	size_t max;
} MarkList;

struct File { /* shared state among windows displaying the same file */
	Text *text;                      /* data structure holding the file content */
	const char *name;                /* file name used when loading/saving */
	volatile sig_atomic_t truncated; /* whether the underlying memory mapped region became invalid (SIGBUS) */
	int fd;                          /* output file descriptor associated with this file or -1 if loaded by file name */
	bool internal;                   /* whether it is an internal file (e.g. used for the prompt) */
	struct stat stat;                /* filesystem information when loaded/saved, used to detect changes outside the editor */
	int refcount;                    /* how many windows are displaying this file? (always >= 1) */
	Array marks[VIS_MARK_INVALID];   /* marks which are shared across windows */
	enum TextSaveMethod save_method; /* whether the file is saved using rename(2) or overwritten */
	Transcript transcript;           /* keeps track of changes performed by sam commands */
	File *next, *prev;
};

struct Win {
	Vis *vis;               /* editor instance to which this window belongs to */
	UiWin *ui;              /* ui object handling visual appearance of this window */
	File *file;             /* file being displayed in this window */
	View *view;             /* currently displayed part of underlying text */
	MarkList jumplist;      /* LRU jump management */
	Array saved_selections; /* register used to store selections */
	Mode modes[VIS_MODE_INVALID]; /* overlay mods used for per window key bindings */
	Win *parent;            /* window which was active when showing the command prompt */
	Mode *parent_mode;      /* mode which was active when showing the command prompt */
	Win *prev, *next;       /* neighbouring windows */
};

struct Vis {
	Ui *ui;                              /* user interface repsonsible for visual appearance */
	File *files;                         /* all files currently managed by this editor instance */
	File *command_file;                  /* special internal file used to store :-command prompt */
	File *search_file;                   /* special internal file used to store /,? search prompt */
	File *error_file;                    /* special internal file used to store lua error messages */
	Win *windows;                        /* all windows currently managed by this editor instance */
	Win *win;                            /* currently active/focused window */
	Win *message_window;                 /* special window to display multi line messages */
	Register registers[VIS_REG_INVALID]; /* registers used for text manipulations yank/put etc. and macros */
	Macro *recording, *last_recording;   /* currently (if non NULL) and least recently recorded macro */
	const Macro *replaying;              /* macro currently being replayed */
	Macro *macro_operator;               /* special macro used to repeat certain operators */
	Mode *mode_before_prompt;            /* user mode which was active before entering prompt */
	char search_char[8];                 /* last used character to search for via 'f', 'F', 't', 'T' */
	int last_totill;                     /* last to/till movement used for ';' and ',' */
	int search_direction;                /* used for `n` and `N` */
	int tabwidth;                        /* how many spaces should be used to display a tab */
	bool expandtab;                      /* whether typed tabs should be converted to spaces */
	bool autoindent;                     /* whether indentation should be copied from previous line on newline */
	bool change_colors;                  /* whether to adjust 256 color palette for true colors */
	char *shell;                         /* shell used to launch external commands */
	Map *cmds;                           /* ":"-commands, used for unique prefix queries */
	Map *usercmds;                       /* user registered ":"-commands */
	Map *options;                        /* ":set"-options */
	Map *keymap;                         /* key translation before any bindings are matched */
	bool keymap_disabled;                /* ignore key map for next key press, gets automatically re-enabled */
	char key[VIS_KEY_LENGTH_MAX];        /* last pressed key as reported from the UI */
	char key_current[VIS_KEY_LENGTH_MAX];/* current key being processed by the input queue */
	char key_prev[VIS_KEY_LENGTH_MAX];   /* previous key which was processed by the input queue */
	Buffer input_queue;                  /* holds pending input keys */
	bool errorhandler;                   /* whether we are currently in an error handler, used to avoid recursion */
	Action action;                       /* current action which is in progress */
	Action action_prev;                  /* last operator action used by the repeat (dot) command */
	Mode *mode;                          /* currently active mode, used to search for keybindings */
	Mode *mode_prev;                     /* previsouly active user mode */
	bool initialized;                    /* whether UI and Lua integration has been initialized */
	int nesting_level;                   /* parsing state to hold keep track of { } nesting level */
	volatile bool running;               /* exit main loop once this becomes false */
	int exit_status;                     /* exit status when terminating main loop */
	volatile sig_atomic_t interrupted;   /* abort command (SIGINT occured) */
	volatile sig_atomic_t sigbus;        /* one of the memory mapped region became unavailable (SIGBUS) */
	volatile sig_atomic_t need_resize;   /* need to resize UI (SIGWINCH occured) */
	volatile sig_atomic_t resume;        /* need to resume UI (SIGCONT occured) */
	volatile sig_atomic_t terminate;     /* need to terminate we were being killed by SIGTERM */
	sigjmp_buf sigbus_jmpbuf;            /* used to jump back to a known good state in the mainloop after (SIGBUS) */
	Map *actions;                        /* registered editor actions / special keys commands */
	Array actions_user;                  /* dynamically allocated editor actions */
	lua_State *lua;                      /* lua context used for syntax highligthing */
	enum TextLoadMethod load_method;     /* how existing files should be loaded */
	VisEvent *event;
	Array operators;
	Array motions;
	Array textobjects;
	Array bindings;
};

enum VisEvents {
	VIS_EVENT_INIT,
	VIS_EVENT_START,
	VIS_EVENT_QUIT,
	VIS_EVENT_FILE_OPEN,
	VIS_EVENT_FILE_SAVE_PRE,
	VIS_EVENT_FILE_SAVE_POST,
	VIS_EVENT_FILE_CLOSE,
	VIS_EVENT_WIN_OPEN,
	VIS_EVENT_WIN_CLOSE,
	VIS_EVENT_WIN_HIGHLIGHT,
	VIS_EVENT_WIN_STATUS,
};

bool vis_event_emit(Vis*, enum VisEvents, ...);

typedef struct {
	char name;
	VIS_HELP_DECL(const char *help;)
} MarkDef;

typedef MarkDef RegisterDef;

/** stuff used by multiple of the vis-* files */

extern Mode vis_modes[VIS_MODE_INVALID];
extern const Movement vis_motions[VIS_MOVE_INVALID];
extern const Operator vis_operators[VIS_OP_INVALID];
extern const TextObject vis_textobjects[VIS_TEXTOBJECT_INVALID];
extern const MarkDef vis_marks[VIS_MARK_a];
extern const RegisterDef vis_registers[VIS_REG_a];

void macro_operator_stop(Vis *vis);
void macro_operator_record(Vis *vis);

void vis_do(Vis *vis);
void action_reset(Action*);
size_t vis_text_insert_nl(Vis*, Text*, size_t pos);

Mode *mode_get(Vis*, enum VisMode);
void mode_set(Vis *vis, Mode *new_mode);
Macro *macro_get(Vis *vis, enum VisRegister);

void window_selection_save(Win *win);
Win *window_new_file(Vis*, File*, enum UiOption);

char *absolute_path(const char *path);

const char *file_name_get(File*);
void file_name_set(File*, const char *name);

bool register_init(Register*);
void register_release(Register*);

void mark_init(Array*);
void mark_release(Array*);

void marklist_init(MarkList*, size_t max);
void marklist_release(MarkList*);

const char *register_get(Vis*, Register*, size_t *len);
const char *register_slot_get(Vis*, Register*, size_t slot, size_t *len);

bool register_put0(Vis*, Register*, const char *data);
bool register_put(Vis*, Register*, const char *data, size_t len);
bool register_slot_put(Vis*, Register*, size_t slot, const char *data, size_t len);

bool register_put_range(Vis*, Register*, Text*, Filerange*);
bool register_slot_put_range(Vis*, Register*, size_t slot, Text*, Filerange*);

size_t vis_register_count(Vis*, Register*);
bool register_resize(Register*, size_t count);

#endif
