/** start by reading from the top of vis.c up until config.h is included */

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
enum {
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

static Mode vis_modes[VIS_MODE_LAST];

/* command recognized at the ':'-prompt. commands are found using a unique
 * prefix match. that is if a command should be available under an abbreviation
 * which is a prefix for another command it has to be added as an alias. the
 * long human readable name should always come first */
static Command cmds[] = {
	/* command name / optional alias, function,       options */
	{ { "bdelete"                  }, cmd_bdelete,    CMD_OPT_FORCE },
	{ { "edit"                     }, cmd_edit,       CMD_OPT_FORCE },
	{ { "help"                     }, cmd_help,       CMD_OPT_NONE  },
	{ { "new"                      }, cmd_new,        CMD_OPT_NONE  },
	{ { "open"                     }, cmd_open,       CMD_OPT_NONE  },
	{ { "qall"                     }, cmd_qall,       CMD_OPT_FORCE },
	{ { "quit", "q"                }, cmd_quit,       CMD_OPT_FORCE },
	{ { "read",                    }, cmd_read,       CMD_OPT_FORCE },
	{ { "saveas"                   }, cmd_saveas,     CMD_OPT_FORCE },
	{ { "set",                     }, cmd_set,        CMD_OPT_ARGS  },
	{ { "split"                    }, cmd_split,      CMD_OPT_NONE  },
	{ { "substitute", "s"          }, cmd_substitute, CMD_OPT_NONE  },
	{ { "vnew"                     }, cmd_vnew,       CMD_OPT_NONE  },
	{ { "vsplit",                  }, cmd_vsplit,     CMD_OPT_NONE  },
	{ { "wq",                      }, cmd_wq,         CMD_OPT_FORCE },
	{ { "write", "w"               }, cmd_write,      CMD_OPT_FORCE },
	{ { "xit",                     }, cmd_xit,        CMD_OPT_FORCE },
	{ { "earlier"                  }, cmd_earlier_later, CMD_OPT_NONE },
	{ { "later"                    }, cmd_earlier_later, CMD_OPT_NONE },
	{ { "!",                       }, cmd_filter,     CMD_OPT_NONE  },
	{ /* array terminator */                                        },
};

/* called before any other keybindings are checked, if the function returns false
 * the key is completely ignored. */
static bool vis_keypress(const char *key) {
	editor_info_hide(vis);
	if (vis->recording && key)
		macro_append(vis->recording, key);
	return true;
}

#define ALIAS(name) .alias = name,
#define ACTION(id) .action = &vis_action[VIS_ACTION_##id],

enum {
	VIS_ACTION_EDITOR_SUSPEND,
	VIS_ACTION_CURSOR_CHAR_PREV,
	VIS_ACTION_CURSOR_CHAR_NEXT,
	VIS_ACTION_CURSOR_WORD_START_PREV,
	VIS_ACTION_CURSOR_WORD_START_NEXT,
	VIS_ACTION_CURSOR_WORD_END_PREV,
	VIS_ACTION_CURSOR_WORD_END_NEXT,
	VIS_ACTION_CURSOR_LONGWORD_START_PREV,
	VIS_ACTION_CURSOR_LONGWORD_START_NEXT,
	VIS_ACTION_CURSOR_LONGWORD_END_PREV,
	VIS_ACTION_CURSOR_LONGWORD_END_NEXT,
	VIS_ACTION_CURSOR_LINE_UP,
	VIS_ACTION_CURSOR_LINE_DOWN,
	VIS_ACTION_CURSOR_LINE_START,
	VIS_ACTION_CURSOR_LINE_FINISH,
	VIS_ACTION_CURSOR_LINE_BEGIN,
	VIS_ACTION_CURSOR_LINE_END,
	VIS_ACTION_CURSOR_SCREEN_LINE_UP,
	VIS_ACTION_CURSOR_SCREEN_LINE_DOWN,
	VIS_ACTION_CURSOR_SCREEN_LINE_BEGIN,
	VIS_ACTION_CURSOR_SCREEN_LINE_MIDDLE,
	VIS_ACTION_CURSOR_SCREEN_LINE_END,
	VIS_ACTION_CURSOR_BRACKET_MATCH,
	VIS_ACTION_CURSOR_PARAGRAPH_PREV,
	VIS_ACTION_CURSOR_PARAGRAPH_NEXT,
	VIS_ACTION_CURSOR_SENTENCE_PREV,
	VIS_ACTION_CURSOR_SENTENCE_NEXT,
	VIS_ACTION_CURSOR_FUNCTION_START_PREV,
	VIS_ACTION_CURSOR_FUNCTION_END_PREV,
	VIS_ACTION_CURSOR_FUNCTION_START_NEXT,
	VIS_ACTION_CURSOR_FUNCTION_END_NEXT,
	VIS_ACTION_CURSOR_COLUMN,
	VIS_ACTION_CURSOR_LINE_FIRST,
	VIS_ACTION_CURSOR_LINE_LAST,
	VIS_ACTION_CURSOR_WINDOW_LINE_TOP,
	VIS_ACTION_CURSOR_WINDOW_LINE_MIDDLE,
	VIS_ACTION_CURSOR_WINDOW_LINE_BOTTOM,
	VIS_ACTION_CURSOR_SEARCH_FORWARD,
	VIS_ACTION_CURSOR_SEARCH_BACKWARD,
	VIS_ACTION_CURSOR_SEARCH_WORD_FORWARD,
	VIS_ACTION_CURSOR_SEARCH_WORD_BACKWARD,
	VIS_ACTION_WINDOW_PAGE_UP,
	VIS_ACTION_WINDOW_PAGE_DOWN,
	VIS_ACTION_WINDOW_HALFPAGE_UP,
	VIS_ACTION_WINDOW_HALFPAGE_DOWN,
	VIS_ACTION_MODE_NORMAL,
	VIS_ACTION_MODE_VISUAL,
	VIS_ACTION_MODE_VISUAL_LINE,
	VIS_ACTION_MODE_INSERT,
	VIS_ACTION_MODE_REPLACE,
	VIS_ACTION_MODE_OPERATOR_PENDING,
	VIS_ACTION_DELETE_CHAR_PREV,
	VIS_ACTION_DELETE_CHAR_NEXT,
	VIS_ACTION_DELETE_LINE_BEGIN,
	VIS_ACTION_DELETE_WORD_PREV,
	VIS_ACTION_JUMPLIST_PREV,
	VIS_ACTION_JUMPLIST_NEXT,
	VIS_ACTION_CHANGELIST_PREV,
	VIS_ACTION_CHANGELIST_NEXT,
	VIS_ACTION_UNDO,
	VIS_ACTION_REDO,
	VIS_ACTION_EARLIER,
	VIS_ACTION_LATER,
	VIS_ACTION_MACRO_RECORD,
	VIS_ACTION_MACRO_REPLAY,
	VIS_ACTION_MARK_SET,
	VIS_ACTION_MARK_GOTO,
	VIS_ACTION_MARK_GOTO_LINE,
	VIS_ACTION_REDRAW,
	VIS_ACTION_REPLACE_CHAR,
	VIS_ACTION_TOTILL_REPEAT,
	VIS_ACTION_TOTILL_REVERSE,
	VIS_ACTION_SEARCH_FORWARD,
	VIS_ACTION_SEARCH_BACKWARD,
	VIS_ACTION_TILL_LEFT,
	VIS_ACTION_TILL_RIGHT,
	VIS_ACTION_TO_LEFT,
	VIS_ACTION_TO_RIGHT,
	VIS_ACTION_REGISTER,
	VIS_ACTION_OPERATOR_CHANGE,
	VIS_ACTION_OPERATOR_DELETE,
	VIS_ACTION_OPERATOR_YANK,
	VIS_ACTION_OPERATOR_SHIFT_LEFT,
	VIS_ACTION_OPERATOR_SHIFT_RIGHT,
	VIS_ACTION_OPERATOR_CASE_LOWER,
	VIS_ACTION_OPERATOR_CASE_UPPER,
	VIS_ACTION_OPERATOR_CASE_SWAP,
	VIS_ACTION_COUNT,
	VIS_ACTION_INSERT_NEWLINE,
	VIS_ACTION_INSERT_TAB,
	VIS_ACTION_INSERT_VERBATIM,
	VIS_ACTION_INSERT_REGISTER,
	VIS_ACTION_WINDOW_NEXT,
	VIS_ACTION_WINDOW_PREV,
	VIS_ACTION_OPEN_LINE_ABOVE,
	VIS_ACTION_OPEN_LINE_BELOW,
	VIS_ACTION_JOIN_LINE_BELOW,
	VIS_ACTION_JOIN_LINES,
	VIS_ACTION_PROMPT_SHOW,
	VIS_ACTION_PROMPT_BACKSPACE,
	VIS_ACTION_PROMPT_ENTER,
	VIS_ACTION_PROMPT_SHOW_VISUAL,
	VIS_ACTION_REPEAT,
	VIS_ACTION_SELECTION_FLIP,
	VIS_ACTION_SELECTION_RESTORE,
	VIS_ACTION_WINDOW_REDRAW_TOP,
	VIS_ACTION_WINDOW_REDRAW_CENTER,
	VIS_ACTION_WINDOW_REDRAW_BOTTOM,
	VIS_ACTION_WINDOW_SLIDE_UP,
	VIS_ACTION_WINDOW_SLIDE_DOWN,
	VIS_ACTION_PUT_AFTER,
	VIS_ACTION_PUT_BEFORE,
	VIS_ACTION_PUT_AFTER_END,
	VIS_ACTION_PUT_BEFORE_END,
	VIS_ACTION_CURSOR_SELECT_WORD,
	VIS_ACTION_CURSORS_NEW_LINE_ABOVE,
	VIS_ACTION_CURSORS_NEW_LINE_BELOW,
	VIS_ACTION_CURSORS_NEW_LINES_BEGIN,
	VIS_ACTION_CURSORS_NEW_LINES_END,
	VIS_ACTION_CURSORS_NEW_MATCH_NEXT,
	VIS_ACTION_CURSORS_NEW_MATCH_SKIP,
	VIS_ACTION_CURSORS_ALIGN,
	VIS_ACTION_CURSORS_REMOVE_ALL,
	VIS_ACTION_CURSORS_REMOVE_LAST,
	VIS_ACTION_TEXT_OBJECT_WORD_OUTER,
	VIS_ACTION_TEXT_OBJECT_WORD_INNER,
	VIS_ACTION_TEXT_OBJECT_LONGWORD_OUTER,
	VIS_ACTION_TEXT_OBJECT_LONGWORD_INNER,
	VIS_ACTION_TEXT_OBJECT_SENTENCE,
	VIS_ACTION_TEXT_OBJECT_PARAGRAPH,
	VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_OUTER,
	VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_INNER,
	VIS_ACTION_TEXT_OBJECT_PARANTHESE_OUTER,
	VIS_ACTION_TEXT_OBJECT_PARANTHESE_INNER,
	VIS_ACTION_TEXT_OBJECT_ANGLE_BRACKET_OUTER,
	VIS_ACTION_TEXT_OBJECT_ANGLE_BRACKET_INNER,
	VIS_ACTION_TEXT_OBJECT_CURLY_BRACKET_OUTER,
	VIS_ACTION_TEXT_OBJECT_CURLY_BRACKET_INNER,
	VIS_ACTION_TEXT_OBJECT_QUOTE_OUTER,
	VIS_ACTION_TEXT_OBJECT_QUOTE_INNER,
	VIS_ACTION_TEXT_OBJECT_SINGLE_QUOTE_OUTER,
	VIS_ACTION_TEXT_OBJECT_SINGLE_QUOTE_INNER,
	VIS_ACTION_TEXT_OBJECT_BACKTICK_OUTER,
	VIS_ACTION_TEXT_OBJECT_BACKTICK_INNER,
	VIS_ACTION_TEXT_OBJECT_ENTIRE_OUTER,
	VIS_ACTION_TEXT_OBJECT_ENTIRE_INNER,
	VIS_ACTION_TEXT_OBJECT_FUNCTION_OUTER,
	VIS_ACTION_TEXT_OBJECT_FUNCTION_INNER,
	VIS_ACTION_TEXT_OBJECT_LINE_OUTER,
	VIS_ACTION_TEXT_OBJECT_LINE_INNER,
	VIS_ACTION_MOTION_CHARWISE,
	VIS_ACTION_MOTION_LINEWISE,
	VIS_ACTION_NOP,
};

static KeyAction vis_action[] = {
	[VIS_ACTION_EDITOR_SUSPEND] = {
		"editor-suspend",
		"Suspend the editor",
		suspend,
	},
	[VIS_ACTION_CURSOR_CHAR_PREV] = {
		"cursor-char-prev",
		"Move cursor left, to the previous character",
		movement, { .i = MOVE_CHAR_PREV }
	},
	[VIS_ACTION_CURSOR_CHAR_NEXT] = {
		"cursor-char-next",
		"Move cursor right, to the next character",
		movement, { .i = MOVE_CHAR_NEXT }
	},
	[VIS_ACTION_CURSOR_WORD_START_PREV] = {
		"cursor-word-start-prev",
		"Move cursor words backwards",
		movement, { .i = MOVE_WORD_START_PREV }
	},
	[VIS_ACTION_CURSOR_WORD_START_NEXT] = {
		"cursor-word-start-next",
		"Move cursor words forwards",
		movement, { .i = MOVE_WORD_START_NEXT }
	},
	[VIS_ACTION_CURSOR_WORD_END_PREV] = {
		"cursor-word-end-prev",
		"Move cursor backwards to the end of word",
		movement, { .i = MOVE_WORD_END_PREV }
	},
	[VIS_ACTION_CURSOR_WORD_END_NEXT] = {
		"cursor-word-end-next",
		"Move cursor forward to the end of word",
		movement, { .i = MOVE_WORD_END_NEXT }
	},
	[VIS_ACTION_CURSOR_LONGWORD_START_PREV] = {
		"cursor-longword-start-prev",
		"Move cursor WORDS backwards",
		movement, { .i = MOVE_LONGWORD_START_PREV }
	},
	[VIS_ACTION_CURSOR_LONGWORD_START_NEXT] = {
		"cursor-longword-start-next",
		"Move cursor WORDS forwards",
		movement, { .i = MOVE_LONGWORD_START_NEXT }
	},
	[VIS_ACTION_CURSOR_LONGWORD_END_PREV] = {
		"cursor-longword-end-prev",
		"Move cursor backwards to the end of WORD",
		movement, { .i = MOVE_LONGWORD_END_PREV }
	},
	[VIS_ACTION_CURSOR_LONGWORD_END_NEXT] = {
		"cursor-longword-end-next",
		"Move cursor forward to the end of WORD",
		movement, { .i = MOVE_LONGWORD_END_NEXT }
	},
	[VIS_ACTION_CURSOR_LINE_UP] = {
		"cursor-line-up",
		"Move cursor line upwards",
		movement, { .i = MOVE_LINE_UP }
	},
	[VIS_ACTION_CURSOR_LINE_DOWN] = {
		"cursor-line-down",
		"Move cursor line downwards",
		movement, { .i = MOVE_LINE_DOWN }
	},
	[VIS_ACTION_CURSOR_LINE_START] = {
		"cursor-line-start",
		"Move cursor to first non-blank character of the line",
		movement, { .i = MOVE_LINE_START }
	},
	[VIS_ACTION_CURSOR_LINE_FINISH] = {
		"cursor-line-finish",
		"Move cursor to last non-blank character of the line",
		movement, { .i = MOVE_LINE_FINISH }
	},
	[VIS_ACTION_CURSOR_LINE_BEGIN] = {
		"cursor-line-begin",
		"Move cursor to first character of the line",
		movement, { .i = MOVE_LINE_BEGIN }
	},
	[VIS_ACTION_CURSOR_LINE_END] = {
		"cursor-line-end",
		"Move cursor to end of the line",
		movement, { .i = MOVE_LINE_LASTCHAR }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_UP] = {
		"cursor-sceenline-up",
		"Move cursor screen/display line upwards",
		movement, { .i = MOVE_SCREEN_LINE_UP }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_DOWN] = {
		"cursor-screenline-down",
		"Move cursor screen/display line downwards",
		movement, { .i = MOVE_SCREEN_LINE_DOWN }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_BEGIN] = {
		"cursor-screenline-begin",
		"Move cursor to beginning of screen/display line",
		movement, { .i = MOVE_SCREEN_LINE_BEGIN }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_MIDDLE] = {
		"cursor-screenline-middle",
		"Move cursor to middle of screen/display line",
		movement, { .i = MOVE_SCREEN_LINE_MIDDLE }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_END] = {
		"cursor-screenline-end",
		"Move cursor to end of screen/display line",
		movement, { .i = MOVE_SCREEN_LINE_END }
	},
	[VIS_ACTION_CURSOR_BRACKET_MATCH] = {
		"cursor-match-bracket",
		"Match corresponding symbol if cursor is on a bracket character",
		movement, { .i = MOVE_BRACKET_MATCH }
	},
	[VIS_ACTION_CURSOR_PARAGRAPH_PREV] = {
		"cursor-paragraph-prev",
		"Move cursor paragraph backward",
		movement, { .i = MOVE_PARAGRAPH_PREV }
	},
	[VIS_ACTION_CURSOR_PARAGRAPH_NEXT] = {
		"cursor-paragraph-next",
		"Move cursor paragraph forward",
		movement, { .i = MOVE_PARAGRAPH_NEXT }
	},
	[VIS_ACTION_CURSOR_SENTENCE_PREV] = {
		"cursor-sentence-prev",
		"Move cursor sentence backward",
		movement, { .i = MOVE_SENTENCE_PREV }
	},
	[VIS_ACTION_CURSOR_SENTENCE_NEXT] = {
		"cursor-sentence-next",
		"Move cursor sentence forward",
		movement, { .i = MOVE_SENTENCE_NEXT }
	},
	[VIS_ACTION_CURSOR_FUNCTION_START_PREV] = {
		"cursor-function-start-prev",
		"Move cursor backwards to start of function",
		movement, { .i = MOVE_FUNCTION_START_PREV }
	},
	[VIS_ACTION_CURSOR_FUNCTION_START_NEXT] = {
		"cursor-function-start-next",
		"Move cursor forwards to start of function",
		movement, { .i = MOVE_FUNCTION_START_NEXT }
	},
	[VIS_ACTION_CURSOR_FUNCTION_END_PREV] = {
		"cursor-function-end-prev",
		"Move cursor backwards to end of function",
		movement, { .i = MOVE_FUNCTION_END_PREV }
	},
	[VIS_ACTION_CURSOR_FUNCTION_END_NEXT] = {
		"cursor-function-end-next",
		"Move cursor forwards to end of function",
		movement, { .i = MOVE_FUNCTION_END_NEXT }
	},
	[VIS_ACTION_CURSOR_COLUMN] = {
		"cursor-column",
		"Move cursor to given column of current line",
		movement, { .i = MOVE_COLUMN }
	},
	[VIS_ACTION_CURSOR_LINE_FIRST] = {
		"cursor-line-first",
		"Move cursor to given line (defaults to first)",
		gotoline, { .i = -1 }
	},
	[VIS_ACTION_CURSOR_LINE_LAST] = {
		"cursor-line-last",
		"Move cursor to given line (defaults to last)",
		gotoline, { .i = +1 }
	},
	[VIS_ACTION_CURSOR_WINDOW_LINE_TOP] = {
		"cursor-window-line-top",
		"Move cursor to top line of the window",
		movement, { .i = MOVE_WINDOW_LINE_TOP }
	},
	[VIS_ACTION_CURSOR_WINDOW_LINE_MIDDLE] = {
		"cursor-window-line-middle",
		"Move cursor to middle line of the window",
		movement, { .i = MOVE_WINDOW_LINE_MIDDLE }
	},
	[VIS_ACTION_CURSOR_WINDOW_LINE_BOTTOM] = {
		"cursor-window-line-bottom",
		"Move cursor to bottom line of the window",
		movement, { .i = MOVE_WINDOW_LINE_BOTTOM }
	},
	[VIS_ACTION_CURSOR_SEARCH_FORWARD] = {
		"cursor-search-forward",
		"Move cursor to bottom line of the window",
		movement, { .i = MOVE_SEARCH_FORWARD }
	},
	[VIS_ACTION_CURSOR_SEARCH_BACKWARD] = {
		"cursor-search-backward",
		"Move cursor to bottom line of the window",
		movement, { .i = MOVE_SEARCH_BACKWARD }
	},
	[VIS_ACTION_CURSOR_SEARCH_WORD_FORWARD] = {
		"cursor-search-word-forward",
		"Move cursor to next occurence of the word under cursor",
		movement, { .i = MOVE_SEARCH_WORD_FORWARD }
	},
	[VIS_ACTION_CURSOR_SEARCH_WORD_BACKWARD] = {
		"cursor-search-word-backward",
		"Move cursor to previous occurence of the word under cursor",
		 movement, { .i = MOVE_SEARCH_WORD_BACKWARD }
	},
	[VIS_ACTION_WINDOW_PAGE_UP] = {
		"window-page-up",
		"Scroll window pages backwards (upwards)",
		wscroll, { .i = -PAGE }
	},
	[VIS_ACTION_WINDOW_HALFPAGE_UP] = {
		"window-halfpage-up",
		"Scroll window half pages backwards (upwards)",
		wscroll, { .i = -PAGE_HALF }
	},
	[VIS_ACTION_WINDOW_PAGE_DOWN] = {
		"window-page-down",
		"Scroll window pages forwards (downwards)",
		wscroll, { .i = +PAGE }
	},
	[VIS_ACTION_WINDOW_HALFPAGE_DOWN] = {
		"window-halfpage-down",
		"Scroll window half pages forwards (downwards)",
		wscroll, { .i = +PAGE_HALF }
	},
	[VIS_ACTION_MODE_NORMAL] = {
		"vis-mode-normal",
		"Enter normal mode",
		switchmode, { .i = VIS_MODE_NORMAL }
	},
	[VIS_ACTION_MODE_VISUAL] = {
		"vis-mode-visual-charwise",
		"Enter characterwise visual mode",
		switchmode, { .i = VIS_MODE_VISUAL }
	},
	[VIS_ACTION_MODE_VISUAL_LINE] = {
		"vis-mode-visual-linewise",
		"Enter linewise visual mode",
		switchmode, { .i = VIS_MODE_VISUAL_LINE }
	},
	[VIS_ACTION_MODE_INSERT] = {
		"vis-mode-insert",
		"Enter insert mode",
		switchmode, { .i = VIS_MODE_INSERT }
	},
	[VIS_ACTION_MODE_REPLACE] = {
		"vis-mode-replace",
		"Enter replace mode",
		switchmode, { .i = VIS_MODE_REPLACE }
	},
	[VIS_ACTION_MODE_OPERATOR_PENDING] = {
		"vis-mode-operator-pending",
		"Enter to operator pending mode",
		switchmode, { .i = VIS_MODE_OPERATOR }
	},
	[VIS_ACTION_DELETE_CHAR_PREV] = {
		"delete-char-prev",
		"Delete the previous character",
		delete, { .i = MOVE_CHAR_PREV }
	},
	[VIS_ACTION_DELETE_CHAR_NEXT] = {
		"delete-char-next",
		"Delete the next character",
		delete, { .i = MOVE_CHAR_NEXT }
	},
	[VIS_ACTION_DELETE_LINE_BEGIN] = {
		"delete-line-begin",
		"Delete until the start of the current line",
		delete, { .i = MOVE_LINE_BEGIN }
	},
	[VIS_ACTION_DELETE_WORD_PREV] = {
		"delete-word-prev",
		"Delete the previous WORD",
		delete, { .i = MOVE_LONGWORD_START_PREV }
	},
	[VIS_ACTION_JUMPLIST_PREV] = {
		"jumplist-prev",
		"Go to older cursor position in jump list",
		jumplist, { .i = -1 }
	},
	[VIS_ACTION_JUMPLIST_NEXT] = {
		"jumplist-next",
		"Go to newer cursor position in jump list",
		jumplist, { .i = +1 }
	},
	[VIS_ACTION_CHANGELIST_PREV] = {
		"changelist-prev",
		"Go to older cursor position in change list",
		changelist, { .i = -1 }
	},
	[VIS_ACTION_CHANGELIST_NEXT] = {
		"changelist-next",
		"Go to newer cursor position in change list",
		changelist, { .i = +1 }
	},
	[VIS_ACTION_UNDO] = {
		"editor-undo",
		"Undo last change",
		undo,
	},
	[VIS_ACTION_REDO] = {
		"editor-redo",
		"Redo last change",
		redo,
	},
	[VIS_ACTION_EARLIER] = {
		"editor-earlier",
		"Goto older text state",
		earlier,
	},
	[VIS_ACTION_LATER] = {
		"editor-later",
		"Goto newer text state",
		later,
	},
	[VIS_ACTION_MACRO_RECORD] = {
		"macro-record",
		"Record macro into given register",
		macro_record,
	},
	[VIS_ACTION_MACRO_REPLAY] = {
		"macro-replay",
		"Replay macro, execute the content of the given register",
		macro_replay,
	},
	[VIS_ACTION_MARK_SET] = {
		"mark-set",
		"Set given mark at current cursor position",
		mark_set,
	},
	[VIS_ACTION_MARK_GOTO] = {
		"mark-goto",
		"Goto the position of the given mark",
		mark,
	},
	[VIS_ACTION_MARK_GOTO_LINE] = {
		"mark-goto-line",
		"Goto first non-blank character of the line containing the given mark",
		mark_line,
	},
	[VIS_ACTION_REDRAW] = {
		"editor-redraw",
		"Redraw current editor content",
		 call, { .f = editor_draw }
	},
	[VIS_ACTION_REPLACE_CHAR] = {
		"replace-char",
		"Replace the character under the cursor",
		replace,
	},
	[VIS_ACTION_TOTILL_REPEAT] = {
		"totill-repeat",
		"Repeat latest to/till motion",
		totill_repeat,
	},
	[VIS_ACTION_TOTILL_REVERSE] = {
		"totill-reverse",
		"Repeat latest to/till motion but in opposite direction",
		totill_reverse,
	},
	[VIS_ACTION_SEARCH_FORWARD] = {
		"search-forward",
		"Search forward",
		prompt_search, { .s = "/" }
	},
	[VIS_ACTION_SEARCH_BACKWARD] = {
		"search-backward",
		"Search backward",
		prompt_search, { .s = "?" }
	},
	[VIS_ACTION_TILL_LEFT] = {
		"till-left",
		"Till after the occurrence of character to the left",
		movement_key, { .i = MOVE_LEFT_TILL }
	},
	[VIS_ACTION_TILL_RIGHT] = {
		"till-right",
		"Till before the occurrence of character to the right",
		movement_key, { .i = MOVE_RIGHT_TILL }
	},
	[VIS_ACTION_TO_LEFT] = {
		"to-left",
		"To the first occurrence of character to the left",
		movement_key, { .i = MOVE_LEFT_TO }
	},
	[VIS_ACTION_TO_RIGHT] = {
		"to-right",
		"To the first occurrence of character to the right",
		movement_key, { .i = MOVE_RIGHT_TO }
	},
	[VIS_ACTION_REGISTER] = {
		"register",
		"Use given register for next operator",
		reg,
	},
	[VIS_ACTION_OPERATOR_CHANGE] = {
		"vis-operator-change",
		"Change operator",
		operator, { .i = OP_CHANGE }
	},
	[VIS_ACTION_OPERATOR_DELETE] = {
		"vis-operator-delete",
		"Delete operator",
		operator, { .i = OP_DELETE }
	},
	[VIS_ACTION_OPERATOR_YANK] = {
		"vis-operator-yank",
		"Yank operator",
		operator, { .i = OP_YANK }
	},
	[VIS_ACTION_OPERATOR_SHIFT_LEFT] = {
		"vis-operator-shift-left",
		"Shift left operator",
		operator, { .i = OP_SHIFT_LEFT }
	},
	[VIS_ACTION_OPERATOR_SHIFT_RIGHT] = {
		"vis-operator-shift-right",
		"Shift right operator",
		operator, { .i = OP_SHIFT_RIGHT }
	},
	[VIS_ACTION_OPERATOR_CASE_LOWER] = {
		"vis-operator-case-lower",
		"Lowercase operator",
		changecase, { .i = -1 }
	},
	[VIS_ACTION_OPERATOR_CASE_UPPER] = {
		"vis-operator-case-upper",
		"Uppercase operator",
		changecase, { .i = +1 }
	},
	[VIS_ACTION_OPERATOR_CASE_SWAP] = {
		"vis-operator-case-swap",
		"Swap case operator",
		changecase, { .i = 0 }
	},
	[VIS_ACTION_COUNT] = {
		"vis-count",
		"Count specifier",
		count,
	},
	[VIS_ACTION_INSERT_NEWLINE] = {
		"insert-newline",
		"Insert a line break (depending on file type)",
		insert_newline,
	},
	[VIS_ACTION_INSERT_TAB] = {
		"insert-tab",
		"Insert a tab (might be converted to spaces)",
		insert_tab,
	},
	[VIS_ACTION_INSERT_VERBATIM] = {
		"insert-verbatim",
		"Insert Unicode character based on code point",
		insert_verbatim,
	},
	[VIS_ACTION_INSERT_REGISTER] = {
		"insert-register",
		"Insert specified register content",
		insert_register,
	},
	[VIS_ACTION_WINDOW_NEXT] = {
		"window-next",
		"Focus next window",
		call, { .f = editor_window_next }
	},
	[VIS_ACTION_WINDOW_PREV] = {
		"window-prev",
		"Focus previous window",
		call, { .f = editor_window_prev }
	},
	[VIS_ACTION_OPEN_LINE_ABOVE] = {
		"open-line-above",
		"Begin a new line above the cursor",
		openline, { .i = MOVE_LINE_PREV }
	},
	[VIS_ACTION_OPEN_LINE_BELOW] = {
		"open-line-below",
		"Begin a new line below the cursor",
		openline, { .i = MOVE_LINE_NEXT }
	},
	[VIS_ACTION_JOIN_LINE_BELOW] = {
		"join-line-below",
		"Join line(s)",
		join, { .i = MOVE_LINE_NEXT },
	},
	[VIS_ACTION_JOIN_LINES] = {
		"join-lines",
		"Join selected lines",
		operator, { .i = OP_JOIN }
	},
	[VIS_ACTION_PROMPT_SHOW] = {
		"prompt-show",
		"Show editor command line prompt",
		prompt_cmd, { .s = "" }
	},
	[VIS_ACTION_PROMPT_BACKSPACE] = {
		"prompt-backspace",
		"Delete previous character in prompt",
		prompt_backspace
	},
	[VIS_ACTION_PROMPT_ENTER] = {
		"prompt-enter",
		"Execute current prompt content",
		prompt_enter
	},
	[VIS_ACTION_PROMPT_SHOW_VISUAL] = {
		"prompt-show-visual",
		"Show editor command line prompt in visual mode",
		prompt_cmd, { .s = "'<,'>" }
	},
	[VIS_ACTION_REPEAT] = {
		"editor-repeat",
		"Repeat latest editor command",
		repeat
	},
	[VIS_ACTION_SELECTION_FLIP] = {
		"selection-flip",
		"Flip selection, move cursor to other end",
		selection_end,
	},
	[VIS_ACTION_SELECTION_RESTORE] = {
		"selection-restore",
		"Restore last selection",
		selection_restore,
	},
	[VIS_ACTION_WINDOW_REDRAW_TOP] = {
		"window-redraw-top",
		"Redraw cursor line at the top of the window",
		window, { .w = view_redraw_top }
	},
	[VIS_ACTION_WINDOW_REDRAW_CENTER] = {
		"window-redraw-center",
		"Redraw cursor line at the center of the window",
		window, { .w = view_redraw_center }
	},
	[VIS_ACTION_WINDOW_REDRAW_BOTTOM] = {
		"window-redraw-bottom",
		"Redraw cursor line at the bottom of the window",
		window, { .w = view_redraw_bottom }
	},
	[VIS_ACTION_WINDOW_SLIDE_UP] = {
		"window-slide-up",
		"Slide window content upwards",
		wslide, { .i = -1 }
	},
	[VIS_ACTION_WINDOW_SLIDE_DOWN] = {
		"window-slide-down",
		"Slide window content downwards",
		wslide, { .i = +1 }
	},
	[VIS_ACTION_PUT_AFTER] = {
		"put-after",
		"Put text after the cursor",
		put, { .i = PUT_AFTER }
	},
	[VIS_ACTION_PUT_BEFORE] = {
		"put-before",
		"Put text before the cursor",
		put, { .i = PUT_BEFORE }
	},
	[VIS_ACTION_PUT_AFTER_END] = {
		"put-after-end",
		"Put text after the cursor, place cursor after new text",
		put, { .i = PUT_AFTER_END }
	},
	[VIS_ACTION_PUT_BEFORE_END] = {
		"put-before-end",
		"Put text before the cursor, place cursor after new text",
		put, { .i = PUT_BEFORE_END }
	},
	[VIS_ACTION_CURSOR_SELECT_WORD] = {
		"cursors-select-word",
		"Select word under cursor",
		cursors_select,
	},
	[VIS_ACTION_CURSORS_NEW_LINE_ABOVE] = {
		"cursors-new-lines-above",
		"Create a new cursor on the line above",
		cursors_new, { .i = -1 }
	},
	[VIS_ACTION_CURSORS_NEW_LINE_BELOW] = {
		"cursor-new-lines-below",
		"Create a new cursor on the line below",
		cursors_new, { .i = +1 }
	},
	[VIS_ACTION_CURSORS_NEW_LINES_BEGIN] = {
		"cursors-new-lines-begin",
		"Create a new cursor at the start of every line covered by selection",
		cursors_split, { .i = -1 }
	},
	[VIS_ACTION_CURSORS_NEW_LINES_END] = {
		"cursors-new-lines-end",
		"Create a new cursor at the end of every line covered by selection",
		cursors_split, { .i = +1 }
	},
	[VIS_ACTION_CURSORS_NEW_MATCH_NEXT] = {
		"cursors-new-match-next",
		"Select the next region matching the current selection",
		cursors_select_next
	},
	[VIS_ACTION_CURSORS_NEW_MATCH_SKIP] = {
		"cursors-new-match-skip",
		"Clear current selection, but select next match",
		cursors_select_skip,
	},
	[VIS_ACTION_CURSORS_ALIGN] = {
		"cursors-align",
		"Try to align all cursors on the same column",
		cursors_align,
	},
	[VIS_ACTION_CURSORS_REMOVE_ALL] = {
		"cursors-remove-all",
		"Remove all but the primary cursor",
		cursors_clear,
	},
	[VIS_ACTION_CURSORS_REMOVE_LAST] = {
		"cursors-remove-last",
		"Remove least recently created cursor",
		cursors_remove,
	},
	[VIS_ACTION_TEXT_OBJECT_WORD_OUTER] = {
		"text-object-word-outer",
		"A word leading and trailing whitespace included",
		textobj, { .i = TEXT_OBJ_OUTER_WORD }
	},
	[VIS_ACTION_TEXT_OBJECT_WORD_INNER] = {
		"text-object-word-inner",
		"A word leading and trailing whitespace excluded",
		textobj, { .i = TEXT_OBJ_INNER_WORD }
	},
	[VIS_ACTION_TEXT_OBJECT_LONGWORD_OUTER] = {
		"text-object-longword-outer",
		"A WORD leading and trailing whitespace included",
		 textobj, { .i = TEXT_OBJ_OUTER_LONGWORD }
	},
	[VIS_ACTION_TEXT_OBJECT_LONGWORD_INNER] = {
		"text-object-longword-inner",
		"A WORD leading and trailing whitespace excluded",
		 textobj, { .i = TEXT_OBJ_INNER_LONGWORD }
	},
	[VIS_ACTION_TEXT_OBJECT_SENTENCE] = {
		"text-object-sentence",
		"A sentence",
		textobj, { .i = TEXT_OBJ_SENTENCE }
	},
	[VIS_ACTION_TEXT_OBJECT_PARAGRAPH] = {
		"text-object-paragraph",
		"A paragraph",
		textobj, { .i = TEXT_OBJ_PARAGRAPH }
	},
	[VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_OUTER] = {
		"text-object-square-bracket-outer",
		"[] block (outer variant)",
		textobj, { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_INNER] = {
		"text-object-square-bracket-inner",
		"[] block (inner variant)",
		textobj, { .i = TEXT_OBJ_INNER_SQUARE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_PARANTHESE_OUTER] = {
		"text-object-parentheses-outer",
		"() block (outer variant)",
		textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE }
	},
	[VIS_ACTION_TEXT_OBJECT_PARANTHESE_INNER] = {
		"text-object-parentheses-inner",
		"() block (inner variant)",
		textobj, { .i = TEXT_OBJ_INNER_PARANTHESE }
	},
	[VIS_ACTION_TEXT_OBJECT_ANGLE_BRACKET_OUTER] = {
		"text-object-angle-bracket-outer",
		"<> block (outer variant)",
		textobj, { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_ANGLE_BRACKET_INNER] = {
		"text-object-angle-bracket-inner",
		"<> block (inner variant)",
		textobj, { .i = TEXT_OBJ_INNER_ANGLE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_CURLY_BRACKET_OUTER] = {
		"text-object-curly-bracket-outer",
		"{} block (outer variant)",
		textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_CURLY_BRACKET_INNER] = {
		"text-object-curly-bracket-inner",
		"{} block (inner variant)",
		textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_QUOTE_OUTER] = {
		"text-object-quote-outer",
		"A quoted string, including the quotation marks",
		textobj, { .i = TEXT_OBJ_OUTER_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_QUOTE_INNER] = {
		"text-object-quote-inner",
		"A quoted string, excluding the quotation marks",
		textobj, { .i = TEXT_OBJ_INNER_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_SINGLE_QUOTE_OUTER] = {
		"text-object-single-quote-outer",
		"A single quoted string, including the quotation marks",
		textobj, { .i = TEXT_OBJ_OUTER_SINGLE_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_SINGLE_QUOTE_INNER] = {
		"text-object-single-quote-inner",
		"A single quoted string, excluding the quotation marks",
		textobj, { .i = TEXT_OBJ_INNER_SINGLE_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_BACKTICK_OUTER] = {
		"text-object-backtick-outer",
		"A backtick delimited string (outer variant)",
		textobj, { .i = TEXT_OBJ_OUTER_BACKTICK }
	},
	[VIS_ACTION_TEXT_OBJECT_BACKTICK_INNER] = {
		"text-object-backtick-inner",
		"A backtick delimited string (inner variant)",
		textobj, { .i = TEXT_OBJ_INNER_BACKTICK }
	},
	[VIS_ACTION_TEXT_OBJECT_ENTIRE_OUTER] = {
		"text-object-entire-outer",
		"The whole text content",
		textobj, { .i = TEXT_OBJ_OUTER_ENTIRE }
	},
	[VIS_ACTION_TEXT_OBJECT_ENTIRE_INNER] = {
		"text-object-entire-inner",
		"The whole text content, except for leading and trailing empty lines",
		textobj, { .i = TEXT_OBJ_INNER_ENTIRE }
	},
	[VIS_ACTION_TEXT_OBJECT_FUNCTION_OUTER] = {
		"text-object-function-outer",
		"A whole C-like function",
		 textobj, { .i = TEXT_OBJ_OUTER_FUNCTION }
	},
	[VIS_ACTION_TEXT_OBJECT_FUNCTION_INNER] = {
		"text-object-function-inner",
		"A whole C-like function body",
		 textobj, { .i = TEXT_OBJ_INNER_FUNCTION }
	},
	[VIS_ACTION_TEXT_OBJECT_LINE_OUTER] = {
		"text-object-line-outer",
		"The whole line",
		 textobj, { .i = TEXT_OBJ_OUTER_LINE }
	},
	[VIS_ACTION_TEXT_OBJECT_LINE_INNER] = {
		"text-object-line-inner",
		"The whole line, excluding leading and trailing whitespace",
		textobj, { .i = TEXT_OBJ_INNER_LINE }
	},
	[VIS_ACTION_MOTION_CHARWISE] = {
		"motion-charwise",
		"Force motion to be charwise",
		motiontype, { .i = CHARWISE }
	},
	[VIS_ACTION_MOTION_LINEWISE] = {
		"motion-linewise",
		"Force motion to be linewise",
		motiontype, { .i = LINEWISE }
	},
	[VIS_ACTION_NOP] = {
		"nop",
		"Ignore key, do nothing",
		nop,
	},
};

static KeyBinding basic_movement[] = {
	{ "<C-z>",         ACTION(EDITOR_SUSPEND)                  },
	{ "<Left>",        ACTION(CURSOR_CHAR_PREV)                },
	{ "<S-Left>",      ACTION(CURSOR_LONGWORD_START_PREV)      },
	{ "<Right>",       ACTION(CURSOR_CHAR_NEXT)                },
	{ "<S-Right>",     ACTION(CURSOR_LONGWORD_START_NEXT)      },
	{ "<Up>",          ACTION(CURSOR_LINE_UP)                  },
	{ "<Down>",        ACTION(CURSOR_LINE_DOWN)                },
	{ "<PageUp>",      ACTION(WINDOW_PAGE_UP)                  },
	{ "<PageDown>",    ACTION(WINDOW_PAGE_DOWN)                },
	{ "<S-PageUp>",    ACTION(WINDOW_HALFPAGE_UP)              },
	{ "<S-PageDown>",  ACTION(WINDOW_HALFPAGE_DOWN)            },
	{ "<Home>",        ACTION(CURSOR_LINE_BEGIN)               },
	{ "<End>",         ACTION(CURSOR_LINE_END)                 },
	{ /* empty last element, array terminator */               },
};

static KeyBinding vis_movements[] = {
	{ "h",             ACTION(CURSOR_CHAR_PREV)                },
	{ "<Backspace>",   ALIAS("h")                              },
	{ "<C-h>",         ALIAS("<Backspace>")                    },
	{ "l",             ACTION(CURSOR_CHAR_NEXT)                },
	{ "<Space>",       ALIAS("l")                              },
	{ "k",             ACTION(CURSOR_LINE_UP)                  },
	{ "C-p",           ALIAS("k")                              },
	{ "j",             ACTION(CURSOR_LINE_DOWN)                },
	{ "<C-j>",         ALIAS("j")                              },
	{ "<C-n>",         ALIAS("j")                              },
	{ "<Enter>",       ALIAS("j")                              },
	{ "gk",            ACTION(CURSOR_SCREEN_LINE_UP)           },
	{ "g<Up>",         ALIAS("gk")                             },
	{ "gj",            ACTION(CURSOR_SCREEN_LINE_DOWN)         },
	{ "g<Down>",       ALIAS("gj")                             },
	{ "^",             ACTION(CURSOR_LINE_START)               },
	{ "g_",            ACTION(CURSOR_LINE_FINISH)              },
	{ "$",             ACTION(CURSOR_LINE_END)                 },
	{ "%",             ACTION(CURSOR_BRACKET_MATCH)            },
	{ "b",             ACTION(CURSOR_WORD_START_PREV)          },
	{ "B",             ACTION(CURSOR_LONGWORD_START_PREV)      },
	{ "w",             ACTION(CURSOR_WORD_START_NEXT)          },
	{ "W",             ACTION(CURSOR_LONGWORD_START_NEXT)      },
	{ "ge",            ACTION(CURSOR_WORD_END_PREV)            },
	{ "gE",            ACTION(CURSOR_LONGWORD_END_PREV)        },
	{ "e",             ACTION(CURSOR_WORD_END_NEXT)            },
	{ "E",             ACTION(CURSOR_LONGWORD_END_NEXT)        },
	{ "{",             ACTION(CURSOR_PARAGRAPH_PREV)           },
	{ "}",             ACTION(CURSOR_PARAGRAPH_NEXT)           },
	{ "(",             ACTION(CURSOR_SENTENCE_PREV)            },
	{ ")",             ACTION(CURSOR_SENTENCE_NEXT)            },
	{ "[[",            ACTION(CURSOR_FUNCTION_START_PREV)      },
	{ "[]",            ACTION(CURSOR_FUNCTION_END_PREV)        },
	{ "][",            ACTION(CURSOR_FUNCTION_START_NEXT)      },
	{ "]]",            ACTION(CURSOR_FUNCTION_END_NEXT)        },
	{ "gg",            ACTION(CURSOR_LINE_FIRST)               },
	{ "g0",            ACTION(CURSOR_SCREEN_LINE_BEGIN)        },
	{ "gm",            ACTION(CURSOR_SCREEN_LINE_MIDDLE)       },
	{ "g$",            ACTION(CURSOR_SCREEN_LINE_END)          },
	{ "G",             ACTION(CURSOR_LINE_LAST)                },
	{ "|",             ACTION(CURSOR_COLUMN)                   },
	{ "n",             ACTION(CURSOR_SEARCH_FORWARD)           },
	{ "N",             ACTION(CURSOR_SEARCH_BACKWARD)          },
	{ "H",             ACTION(CURSOR_WINDOW_LINE_TOP)          },
	{ "M",             ACTION(CURSOR_WINDOW_LINE_MIDDLE)       },
	{ "L",             ACTION(CURSOR_WINDOW_LINE_BOTTOM)       },
	{ "*",             ACTION(CURSOR_SEARCH_WORD_FORWARD)      },
	{ "#",             ACTION(CURSOR_SEARCH_WORD_BACKWARD)     },
	{ "f",             ACTION(TO_RIGHT)                        },
	{ "F",             ACTION(TO_LEFT)                         },
	{ "t",             ACTION(TILL_RIGHT)                      },
	{ "T",             ACTION(TILL_LEFT)                       },
	{ ";",             ACTION(TOTILL_REPEAT)                   },
	{ ",",             ACTION(TOTILL_REVERSE)                  },
	{ "/",             ACTION(SEARCH_FORWARD)                  },
	{ "?",             ACTION(SEARCH_BACKWARD)                 },
	{ "`",             ACTION(MARK_GOTO)                       },
	{ "'",             ACTION(MARK_GOTO_LINE)                  },
	{ /* empty last element, array terminator */               },
};

static KeyBinding vis_textobjs[] = {
	{ "aw",  ACTION(TEXT_OBJECT_WORD_OUTER)                  },
	{ "aW",  ACTION(TEXT_OBJECT_LONGWORD_OUTER)              },
	{ "as",  ACTION(TEXT_OBJECT_SENTENCE)                    },
	{ "ap",  ACTION(TEXT_OBJECT_PARAGRAPH)                   },
	{ "a[",  ACTION(TEXT_OBJECT_SQUARE_BRACKET_OUTER)        },
	{ "a]",  ALIAS("a[")                                     },
	{ "a(",  ACTION(TEXT_OBJECT_PARANTHESE_OUTER)            },
	{ "a)",  ALIAS("a(")                                     },
	{ "ab",  ALIAS("a(")                                     },
	{ "a<",  ACTION(TEXT_OBJECT_ANGLE_BRACKET_OUTER)         },
	{ "a>",  ALIAS("a<")                                     },
	{ "a{",  ACTION(TEXT_OBJECT_CURLY_BRACKET_OUTER)         },
	{ "a}",  ALIAS("a{")                                     },
	{ "aB",  ALIAS("a{")                                     },
	{ "a\"", ACTION(TEXT_OBJECT_QUOTE_OUTER)                 },
	{ "a\'", ACTION(TEXT_OBJECT_SINGLE_QUOTE_OUTER)          },
	{ "a`",  ACTION(TEXT_OBJECT_BACKTICK_OUTER)              },
	{ "ae",  ACTION(TEXT_OBJECT_ENTIRE_OUTER)                },
	{ "af",  ACTION(TEXT_OBJECT_FUNCTION_OUTER)              },
	{ "al",  ACTION(TEXT_OBJECT_LINE_OUTER)                  },
	{ "iw",  ACTION(TEXT_OBJECT_WORD_INNER)                  },
	{ "iW",  ACTION(TEXT_OBJECT_LONGWORD_INNER)              },
	{ "is",  ACTION(TEXT_OBJECT_SENTENCE)                    },
	{ "ip",  ACTION(TEXT_OBJECT_PARAGRAPH)                   },
	{ "i[",  ACTION(TEXT_OBJECT_SQUARE_BRACKET_INNER)        },
	{ "i]",  ALIAS("i[")                                     },
	{ "i(",  ACTION(TEXT_OBJECT_PARANTHESE_INNER)            },
	{ "i)",  ALIAS("i(")                                     },
	{ "ib",  ALIAS("ib")                                     },
	{ "i<",  ACTION(TEXT_OBJECT_ANGLE_BRACKET_INNER)         },
	{ "i>",  ALIAS("i<")                                     },
	{ "i{",  ACTION(TEXT_OBJECT_CURLY_BRACKET_INNER)         },
	{ "i}",  ALIAS("i{")                                     },
	{ "iB",  ALIAS("i{")                                     },
	{ "i\"", ACTION(TEXT_OBJECT_QUOTE_INNER)                 },
	{ "i\'", ACTION(TEXT_OBJECT_SINGLE_QUOTE_INNER)          },
	{ "i`",  ACTION(TEXT_OBJECT_BACKTICK_INNER)              },
	{ "ie",  ACTION(TEXT_OBJECT_ENTIRE_INNER)                },
	{ "if",  ACTION(TEXT_OBJECT_FUNCTION_INNER)              },
	{ "il",  ACTION(TEXT_OBJECT_LINE_INNER)                  },
	{ /* empty last element, array terminator */             },
};

static KeyBinding vis_operators[] = {
	{ "0",              ACTION(COUNT)                             },
	{ "1",              ACTION(COUNT)                             },
	{ "2",              ACTION(COUNT)                             },
	{ "3",              ACTION(COUNT)                             },
	{ "4",              ACTION(COUNT)                             },
	{ "5",              ACTION(COUNT)                             },
	{ "6",              ACTION(COUNT)                             },
	{ "7",              ACTION(COUNT)                             },
	{ "8",              ACTION(COUNT)                             },
	{ "9",              ACTION(COUNT)                             },
	{ "d",              ACTION(OPERATOR_DELETE)                   },
	{ "c",              ACTION(OPERATOR_CHANGE)                   },
	{ "y",              ACTION(OPERATOR_YANK)                     },
	{ "p",              ACTION(PUT_AFTER)                         },
	{ "P",              ACTION(PUT_BEFORE)                        },
	{ "gp",             ACTION(PUT_AFTER_END)                     },
	{ "gP",             ACTION(PUT_BEFORE_END)                    },
	{ ">",              ACTION(OPERATOR_SHIFT_RIGHT)              },
	{ "<",              ACTION(OPERATOR_SHIFT_LEFT)               },
	{ "gU",             ACTION(OPERATOR_CASE_UPPER)               },
	{ "~",              ACTION(OPERATOR_CASE_SWAP)                },
	{ "g~",             ACTION(OPERATOR_CASE_SWAP)                },
	{ "gu",             ACTION(OPERATOR_CASE_LOWER)               },
	{ "\"",             ACTION(REGISTER)                          },
	{ /* empty last element, array terminator */                  },
};

static void vis_mode_operator_enter(Vis *vis, Mode *old) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_OPERATOR_OPTION];
}

static void vis_mode_operator_leave(Vis *vis, Mode *new) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
}

static void vis_mode_operator_input(Vis *vis, const char *str, size_t len) {
	/* invalid operator */
	action_reset(&vis->action);
	switchmode_to(vis->mode_prev);
}

static KeyBinding vis_operator_options[] = {
	{ "v",    ACTION(MOTION_CHARWISE)                               },
	{ "V",    ACTION(MOTION_LINEWISE)                               },
	{ /* empty last element, array terminator */                    },
};

static KeyBinding vis_mode_normal[] = {
	{ "<Escape>",         ACTION(CURSORS_REMOVE_ALL)                    },
	{ "<C-k>",            ACTION(CURSORS_NEW_LINE_ABOVE)                },
	{ "<C-j>",            ACTION(CURSORS_NEW_LINE_BELOW)                },
	{ "<C-a>",            ACTION(CURSORS_ALIGN)                         },
	{ "<C-n>",            ACTION(CURSOR_SELECT_WORD)                    },
	{ "<C-p>",            ACTION(CURSORS_REMOVE_LAST)                   },
	{ "<C-w>n",           ALIAS(":open<Enter>")                         },
	{ "<C-w>c",           ALIAS(":q<Enter>")                            },
	{ "<C-w>s",           ALIAS(":split<Enter>")                        },
	{ "<C-w>v",           ALIAS(":vsplit<Enter>")                       },
	{ "<C-w>j",           ACTION(WINDOW_NEXT)                           },
	{ "<C-w>l",           ALIAS("<C-w>j")                               },
	{ "<C-w><C-w>",       ALIAS("<C-w>j")                               },
	{ "<C-w><C-j>",       ALIAS("<C-w>j")                               },
	{ "<C-w><C-l>",       ALIAS("<C-w>j")                               },
	{ "<C-w>k",           ACTION(WINDOW_PREV)                           },
	{ "<C-w>h",           ALIAS("<C-w>k")                               },
	{ "<C-w><C-h>",       ALIAS("<C-w>k")                               },
	{ "<C-w><C-k>",       ALIAS("<C-w>k")                               },
	{ "<C-w><Backspace>", ALIAS("<C-w>k")                               },
	{ "<C-b>",            ALIAS("<PageUp>")                             },
	{ "<C-f>",            ALIAS("<PageDown>")                           },
	{ "<C-u>",            ALIAS("<S-PageUp>")                           },
	{ "<C-d>",            ALIAS("<S-PageDown>")                         },
	{ "<C-e>",            ACTION(WINDOW_SLIDE_UP)                       },
	{ "<C-y>",            ACTION(WINDOW_SLIDE_DOWN)                     },
	{ "<C-o>",            ACTION(JUMPLIST_PREV)                         },
	{ "<C-i>",            ACTION(JUMPLIST_NEXT)                         },
	{ "g;",               ACTION(CHANGELIST_PREV)                       },
	{ "g,",               ACTION(CHANGELIST_NEXT)                       },
	{ "a",                ALIAS("li")                                   },
	{ "A",                ALIAS("$a")                                   },
	{ "C",                ALIAS("c$")                                   },
	{ "D",                ALIAS("d$")                                   },
	{ "I",                ALIAS("^i")                                   },
	{ ".",                ACTION(REPEAT)                                },
	{ "o",                ACTION(OPEN_LINE_BELOW)                       },
	{ "O",                ACTION(OPEN_LINE_ABOVE)                       },
	{ "J",                ACTION(JOIN_LINE_BELOW)                       },
	{ "x",                ACTION(DELETE_CHAR_NEXT)                      },
	{ "r",                ACTION(REPLACE_CHAR)                          },
	{ "i",                ACTION(MODE_INSERT)                           },
	{ "v",                ACTION(MODE_VISUAL)                           },
	{ "V",                ACTION(MODE_VISUAL_LINE)                      },
	{ "R",                ACTION(MODE_REPLACE)                          },
	{ "S",                ALIAS("cc")                                   },
	{ "s",                ALIAS("cl")                                   },
	{ "Y",                ALIAS("yy")                                   },
	{ "X",                ALIAS("dh")                                   },
	{ "u",                ACTION(UNDO)                                  },
	{ "<C-r>",            ACTION(REDO)                                  },
	{ "g+",               ACTION(LATER)                                 },
	{ "g-",               ACTION(EARLIER)                               },
	{ "<C-l>",            ACTION(REDRAW)                                },
	{ ":",                ACTION(PROMPT_SHOW)                           },
	{ "ZZ",               ALIAS(":wq<Enter>")                           },
	{ "ZQ",               ALIAS(":q!<Enter>")                           },
	{ "zt",               ACTION(WINDOW_REDRAW_TOP)                     },
	{ "zz",               ACTION(WINDOW_REDRAW_CENTER)                  },
	{ "zb",               ACTION(WINDOW_REDRAW_BOTTOM)                  },
	{ "q",                ACTION(MACRO_RECORD)                          },
	{ "@",                ACTION(MACRO_REPLAY)                          },
	{ "gv",               ACTION(SELECTION_RESTORE)                     },
	{ "m",                ACTION(MARK_SET)                              },
	{ /* empty last element, array terminator */                        },
};

static KeyBinding vis_mode_visual[] = {
	{ "<C-n>",              ACTION(CURSORS_NEW_MATCH_NEXT)                },
	{ "<C-x>",              ACTION(CURSORS_NEW_MATCH_SKIP)                },
	{ "<C-p>",              ACTION(CURSORS_REMOVE_LAST)                   },
	{ "I",                  ACTION(CURSORS_NEW_LINES_BEGIN)               },
	{ "A",                  ACTION(CURSORS_NEW_LINES_END)                 },
	{ "<Backspace>",        ALIAS("d")                                    },
	{ "<C-h>",              ALIAS("<Backspace>")                          },
	{ "<DEL>",              ALIAS("d")                                    },
	{ "<Escape>",           ACTION(MODE_NORMAL)                           },
	{ "<C-c>",              ALIAS("<Escape>")                             },
	{ "v",                  ALIAS("<Escape>")                             },
	{ "V",                  ACTION(MODE_VISUAL_LINE)                      },
	{ ":",                  ACTION(PROMPT_SHOW_VISUAL)                    },
	{ "x",                  ALIAS("d")                                    },
	{ "r",                  ALIAS("c")                                    },
	{ "s",                  ALIAS("c")                                    },
	{ "J",                  ACTION(JOIN_LINES)                            },
	{ "o",                  ACTION(SELECTION_FLIP)                        },
	{ /* empty last element, array terminator */                          },
};

static void vis_mode_visual_enter(Vis *vis, Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
}

static void vis_mode_visual_leave(Vis *vis, Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	}
}

static KeyBinding vis_mode_visual_line[] = {
	{ "v",      ACTION(MODE_VISUAL)                                   },
	{ "V",      ACTION(MODE_NORMAL)                                   },
	{ /* empty last element, array terminator */                      },
};

static void vis_mode_visual_line_enter(Vis *vis, Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
	movement(vis, NULL, &(const Arg){ .i = MOVE_LINE_END });
}

static void vis_mode_visual_line_leave(Vis *vis, Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	} else {
		view_cursor_to(vis->win->view, view_cursor_get(vis->win->view));
	}
}

static KeyBinding vis_mode_readline[] = {
	{ "<Backspace>",    ACTION(DELETE_CHAR_PREV)                    },
	{ "<C-h>",          ALIAS("<Backspace>")                        },
	{ "<Escape>",       ACTION(MODE_NORMAL)                         },
	{ "<C-c>",          ALIAS("<Enter>")                            },
	{ "<C-d>",          ACTION(DELETE_CHAR_NEXT)                    },
	{ "<C-w>",          ACTION(DELETE_WORD_PREV)                    },
	{ "<C-u>",          ACTION(DELETE_LINE_BEGIN)                   },
	{ /* empty last element, array terminator */                    },
};

static KeyBinding vis_mode_prompt[] = {
	{ "<Backspace>",    ACTION(PROMPT_BACKSPACE)                    },
	{ "<C-h>",          ALIAS("<Backspace>")                        },
	{ "<Enter>",        ACTION(PROMPT_ENTER)                        },
	{ "<C-j>",          ALIAS("<Enter>")                            },
	{ "<Tab>",          ACTION(NOP)                                 },
	{ /* empty last element, array terminator */                    },
};

static void vis_mode_prompt_input(Vis *vis, const char *str, size_t len) {
	editor_insert_key(vis, str, len);
}

static void vis_mode_prompt_enter(Vis *vis, Mode *old) {
	if (old->isuser && old != &vis_modes[VIS_MODE_PROMPT])
		vis->mode_before_prompt = old;
}

static void vis_mode_prompt_leave(Vis *vis, Mode *new) {
	if (new->isuser)
		editor_prompt_hide(vis);
}

static KeyBinding vis_mode_insert[] = {
	{ "<Escape>",           ACTION(MODE_NORMAL)                         },
	{ "<C-l>",              ALIAS("<Escape>")                           },
	{ "<C-i>",              ALIAS("<Tab>")                              },
	{ "<Enter>",            ACTION(INSERT_NEWLINE)                      },
	{ "<C-j>",              ALIAS("<Enter>")                            },
	{ "<C-m>",              ALIAS("<Enter>")                            },
	{ "<C-o>",              ACTION(MODE_OPERATOR_PENDING)               },
	{ "<C-v>",              ACTION(INSERT_VERBATIM)                     },
	{ "<C-d>",              ALIAS("<Escape><<i")                        },
	{ "<C-t>",              ALIAS("<Escape>>>i")                        },
	{ "<C-x><C-e>",         ACTION(WINDOW_SLIDE_UP)                     },
	{ "<C-x><C-y>",         ACTION(WINDOW_SLIDE_DOWN)                   },
	{ "<Tab>",              ACTION(INSERT_TAB)                          },
	{ "<C-r>",              ACTION(INSERT_REGISTER)                     },
	{ /* empty last element, array terminator */                        },
};

static void vis_mode_insert_leave(Vis *vis, Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_idle(Vis *vis) {
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_input(Vis *vis, const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = view_cursor_get(vis->win->view);
	if (pos != oldpos)
		buffer_truncate(&vis->buffer_repeat);
	buffer_append(&vis->buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(&vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_INSERT];
	editor_insert_key(vis, str, len);
}

static KeyBinding vis_mode_replace[] = {
	{ /* empty last element, array terminator */                           },
};

static void vis_mode_replace_leave(Vis *vis, Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
}

static void vis_mode_replace_input(Vis *vis, const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = view_cursor_get(vis->win->view);
	if (pos != oldpos)
		buffer_truncate(&vis->buffer_repeat);
	buffer_append(&vis->buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(&vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_REPLACE];
	editor_replace_key(vis, str, len);
}

/*
 * the tree of modes currently looks like this. the double line between OPERATOR-OPTION
 * and OPERATOR is only in effect once an operator is detected. that is when entering the
 * OPERATOR mode its parent is set to OPERATOR-OPTION which makes {INNER-,}TEXTOBJ
 * reachable. once the operator is processed (i.e. the OPERATOR mode is left) its parent
 * mode is reset back to MOVE.
 *
 * Similarly the +-ed line between OPERATOR and TEXTOBJ is only active within the visual
 * modes.
 *
 *
 *                                         BASIC
 *                                    (arrow keys etc.)
 *                                    /      |
 *               /-------------------/       |
 *            READLINE                      MOVE
 *            /       \                 (h,j,k,l ...)
 *           /         \                     |       \-----------------\
 *          /           \                    |                         |
 *       INSERT       PROMPT             OPERATOR ++++           INNER-TEXTOBJ
 *          |      (history etc)       (d,c,y,p ..)   +      (i [wsp[]()b<>{}B"'`] )
 *          |                                |     \\  +               |
 *          |                                |      \\  +              |
 *       REPLACE                           NORMAL    \\  +          TEXTOBJ
 *                                           |        \\  +     (a [wsp[]()b<>{}B"'`] )
 *                                           |         \\  +   +       |
 *                                           |          \\  + +        |
 *                                         VISUAL        \\     OPERATOR-OPTION
 *                                           |            \\        (v,V)
 *                                           |             \\        //
 *                                           |              \\======//
 *                                      VISUAL-LINE
 */

static Mode vis_modes[] = {
	[VIS_MODE_BASIC] = {
		.name = "BASIC",
		.parent = NULL,
		.default_bindings = basic_movement,
	},
	[VIS_MODE_MOVE] = {
		.name = "MOVE",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.default_bindings = vis_movements,
	},
	[VIS_MODE_TEXTOBJ] = {
		.name = "TEXT-OBJECTS",
		.parent = &vis_modes[VIS_MODE_MOVE],
		.default_bindings = vis_textobjs,
	},
	[VIS_MODE_OPERATOR_OPTION] = {
		.name = "OPERATOR-OPTION",
		.parent = &vis_modes[VIS_MODE_TEXTOBJ],
		.default_bindings = vis_operator_options,
	},
	[VIS_MODE_OPERATOR] = {
		.name = "OPERATOR",
		.parent = &vis_modes[VIS_MODE_MOVE],
		.default_bindings = vis_operators,
		.enter = vis_mode_operator_enter,
		.leave = vis_mode_operator_leave,
		.input = vis_mode_operator_input,
	},
	[VIS_MODE_NORMAL] = {
		.name = "NORMAL",
		.status = "",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.default_bindings = vis_mode_normal,
	},
	[VIS_MODE_VISUAL] = {
		.name = "VISUAL",
		.status = "--VISUAL--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.default_bindings = vis_mode_visual,
		.enter = vis_mode_visual_enter,
		.leave = vis_mode_visual_leave,
		.visual = true,
	},
	[VIS_MODE_VISUAL_LINE] = {
		.name = "VISUAL LINE",
		.status = "--VISUAL LINE--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_VISUAL],
		.default_bindings = vis_mode_visual_line,
		.enter = vis_mode_visual_line_enter,
		.leave = vis_mode_visual_line_leave,
		.visual = true,
	},
	[VIS_MODE_READLINE] = {
		.name = "READLINE",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.default_bindings = vis_mode_readline,
	},
	[VIS_MODE_PROMPT] = {
		.name = "PROMPT",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.default_bindings = vis_mode_prompt,
		.input = vis_mode_prompt_input,
		.enter = vis_mode_prompt_enter,
		.leave = vis_mode_prompt_leave,
	},
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.status = "--INSERT--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.default_bindings = vis_mode_insert,
		.leave = vis_mode_insert_leave,
		.input = vis_mode_insert_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.status = "--REPLACE--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_INSERT],
		.default_bindings = vis_mode_replace,
		.leave = vis_mode_replace_leave,
		.input = vis_mode_replace_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
};

/* list of vis configurations, first entry is default. name is matched with
 * argv[0] i.e. program name upon execution
 */
static Config editors[] = {
	{
		.name = "vis",
		.mode = &vis_modes[VIS_MODE_NORMAL],
		.keypress = vis_keypress,
	},
};

/* default editor configuration to use */
static Config *config = &editors[0];

/* null terminated default settings/commands executed once on editor startup */
static const char *settings[] = {
	NULL
};

/* Color definitions for use in the sytax highlighting rules below. A fore
 * or background color of -1 specifies the default terminal color. */
enum {
	COLOR_NOHILIT,
	COLOR_SYNTAX0,
	COLOR_SYNTAX1,
	COLOR_SYNTAX2,
	COLOR_SYNTAX3,
	COLOR_SYNTAX4,
	COLOR_SYNTAX5,
	COLOR_SYNTAX6,
	COLOR_SYNTAX7,
	COLOR_SYNTAX8,
	COLOR_SYNTAX9,
	COLOR_SYNTAX_LAST, /* below are only aliases */
	COLOR_KEYWORD      = COLOR_SYNTAX1,
	COLOR_CONSTANT     = COLOR_SYNTAX4,
	COLOR_DATATYPE     = COLOR_SYNTAX2,
	COLOR_OPERATOR     = COLOR_SYNTAX2,
	COLOR_CONTROL      = COLOR_SYNTAX3,
	COLOR_PREPROCESSOR = COLOR_SYNTAX4,
	COLOR_PRAGMA       = COLOR_SYNTAX4,
	COLOR_KEYWORD2     = COLOR_SYNTAX4,
	COLOR_BRACKETS     = COLOR_SYNTAX5,
	COLOR_STRING       = COLOR_SYNTAX6,
	COLOR_LITERAL      = COLOR_SYNTAX6,
	COLOR_VARIABLE     = COLOR_SYNTAX6,
	COLOR_TARGET       = COLOR_SYNTAX5,
	COLOR_COMMENT      = COLOR_SYNTAX7,
	COLOR_IDENTIFIER   = COLOR_SYNTAX8,
	COLOR_TYPE         = COLOR_SYNTAX9,
	COLOR_WHITESPACE   = COLOR_COMMENT,
	COLOR_SPACES       = COLOR_WHITESPACE,
	COLOR_TABS         = COLOR_WHITESPACE,
	COLOR_EOL          = COLOR_WHITESPACE,
	COLOR_EOF          = COLOR_WHITESPACE,
};

static const char *styles[] = {
	[COLOR_NOHILIT] = "",
	[COLOR_SYNTAX0] = "fore:red,bold",
	[COLOR_SYNTAX1] = "fore:green,bold",
	[COLOR_SYNTAX2] = "fore:green",
	[COLOR_SYNTAX3] = "fore:magenta,bold",
	[COLOR_SYNTAX4] = "fore:magenta",
	[COLOR_SYNTAX5] = "fore:blue,bold",
	[COLOR_SYNTAX6] = "fore:red",
	[COLOR_SYNTAX7] = "fore:blue",
	[COLOR_SYNTAX8] = "fore:cyan",
	[COLOR_SYNTAX9] = "fore:yellow",
	[COLOR_SYNTAX_LAST] = NULL,
};

/* Syntax color definitions per file type. Each rule consists of a regular
 * expression, a color to apply in case of a match and boolean flag inidcating
 * whether it is a multiline rule.
 *
 * The syntax rules where initially imported from the sandy editor, written by
 * Rafael Garcia  <rafael.garcia.gallego@gmail.com>
 */
#define B "\\b"
/* Use this if \b is not in your libc's regex implementation */
// #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$"

/* common rules, used by multiple languages */

#define SYNTAX_MULTILINE_COMMENT {    \
	"(/\\*([^*]|\\*+[^*/])*\\*+/|/\\*([^*]|\\*+[^*/])*$|^([^/]|/+[^/*])*\\*/)", \
	COLOR_COMMENT,                \
	true, /* multiline */         \
}

#define SYNTAX_SINGLE_LINE_COMMENT { \
	"(//.*)",                    \
	COLOR_COMMENT,               \
}

#define SYNTAX_LITERAL {                             \
	"('(\\\\.|.)')|"B"(0x[0-9A-Fa-f]+|[0-9]+)"B, \
	COLOR_LITERAL,                               \
}

#define SYNTAX_STRING {         \
	"(\"(\\\\.|[^\"])*\")", \
	COLOR_STRING,           \
	false, /* multiline */  \
}

#define SYNTAX_CONSTANT {        \
	B"[A-Z_][0-9A-Z_]+"B,    \
	COLOR_CONSTANT,          \
}

#define SYNTAX_BRACKET {             \
	"(\\(|\\)|\\{|\\}|\\[|\\])", \
	COLOR_BRACKETS,              \
}

#define SYNTAX_C_PREPROCESSOR { \
	"(^#[\\t ]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma)?)", \
	COLOR_PREPROCESSOR, \
}

#define SYNTAX_SPACES    { "\xC2\xB7",     COLOR_SPACES }
#define SYNTAX_TABS      { "\xE2\x96\xB6", COLOR_TABS   }
#define SYNTAX_TABS_FILL { " ",            COLOR_TABS   }
#define SYNTAX_EOL       { "\xE2\x8F\x8E", COLOR_EOL    }
#define SYNTAX_EOF       { "~",            COLOR_EOF    }

/* these rules are applied top to bottom, first match wins. Therefore more 'greedy'
 * rules such as for comments should be the first entries.
 *
 * The array of syntax definition must be terminated with an empty element.
 */
static Syntax syntaxes[] = {{
	.name = "c",
	.file = "\\.(c(pp|xx)?|h(pp|xx)?|cc)$",
	.settings = (const char*[]){
		"set number",
		"set autoindent",
		"set show spaces=0 tabs=1 newlines=1",
		NULL
	},
	.styles = styles,
	.symbols = {
		SYNTAX_SPACES,
		SYNTAX_TABS,
		SYNTAX_TABS_FILL,
		SYNTAX_EOL,
		SYNTAX_EOF,
	},
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		"<[a-zA-Z0-9\\.\\-_/]+\\.(c(pp|xx)?|h(pp|xx)?|cc)>",
		COLOR_STRING,
	},
		SYNTAX_C_PREPROCESSOR,
	{
		B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		COLOR_KEYWORD,
	},{
		B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|"
		"typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|"
		"namespace|template|public|protected|private|typename|this|friend|virtual|using|"
		"mutable|volatile|register|explicit)"B,
		COLOR_DATATYPE,
	},{
		B"(goto|continue|break|return)"B,
		COLOR_CONTROL,
	}}
},{
	.name = "sh",
	.file = "\\.sh$",
	.styles = styles,
	.rules = {{
		"#.*$",
		COLOR_COMMENT,
	},
		SYNTAX_STRING,
	{
		"^[0-9A-Z_]+\\(\\)",
		COLOR_CONSTANT,
	},{
		"\\$[?!@#$?*-]",
		COLOR_VARIABLE,
	},{
		"\\$\\{[A-Za-z_][0-9A-Za-z_]+\\}",
		COLOR_VARIABLE,
	},{
		"\\$[A-Za-z_][0-9A-Za-z_]+",
		COLOR_VARIABLE,
	},{
		B"(case|do|done|elif|else|esac|exit|fi|for|function|if|in|local|read|return|select|shift|then|time|until|while)"B,
		COLOR_KEYWORD,
	},{
		"(\\{|\\}|\\(|\\)|\\;|\\]|\\[|`|\\\\|\\$|<|>|!|=|&|\\|)",
		COLOR_BRACKETS,
	}}
},{
	.name = "makefile",
	.file = "(Makefile[^/]*|\\.mk)$",
	.styles = styles,
	.rules = {{
		"#.*$",
		COLOR_COMMENT,
	},{
		"\\$+[{(][a-zA-Z0-9_-]+[})]",
		COLOR_VARIABLE,
	},{
		B"(if|ifeq|else|endif)"B,
		COLOR_CONTROL,
	},{
		"^[^ 	]+:",
		COLOR_TARGET,
	},{
		"[:(+?=)]",
		COLOR_BRACKETS,
	}}
},{
	.name = "man",
	.file = "\\.[1-9]x?$",
	.styles = styles,
	.rules = {{
		"\\.(BR?|I[PR]?).*$",
		COLOR_SYNTAX0,
	},{
		"\\.(S|T)H.*$",
		COLOR_SYNTAX2,
	},{
		"\\.(br|DS|RS|RE|PD)",
		COLOR_SYNTAX3,
	},{
		"(\\.(S|T)H|\\.TP)",
		COLOR_SYNTAX4,
	},{
		"\\.(BR?|I[PR]?|PP)",
		COLOR_SYNTAX5,
	},{
		"\\\\f[BIPR]",
		COLOR_SYNTAX6,
	}}
},{
	.name = "vala",
	.file = "\\.(vapi|vala)$",
	.styles = styles,
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		B"(for|if|while|do|else|case|default|switch|get|set|value|out|ref|enum)"B,
		COLOR_KEYWORD,
	},{
		B"(uint|uint8|uint16|uint32|uint64|bool|byte|ssize_t|size_t|char|double|string|float|int|long|short|this|base|transient|void|true|false|null|unowned|owned)"B,
		COLOR_DATATYPE,
	},{
		B"(try|catch|throw|finally|continue|break|return|new|sizeof|signal|delegate)"B,
		COLOR_CONTROL,
	},{
		B"(abstract|class|final|implements|import|instanceof|interface|using|private|public|static|strictfp|super|throws)"B,
		COLOR_KEYWORD2,
	}}
},{
	.name = "java",
	.file = "\\.java$",
	.styles = styles,
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		B"(for|if|while|do|else|case|default|switch)"B,
		COLOR_KEYWORD,
	},{
		B"(boolean|byte|char|double|float|int|long|short|transient|void|true|false|null)"B,
		COLOR_DATATYPE,
	},{
		B"(try|catch|throw|finally|continue|break|return|new)"B,
		COLOR_CONTROL,
	},{
		B"(abstract|class|extends|final|implements|import|instanceof|interface|native|package|private|protected|public|static|strictfp|this|super|synchronized|throws|volatile)"B,
		COLOR_KEYWORD2,
	}}
},{
	.name = "javascript",
	.file = "\\.(js|json)$",
	.styles = styles,
	.rules = {
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_BRACKET,
	{
		B"(true|false|null|undefined)"B,
		COLOR_DATATYPE,
	},{
		B"(NaN|Infinity)"B,
		COLOR_LITERAL,
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		COLOR_STRING,
	},{
		B"(for|if|while|do|in|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		COLOR_KEYWORD,
	},{
		B"(continue|break|return)"B,
		COLOR_CONTROL,
	},{
		B"(case|class|const|debugger|default|enum|export|extends|finally|function|implements|import|instanceof|let|this|typeof|var|with|yield)"B,
		COLOR_KEYWORD2,
	}}
},{
	.name = "lua",
	.file = "\\.lua$",
	.settings = (const char*[]){
		"set number",
		"set autoindent",
		NULL
	},
	.styles = styles,
	.rules = {{
		"--\\[(=*)\\[([^]]*)\\](=*)\\]",
		COLOR_COMMENT,
		true,
	},{
		"--.*$",
		COLOR_COMMENT,
	},{
		"(\\[(=*)\\[([^]]*)\\](=*)\\]|^([^][]*)\\](=*)\\])",
		COLOR_STRING,
		true,
	},
		SYNTAX_STRING,
	{
		B"([0-9]*\\.)?[0-9]+([eE]([\\+-])?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"0x[0-9a-fA-F]+"B,
		COLOR_LITERAL,
	},{
		B"(false|nil|true)"B,
		COLOR_CONSTANT,
	},{
		"(\\.\\.\\.)",
		COLOR_CONSTANT,
	},{
		B"(break|do|else|elseif|end|for|function|if|in|local|repeat|return|then|until|while)"B,
		COLOR_KEYWORD,
	},{
		B"(and|not|or)"B,
		COLOR_OPERATOR,
	},{
		"(\\+|-|\\*|/|%|\\^|#|[=~<>]=|<|>|\\.\\.)",
		COLOR_OPERATOR,
	},
		SYNTAX_BRACKET,
	}
},{
	.name = "ruby",
	.file = "\\.rb$",
	.styles = styles,
	.rules = {{
		"(#[^{].*$|#$)",
		COLOR_COMMENT,
	},{
		"(\\$|@|@@)?"B"[A-Z]+[0-9A-Z_a-z]*",
		COLOR_VARIABLE,
	},{
		B"(__FILE__|__LINE__|BEGIN|END|alias|and|begin|break|case|class|def|defined\?|do|else|elsif|end|ensure|false|for|if|in|module|next|nil|not|or|redo|rescue|retry|return|self|super|then|true|undef|unless|until|when|while|yield)"B,
		COLOR_KEYWORD,
	},{
		"([ 	]|^):[0-9A-Z_]+"B,
		COLOR_SYNTAX2,
	},{
		"(/([^/]|(\\/))*/[iomx]*|%r\\{([^}]|(\\}))*\\}[iomx]*)",
		COLOR_SYNTAX3,
	},{
		"(`[^`]*`|%x\\{[^}]*\\})",
		COLOR_SYNTAX4,
	},{
		"(\"([^\"]|(\\\\\"))*\"|%[QW]?\\{[^}]*\\}|%[QW]?\\([^)]*\\)|%[QW]?<[^>]*>|%[QW]?\\[[^]]*\\]|%[QW]?\\$[^$]*\\$|%[QW]?\\^[^^]*\\^|%[QW]?![^!]*!|\'([^\']|(\\\\\'))*\'|%[qw]\\{[^}]*\\}|%[qw]\\([^)]*\\)|%[qw]<[^>]*>|%[qw]\\[[^]]*\\]|%[qw]\\$[^$]*\\$|%[qw]\\^[^^]*\\^|%[qw]![^!]*!)",
		COLOR_SYNTAX5,
	},{
		"#\\{[^}]*\\}",
		COLOR_SYNTAX6,
	}}
},{
	.name = "python",
	.file = "\\.py$",
	.styles = styles,
	.rules = {{
		"(#.*$|#$)",
		COLOR_COMMENT,
	},{
		"(\"\"\".*\"\"\")",
		COLOR_COMMENT,
		true, /* multiline */
	},{
		B"(and|class|def|not|or|return|yield|is)"B,
		COLOR_KEYWORD2,
	},{
		B"(from|import|as)"B,
		COLOR_KEYWORD,
	},{
		B"(if|elif|else|while|for|in|try|with|except|in|break|continue|finally)"B,
		COLOR_CONTROL,
	},{
		B"(int|str|float|unicode|int|bool|chr|type|list|dict|tuple)",
		COLOR_DATATYPE,
	},{
		"(True|False|None)",
		COLOR_LITERAL,
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		COLOR_LITERAL,
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		COLOR_STRING,
		false, /* multiline */  
	},{
		"(__init__|__str__|__unicode__|__gt__|__lt__|__eq__|__enter__|__exit__|__next__|__getattr__|__getitem__|__setitem__|__call__|__contains__|__iter__|__bool__|__all__|__name__)",
		COLOR_SYNTAX2,
	}}
},{
	.name = "php",
	.file = "\\.php$",
	.styles = styles,
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_BRACKET,
	{
		"(#.*$|#$)",
		COLOR_COMMENT,
	},{
		"(\"\"\".*\"\"\")",
		COLOR_COMMENT,
		true, /* multiline */
	},{
		B"(class|interface|extends|implements|new|__construct|__destruct|use|namespace|return)"B,
		COLOR_KEYWORD2,
	},{
		B"(public|private|protected|const|parent|function|->)"B,
		COLOR_KEYWORD,
	},{
		B"(if|else|while|do|for|foreach|in|try|catch|finally|switch|case|default|break|continue|as|=>)"B,
		COLOR_CONTROL,
	},{
		B"(array|true|false|null)",
		COLOR_DATATYPE,
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		COLOR_LITERAL,
	},{
		"\\$[a-zA-Z0-9_\\-]+",
		COLOR_VARIABLE,
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		COLOR_STRING,
		false, /* multiline */
	},{
		"(php|echo|print|var_dump|print_r)",
		COLOR_SYNTAX2,
	}}
},{
	.name = "haskell",
	.file = "\\.hs$",
	.styles = styles,
	.rules = {{
		"\\{-#.*#-\\}",
		COLOR_PRAGMA,
	},{
		"---*([^-!#$%&\\*\\+./<=>\?@\\^|~].*)?$",
		COLOR_COMMENT,
	}, {
		// These are allowed to be nested, but we can't express that
		// with regular expressions
		"\\{-.*-\\}",
		COLOR_COMMENT,
		true
	},
		SYNTAX_STRING,
		SYNTAX_C_PREPROCESSOR,
	{
		// as and hiding are only keywords when part of an import, but
		// I don't want to highlight the whole import line.
		// capture group coloring or similar would be nice
		"(^import( qualified)?)|"B"(as|hiding|infix[lr]?)"B,
		COLOR_KEYWORD2,
	},{
		B"(module|class|data|deriving|instance|default|where|type|newtype)"B,
		COLOR_KEYWORD,
	},{
		B"(do|case|of|let|in|if|then|else)"B,
		COLOR_CONTROL,
	},{
		"('(\\\\.|.)')",
		COLOR_LITERAL,
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		COLOR_LITERAL,
	},{
		"("B"[A-Z][a-zA-Z0-9_']*\\.)*"B"[a-zA-Z][a-zA-Z0-9_']*"B,
		COLOR_NOHILIT,
	},{
		"("B"[A-Z][a-zA-Z0-9_']*\\.)?[-!#$%&\\*\\+/<=>\\?@\\\\^|~:.][-!#$%&\\*\\+/<=>\\?@\\\\^|~:.]*",
		COLOR_OPERATOR,
	},{
		"`("B"[A-Z][a-zA-Z0-9_']*\\.)?[a-z][a-zA-Z0-9_']*`",
		COLOR_OPERATOR,
	},{
		"\\(|\\)|\\[|\\]|,|;|_|\\{|\\}",
		COLOR_BRACKETS,
	}}
},{
	.name = "markdown",
	.file = "\\.(md|mdwn)$",
	.styles = styles,
	.rules = {{
		"(^#{1,6}.*$)", //titles
		COLOR_SYNTAX5,
	},{
		"((\\* *){3,}|(_ *){3,}|(- *){3,})", // horizontal rules
		COLOR_SYNTAX2,
	},{
		"(\\*\\*.*\\*\\*)|(__.*__)", // super-bolds
		COLOR_SYNTAX4,
	},{
		"(\\*.*\\*)|(_.*_)", // bolds
		COLOR_SYNTAX3,
	},{
		"(\\[.*\\]\\(.*\\))", //links
		COLOR_SYNTAX6,
	},{
		"(^ *([-\\*\\+]|[0-9]+\\.))", //lists
		COLOR_SYNTAX2,
	},{
		"(^( {4,}|\t+).*$)", // code blocks
		COLOR_SYNTAX7,
	},{
		"(`+.*`+)", // inline code
		COLOR_SYNTAX7,
	},{
		"(^>+.*)", // quotes
		COLOR_SYNTAX7,
	}}
},{
	.name = "ledger",
	.file = "\\.(journal|ledger)$",
	.styles = styles,
	.rules = {
	{ /* comment */
		"^[;#].*",
		COLOR_COMMENT,
	},{ /* value tag */
		"(  |\t|^ )*; :([^ ][^:]*:)+[ \\t]*$",
		COLOR_DATATYPE,
	},{ /* typed tag */
		"(  |\t|^ )*; [^:]+::.*",
		COLOR_DATATYPE,
	},{ /* tag */
		"(  |\t|^ )*; [^:]+:.*",
		COLOR_TYPE,
	},{ /* metadata */
		"(  |\t|^ )*;.*",
		COLOR_CONSTANT,
	},{ /* date */
		"^[0-9][^ \t]+",
		COLOR_LITERAL,
	},{ /* account */
		"^[ \t]+[a-zA-Z:'!*()%&]+",
		COLOR_IDENTIFIER,
	},{ /* amount */
		"(  |\t)[^;]*",
		COLOR_LITERAL,
	},{ /* automated transaction */
		"^[=~].*",
		COLOR_TYPE,
	},{ /* directives */
		"^[!@]?(account|alias|assert|bucket|capture|check|comment|commodity|define|end|fixed|endfixed|include|payee|apply|tag|test|year|[AYNDCIiOobh])"B".*",
		COLOR_DATATYPE,
	}}
},{
	.name = "apl",
	.file = "\\.apl$",
	.settings = (const char*[]){
		"set number",
		NULL
	},
	.styles = styles,
	.rules = {{
		"(⍝|#).*$",
		COLOR_COMMENT,
	},{
		"('([^']|'')*')|(\"([^\"]|\"\")*\")",
		COLOR_STRING,
	},{
		"^ *(∇|⍫)",
		COLOR_SYNTAX9,
	},{
		"(⎕[a-zA-Z]*)|[⍞χ⍺⍶⍵⍹]",
		COLOR_KEYWORD,
	},{
		"[∆⍙_a-zA-Z][∆⍙_¯a-zA-Z0-9]* *:",
		COLOR_SYNTAX2,
	},{
		"[∆⍙_a-zA-Z][∆⍙_¯a-zA-Z0-9]*",
		COLOR_IDENTIFIER,
	},{
		"¯?(([0-9]+(\\.[0-9]+)?)|\\.[0-9]+)([eE]¯?[0-9]+)?([jJ]¯?(([0-9]+(\\.[0-9]+)?)|\\.[0-9]+)([eE]¯?[0-9]+)?)?",
		COLOR_CONSTANT,
	},{
		"[][(){}]",
		COLOR_BRACKETS,
	},{
		"[←→◊]",
		COLOR_SYNTAX3,
	}}
},{
	/* empty last element, array terminator */
}};
