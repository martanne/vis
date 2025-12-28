#ifndef VIS_H
#define VIS_H

#ifndef VIS_EXPORT
  #define VIS_EXPORT
#endif

#include <signal.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct Vis Vis;
typedef struct File File;
typedef struct Win Win;

#include "ui.h"
#include "view.h"
#include "buffer.h"

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

/** Union used to pass arguments to key action functions. */
typedef union {
	bool b;
	int i;
	const char *s;
	const void *v;
	void (*w)(View*);
	void (*f)(Vis*);
} Arg;

/**
 * Key action handling function.
 * @param vis The editor instance.
 * @param keys Input queue content *after* the binding which invoked this function.
 * @param arg Arguments for the key action.
 * @rst
 * .. note:: An empty string ``""`` indicates that no further input is available.
 * @endrst
 * @return Pointer to first non-consumed key.
 * @rst
 * .. warning:: Must be in range ``[keys, keys+strlen(keys)]`` or ``NULL`` to
 * indicate that not enough input was available. In the latter case
 * the function will be called again once more input has been received.
 * @endrst
 * @ingroup vis_action
 */
#define KEY_ACTION_FN(name) const char *name(Vis *vis, const char *keys, const Arg *arg)
typedef KEY_ACTION_FN(KeyActionFunction);

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

/*
---
## Core Lifecycle Functions
*/

/**
 * @defgroup vis_lifecycle Vis Lifecycle
 * @{
 */
/**
 * Initializes a new editor instance.
 * @param vis The editor instance.
 */
VIS_EXPORT bool vis_init(Vis*);
/** Release all resources associated with this editor instance, terminates UI. */
VIS_EXPORT void vis_cleanup(Vis*);
/**
 * Enter main loop, start processing user input.
 * @param vis The editor instance.
 * @return The editor exit status code.
 */
VIS_EXPORT int vis_run(Vis*);
/**
 * Terminate editing session, the given ``status`` will be the return value of `vis_run`.
 * @param vis The editor instance.
 * @param status The exit status.
 */
VIS_EXPORT void vis_exit(Vis *vis, int status);
/**
 * Emergency exit, print given message, perform minimal UI cleanup and exit process.
 * @param vis The editor instance.
 * @param msg The message to print.
 * @rst
 * .. note:: This function does not return.
 * @endrst
 */
VIS_EXPORT void vis_die(Vis *vis, const char *msg, ...) __attribute__((noreturn,format(printf, 2, 3)));

/**
 * Inform the editor core that a signal occurred.
 * @param vis The editor instance.
 * @param signum The signal number.
 * @param siginfo Pointer to `siginfo_t` structure.
 * @param context Pointer to context.
 * @return Whether the signal was handled.
 * @rst
 * .. note:: Being designed as a library the editor core does *not* register any
 * signal handlers on its own.
 * .. note:: The remaining arguments match the prototype of ``sa_sigaction`` as
 * specified in `sigaction(2)`.
 * @endrst
 */
VIS_EXPORT bool vis_signal_handler(Vis *vis, int signum, const siginfo_t *siginfo, const void *context);

/*
---
## Drawing and Redrawing
*/

/**
 * @defgroup vis_draw Vis Drawing
 * @{
 */
/**
 * Draw user interface.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_draw(Vis*);
/**
 * Completely redraw user interface.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_redraw(Vis*);
/** @} */

/*
---
## Window Management
*/

/**
 * @defgroup vis_windows Vis Windows
 * @{
 */
/**
 * Create a new window and load the given file.
 * @param vis The editor instance.
 * @param filename If ``NULL`` an unnamed, empty buffer is created.
 * @rst
 * .. note:: If the given file name is already opened in another window,
 * the underlying File object is shared.
 * @endrst
 */
VIS_EXPORT bool vis_window_new(Vis *vis, const char *filename);
/**
 * Create a new window associated with a file descriptor.
 * @param vis The editor instance.
 * @param fd The file descriptor to associate with the window.
 * @rst
 * .. note:: No data is read from `fd`, but write commands without an
 * explicit filename will instead write to the file descriptor.
 * @endrst
 */
VIS_EXPORT bool vis_window_new_fd(Vis *vis, int fd);
/**
 * Reload the file currently displayed in the window from disk.
 * @param win The window to reload.
 */
VIS_EXPORT bool vis_window_reload(Win*);
/**
 * Change the file currently displayed in the window.
 * @param win The window to change.
 * @param filename The new file to display.
 */
VIS_EXPORT bool vis_window_change_file(Win *win, const char *filename);
/**
 * Check whether closing the window would loose unsaved changes.
 * @param win The window to check.
 */
VIS_EXPORT bool vis_window_closable(Win*);
/**
 * Close window, redraw user interface.
 * @param win The window to close.
 */
VIS_EXPORT void vis_window_close(Win*);
/**
 * Split the window, shares the underlying file object.
 * @param original The window to split.
 */
VIS_EXPORT bool vis_window_split(Win*);
/**
 * Draw a specific window.
 * @param win The window to draw.
 */
VIS_EXPORT void vis_window_draw(Win*);
/**
 * Invalidate a window, forcing a redraw.
 * @param win The window to invalidate.
 */
VIS_EXPORT void vis_window_invalidate(Win*);
/**
 * Focus next window.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_window_next(Vis*);
/**
 * Focus previous window.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_window_prev(Vis*);
/**
 * Change currently focused window, receiving user input.
 * @param win The window to focus.
 */
VIS_EXPORT void vis_window_focus(Win*);
/**
 * Swap location of two windows.
 * @param win1 The first window.
 * @param win2 The second window.
 */
VIS_EXPORT void vis_window_swap(Win *win1, Win *win2);
/** @} */

/*
---
## Information and Messaging
*/

/**
 * @defgroup vis_info Vis Info
 * @{
 */
/**
 * Display a user prompt with a certain title.
 * @param vis The editor instance.
 * @param title The title of the prompt.
 * @rst
 * .. note:: The prompt is currently implemented as a single line height window.
 * @endrst
 */
VIS_EXPORT void vis_prompt_show(Vis *vis, const char *title);

/**
 * Display a single line message.
 * @param vis The editor instance.
 * @param msg The message to display.
 * @rst
 * .. note:: The message will automatically be hidden upon next input.
 * @endrst
 */
VIS_EXPORT void vis_info_show(Vis *vis, const char *msg, ...) __attribute__((format(printf, 2, 3)));

/**
 * Display arbitrary long message in a dedicated window.
 * @param vis The editor instance.
 * @param msg The message to display.
 */
VIS_EXPORT void vis_message_show(Vis *vis, const char *msg);
/** @} */

/*
---
## Text Changes
*/

/**
 * @defgroup vis_changes Vis Changes
 * @{
 */
/**
 * Insert data into the file.
 * @param vis The editor instance.
 * @param pos The position to insert at.
 * @param data The data to insert.
 * @param len The length of the data to insert.
 */
VIS_EXPORT void vis_insert(Vis *vis, size_t pos, const char *data, size_t len);
/**
 * Replace data in the file.
 * @param vis The editor instance.
 * @param pos The position to replace at.
 * @param data The new data.
 * @param len The length of the new data.
 */
VIS_EXPORT void vis_replace(Vis *vis, size_t pos, const char *data, size_t len);
/**
 * Perform insertion at all cursor positions.
 * @param vis The editor instance.
 * @param data The data to insert.
 * @param len The length of the data.
 */
VIS_EXPORT void vis_insert_key(Vis *vis, const char *data, size_t len);
/**
 * Perform character substitution at all cursor positions.
 * @param vis The editor instance.
 * @param data The character data to substitute.
 * @param len The length of the data.
 * @rst
 * .. note:: Does not replace new line characters.
 * @endrst
 */
VIS_EXPORT void vis_replace_key(Vis *vis, const char *data, size_t len);
/**
 * Insert a tab at all cursor positions.
 * @param vis The editor instance.
 * @rst
 * .. note:: Performs tab expansion according to current settings.
 * @endrst
 */
VIS_EXPORT void vis_insert_tab(Vis*);
/**
 * Inserts a new line character at every cursor position.
 * @param vis The editor instance.
 * @rst
 * .. note:: Performs auto indentation according to current settings.
 * @endrst
 */
VIS_EXPORT void vis_insert_nl(Vis*);

/** @} */

/*
---
## Editor Modes
*/

/**
 * @defgroup vis_modes Vis Modes
 * @{
 */

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
 * Switch mode.
 * @param vis The editor instance.
 * @param mode The mode to switch to.
 * @rst
 * .. note:: Will first trigger the leave event of the currently active
 * mode, followed by an enter event of the new mode.
 * No events are emitted, if the specified mode is already active.
 * @endrst
 */
VIS_EXPORT void vis_mode_switch(Vis *vis, enum VisMode mode);
/**
 * Translate human readable mode name to constant.
 * @param vis The editor instance.
 * @param name The name of the mode.
 */
VIS_EXPORT enum VisMode vis_mode_from(Vis *vis, const char *name);

/** @} */

/*
---
## Keybinding and Actions
*/

/**
 * @defgroup vis_keybind Vis Keybindings
 * @{
 */
/**
 * Create a new key binding.
 * @param vis The editor instance.
 */
VIS_EXPORT KeyBinding *vis_binding_new(Vis*);
/**
 * Free a key binding.
 * @param vis The editor instance.
 * @param binding The key binding to free.
 */
VIS_EXPORT void vis_binding_free(Vis *vis, KeyBinding *binding);

/**
 * Set up a key binding.
 * @param vis The editor instance.
 * @param mode The mode in which the binding applies.
 * @param force Whether an existing mapping should be discarded.
 * @param key The symbolic key to map.
 * @param binding The binding to map.
 * @rst
 * .. note:: ``binding->key`` is always ignored in favor of ``key``.
 * @endrst
 */
VIS_EXPORT bool vis_mode_map(Vis *vis, enum VisMode mode, bool force, const char *key, const KeyBinding *binding);
/**
 * Analogous to `vis_mode_map`, but window specific.
 * @param win The window for the mapping.
 * @param mode The mode in which the binding applies.
 * @param force Whether an existing mapping should be discarded.
 * @param key The symbolic key to map.
 * @param binding The binding to map.
 */
VIS_EXPORT bool vis_window_mode_map(Win *win, enum VisMode mode, bool force, const char *key, const KeyBinding *binding);
/**
 * Unmap a symbolic key in a given mode.
 * @param vis The editor instance.
 * @param mode The mode from which to unmap.
 * @param key The symbolic key to unmap.
 */
VIS_EXPORT bool vis_mode_unmap(Vis *vis, enum VisMode mode, const char *key);
/**
 * Analogous to `vis_mode_unmap`, but window specific.
 * @param win The window from which to unmap.
 * @param mode The mode from which to unmap.
 * @param key The symbolic key to unmap.
 */
VIS_EXPORT bool vis_window_mode_unmap(Win *win, enum VisMode mode, const char *key);
/** @} */

/*
---
## Key Actions
*/

/**
 * @defgroup vis_action Vis Actions
 * @{
 */
/**
 * Create new key action.
 * @param vis The editor instance.
 * @param name The name to be used as symbolic key when registering.
 * @param help Optional single line help text.
 * @param func The function implementing the key action logic.
 * @param arg Argument passed to function.
 */
VIS_EXPORT KeyAction *vis_action_new(Vis *vis, const char *name, const char *help, KeyActionFunction *func, Arg arg);
/**
 * Free a key action.
 * @param vis The editor instance.
 * @param action The key action to free.
 */
VIS_EXPORT void vis_action_free(Vis *vis, KeyAction *action);
/**
 * Register key action.
 * @param vis The editor instance.
 * @param keyaction The key action to register.
 * @rst
 * .. note:: Makes the key action available under the pseudo key name specified
 * in ``keyaction->name``.
 * @endrst
 */
VIS_EXPORT bool vis_action_register(Vis *vis, const KeyAction *keyaction);

/** @} */

/*
---
## Keymap
*/

/**
 * @defgroup vis_keymap Vis Keymap
 * @{
 */

/**
 * Add a key translation.
 * @param vis The editor instance.
 * @param key The key to translate.
 * @param mapping The string to map the key to.
 */
VIS_EXPORT bool vis_keymap_add(Vis *vis, const char *key, const char *mapping);
/**
 * Temporarily disable the keymap for the next key press.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_keymap_disable(Vis*);

/** @} */

/*
---
## Operators
*/

/**
 * @defgroup vis_operators Vis Operators
 * @{
 */

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

typedef struct OperatorContext OperatorContext;

/**
 * An operator performs a certain function on a given text range.
 * @param vis The editor instance.
 * @param text The text buffer to operate on.
 * @param context Operator-specific context.
 * @rst
 * .. note:: The operator must return the new cursor position or ``EPOS`` if
 * the cursor should be disposed.
 * .. note:: The last used operator can be repeated using `vis_repeat`.
 * @endrst
 */
typedef size_t (VisOperatorFunction)(Vis *vis, Text *text, OperatorContext *context);

/**
 * Register an operator.
 * @param vis The editor instance.
 * @param func The function implementing the operator logic.
 * @param context User-supplied context for the operator.
 * @return Operator ID to be used with `vis_operator`.
 */
VIS_EXPORT int vis_operator_register(Vis *vis, VisOperatorFunction *func, void *context);

/**
 * Set operator to execute.
 * @param vis The editor instance.
 * @param op The operator to perform.
 * @param ... Additional arguments depending on the operator type.
 *
 * Has immediate effect if:
 * - A visual mode is active.
 * - The same operator was already set (range will be the current line).
 *
 * Otherwise the operator will be executed on the range determined by:
 * - A motion (see `vis_motion`).
 * - A text object (`vis_textobject`).
 *
 * The expected varying arguments are:
 *
 * - `VIS_OP_JOIN`         a char pointer referring to the text to insert between lines.
 * - `VIS_OP_MODESWITCH`   an ``enum VisMode`` indicating the mode to switch to.
 * - `VIS_OP_REPLACE`      a char pointer referring to the replacement character.
 */
VIS_EXPORT bool vis_operator(Vis *vis, enum VisOperator op, ...);

/**
 * Repeat last operator, possibly with a new count if one was provided in the meantime.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_repeat(Vis*);

/**
 * Cancel pending operator, reset count, motion, text object, register etc.
 * @param vis The editor instance.
 */
VIS_EXPORT void vis_cancel(Vis*);

/** @} */

/*
---
## Motions
*/

/**
 * @defgroup vis_motions Vis Motions
 * @{
 */

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
	VIS_MOVE_TO_LEFT,
	VIS_MOVE_TO_RIGHT,
	VIS_MOVE_TO_LINE_LEFT,
	VIS_MOVE_TO_LINE_RIGHT,
	VIS_MOVE_TILL_LEFT,
	VIS_MOVE_TILL_RIGHT,
	VIS_MOVE_TILL_LINE_LEFT,
	VIS_MOVE_TILL_LINE_RIGHT,
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
 * Set motion to perform.
 * @param vis The editor instance.
 * @param motion The motion to perform.
 * @param ... Additional arguments depending on the motion type.
 *
 * The following motions take an additional argument:
 *
 * - `VIS_MOVE_SEARCH_FORWARD` and `VIS_MOVE_SEARCH_BACKWARD`
 *
 * The search pattern as ``const char *``.
 *
 * - ``VIS_MOVE_{LEFT,RIGHT}_{TO,TILL}``
 *
 * The character to search for as ``const char *``.
 */
VIS_EXPORT bool vis_motion(Vis *vis, enum VisMotion motion, ...);

enum VisMotionType {
	VIS_MOTIONTYPE_LINEWISE  = 1 << 0,
	VIS_MOTIONTYPE_CHARWISE  = 1 << 1,
};

/**
 * Force currently specified motion to behave in line or character wise mode.
 * @param vis The editor instance.
 * @param type The motion type (line-wise or character-wise).
 */
VIS_EXPORT void vis_motion_type(Vis *vis, enum VisMotionType type);

/**
 * Motions take a starting position and transform it to an end position.
 * @param vis The editor instance.
 * @param win The window in which the motion is performed.
 * @param context User-supplied context for the motion.
 * @param pos The starting position.
 * @rst
 * .. note:: Should a motion not be possible, the original position must be returned.
 * TODO: we might want to change that to ``EPOS``?
 * @endrst
 */
typedef size_t (VisMotionFunction)(Vis *vis, Win *win, void *context, size_t pos);

/**
 * Register a motion function.
 * @param vis The editor instance.
 * @param context User-supplied context for the motion.
 * @param func The function implementing the motion logic.
 * @return Motion ID to be used with `vis_motion`.
 */
VIS_EXPORT int vis_motion_register(Vis *vis, void *context, VisMotionFunction *func);

/** @} */

/*
---
## Count Iteration
*/

/**
 * @defgroup vis_count Vis Count
 * @{
 */
/** No count was specified. */
#define VIS_COUNT_UNKNOWN (-1)
#define VIS_COUNT_DEFAULT(count, def) ((count) == VIS_COUNT_UNKNOWN ? (def) : (count))
#define VIS_COUNT_NORMALIZE(count)    ((count) < 0 ? VIS_COUNT_UNKNOWN : (count))
/**
 * Set the shell.
 * @param vis The editor instance.
 * @param new_shell The new shell to set.
 */
VIS_EXPORT void vis_shell_set(Vis *vis, const char *new_shell);

typedef struct {
	Vis *vis;
	int iteration;
	int count;
} VisCountIterator;

/**
 * Get iterator initialized with current count or ``def`` if not specified.
 * @param vis The editor instance.
 * @param def The default count if none is specified.
 */
VIS_EXPORT VisCountIterator vis_count_iterator_get(Vis *vis, int def);
/**
 * Get iterator initialized with a count value.
 * @param vis The editor instance.
 * @param count The count value to initialize with.
 */
VIS_EXPORT VisCountIterator vis_count_iterator_init(Vis *vis, int count);
/**
 * Increment iterator counter.
 * @param iter Pointer to the iterator.
 * @return Whether iteration should continue.
 * @rst
 * .. note:: Terminates iteration if the editor was
 * `interrupted <vis_interrupt>`_ in the meantime.
 * @endrst
 */
VIS_EXPORT bool vis_count_iterator_next(VisCountIterator *iter);

/** @} */

/*
---
## Text Objects
*/

/**
 * @defgroup vis_textobjs Vis Text Objects
 * @{
 */

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
 * Text objects take a starting position and return a text range.
 * @param vis The editor instance.
 * @param win The window in which the text object is applied.
 * @param context User-supplied context for the text object.
 * @param pos The starting position.
 * @rst
 * .. note:: The originating position does not necessarily have to be contained in
 * the resulting range.
 * @endrst
 */
typedef Filerange (VisTextObjectFunction)(Vis *vis, Win *win, void *context, size_t pos);

/**
 * Register a new text object.
 * @param vis The editor instance.
 * @param type The type of the text object.
 * @param data User-supplied data for the text object.
 * @param func The function implementing the text object logic.
 * @return Text object ID to be used with `vis_textobject`.
 */
VIS_EXPORT int vis_textobject_register(Vis *vis, int type, void *data, VisTextObjectFunction *func);

/**
 * Set text object to use.
 * @param vis The editor instance.
 * @param textobj The text object to set.
 */
VIS_EXPORT bool vis_textobject(Vis *vis, enum VisTextObject textobj);

/** @} */

/*
---
## Marks
*/

/**
 * @defgroup vis_marks Vis Marks
 * @{
 */

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
 * Translate between single character mark name and corresponding constant.
 * @param vis The editor instance.
 * @param mark The character representing the mark.
 */
VIS_EXPORT enum VisMark vis_mark_from(Vis *vis, char mark);
/**
 * Translate between mark constant and single character mark name.
 * @param vis The editor instance.
 * @param mark The mark constant.
 */
VIS_EXPORT char vis_mark_to(Vis *vis, enum VisMark mark);
/**
 * Specify mark to use.
 * @param vis The editor instance.
 * @param mark The mark to use.
 * @rst
 * .. note:: If none is specified `VIS_MARK_DEFAULT` will be used.
 * @endrst
 */
VIS_EXPORT void vis_mark(Vis *vis, enum VisMark mark);
/**
 * Get the currently used mark.
 * @param vis The editor instance.
 */
VIS_EXPORT enum VisMark vis_mark_used(Vis*);
/**
 * Store a set of ``Filerange``s in a mark.
 *
 * @param vis The editor instance.
 * @param win The window whose selections to store.
 * @param id The mark ID to use.
 * @param ranges The list of file ranges.
 */
VIS_EXPORT void vis_mark_set(Vis *vis, Win *win, enum VisMark id, FilerangeList ranges);
/**
 * Get an array of file ranges stored in the mark.
 * @param vis The editor instance.
 * @param win The window whose mark to retrieve.
 * @param id The mark ID to retrieve.
 * @return A list of file ranges.
 * @rst
 * .. warning:: The caller is responsible for freeing the list with ``da_release``.
 * @endrst
 */
VIS_EXPORT FilerangeList vis_mark_get(Vis *vis, Win *win, enum VisMark id);
/**
 * Normalize a list of Fileranges.
 * @param ranges The list of file ranges to normalize.
 *
 * Removes invalid ranges, merges overlapping ones and sorts
 * according to the start position.
 */
VIS_EXPORT void vis_mark_normalize(FilerangeList *ranges);
/**
 * Add selections of focused window to jump list. Equivalent to vis_jumplist(vis, 0).
 * @param vis The editor instance.
 */
#define vis_jumplist_save(vis) vis_jumplist((vis), 0)
/**
 * Navigate jump list by a specified amount. Wraps if advance exceeds list size.
 * @param vis The editor instance.
 * @param advance The amount to advance the cursor by. 0 saves the current selections.
 */
VIS_EXPORT void vis_jumplist(Vis *, int advance);
/** @} */

/*
---
## Registers
*/

/**
 * @defgroup vis_registers Vis Registers
 * @{
 */

/** Register specifiers. */
enum VisRegister {
	VIS_REG_DEFAULT,    /* used when no other register is specified */
	VIS_REG_ZERO,       /* yank register */
	VIS_REG_AMPERSAND,  /* last regex match */
	VIS_REG_1,          /* 1-9 last sub-expression matches */
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
 * Translate between single character register name and corresponding constant.
 * @param vis The editor instance.
 * @param reg The character representing the register.
 */
VIS_EXPORT enum VisRegister vis_register_from(Vis *vis, char reg);
/**
 * Translate between register constant and single character register name.
 * @param vis The editor instance.
 * @param reg The register constant.
 */
VIS_EXPORT char vis_register_to(Vis *vis, enum VisRegister reg);
/**
 * Specify register to use.
 * @param vis The editor instance.
 * @param reg The register to use.
 * @rst
 * .. note:: If none is specified `VIS_REG_DEFAULT` will be used.
 * @endrst
 */
VIS_EXPORT void vis_register(Vis *vis, enum VisRegister reg);
/**
 * Get the currently used register.
 * @param vis The editor instance.
 */
VIS_EXPORT enum VisRegister vis_register_used(Vis*);
/**
 * Get register content.
 * @param vis The editor instance.
 * @param id The register ID to retrieve.
 * @return An array of ``TextString`` structs.
 * @rst
 * .. warning:: The caller must eventually free the array resources using
 * ``array_release``.
 * @endrst
 */
VIS_EXPORT str8_list vis_register_get(Vis *vis, enum VisRegister id);
/**
 * Set register content.
 * @param vis The editor instance.
 * @param id The register ID to set.
 * @param strings The list of ``TextString``s.
 */
VIS_EXPORT bool vis_register_set(Vis *vis, enum VisRegister id, str8_list strings);
/** @} */

/*
---
## Macros
*/

/**
 * @defgroup vis_macros Vis Macros
 * @{
 */
/**
 * Start recording a macro.
 * @param vis The editor instance.
 * @param reg The register to record the macro into.
 * @rst
 * .. note:: Fails if a recording is already ongoing.
 * @endrst
 */
VIS_EXPORT bool vis_macro_record(Vis *vis, enum VisRegister reg);
/**
 * Stop recording, fails if there is nothing to stop.
 * @param vis The editor instance.
 */
VIS_EXPORT bool vis_macro_record_stop(Vis*);
/**
 * Check whether a recording is currently ongoing.
 * @param vis The editor instance.
 */
VIS_EXPORT bool vis_macro_recording(Vis*);
/**
 * Replay a macro.
 * @param vis The editor instance.
 * @param reg The register containing the macro to replay.
 * @rst
 * .. note:: A macro currently being recorded can not be replayed.
 * @endrst
 */
VIS_EXPORT bool vis_macro_replay(Vis *vis, enum VisRegister reg);

/** @} */

/*
---
## Commands
*/

/**
 * @defgroup vis_cmds Vis Commands
 * @{
 */

/**
 * Execute a ``:``-command.
 * @param vis The editor instance.
 * @param cmd The command string to execute.
 */
VIS_EXPORT bool vis_cmd(Vis *vis, const char *cmd);

/** Command handler function. */
typedef bool (VisCommandFunction)(Vis*, Win*, void *data, bool force,
	const char *argv[], Selection*, Filerange*);
/**
 * Register new ``:``-command.
 * @param vis The editor instance.
 * @param name The command name.
 * @param help Optional single line help text.
 * @param context User supplied context pointer passed to the handler function.
 * @param func The function implementing the command logic.
 * @rst
 * .. note:: Any unique prefix of the command name will invoke the command.
 * @endrst
 */
VIS_EXPORT bool vis_cmd_register(Vis *vis, const char *name, const char *help, void *context, VisCommandFunction *func);

/**
 * Unregister ``:``-command.
 * @param vis The editor instance.
 * @param name The name of the command to unregister.
 */
VIS_EXPORT bool vis_cmd_unregister(Vis *vis, const char *name);

/** @} */

/*
---
## Options
*/

/**
 * @defgroup vis_options Vis Options
 * @{
 */
/** Option properties. */
enum VisOption {
	VIS_OPTION_TYPE_BOOL = 1 << 0,
	VIS_OPTION_TYPE_STRING = 1 << 1,
	VIS_OPTION_TYPE_NUMBER = 1 << 2,
	VIS_OPTION_VALUE_OPTIONAL = 1 << 3,
	VIS_OPTION_NEED_WINDOW = 1 << 4,
	VIS_OPTION_DEPRECATED = 1 << 5,
};

/**
 * Option handler function.
 * @param vis The editor instance.
 * @param win The window to which option should apply, might be ``NULL``.
 * @param context User provided context pointer as given to `vis_option_register`.
 * @param force Whether the option was specified with a bang ``!``.
 * @param option_flags The applicable option flags.
 * @param name Name of option which was set.
 * @param value The new option value.
 */
typedef bool (VisOptionFunction)(Vis *vis, Win *win, void *context, bool force,
                                 enum VisOption option_flags, const char *name, Arg *value);

/**
 * Register a new ``:set`` option.
 * @param vis The editor instance.
 * @param names A ``NULL`` terminated array of option names.
 * @param option_flags The applicable option flags.
 * @param func The function handling the option.
 * @param context User supplied context pointer passed to the handler function.
 * @param help Optional single line help text.
 * @rst
 * .. note:: Fails if any of the given option names is already registered.
 * @endrst
 */
VIS_EXPORT bool vis_option_register(Vis *vis, const char *names[], enum VisOption option_flags,
                                    VisOptionFunction *func, void *context, const char *help);
/**
 * Unregister an existing ``:set`` option.
 * @param vis The editor instance.
 * @param name The name of the option to unregister.
 * @rst
 * .. note:: Also unregisters all aliases as given to `vis_option_register`.
 * @endrst
 */
VIS_EXPORT bool vis_option_unregister(Vis *vis, const char *name);

/**
 * Execute any kind (``:``, ``?``, ``/``) of prompt command
 * @param vis The editor instance.
 * @param cmd The command string.
 */
VIS_EXPORT bool vis_prompt_cmd(Vis *vis, const char *cmd);

/**
 * Write newline separated list of available commands to ``buf``
 * @param vis The editor instance.
 * @param buf The buffer to write to.
 * @param prefix Prefix to filter command list by.
 */
VIS_EXPORT void vis_print_cmds(Vis*, Buffer *buf, const char *prefix);

/**
 * Pipe a given file range to an external process.
 * @param vis The editor instance.
 * @param file The file to pipe.
 * @param range The file range to pipe.
 * @param argv Argument list, must be ``NULL`` terminated.
 * @param stdout_context Context for stdout callback.
 * @param read_stdout Callback for stdout data.
 * @param stderr_context Context for stderr callback.
 * @param read_stderr Callback for stderr data.
 * @param fullscreen Whether the external process is a fullscreen program (e.g. curses based)
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
 * If ``fullscreen`` is set to ``true`` the external process is assumed to
 * be a fullscreen program (e.g. curses based) and the ui context is
 * restored accordingly.
 *
 * @rst
 * .. warning:: The editor core is blocked until this function returns.
 * @endrst
 *
 * @return The exit status of the forked process.
 */
VIS_EXPORT int vis_pipe(Vis *vis, File *file, Filerange *range, const char *argv[],
	void *stdout_context, ssize_t (*read_stdout)(void *stdout_context, char *data, size_t len),
	void *stderr_context, ssize_t (*read_stderr)(void *stderr_context, char *data, size_t len),
	bool fullscreen);

/**
 * Pipe a Filerange to an external process, return its exit status and capture
 * everything that is written to stdout/stderr.
 * @param vis The editor instance.
 * @param file The file to pipe.
 * @param range The file range to pipe.
 * @param argv Argument list, must be ``NULL`` terminated.
 * @param out Data written to ``stdout``, will be ``NUL`` terminated.
 * @param err Data written to ``stderr``, will be ``NUL`` terminated.
 * @param fullscreen Whether the external process is a fullscreen program (e.g. curses based)
 * @rst
 * .. warning:: The pointers stored in ``out`` and ``err`` need to be `free(3)`-ed
 * by the caller.
 * @endrst
 */
VIS_EXPORT int vis_pipe_collect(Vis *vis, File *file, Filerange *range, const char *argv[], char **out, char **err, bool fullscreen);

/**
 * Pipe a buffer to an external process, return its exit status and capture
 * everything that is written to stdout/stderr.
 * @param vis The editor instance.
 * @param buf The buffer to pipe.
 * @param argv Argument list, must be ``NULL`` terminated.
 * @param out Data written to ``stdout``, will be ``NUL`` terminated.
 * @param err Data written to ``stderr``, will be ``NUL`` terminated.
 * @param fullscreen Whether the external process is a fullscreen program (e.g. curses based)
 * @rst
 * .. warning:: The pointers stored in ``out`` and ``err`` need to be `free(3)`-ed
 * by the caller.
 * @endrst
 */
VIS_EXPORT int vis_pipe_buf_collect(Vis *vis, const char *buf, const char *argv[], char **out, char **err, bool fullscreen);

/** @} */

/*
---
## Keys
*/

/**
 * @defgroup vis_keys Vis Keys
 * @{
 */
/**
 * Advance to the start of the next symbolic key.
 * @param vis The editor instance.
 * @param keys The current symbolic key string.
 *
 * Given the start of a symbolic key, returns a pointer to the start of the one
 * immediately following it.
 */
VIS_EXPORT const char *vis_keys_next(Vis *vis, const char *keys);
/**
 * Convert next symbolic key to an Unicode code point, returns ``-1`` for unknown keys.
 * @param vis The editor instance.
 * @param keys The symbolic key string.
 */
VIS_EXPORT long vis_keys_codepoint(Vis *vis, const char *keys);
/**
 * Convert next symbolic key to a UTF-8 sequence.
 * @param vis The editor instance.
 * @param keys The symbolic key string.
 * @param utf8 Buffer to store the UTF-8 sequence.
 * @return Whether conversion was successful, if not ``utf8`` is left unmodified.
 * @rst
 * .. note:: Guarantees that ``utf8`` is NUL terminated on success.
 * @endrst
 */
VIS_EXPORT bool vis_keys_utf8(Vis *vis, const char *keys, char utf8[4+1]);
/**
 * Process symbolic keys as if they were user originated input.
 * @param vis The editor instance.
 * @param keys The symbolic key string to feed.
 */
VIS_EXPORT void vis_keys_feed(Vis *vis, const char *keys);
/** @} */

/*
---
## Miscellaneous
*/

/**
 * @defgroup vis_misc Vis Miscellaneous
 * @{
 */

/**
 * Get a regex object matching pattern.
 * @param vis The editor instance.
 * @param pattern The regex pattern to compile, if ``NULL`` the most recently used
 * one is substituted.
 * @return A Regex object or ``NULL`` in case of an error.
 * @rst
 * .. warning:: The caller must free the regex object using `text_regex_free`.
 * @endrst
 */
VIS_EXPORT Regex *vis_regex(Vis *vis, const char *pattern);

/**
 * Take an undo snapshot to which we can later revert.
 * @param vis The editor instance.
 * @param file The file for which to take a snapshot.
 * @rst
 * .. note:: Does nothing when invoked while replaying a macro.
 * @endrst
 */
VIS_EXPORT void vis_file_snapshot(Vis *vis, File *file);
/** @} */

/* TODO: expose proper API to iterate through files etc */
VIS_EXPORT Text *vis_text(Vis*);
VIS_EXPORT View *vis_view(Vis*);

#endif
