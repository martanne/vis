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
#include "register.h"

typedef struct {
	void (*vis_start)(Vis*);
	void (*vis_quit)(Vis*);
	void (*file_open)(Vis*, File*);
	void (*file_save)(Vis*, File*);
	void (*file_close)(Vis*, File*);
	void (*win_open)(Vis*, Win*);
	void (*win_close)(Vis*, Win*);
} VisEvent;

typedef union { /* various types of arguments passed to key action functions */
	bool b;
	int i;
	const char *s;
	void (*w)(View*);
	void (*f)(Vis*);
} Arg;

typedef struct {             /* a KeyAction can be bound to a key binding */
	const char *name;    /* aliases can refer to this action by means of a pseudo key <name> */
	const char *help;    /* short (one line) human readable description, displayed by :help */
	/* action handling function, keys refers to the next characters found in the input queue
	 * (an empty string "" indicates an empty queue). The return value of func has to point to
	 * the first non consumed key. Returning NULL indicates that not enough keys were available
	 * to complete the action. In this case the function will be called again when more input
	 * becomes available */
	const char* (*func)(Vis*, const char *keys, const Arg*);
	const Arg arg;       /* additional arguments which will be passed as to func */
} KeyAction;

typedef struct {           /* a key binding either refers to an action or an alias */
	const char *key;   /* symbolic key to trigger this binding */
	KeyAction *action; /* action to launch upon triggering this binding */
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

/* creates a new window, and loads the given file. if filename is NULL
 * an unamed / empty buffer is created. If the given file is already opened
 * in another window, share the underlying text that is changes will be
 * visible in both windows */
bool vis_window_new(Vis*, const char *filename);
/* reload the file currently displayed in the window from disk */
bool vis_window_reload(Win*);
/* close window, redraw user interface */
void vis_window_close(Win*);
/* split the given window. changes to the displayed text will be reflected
 * in both windows */
bool vis_window_split(Win*);
/* change file name associated with this window, affects syntax coloring */
void vis_window_name(Win*, const char *filename);
/* focus the next / previous window */
void vis_window_next(Vis*);
void vis_window_prev(Vis*);
/* display a user prompt with a certain title and default text */
void vis_prompt_show(Vis*, const char *title);

/* display a message to the user */
void vis_info_show(Vis*, const char *msg, ...);
void vis_info_hide(Vis*);

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
void vis_die(Vis*, const char *msg, ...) __attribute__((noreturn));

/* user facing modes are: NORMAL, VISUAL, VISUAL_LINE, INSERT, REPLACE.
 * the others should be considered as implementation details (TODO: do not expose them?) */
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
/* in the specified mode: map a given key to a binding (binding->key is ignored),
 * fails if key is already mapped */
bool vis_mode_map(Vis*, enum VisMode, const char *key, const KeyBinding*);
bool vis_window_mode_map(Win*, enum VisMode, const char *key, const KeyBinding*);
/* in the specified mode: unmap a given key, fails if the key is not currently mapped */
bool vis_mode_unmap(Vis*, enum VisMode, const char *key);
bool vis_window_mode_unmap(Win*, enum VisMode, const char *key);
/* get the current mode's status line indicator */
const char *vis_mode_status(Vis*);
/* associates the special pseudo key <keyaction->name> with the given key action.
 * after successfull registration the pseudo key can be used key binding aliases */
bool vis_action_register(Vis*, KeyAction*);

enum VisOperator {
	VIS_OP_DELETE,
	VIS_OP_CHANGE,
	VIS_OP_YANK,
	VIS_OP_PUT_AFTER,
	VIS_OP_SHIFT_RIGHT,
	VIS_OP_SHIFT_LEFT,
	VIS_OP_JOIN,
	VIS_OP_INSERT,
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
 */
bool vis_operator(Vis*, enum VisOperator, ...);

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
	VIS_MOVE_WORD_START_NEXT,
	VIS_MOVE_WORD_END_PREV,
	VIS_MOVE_WORD_END_NEXT,
	VIS_MOVE_WORD_START_PREV,
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
	VIS_MOVE_SEARCH_NEXT,
	VIS_MOVE_SEARCH_PREV,
	VIS_MOVE_WINDOW_LINE_TOP,
	VIS_MOVE_WINDOW_LINE_MIDDLE,
	VIS_MOVE_WINDOW_LINE_BOTTOM,
	VIS_MOVE_CHANGELIST_NEXT,
	VIS_MOVE_CHANGELIST_PREV,
	VIS_MOVE_JUMPLIST_NEXT,
	VIS_MOVE_JUMPLIST_PREV,
	VIS_MOVE_NOP,
	VIS_MOVE_INVALID, /* denotes the end of the "real" motions */
	/* pseudo motions: keep them at the end to save space in array definition */
	VIS_MOVE_TOTILL_REPEAT,
	VIS_MOVE_TOTILL_REVERSE,
	VIS_MOVE_SEARCH_FORWARD,
	VIS_MOVE_SEARCH_BACKWARD,
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

/* a count of zero indicates that so far no special count was given.
 * operators, motions and text object will always perform their function
 * as if a minimal count of 1 was given */
int vis_count_get(Vis*);
void vis_count_set(Vis*, int count);

enum VisMotionType {
	VIS_MOTIONTYPE_LINEWISE  = 1 << 0,
	VIS_MOTIONTYPE_CHARWISE  = 1 << 1,
};
/* force certain motion to behave in line or character wise mode */
void vis_motion_type(Vis *vis, enum VisMotionType);

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
	VIS_TEXTOBJECT_INVALID,
};

void vis_textobject(Vis*, enum VisTextObject);

/* macro REPEAT and INVALID should be considered as implementation details (TODO: hide them?) */
enum VisMacro {
	VIS_MACRO_a, VIS_MACRO_b, VIS_MACRO_c, VIS_MACRO_d, VIS_MACRO_e,
	VIS_MACRO_f, VIS_MACRO_g, VIS_MACRO_h, VIS_MACRO_i, VIS_MACRO_j,
	VIS_MACRO_k, VIS_MACRO_l, VIS_MACRO_m, VIS_MACRO_n, VIS_MACRO_o,
	VIS_MACRO_p, VIS_MACRO_q, VIS_MACRO_r, VIS_MACRO_s, VIS_MACRO_t,
	VIS_MACRO_u, VIS_MACRO_v, VIS_MACRO_w, VIS_MACRO_x, VIS_MACRO_y,
	VIS_MACRO_z,
	VIS_MACRO_OPERATOR,      /* records entered keys after an operator */
	VIS_MACRO_REPEAT,        /* copy of the above macro once the recording is finished */
	VIS_MACRO_INVALID,       /* denotes the end of "real" macros */
	VIS_MACRO_LAST_RECORDED, /* pseudo macro referring to last recorded one */
};

/* start a macro recording, fails if a recording is already on going */
bool vis_macro_record(Vis*, enum VisMacro);
/* stop recording, fails if there is nothing to stop */
bool vis_macro_record_stop(Vis*);
/* check whether a recording is currently on going */
bool vis_macro_recording(Vis*);
/* replay a macro. a macro currently being recorded can't be replayed */
bool vis_macro_replay(Vis*, enum VisMacro);

enum VisMark {
	VIS_MARK_a, VIS_MARK_b, VIS_MARK_c, VIS_MARK_d, VIS_MARK_e,
	VIS_MARK_f, VIS_MARK_g, VIS_MARK_h, VIS_MARK_i, VIS_MARK_j,
	VIS_MARK_k, VIS_MARK_l, VIS_MARK_m, VIS_MARK_n, VIS_MARK_o,
	VIS_MARK_p, VIS_MARK_q, VIS_MARK_r, VIS_MARK_s, VIS_MARK_t,
	VIS_MARK_u, VIS_MARK_v, VIS_MARK_w, VIS_MARK_x, VIS_MARK_y,
	VIS_MARK_z,
	VIS_MARK_SELECTION_START, /* '< */
	VIS_MARK_SELECTION_END,   /* '> */
	VIS_MARK_INVALID,     /* has to be the last enum member */
};

void vis_mark_set(Vis*, enum VisMark mark, size_t pos);

enum VisRegister {
	VIS_REG_a, VIS_REG_b, VIS_REG_c, VIS_REG_d, VIS_REG_e,
	VIS_REG_f, VIS_REG_g, VIS_REG_h, VIS_REG_i, VIS_REG_j,
	VIS_REG_k, VIS_REG_l, VIS_REG_m, VIS_REG_n, VIS_REG_o,
	VIS_REG_p, VIS_REG_q, VIS_REG_r, VIS_REG_s, VIS_REG_t,
	VIS_REG_u, VIS_REG_v, VIS_REG_w, VIS_REG_x, VIS_REG_y,
	VIS_REG_z,
	VIS_REG_DEFAULT, /* used when no other register is specified */
	VIS_REG_PROMPT,  /* internal register which shadows DEFAULT in PROMPT mode */
	VIS_REG_INVALID, /* has to be the last enum member */
};

/* set the register to use, if none is given the DEFAULT register is used */
void vis_register_set(Vis*, enum VisRegister);
Register *vis_register_get(Vis*, enum VisRegister);

/* repeat last operator, possibly with a new count if one was provided in the meantime */
void vis_repeat(Vis*);

/* cancel pending operator, reset count, motion, text object, register etc. */
void vis_cancel(Vis*);

/* execute a :-command (including an optinal range specifier) */
bool vis_cmd(Vis*, const char *cmd);

/* given the start of a key, returns a pointer to the start of the one immediately
 * following as will be processed by the input system. skips over special keys
 * such as <Enter> as well as pseudo keys registered via vis_action_register. */
const char *vis_keys_next(Vis*, const char *keys);
/* vis operates as a finite state machine (FSM), feeding keys from an input
 * queue (or a previously recorded macro) to key handling functions (see struct
 * KeyAction) which consume the input.
 *
 * this functions pushes/appends further input to the end of the input queue
 * and immediately tries to interpret them */
const char *vis_keys_push(Vis*, const char *input);
/* inject new input keys at the position indicated by a pointer to a valid, not
 * yet consumed key from the current input queue. does not try to interpret the
 * new queue content.
 *
 * typically only called from within key handling functions */
bool vis_keys_inject(Vis*, const char *pos, const char *input);
/* inform vis that a signal occured, the return value indicates whether the signal
 * was handled by vis */
bool vis_signal_handler(Vis*, int signum, const siginfo_t *siginfo, const void *context);

/* TODO: expose proper API to iterate through files etc */
Text *vis_text(Vis*);
View *vis_view(Vis*);
Text *vis_file_text(File*);
const char *vis_file_name(File*);

bool vis_theme_load(Vis*, const char *name);

#endif
