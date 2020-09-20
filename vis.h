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
#include "array.h"

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

/**
 * Editor event handlers.
 */
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
	void (*term_csi)(Vis*, const long *);
} VisEvent;

/** Union used to pass arguments to key action functions. */
typedef union {
	bool b;
	int i;
	const char *s;
	const void *v;
	void (*w)(View*);
	void (*f)(Vis*);
	Filerange (*combine)(const Filerange*, const Filerange*);
} Arg;

/**
 * Key action handling function.
 * @param keys Input queue content *after* the binding which invoked this function.
 * @rst
 * .. note:: An empty string ``""`` indicates that no further input is available.
 * @endrst
 * @return Pointer to first non-cosumed key.
 * @rst
 * .. warning:: Must be in range ``[keys, keys+strlen(keys)]`` or ``NULL`` to
 *              indicate that not enough input was available. In the latter case
 *              the function will be called again once more input has been received.
 * @endrst
 * @ingroup vis_action
 */
typedef const char *KeyActionFunction(Vis*, const char *keys, const Arg*);

/** Key action definition. */
typedef struct {
	const char *name;                /**< Name of a pseudo key ``<name>`` which can be used in mappings. */
	VIS_HELP_DECL(const char *help;) /**< One line human readable description, displayed by ``:help``. */
	KeyActionFunction *func;         /**< Key action implementation function. */
	Arg arg;                         /**< Options passes as last argument to ``func``. */
} KeyAction;

/**
 * A key binding, refers to an action or an alias
 * @rst
 * .. note:: Either ``action`` or ``alias`` must be ``NULL``.
 * @endrst
 */
typedef struct {
	const char *key;         /**< Symbolic key to trigger this binding. */
	const KeyAction *action; /**< Action to invoke when triggering this binding. */
	const char *alias;       /**< Replaces ``key`` with ``alias`` at the front of the input queue. */
} KeyBinding;

/**
 * @defgroup vis_lifecycle
 * @{
 */
/** Create a new editor instance using the given user interface and event handlers. */
Vis *vis_new(Ui*, VisEvent*);
/** Free all resources associated with this editor instance, terminates UI. */
void vis_free(Vis*);
/**
 * Enter main loop, start processing user input. 
 * @return The editor exit status code.
 */
int vis_run(Vis*);
/** Terminate editing session, the given ``status`` will be the return value of `vis_run`. */
void vis_exit(Vis*, int status);
/**
 * Emergency exit, print given message, perform minimal UI cleanup and exit process.
 * @rst
 * .. note:: This function does not return.
 * @endrst
 */
void vis_die(Vis*, const char *msg, ...) __attribute__((noreturn,format(printf, 2, 3)));

/**
 * Temporarily supsend the editor process.
 * @rst
 * .. note:: This function will generate a ``SIGTSTP`` signal.
 * @endrst
 **/
void vis_suspend(Vis*);
/**
 * Resume editor process.
 * @rst
 * .. note:: This function is usually called in response to a ``SIGCONT`` signal.
 * @endrst
 */
void vis_resume(Vis*);
/**
 * Inform the editor core that a signal occured.
 * @return Whether the signal was handled.
 * @rst
 * .. note:: Being designed as a library the editor core does *not* register any
 *           signal handlers on its own.
 * .. note:: The remaining arguments match the prototype of ``sa_sigaction`` as
 *           specified in `sigaction(2)`.
 * @endrst
 */
bool vis_signal_handler(Vis*, int signum, const siginfo_t *siginfo, const void *context);
/**
 * Interrupt long running operation.
 * @rst
 * .. warning:: There is no guarantee that a long running operation is actually
 *              interrupted. It is analogous to cooperative multitasking where
 *              the operation has to voluntarily yield control.
 * .. note:: It is invoked from `vis_signal_handler` when receiving ``SIGINT``.
 * @endrst
 */
void vis_interrupt(Vis*);
/** Check whether interruption was requested. */
bool vis_interrupt_requested(Vis*);
/**
 * @}
 * @defgroup vis_draw
 * @{
 */
/** Draw user interface. */
void vis_draw(Vis*);
/** Completely redraw user interface. */
void vis_redraw(Vis*);
/** Blit user interface state to output device. */
void vis_update(Vis*);
/**
 * @}
 * @defgroup vis_windows
 * @{
 */
/**
 * Create a new window and load the given file.
 * @param filename If ``NULL`` a unamed, empty buffer is created.
 * @rst
 * .. note:: If the given file name is already opened in another window,
 *           the underlying File object is shared.
 * .. warning:: This duplication detection is currently based on normalized,
 *              absolute file names. TODO: compare inodes instead.
 * @endrst
 */
bool vis_window_new(Vis*, const char *filename);
/**
 * Create a new window associated with a file descriptor.
 * @rst
 * .. note:: No data is read from `fd`, but write commands without an
 *           explicit filename will instead write to the file descriptor.
 * @endrst
 */
bool vis_window_new_fd(Vis*, int fd);
/** Reload the file currently displayed in the window from disk. */
bool vis_window_reload(Win*);
/** Check whether closing the window would loose unsaved changes. */
bool vis_window_closable(Win*);
/** Close window, redraw user interface. */
void vis_window_close(Win*);
/** Split the window, shares the underlying file object. */
bool vis_window_split(Win*);
/** Change status message of this window. */
void vis_window_status(Win*, const char *status);
void vis_window_draw(Win*);
void vis_window_invalidate(Win*);
/** Focus next window. */
void vis_window_next(Vis*);
/** Focus previous window. */
void vis_window_prev(Vis*);
/** Change currently focused window, receiving user input. */
void vis_window_focus(Win*);
/** Swap location of two windows. */
void vis_window_swap(Win*, Win*);
/** Query window dimension. */
int vis_window_width_get(const Win*);
/** Query window dimension. */
int vis_window_height_get(const Win*);
/**
 * @}
 * @defgroup vis_info
 * @{
 */
/**
 * Display a user prompt with a certain title.
 * @rst
 * .. note:: The prompt is currently implemented as a single line height window.
 * @endrst
 */
void vis_prompt_show(Vis*, const char *title);

/**
 * Display a single line message.
 * @rst
 * .. note:: The message will automatically be hidden upon next input.
 * @endrst
 */
void vis_info_show(Vis*, const char *msg, ...) __attribute__((format(printf, 2, 3)));
/** Hide informational message. */
void vis_info_hide(Vis*);

/** Display arbitrary long message in a dedicated window. */
void vis_message_show(Vis*, const char *msg);
/** Close message window. */
void vis_message_hide(Vis*);
/**
 * @}
 * @defgroup vis_changes
 * @{
 */
void vis_insert(Vis*, size_t pos, const char *data, size_t len);
void vis_delete(Vis*, size_t pos, size_t len);
void vis_replace(Vis*, size_t pos, const char *data, size_t len);
/** Perform insertion at all cursor positions. */
void vis_insert_key(Vis*, const char *data, size_t len);
/**
 * Perform character subsitution at all cursor positions.
 * @rst
 * .. note:: Does not replace new line characters.
 * @endrst
 */
void vis_replace_key(Vis*, const char *data, size_t len);
/**
 * Insert a tab at all cursor positions.
 * @rst
 * .. note:: Performs tab expansion according to current settings.
 * @endrst
 */
void vis_insert_tab(Vis*);
/**
 * Inserts a new line character at every cursor position.
 * @rst
 * .. note:: Performs auto indentation according to current settings.
 * @endrst
 */
void vis_insert_nl(Vis*);

/** @} */
/** Mode specifiers. */
enum VisMode {
	VIS_MODE_NORMAL,
	VIS_MODE_OPERATOR_PENDING,
	VIS_MODE_VISUAL,
	VIS_MODE_VISUAL_LINE, /**< Sub mode of `VIS_MODE_VISUAL`. */
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE, /**< Sub mode of `VIS_MODE_INSERT`. */
	VIS_MODE_INVALID,
};

/**
 * @defgroup vis_modes
 * @{
 */
/**
 * Switch mode.
 * @rst
 * .. note:: Will first trigger the leave event of the currently active
 *           mode, followed by an enter event of the new mode.
 *           No events are emitted, if the specified mode is already active.
 * @endrst
 */
void vis_mode_switch(Vis*, enum VisMode);
/** Get currently active mode. */
enum VisMode vis_mode_get(Vis*);
/** Translate human readable mode name to constant. */
enum VisMode vis_mode_from(Vis*, const char *name);

/**
 * @}
 * @defgroup vis_keybind
 * @{
 */
KeyBinding *vis_binding_new(Vis*);
void vis_binding_free(Vis*, KeyBinding*);

/**
 * Set up a key binding.
 * @param force Whether an existing mapping should be discarded.
 * @param key The symbolic key to map.
 * @param binding The binding to map.
 * @rst
 * .. note:: ``binding->key`` is always ignored in favor of ``key``.
 * @endrst
 */
bool vis_mode_map(Vis*, enum VisMode, bool force, const char *key, const KeyBinding*);
/** Analogous to `vis_mode_map`, but window specific. */
bool vis_window_mode_map(Win*, enum VisMode, bool force, const char *key, const KeyBinding*);
/** Unmap a symbolic key in a given mode. */
bool vis_mode_unmap(Vis*, enum VisMode, const char *key);
/** Analogous to `vis_mode_unmap`, but window specific. */
bool vis_window_mode_unmap(Win*, enum VisMode, const char *key);
/**
 * @}
 * @defgroup vis_action
 * @{
 */
/**
 * Create new key action.
 * @param name The name to be used as symbolic key when registering.
 * @param help Optional single line help text.
 * @param func The function implementing the key action logic.
 * @param arg Argument passed to function.
 */
KeyAction *vis_action_new(Vis*, const char *name, const char *help, KeyActionFunction*, Arg);
void vis_action_free(Vis*, KeyAction*);
/**
 * Register key action.
 * @rst
 * .. note:: Makes the key action available under the pseudo key name specified
 *           in ``keyaction->name``.
 * @endrst
 */
bool vis_action_register(Vis*, const KeyAction*);

/**
 * @}
 * @defgroup vis_keymap
 * @{
 */

/** Add a key translation. */
bool vis_keymap_add(Vis*, const char *key, const char *mapping);
/** Temporarily disable the keymap for the next key press. */
void vis_keymap_disable(Vis*);

/** @} */
/** Operator specifiers. */
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
	VIS_OP_INVALID, /* denotes the end of the "real" operators */
	/* pseudo operators: keep them at the end to save space in array definition */
	VIS_OP_CURSOR_EOL,
	VIS_OP_PUT_AFTER_END,
	VIS_OP_PUT_BEFORE,
	VIS_OP_PUT_BEFORE_END,
	VIS_OP_LAST, /* has to be last enum member */
};

/**
 * @defgroup vis_operators
 * @{
 */
typedef struct OperatorContext OperatorContext;

/**
 * An operator performs a certain function on a given text range.
 * @rst
 * .. note:: The operator must return the new cursor position or ``EPOS`` if
 *           the cursor should be disposed.
 * .. note:: The last used operator can be repeated using `vis_repeat`.
 * @endrst
 */
typedef size_t (VisOperatorFunction)(Vis*, Text*, OperatorContext*);

/**
 * Register an operator.
 * @return Operator ID. Negative values indicate an error, positive ones can be
 *         used with `vis_operator`.
 */
int vis_operator_register(Vis*, VisOperatorFunction*, void *context);

/**
 * Set operator to execute.
 *
 * Has immediate effect if:
 *  - A visual mode is active.
 *  - The same operator was already set (range will be the current line).
 *
 * Otherwise the operator will be executed on the range determinded by:
 *  - A motion (see `vis_motion`).
 *  - A text object (`vis_textobject`).
 *
 * The expected varying arguments are:
 *
 *  - `VIS_OP_JOIN`       a char pointer referring to the text to insert between lines.
 *  - `VIS_OP_MODESWITCH` an ``enum VisMode`` indicating the mode to switch to.
 *  - `VIS_OP_REPLACE`    a char pointer reffering to the replacement character.
 */
bool vis_operator(Vis*, enum VisOperator, ...);

/** Repeat last operator, possibly with a new count if one was provided in the meantime. */
void vis_repeat(Vis*);

/** Cancel pending operator, reset count, motion, text object, register etc. */
void vis_cancel(Vis*);

/** @} */
/** Motion specifiers. */
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
	VIS_MOVE_PARENTHESIS_START,
	VIS_MOVE_PARENTHESIS_END,
	VIS_MOVE_BRACKET_MATCH,
	VIS_MOVE_LEFT_TO,
	VIS_MOVE_RIGHT_TO,
	VIS_MOVE_LEFT_TILL,
	VIS_MOVE_RIGHT_TILL,
	VIS_MOVE_FILE_BEGIN,
	VIS_MOVE_FILE_END,
	VIS_MOVE_SEARCH_WORD_FORWARD,
	VIS_MOVE_SEARCH_WORD_BACKWARD,
	VIS_MOVE_SEARCH_REPEAT_FORWARD,
	VIS_MOVE_SEARCH_REPEAT_BACKWARD,
	VIS_MOVE_WINDOW_LINE_TOP,
	VIS_MOVE_WINDOW_LINE_MIDDLE,
	VIS_MOVE_WINDOW_LINE_BOTTOM,
	VIS_MOVE_CHANGELIST_NEXT,
	VIS_MOVE_CHANGELIST_PREV,
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

/**
 * @defgroup vis_motions
 * @{
 */
/**
 * Set motion to perform.
 *
 * The following motions take an additional argument:
 *
 *  - `VIS_MOVE_SEARCH_FORWARD` and `VIS_MOVE_SEARCH_BACKWARD`
 *
 *     The search pattern as ``const char *``.
 *
 *  - ``VIS_MOVE_{LEFT,RIGHT}_{TO,TILL}``
 *
 *     The character to search for as ``const char *``.
 */
bool vis_motion(Vis*, enum VisMotion, ...);

enum VisMotionType {
	VIS_MOTIONTYPE_LINEWISE  = 1 << 0,
	VIS_MOTIONTYPE_CHARWISE  = 1 << 1,
};

/** Force currently specified motion to behave in line or character wise mode. */
void vis_motion_type(Vis *vis, enum VisMotionType);

/**
 * Motions take a starting position and transform it to an end position.
 * @rst
 * .. note:: Should a motion not be possible, the original position must be returned.
 *           TODO: we might want to change that to ``EPOS``?
 * @endrst
 */
typedef size_t (VisMotionFunction)(Vis*, Win*, void *context, size_t pos);

/**
 * Register a motion function.
 * @return Motion ID. Negative values indicate an error, positive ones can be
 *         used with `vis_motion`.
 */
int vis_motion_register(Vis*, void *context, VisMotionFunction*);

/**
 * @}
 * @defgroup vis_count
 * @{
 */
/** No count was specified. */
#define VIS_COUNT_UNKNOWN (-1)
/** Get count, might return `VIS_COUNT_UNKNOWN`. */
int vis_count_get(Vis*);
/** Get count, if none was specified, return ``def``. */
int vis_count_get_default(Vis*, int def);
/** Set a count. */
void vis_count_set(Vis*, int count);

typedef struct {
	Vis *vis;
	int iteration;
	int count;
} VisCountIterator;

/** Get iterator initialized with current count or ``def`` if not specified. */
VisCountIterator vis_count_iterator_get(Vis*, int def);
/** Get iterator initialized with a count value. */
VisCountIterator vis_count_iterator_init(Vis*, int count);
/**
 * Increment iterator counter.
 * @return Whether iteration should continue.
 * @rst
 * .. note:: Terminates iteration if the edtior was
 *           `interrupted <vis_interrupt>`_ in the meantime.
 * @endrst
 */
bool vis_count_iterator_next(VisCountIterator*);

/** @} */
/** Text object specifier. */
enum VisTextObject {
	VIS_TEXTOBJECT_INNER_WORD,
	VIS_TEXTOBJECT_OUTER_WORD,
	VIS_TEXTOBJECT_INNER_LONGWORD,
	VIS_TEXTOBJECT_OUTER_LONGWORD,
	VIS_TEXTOBJECT_SENTENCE,
	VIS_TEXTOBJECT_PARAGRAPH,
	VIS_TEXTOBJECT_PARAGRAPH_OUTER,
	VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET,
	VIS_TEXTOBJECT_INNER_SQUARE_BRACKET,
	VIS_TEXTOBJECT_OUTER_CURLY_BRACKET,
	VIS_TEXTOBJECT_INNER_CURLY_BRACKET,
	VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET,
	VIS_TEXTOBJECT_INNER_ANGLE_BRACKET,
	VIS_TEXTOBJECT_OUTER_PARENTHESIS,
	VIS_TEXTOBJECT_INNER_PARENTHESIS,
	VIS_TEXTOBJECT_OUTER_QUOTE,
	VIS_TEXTOBJECT_INNER_QUOTE,
	VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE,
	VIS_TEXTOBJECT_INNER_SINGLE_QUOTE,
	VIS_TEXTOBJECT_OUTER_BACKTICK,
	VIS_TEXTOBJECT_INNER_BACKTICK,
	VIS_TEXTOBJECT_OUTER_LINE,
	VIS_TEXTOBJECT_INNER_LINE,
	VIS_TEXTOBJECT_INDENTATION,
	VIS_TEXTOBJECT_SEARCH_FORWARD,
	VIS_TEXTOBJECT_SEARCH_BACKWARD,
	VIS_TEXTOBJECT_INVALID,
};

/**
 * @defgroup vis_textobjs
 * @{
 */

/**
 * Text objects take a starting position and return a text range.
 * @rst
 * .. note:: The originating position does not necessarily have to be contained in
 *           the resulting range.
 * @endrst
 */
typedef Filerange (VisTextObjectFunction)(Vis*, Win*, void *context, size_t pos);

/**
 * Register a new text object.
 * @return Text object ID. Negative values indicate an error, positive ones can be
 *         used with `vis_textobject`.
 */
int vis_textobject_register(Vis*, int type, void *data, VisTextObjectFunction*);

/** Set text object to use. */
bool vis_textobject(Vis*, enum VisTextObject);

/** @} */
/** Mark specifiers. */
enum VisMark {
	VIS_MARK_DEFAULT,
	VIS_MARK_SELECTION,
	VIS_MARK_a, VIS_MARK_b, VIS_MARK_c, VIS_MARK_d, VIS_MARK_e,
	VIS_MARK_f, VIS_MARK_g, VIS_MARK_h, VIS_MARK_i, VIS_MARK_j,
	VIS_MARK_k, VIS_MARK_l, VIS_MARK_m, VIS_MARK_n, VIS_MARK_o,
	VIS_MARK_p, VIS_MARK_q, VIS_MARK_r, VIS_MARK_s, VIS_MARK_t,
	VIS_MARK_u, VIS_MARK_v, VIS_MARK_w, VIS_MARK_x, VIS_MARK_y,
	VIS_MARK_z,
	VIS_MARK_INVALID,     /* has to be the last enum member */
};

/**
 * @}
 * @defgroup vis_marks
 * @{
 */
/** Translate between single character mark name and corresponding constant. */
enum VisMark vis_mark_from(Vis*, char mark);
char vis_mark_to(Vis*, enum VisMark);
/**
 * Specify mark to use.
 * @rst
 * .. note:: If none is specified `VIS_MARK_DEFAULT` will be used.
 * @endrst
 */
void vis_mark(Vis*, enum VisMark);
enum VisMark vis_mark_used(Vis*);
/**
 * Store a set of ``Filerange``s in a mark.
 *
 * @param id The register to use.
 * @param sel The array containing the file ranges.
 */
void vis_mark_set(Win*, enum VisMark id, Array *sel);
/**
 * Get an array of file ranges stored in the mark.
 *
 * @rst
 * .. warning:: The caller must eventually free the Array by calling
 *              ``array_release``.
 * @endrst
 */
Array vis_mark_get(Win*, enum VisMark id);
/**
 * Normalize an Array of Fileranges.
 *
 * Removes invalid ranges, merges overlapping ones and sorts
 * according to the start position.
 */
void vis_mark_normalize(Array*);
/** Add selections of focused window to jump list. */
bool vis_jumplist_save(Vis*);
/** Navigate jump list backwards. */
bool vis_jumplist_prev(Vis*);
/** Navigate jump list forwards. */
bool vis_jumplist_next(Vis*);
/** @} */
/** Register specifiers. */
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
	VIS_REG_PRIMARY,    /* system primary clipboard register */
	VIS_REG_DOT,        /* last inserted text, copy of VIS_MACRO_OPERATOR */
	VIS_REG_SEARCH,     /* last used search pattern "/ */
	VIS_REG_COMMAND,    /* last used :-command ": */
	VIS_REG_SHELL,      /* last used shell command given to either <, >, |, or ! */
	VIS_REG_NUMBER,     /* cursor number */
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

/**
 * @defgroup vis_registers
 * @{
 */
/** Translate between single character register name and corresponding constant. */
enum VisRegister vis_register_from(Vis*, char reg);
char vis_register_to(Vis*, enum VisRegister);
/**
 * Specify register to use.
 * @rst
 * .. note:: If none is specified `VIS_REG_DEFAULT` will be used.
 * @endrst
 */
void vis_register(Vis*, enum VisRegister);
enum VisRegister vis_register_used(Vis*);
/**
 * Get register content.
 * @return An array of ``TextString`` structs.
 * @rst
 * .. warning:: The caller must eventually free the array ressources using
 *              ``array_release``.
 * @endrst
 */
Array vis_register_get(Vis*, enum VisRegister);
/**
 * Set register content.
 * @param data The array comprised of ``TextString`` structs.
 */
bool vis_register_set(Vis*, enum VisRegister, Array *data);
/**
 * @}
 * @defgroup vis_macros
 * @{
 */
/**
 * Start recording a macro.
 * @rst
 * .. note:: Fails if a recording is already ongoing.
 * @endrst
 */
bool vis_macro_record(Vis*, enum VisRegister);
/** Stop recording, fails if there is nothing to stop. */
bool vis_macro_record_stop(Vis*);
/** Check whether a recording is currently ongoing. */
bool vis_macro_recording(Vis*);
/**
 * Replay a macro.
 * @rst
 * .. note:: A macro currently being recorded can not be replayed.
 * @endrst
 */
bool vis_macro_replay(Vis*, enum VisRegister);

/**
 * @}
 * @defgroup vis_cmds
 * @{
 */

/** Execute a ``:``-command. */
bool vis_cmd(Vis*, const char *cmd);

/** Command handler function. */
typedef bool (VisCommandFunction)(Vis*, Win*, void *data, bool force,
	const char *argv[], Selection*, Filerange*);
/**
 * Register new ``:``-command.
 * @param name The command name.
 * @param help Optional single line help text.
 * @param context User supplied context pointer passed to the handler function.
 * @param func The function implementing the command logic.
 * @rst
 * .. note:: Any unique prefix of the command name will invoke the command.
 * @endrst
 */
bool vis_cmd_register(Vis*, const char *name, const char *help, void *context, VisCommandFunction*);

/** Unregister ``:``-command. */
bool vis_cmd_unregister(Vis*, const char *name);

/**
 * @}
 * @defgroup vis_options
 * @{
 */
/** Option properties. */
enum VisOption {
	VIS_OPTION_TYPE_BOOL = 1 << 0,
	VIS_OPTION_TYPE_STRING = 1 << 1,
	VIS_OPTION_TYPE_NUMBER = 1 << 2,
	VIS_OPTION_VALUE_OPTIONAL = 1 << 3,
	VIS_OPTION_NEED_WINDOW = 1 << 4,
};

/**
 * Option handler function.
 * @param win The window to which option should apply, might be ``NULL``.
 * @param context User provided context pointer as given to `vis_option_register`.
 * @param force Whether the option was specfied with a bang ``!``.
 * @param name Name of option which was set.
 * @param arg The new option value.
 */
typedef bool (VisOptionFunction)(Vis*, Win*, void *context, bool toggle,
                                 enum VisOption, const char *name, Arg *value);

/**
 * Register a new ``:set`` option.
 * @param names A ``NULL`` terminated array of option names.
 * @param option Option properties.
 * @param func The function handling the option.
 * @param context User supplied context pointer passed to the handler function.
 * @param help Optional single line help text.
 * @rst
 * .. note:: Fails if any of the given option names is already registered.
 * @endrst
 */
bool vis_option_register(Vis*, const char *names[], enum VisOption,
                         VisOptionFunction*, void *context, const char *help);
/**
 * Unregister an existing ``:set`` option.
 * @rst
 * .. note:: Also unregisters all aliases as given to `vis_option_register`.
 * @endrst
 */
bool vis_option_unregister(Vis*, const char *name);

/** Execute any kind (``:``, ``?``, ``/``) of prompt command */
bool vis_prompt_cmd(Vis*, const char *cmd);

/**
 * Pipe a given file range to an external process.
 *
 * If the range is invalid 'interactive' mode is enabled, meaning that
 * stdin and stderr are passed through the underlying command, stdout
 * points to vis' stderr.
 *
 * If ``argv`` contains only one non-NULL element the command is executed
 * through an intermediate shell (using ``/bin/sh -c argv[0]``) that is
 * argument expansion is performed by the shell. Otherwise the argument
 * list will be passed unmodified to ``execvp(argv[0], argv)``.
 *
 * If the ``read_stdout`` and ``read_stderr`` callbacks are non-NULL they
 * will be invoked when output from the forked process is available.
 *
 * @rst
 * .. warning:: The editor core is blocked until this function returns.
 * @endrst
 *
 * @return The exit status of the forked process.
 */
int vis_pipe(Vis*, File*, Filerange*, const char *argv[],
	void *stdout_context, ssize_t (*read_stdout)(void *stdout_context, char *data, size_t len),
	void *stderr_context, ssize_t (*read_stderr)(void *stderr_context, char *data, size_t len));

/**
 * Pipe a Filerange to an external process, return its exit status and capture
 * everything that is written to stdout/stderr.
 * @param argv Argument list, must be ``NULL`` terminated.
 * @param out Data written to ``stdout``, will be ``NUL`` terminated.
 * @param err Data written to ``stderr``, will be ``NUL`` terminated.
 * @rst
 * .. warning:: The pointers stored in ``out`` and ``err`` need to be `free(3)`-ed
 *              by the caller.
 * @endrst
 */
int vis_pipe_collect(Vis*, File*, Filerange*, const char *argv[], char **out, char **err);

/**
 * @}
 * @defgroup vis_keys
 * @{
 */
/**
 * Advance to the start of the next symbolic key.
 *
 * Given the start of a symbolic key, returns a pointer to the start of the one
 * immediately following it.
 */
const char *vis_keys_next(Vis*, const char *keys);
/** Convert next symbolic key to an Unicode code point, returns ``-1`` for unknown keys. */
long vis_keys_codepoint(Vis*, const char *keys);
/**
 * Convert next symbolic key to a UTF-8 sequence.
 * @return Whether conversion was successful, if not ``utf8`` is left unmodified.
 * @rst
 * .. note:: Guarantees that ``utf8`` is NUL terminated on success.
 * @endrst
 */
bool vis_keys_utf8(Vis*, const char *keys, char utf8[static UTFmax+1]);
/** Process symbolic keys as if they were user originated input. */
void vis_keys_feed(Vis*, const char *keys);
/**
 * @}
 * @defgroup vis_misc
 * @{
 */

/**
 * Get a regex object matching pattern.
 * @param regex The regex pattern to compile, if ``NULL`` the most recently used
 *        one is substituted.
 * @return A Regex object or ``NULL`` in case of an error.
 * @rst
 * .. warning:: The caller must free the regex object using `text_regex_free`.
 * @endrst
 */
Regex *vis_regex(Vis*, const char *pattern);

/**
 * Take an undo snaphost to which we can later revert to.
 * @rst
 * .. note:: Does nothing when invoked while replaying a macro.
 * @endrst
 */
void vis_file_snapshot(Vis*, File*);
/** @} */

/* TODO: expose proper API to iterate through files etc */
Text *vis_text(Vis*);
View *vis_view(Vis*);
Win *vis_window(Vis*);

/* Get value of autoindent */
bool vis_get_autoindent(const Vis*);

#endif
