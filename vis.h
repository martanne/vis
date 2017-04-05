#ifndef VIS_H
#define VIS_H

#include <signal.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct Vis Vis;
typedef struct File File;
typedef struct Win Win;

#include "ui.h"
#include "view.h"
#include "text-regex.h"
#include "libutf.h"

#ifndef CONFIG_HELP
#define CONFIG_HELP 1
#endif

#if CONFIG_HELP
#define VIS_HELP_DECL(x) x
#define VIS_HELP_USE(x) x
#define VIS_HELP(x) (x),
#else
#define VIS_HELP_DECL(x)
#define VIS_HELP_USE(x) NULL
#define VIS_HELP(x)
#endif

/* simplify utility renames by distribution packagers */
#ifndef VIS_OPEN
#define VIS_OPEN "vis-open"
#endif
#ifndef VIS_CLIPBOARD
#define VIS_CLIPBOARD "vis-clipboard"
#endif

/* maximum bytes needed for string representation of a (pseudo) key */
#define VIS_KEY_LENGTH_MAX 64

typedef struct {
	void (*init)(Vis*);
	void (*start)(Vis*);
	void (*quit)(Vis*);
	void (*mode_insert_input)(Vis*, const char *key, size_t len);
	void (*mode_replace_input)(Vis*, const char *key, size_t len);
	void (*file_open)(Vis*, File*);
	bool (*file_save_pre)(Vis*, File*, const char *path);
	void (*file_save_post)(Vis*, File*, const char *path);
	void (*file_close)(Vis*, File*);
	void (*win_open)(Vis*, Win*);
	void (*win_close)(Vis*, Win*);
	void (*win_highlight)(Vis*, Win*);
	void (*win_status)(Vis*, Win*);
} VisEvent;

typedef union { /* various types of arguments passed to key action functions */
	bool b;
	int i;
	const char *s;
	const void *v;
	void (*w)(View*);
	void (*f)(Vis*);
} Arg;

/* action handling function, keys refers to the next characters found in the input queue
 * (an empty string "" indicates an empty queue). The return value of func has to point to
 * the first non consumed key. Returning NULL indicates that not enough keys were available
 * to complete the action. In this case the function will be called again when more input
 * becomes available */
typedef const char *KeyActionFunction(Vis*, const char *keys, const Arg*);

typedef struct {             /* a KeyAction can be bound to a key binding */
	const char *name;    /* aliases can refer to this action by means of a pseudo key <name> */
	VIS_HELP_DECL(const char *help;)    /* short (one line) human readable description, displayed by :help */
	KeyActionFunction *func; /* action implementation */
	Arg arg;       /* additional arguments which will be passed as to func */
} KeyAction;

typedef struct {           /* a key binding either refers to an action or an alias */
	const char *key;   /* symbolic key to trigger this binding */
	const KeyAction *action; /* action to launch upon triggering this binding */
	const char *alias; /* replaces key with alias in the input queue */
} KeyBinding;

/* creates a new editor instance using the specified user interface */
Vis *vis_new(Ui*, VisEvent*);
/* frees all resources associated with this editor instance, terminates ui */
void vis_free(Vis*);
/* instructs the user interface to draw to an internal buffer */
void vis_draw(Vis*);
void vis_redraw(Vis*);
/* flushes the state of the internal buffer to the output device */
void vis_update(Vis*);
/* temporarily supsend the editor process, resumes upon receiving SIGCONT */
void vis_suspend(Vis*);
void vis_resume(Vis*);

/* creates a new window, and loads the given file. if filename is NULL
 * an unamed / empty buffer is created. If the given file is already opened
 * in another window, share the underlying text that is changes will be
 * visible in both windows */
Win *vis_window_new(Vis*, const char *filename);
/* Creates a new window and underlying file object associated with the
 * given output file descriptor. No data is read from `fd`, but write
 * commands without an explicit filename will instead write to the file
 * descriptor */
bool vis_window_new_fd(Vis*, int fd);
/* reload the file currently displayed in the window from disk */
bool vis_window_reload(Win*);
/* check whether closing the window would loose unsaved changes */
bool vis_window_closable(Win*);
/* close window, redraw user interface */
void vis_window_close(Win*);
/* split the given window. changes to the displayed text will be reflected
 * in both windows */
bool vis_window_split(Win*);
/* change status message of this window */
void vis_window_status(Win*, const char *status);
void vis_window_draw(Win*);
void vis_window_invalidate(Win*);
/* focus the next / previous window */
void vis_window_next(Vis*);
void vis_window_prev(Vis*);
/* change currently focused window, receiving user input */
void vis_window_focus(Win*);
/* swap location of two windows */
void vis_window_swap(Win*, Win*);

int vis_window_width_get(const Win*);
int vis_window_height_get(const Win*);

/* take an undo snaphost to which we can later revert to */
void vis_file_snapshot(Vis*, File*);

/* display a user prompt with a certain title and default text */
void vis_prompt_show(Vis*, const char *title);

/* display a one line message to the user, will be hidden upon keypress */
void vis_info_show(Vis*, const char *msg, ...) __attribute__((format(printf, 2, 3)));
void vis_info_hide(Vis*);

/* display an arbitrary long message in a special window/file */
void vis_message_show(Vis*, const char *msg);
void vis_message_hide(Vis*);

/* these function operate on the currently focused window but make sure
 * that all windows which show the affected region are redrawn too. */
void vis_insert(Vis*, size_t pos, const char *data, size_t len);
void vis_delete(Vis*, size_t pos, size_t len);
void vis_replace(Vis*, size_t pos, const char *data, size_t len);
/* these functions perform their operation at the current cursor position(s) */
void vis_insert_key(Vis*, const char *data, size_t len);
void vis_replace_key(Vis*, const char *data, size_t len);
/* inserts a tab (peforms tab expansion based on current editing settings),
 * at all current cursor positions */
void vis_insert_tab(Vis*);
/* inserts a new line sequence (depending on the file type this might be \n or
 * \r\n) at all current cursor positions */
void vis_insert_nl(Vis*);

/* processes the given command line arguments and starts the main loop, won't
 * return until editing session is terminated */
int vis_run(Vis*, int argc, char *argv[]);
/* terminate editing session, given status will be the return value of vis_run */
void vis_exit(Vis*, int status);
/* emergency exit, print given message, perform minimal ui cleanup and exit process */
void vis_die(Vis*, const char *msg, ...) __attribute__((noreturn,format(printf, 2, 3)));

KeyBinding *vis_binding_new(Vis*);
void vis_binding_free(Vis*, KeyBinding*);

enum VisMode {
	VIS_MODE_NORMAL,
	VIS_MODE_OPERATOR_PENDING,
	VIS_MODE_VISUAL,
	VIS_MODE_VISUAL_LINE,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
	VIS_MODE_INVALID,
};

void vis_mode_switch(Vis*, enum VisMode);
enum VisMode vis_mode_get(Vis*);
enum VisMode vis_mode_from(Vis*, const char *name);
/* In the specified mode: map a given key to a binding (binding->key is ignored).
 * Fails if a prefix of `key' is already mapped and `force' is false. Otherwise
 * all such prefixes are unmapped. */
bool vis_mode_map(Vis*, enum VisMode, bool force, const char *key, const KeyBinding*);
bool vis_window_mode_map(Win*, enum VisMode, bool force, const char *key, const KeyBinding*);
/* in the specified mode: unmap a given key, fails if the key is not currently mapped */
bool vis_mode_unmap(Vis*, enum VisMode, const char *key);
bool vis_window_mode_unmap(Win*, enum VisMode, const char *key);

KeyAction *vis_action_new(Vis*, const char *name, const char *help, KeyActionFunction*, Arg);
void vis_action_free(Vis*, KeyAction*);
/* associates the special pseudo key <keyaction->name> with the given key action.
 * after successfull registration the pseudo key can be used key binding aliases */
bool vis_action_register(Vis*, const KeyAction*);
/* add a key mapping which is applied for all modes except insert/replace
 * before any key bindings are evaluated */
bool vis_keymap_add(Vis*, const char *key, const char *mapping);
/* disable the keymap for the next key press */
void vis_keymap_disable(Vis*);

enum VisOperator {
	VIS_OP_DELETE,
	VIS_OP_CHANGE,
	VIS_OP_YANK,
	VIS_OP_PUT_AFTER,
	VIS_OP_SHIFT_RIGHT,
	VIS_OP_SHIFT_LEFT,
	VIS_OP_JOIN,
	VIS_OP_MODESWITCH,
	VIS_OP_REPLACE,
	VIS_OP_CURSOR_SOL,
	VIS_OP_CASE_SWAP,
	VIS_OP_FILTER,
	VIS_OP_INVALID, /* denotes the end of the "real" operators */
	/* pseudo operators: keep them at the end to save space in array definition */
	VIS_OP_CASE_LOWER,
	VIS_OP_CASE_UPPER,
	VIS_OP_CURSOR_EOL,
	VIS_OP_PUT_AFTER_END,
	VIS_OP_PUT_BEFORE,
	VIS_OP_PUT_BEFORE_END,
	VIS_OP_LAST, /* has to be last enum member */
};

/* set operator to execute, has immediate effect if
 *  - a visual mode is active
 *  - the same operator was already set (range will be the current line)
 * otherwise waits until a range is determinded i.e.
 *  - a motion is provided (see vis_motion)
 *  - a text object is provided (vis_textobject)
 *
 * the expected varying arguments are as follows:
 *
 *  - VIS_OP_FILTER     a char pointer referring to the command to run
 *  - VIS_OP_JOIN       a char pointer referring to the text to insert between lines
 *  - VIS_OP_MODESWITCH a enum VisMode constant indicating the mode to switch to
 *  - VIS_OP_REPLACE    a char pointer reffering to the replacement character
 */
bool vis_operator(Vis*, enum VisOperator, ...);

typedef struct OperatorContext OperatorContext;
typedef size_t (VisOperatorFunction)(Vis*, Text*, OperatorContext*);
int vis_operator_register(Vis*, VisOperatorFunction*, void *context);

enum VisMotion {
	VIS_MOVE_LINE_DOWN,
	VIS_MOVE_LINE_UP,
	VIS_MOVE_SCREEN_LINE_UP,
	VIS_MOVE_SCREEN_LINE_DOWN,
	VIS_MOVE_SCREEN_LINE_BEGIN,
	VIS_MOVE_SCREEN_LINE_MIDDLE,
	VIS_MOVE_SCREEN_LINE_END,
	VIS_MOVE_LINE_PREV,
	VIS_MOVE_LINE_BEGIN,
	VIS_MOVE_LINE_START,
	VIS_MOVE_LINE_FINISH,
	VIS_MOVE_LINE_LASTCHAR,
	VIS_MOVE_LINE_END,
	VIS_MOVE_LINE_NEXT,
	VIS_MOVE_LINE,
	VIS_MOVE_COLUMN,
	VIS_MOVE_CHAR_PREV,
	VIS_MOVE_CHAR_NEXT,
	VIS_MOVE_LINE_CHAR_PREV,
	VIS_MOVE_LINE_CHAR_NEXT,
	VIS_MOVE_CODEPOINT_PREV,
	VIS_MOVE_CODEPOINT_NEXT,
	VIS_MOVE_WORD_NEXT,
	VIS_MOVE_WORD_START_NEXT,
	VIS_MOVE_WORD_END_PREV,
	VIS_MOVE_WORD_END_NEXT,
	VIS_MOVE_WORD_START_PREV,
	VIS_MOVE_LONGWORD_NEXT,
	VIS_MOVE_LONGWORD_START_PREV,
	VIS_MOVE_LONGWORD_START_NEXT,
	VIS_MOVE_LONGWORD_END_PREV,
	VIS_MOVE_LONGWORD_END_NEXT,
	VIS_MOVE_SENTENCE_PREV,
	VIS_MOVE_SENTENCE_NEXT,
	VIS_MOVE_PARAGRAPH_PREV,
	VIS_MOVE_PARAGRAPH_NEXT,
	VIS_MOVE_FUNCTION_START_PREV,
	VIS_MOVE_FUNCTION_START_NEXT,
	VIS_MOVE_FUNCTION_END_PREV,
	VIS_MOVE_FUNCTION_END_NEXT,
	VIS_MOVE_BLOCK_START,
	VIS_MOVE_BLOCK_END,
	VIS_MOVE_PARENTHESE_START,
	VIS_MOVE_PARENTHESE_END,
	VIS_MOVE_BRACKET_MATCH,
	VIS_MOVE_LEFT_TO,
	VIS_MOVE_RIGHT_TO,
	VIS_MOVE_LEFT_TILL,
	VIS_MOVE_RIGHT_TILL,
	VIS_MOVE_FILE_BEGIN,
	VIS_MOVE_FILE_END,
	VIS_MOVE_MARK,
	VIS_MOVE_MARK_LINE,
	VIS_MOVE_SEARCH_WORD_FORWARD,
	VIS_MOVE_SEARCH_WORD_BACKWARD,
	VIS_MOVE_SEARCH_REPEAT_FORWARD,
	VIS_MOVE_SEARCH_REPEAT_BACKWARD,
	VIS_MOVE_WINDOW_LINE_TOP,
	VIS_MOVE_WINDOW_LINE_MIDDLE,
	VIS_MOVE_WINDOW_LINE_BOTTOM,
	VIS_MOVE_CHANGELIST_NEXT,
	VIS_MOVE_CHANGELIST_PREV,
	VIS_MOVE_JUMPLIST_NEXT,
	VIS_MOVE_JUMPLIST_PREV,
	VIS_MOVE_NOP,
	VIS_MOVE_PERCENT,
	VIS_MOVE_BYTE,
	VIS_MOVE_BYTE_LEFT,
	VIS_MOVE_BYTE_RIGHT,
	VIS_MOVE_INVALID, /* denotes the end of the "real" motions */
	/* pseudo motions: keep them at the end to save space in array definition */
	VIS_MOVE_TOTILL_REPEAT,
	VIS_MOVE_TOTILL_REVERSE,
	VIS_MOVE_SEARCH_FORWARD,
	VIS_MOVE_SEARCH_BACKWARD,
	VIS_MOVE_SEARCH_REPEAT,
	VIS_MOVE_SEARCH_REPEAT_REVERSE,
	VIS_MOVE_LAST, /* denotes the end of all motions */
};

/* set motion to perform, the following take an additional argument:
 *
 *  - VIS_MOVE_SEARCH_FORWARD and VIS_MOVE_SEARCH_BACKWARD
 *
 *     expect the search pattern as const char *
 *
 *  - VIS_MOVE_{LEFT,RIGHT}_{TO,TILL}
 *
 *     expect the character to search for as const char *
 *
 *  - VIS_MOVE_MARK and VIS_MOVE_MARK_LINE
 *
 *     expect a valid enum VisMark
 */
bool vis_motion(Vis*, enum VisMotion, ...);

/* If no count is explicitly specified, operators, motions and
 * text object will always perform their function as if a minimal
 * count of 1 was given */
#define VIS_COUNT_UNKNOWN (-1)
int vis_count_get(Vis*);
int vis_count_get_default(Vis*, int def);
void vis_count_set(Vis*, int count);

enum VisMotionType {
	VIS_MOTIONTYPE_LINEWISE  = 1 << 0,
	VIS_MOTIONTYPE_CHARWISE  = 1 << 1,
};
/* force certain motion to behave in line or character wise mode */
void vis_motion_type(Vis *vis, enum VisMotionType);

/* register a motion function, if positive the return value can be used
 * as an id for the vis_motion funntion. A negative return value indicates
 * an error */
int vis_motion_register(Vis*, enum VisMotionType, void *data,
	size_t (*motion)(Vis*, Win*, void*, size_t pos));

enum VisTextObject {
	VIS_TEXTOBJECT_INNER_WORD,
	VIS_TEXTOBJECT_OUTER_WORD,
	VIS_TEXTOBJECT_INNER_LONGWORD,
	VIS_TEXTOBJECT_OUTER_LONGWORD,
	VIS_TEXTOBJECT_SENTENCE,
	VIS_TEXTOBJECT_PARAGRAPH,
	VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET,
	VIS_TEXTOBJECT_INNER_SQUARE_BRACKET,
	VIS_TEXTOBJECT_OUTER_CURLY_BRACKET,
	VIS_TEXTOBJECT_INNER_CURLY_BRACKET,
	VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET,
	VIS_TEXTOBJECT_INNER_ANGLE_BRACKET,
	VIS_TEXTOBJECT_OUTER_PARANTHESE,
	VIS_TEXTOBJECT_INNER_PARANTHESE,
	VIS_TEXTOBJECT_OUTER_QUOTE,
	VIS_TEXTOBJECT_INNER_QUOTE,
	VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE,
	VIS_TEXTOBJECT_INNER_SINGLE_QUOTE,
	VIS_TEXTOBJECT_OUTER_BACKTICK,
	VIS_TEXTOBJECT_INNER_BACKTICK,
	VIS_TEXTOBJECT_OUTER_ENTIRE,
	VIS_TEXTOBJECT_INNER_ENTIRE,
	VIS_TEXTOBJECT_OUTER_FUNCTION,
	VIS_TEXTOBJECT_INNER_FUNCTION,
	VIS_TEXTOBJECT_OUTER_LINE,
	VIS_TEXTOBJECT_INNER_LINE,
	VIS_TEXTOBJECT_INDENTATION,
	VIS_TEXTOBJECT_SEARCH_FORWARD,
	VIS_TEXTOBJECT_SEARCH_BACKWARD,
	VIS_TEXTOBJECT_INVALID,
};

bool vis_textobject(Vis*, enum VisTextObject);

/* register a new text object, if successful the returned id is positive
 * and can be used as argument for the vis_textobject function. */
int vis_textobject_register(Vis*, int type, void *data,
	Filerange (*textobject)(Vis*, Win*, void*, size_t pos));


enum VisMark {
	VIS_MARK_SELECTION_START, /* '< */
	VIS_MARK_SELECTION_END,   /* '> */
	VIS_MARK_a, VIS_MARK_b, VIS_MARK_c, VIS_MARK_d, VIS_MARK_e,
	VIS_MARK_f, VIS_MARK_g, VIS_MARK_h, VIS_MARK_i, VIS_MARK_j,
	VIS_MARK_k, VIS_MARK_l, VIS_MARK_m, VIS_MARK_n, VIS_MARK_o,
	VIS_MARK_p, VIS_MARK_q, VIS_MARK_r, VIS_MARK_s, VIS_MARK_t,
	VIS_MARK_u, VIS_MARK_v, VIS_MARK_w, VIS_MARK_x, VIS_MARK_y,
	VIS_MARK_z,
	VIS_MARK_INVALID,     /* has to be the last enum member */
};

enum VisMark vis_mark_from(Vis*, char mark);
void vis_mark_set(Vis*, enum VisMark mark, size_t pos);

enum VisRegister {
	VIS_REG_DEFAULT,    /* used when no other register is specified */
	VIS_REG_ZERO,       /* yank register */
	VIS_REG_AMPERSAND,  /* last regex match */
	VIS_REG_1,	    /* 1-9 last sub-expression matches */
	VIS_REG_2,
	VIS_REG_3,
	VIS_REG_4,
	VIS_REG_5,
	VIS_REG_6,
	VIS_REG_7,
	VIS_REG_8,
	VIS_REG_9,
	VIS_REG_BLACKHOLE,  /* /dev/null register */
	VIS_REG_CLIPBOARD,  /* system clipboard register */
	VIS_REG_DOT,        /* last inserted text, copy of VIS_MACRO_OPERATOR */
	VIS_REG_SEARCH,     /* last used search pattern "/ */
	VIS_REG_COMMAND,    /* last used :-command ": */
	VIS_REG_SHELL,      /* last used shell command given to either <, >, |, or ! */
	VIS_REG_a, VIS_REG_b, VIS_REG_c, VIS_REG_d, VIS_REG_e,
	VIS_REG_f, VIS_REG_g, VIS_REG_h, VIS_REG_i, VIS_REG_j,
	VIS_REG_k, VIS_REG_l, VIS_REG_m, VIS_REG_n, VIS_REG_o,
	VIS_REG_p, VIS_REG_q, VIS_REG_r, VIS_REG_s, VIS_REG_t,
	VIS_REG_u, VIS_REG_v, VIS_REG_w, VIS_REG_x, VIS_REG_y,
	VIS_REG_z,
	VIS_MACRO_OPERATOR, /* records entered keys after an operator */
	VIS_REG_PROMPT, /* internal register which shadows DEFAULT in PROMPT mode */
	VIS_REG_INVALID, /* has to be the last 'real' register */
	VIS_REG_A, VIS_REG_B, VIS_REG_C, VIS_REG_D, VIS_REG_E,
	VIS_REG_F, VIS_REG_G, VIS_REG_H, VIS_REG_I, VIS_REG_J,
	VIS_REG_K, VIS_REG_L, VIS_REG_M, VIS_REG_N, VIS_REG_O,
	VIS_REG_P, VIS_REG_Q, VIS_REG_R, VIS_REG_S, VIS_REG_T,
	VIS_REG_U, VIS_REG_V, VIS_REG_W, VIS_REG_X, VIS_REG_Y,
	VIS_REG_Z,
	VIS_MACRO_LAST_RECORDED, /* pseudo macro referring to last recorded one */
};

enum VisRegister vis_register_from(Vis*, char reg);
/* set the register to use, if none is given the DEFAULT register is used */
void vis_register_set(Vis*, enum VisRegister);
/* get register content */
const char *vis_register_get(Vis*, enum VisRegister, size_t *len);

/* start a macro recording, fails if a recording is already on going */
bool vis_macro_record(Vis*, enum VisRegister);
/* stop recording, fails if there is nothing to stop */
bool vis_macro_record_stop(Vis*);
/* check whether a recording is currently on going */
bool vis_macro_recording(Vis*);
/* replay a macro. a macro currently being recorded can't be replayed */
bool vis_macro_replay(Vis*, enum VisRegister);

/* repeat last operator, possibly with a new count if one was provided in the meantime */
void vis_repeat(Vis*);

/* cancel pending operator, reset count, motion, text object, register etc. */
void vis_cancel(Vis*);

/* execute a :-command (including an optinal range specifier) */
bool vis_cmd(Vis*, const char *cmd);

/* type of user defined function which can be registered */
typedef bool (CmdFunc)(Vis*, Win*, void *data, bool force,
	const char *argv[], Cursor*, Filerange*);
/* the function will be invoked whenever a command which matches a
 * unique prefix of the given name is executed */
bool vis_cmd_register(Vis*, const char *name, const char *help, void *data, CmdFunc*);
bool vis_cmd_unregister(Vis*, const char *name);

enum VisOption {
	VIS_OPTION_TYPE_BOOL = 1 << 0,
	VIS_OPTION_TYPE_STRING = 1 << 1,
	VIS_OPTION_TYPE_NUMBER = 1 << 2,
	VIS_OPTION_VALUE_OPTIONAL = 1 << 3,
	VIS_OPTION_NEED_WINDOW = 1 << 4,
};

typedef bool (VisOptionFunction)(Vis*, Win*, void *context, bool toggle,
                                 enum VisOption, const char *name, Arg *value);
bool vis_option_register(Vis*, const char *names[], enum VisOption,
                         VisOptionFunction*, void *context, const char *help);
bool vis_option_unregister(Vis*, const char *name);

/* execute any kind (:,?,/) of prompt command */
bool vis_prompt_cmd(Vis*, const char *cmd);

/* pipe a given file range to an external process
 *
 * if the range is invalid 'interactive' mode is enabled, meaning that
 * stdin and stderr are passed through the underlying command, stdout
 * points to vis' stderr.
 *
 * if argv contains only one non-NULL element the command is executed using
 * /bin/sh -c (i.e. argument expansion is performed by the shell). In contrast
 * if argv contains more than one non-NULL element execvp(argv[0], argv); will
 * be used.
 *
 * if read_std{out,err} are non-NULL they will be called when output from
 * the forked process is available.
 */
int vis_pipe(Vis*, File*, Filerange*, const char *argv[],
	void *stdout_context, ssize_t (*read_stdout)(void *stdout_context, char *data, size_t len),
	void *stderr_context, ssize_t (*read_stderr)(void *stderr_context, char *data, size_t len));

/* pipe a range to an external application, return its exit status and store
 * everything that is written to stdout/stderr in the gitven char pointers
 * which have to be free(3)-ed by the caller */
int vis_pipe_collect(Vis*, File*, Filerange*, const char *argv[], char **out, char **err);

/* given the start of a key, returns a pointer to the start of the one immediately
 * following as will be processed by the input system. skips over special keys
 * such as <Enter> as well as pseudo keys registered via vis_action_register. */
const char *vis_keys_next(Vis*, const char *keys);
/* Tries to convert next symbolic key to a raw code point, returns -1 for unknown keys */
long vis_keys_codepoint(Vis*, const char *keys);
/* Tries to convert next symbolic key to a UTF-8 sequence. Returns false for unknown keys
 * and leaves `utf8` untouched. Guarantees that `utf8` is NUL terminated on success */
bool vis_keys_utf8(Vis*, const char *keys, char utf8[static UTFmax+1]);
/* vis operates as a finite state machine (FSM), feeding keys from an input
 * queue (or a previously recorded macro) to key handling functions (see struct
 * KeyAction) which consume the input.
 *
 * this functions pushes/appends further input to the end of the input queue
 * and immediately interprets them as if they were entered by a user. */
void vis_keys_feed(Vis*, const char *keys);

/* inform vis that a signal occured, the return value indicates whether the signal
 * was handled by vis */
bool vis_signal_handler(Vis*, int signum, const siginfo_t *siginfo, const void *context);

/* remember last search pattern, freeing the regex is the callers responsibility */
Regex *vis_regex(Vis*, const char *pattern);

/* TODO: expose proper API to iterate through files etc */
Text *vis_text(Vis*);
View *vis_view(Vis*);
Win *vis_window(Vis*);

/* Get value of autoindent */
bool vis_get_autoindent(const Vis*);

#endif
