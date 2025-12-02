#include <signal.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ui.h"
#include "vis.h"
#include "vis-lua.h"
#include "text-util.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"
#include "libutf.h"
#include "array.h"
#include "buffer.h"

static Vis vis[1];

#define PAGE      INT_MAX
#define PAGE_HALF (INT_MAX-1)

/** functions to be called from keybindings */
/* X(impl, enum, argument, lua_name, help) */
#define KEY_ACTION_LIST(X) \
	X(ka_call,                            INSERT_NEWLINE,                   .f = vis_insert_nl,                       "vis-insert-newline",                  "Insert a line break (depending on file type)") \
	X(ka_call,                            INSERT_TAB,                       .f = vis_insert_tab,                      "vis-insert-tab",                      "Insert a tab (might be converted to spaces)") \
	X(ka_call,                            REDRAW,                           .f = vis_redraw,                          "vis-redraw",                          "Redraw current editor content") \
	X(ka_call,                            WINDOW_NEXT,                      .f = vis_window_next,                     "vis-window-next",                     "Focus next window") \
	X(ka_call,                            WINDOW_PREV,                      .f = vis_window_prev,                     "vis-window-prev",                     "Focus previous window") \
	X(ka_count,                           COUNT,                            0,                                        "vis-count",                           "Count specifier") \
	X(ka_delete,                          DELETE_CHAR_NEXT,                 .i = VIS_MOVE_CHAR_NEXT,                  "vis-delete-char-next",                "Delete the next character") \
	X(ka_delete,                          DELETE_CHAR_PREV,                 .i = VIS_MOVE_CHAR_PREV,                  "vis-delete-char-prev",                "Delete the previous character") \
	X(ka_delete,                          DELETE_LINE_BEGIN,                .i = VIS_MOVE_LINE_BEGIN,                 "vis-delete-line-begin",               "Delete until the start of the current line") \
	X(ka_delete,                          DELETE_WORD_PREV,                 .i = VIS_MOVE_WORD_START_PREV,            "vis-delete-word-prev",                "Delete the previous WORD") \
	X(ka_earlier,                         EARLIER,                          0,                                        "vis-earlier",                         "Goto older text state") \
	X(ka_gotoline,                        CURSOR_LINE_FIRST,                .i = -1,                                  "vis-motion-line-first",               "Move cursor to given line (defaults to first)") \
	X(ka_gotoline,                        CURSOR_LINE_LAST,                 .i = +1,                                  "vis-motion-line-last",                "Move cursor to given line (defaults to last)") \
	X(ka_insert_register,                 INSERT_REGISTER,                  0,                                        "vis-insert-register",                 "Insert specified register content") \
	X(ka_insert_verbatim,                 INSERT_VERBATIM,                  0,                                        "vis-insert-verbatim",                 "Insert Unicode character based on code point") \
	X(ka_insertmode,                      APPEND_CHAR_NEXT,                 .i = VIS_MOVE_LINE_CHAR_NEXT,             "vis-append-char-next",                "Append text after the cursor") \
	X(ka_insertmode,                      APPEND_LINE_END,                  .i = VIS_MOVE_LINE_END,                   "vis-append-line-end",                 "Append text after the end of the line") \
	X(ka_insertmode,                      INSERT_LINE_START,                .i = VIS_MOVE_LINE_START,                 "vis-insert-line-start",               "Insert text before the first non-blank in the line") \
	X(ka_insertmode,                      MODE_INSERT,                      .i = VIS_MOVE_NOP,                        "vis-mode-insert",                     "Enter insert mode") \
	X(ka_join,                            JOIN_LINES,                       .s = " ",                                 "vis-join-lines",                      "Join selected lines") \
	X(ka_join,                            JOIN_LINES_TRIM,                  .s = "",                                  "vis-join-lines-trim",                 "Join selected lines, remove white space") \
	X(ka_jumplist,                        JUMPLIST_NEXT,                    .i = +1,                                  "vis-jumplist-next",                   "Go to newer cursor position in jump list") \
	X(ka_jumplist,                        JUMPLIST_PREV,                    .i = -1,                                  "vis-jumplist-prev",                   "Go to older cursor position in jump list") \
	X(ka_jumplist,                        JUMPLIST_SAVE,                    .i = 0,                                   "vis-jumplist-save",                   "Save current selections in jump list") \
	X(ka_later,                           LATER,                            0,                                        "vis-later",                           "Goto newer text state") \
	X(ka_macro_record,                    MACRO_RECORD,                     0,                                        "vis-macro-record",                    "Record macro into given register") \
	X(ka_macro_replay,                    MACRO_REPLAY,                     0,                                        "vis-macro-replay",                    "Replay macro, execute the content of the given register") \
	X(ka_mark,                            MARK,                             0,                                        "vis-mark",                            "Use given mark for next action") \
	X(ka_movement,                        CURSOR_BLOCK_END,                 .i = VIS_MOVE_BLOCK_END,                  "vis-motion-block-end",                "Move cursor to the closing curly brace in a block") \
	X(ka_movement,                        CURSOR_BLOCK_START,               .i = VIS_MOVE_BLOCK_START,                "vis-motion-block-start",              "Move cursor to the opening curly brace in a block") \
	X(ka_movement,                        CURSOR_BYTE,                      .i = VIS_MOVE_BYTE,                       "vis-motion-byte",                     "Move to absolute byte position") \
	X(ka_movement,                        CURSOR_BYTE_LEFT,                 .i = VIS_MOVE_BYTE_LEFT,                  "vis-motion-byte-left",                "Move count bytes to the left") \
	X(ka_movement,                        CURSOR_BYTE_RIGHT,                .i = VIS_MOVE_BYTE_RIGHT,                 "vis-motion-byte-right",               "Move count bytes to the right") \
	X(ka_movement,                        CURSOR_CHAR_NEXT,                 .i = VIS_MOVE_CHAR_NEXT,                  "vis-motion-char-next",                "Move cursor right, to the next character") \
	X(ka_movement,                        CURSOR_CHAR_PREV,                 .i = VIS_MOVE_CHAR_PREV,                  "vis-motion-char-prev",                "Move cursor left, to the previous character") \
	X(ka_movement,                        CURSOR_CODEPOINT_NEXT,            .i = VIS_MOVE_CODEPOINT_NEXT,             "vis-motion-codepoint-next",           "Move to the next Unicode codepoint") \
	X(ka_movement,                        CURSOR_CODEPOINT_PREV,            .i = VIS_MOVE_CODEPOINT_PREV,             "vis-motion-codepoint-prev",           "Move to the previous Unicode codepoint") \
	X(ka_movement,                        CURSOR_COLUMN,                    .i = VIS_MOVE_COLUMN,                     "vis-motion-column",                   "Move cursor to given column of current line") \
	X(ka_movement,                        CURSOR_LINE_BEGIN,                .i = VIS_MOVE_LINE_BEGIN,                 "vis-motion-line-begin",               "Move cursor to first character of the line") \
	X(ka_movement,                        CURSOR_LINE_CHAR_NEXT,            .i = VIS_MOVE_LINE_CHAR_NEXT,             "vis-motion-line-char-next",           "Move cursor right, to the next character on the same line") \
	X(ka_movement,                        CURSOR_LINE_CHAR_PREV,            .i = VIS_MOVE_LINE_CHAR_PREV,             "vis-motion-line-char-prev",           "Move cursor left,  to the previous character on the same line") \
	X(ka_movement,                        CURSOR_LINE_DOWN,                 .i = VIS_MOVE_LINE_DOWN,                  "vis-motion-line-down",                "Move cursor line downwards") \
	X(ka_movement,                        CURSOR_LINE_END,                  .i = VIS_MOVE_LINE_END,                   "vis-motion-line-end",                 "Move cursor to end of the line") \
	X(ka_movement,                        CURSOR_LINE_FINISH,               .i = VIS_MOVE_LINE_FINISH,                "vis-motion-line-finish",              "Move cursor to last non-blank character of the line") \
	X(ka_movement,                        CURSOR_LINE_START,                .i = VIS_MOVE_LINE_START,                 "vis-motion-line-start",               "Move cursor to first non-blank character of the line") \
	X(ka_movement,                        CURSOR_LINE_UP,                   .i = VIS_MOVE_LINE_UP,                    "vis-motion-line-up",                  "Move cursor line upwards") \
	X(ka_movement,                        CURSOR_LONGWORD_END_NEXT,         .i = VIS_MOVE_LONGWORD_END_NEXT,          "vis-motion-bigword-end-next",         "Move cursor forward to the end of WORD") \
	X(ka_movement,                        CURSOR_LONGWORD_END_PREV,         .i = VIS_MOVE_LONGWORD_END_PREV,          "vis-motion-bigword-end-prev",         "Move cursor backwards to the end of WORD") \
	X(ka_movement,                        CURSOR_LONGWORD_START_NEXT,       .i = VIS_MOVE_LONGWORD_START_NEXT,        "vis-motion-bigword-start-next",       "Move cursor WORDS forwards") \
	X(ka_movement,                        CURSOR_LONGWORD_START_PREV,       .i = VIS_MOVE_LONGWORD_START_PREV,        "vis-motion-bigword-start-prev",       "Move cursor WORDS backwards") \
	X(ka_movement,                        CURSOR_PARAGRAPH_NEXT,            .i = VIS_MOVE_PARAGRAPH_NEXT,             "vis-motion-paragraph-next",           "Move cursor paragraph forward") \
	X(ka_movement,                        CURSOR_PARAGRAPH_PREV,            .i = VIS_MOVE_PARAGRAPH_PREV,             "vis-motion-paragraph-prev",           "Move cursor paragraph backward") \
	X(ka_movement,                        CURSOR_PARENTHESIS_END,           .i = VIS_MOVE_PARENTHESIS_END,            "vis-motion-parenthesis-end",          "Move cursor to the closing parenthesis inside a pair of parentheses") \
	X(ka_movement,                        CURSOR_PARENTHESIS_START,         .i = VIS_MOVE_PARENTHESIS_START,          "vis-motion-parenthesis-start",        "Move cursor to the opening parenthesis inside a pair of parentheses") \
	X(ka_movement,                        CURSOR_SCREEN_LINE_BEGIN,         .i = VIS_MOVE_SCREEN_LINE_BEGIN,          "vis-motion-screenline-begin",         "Move cursor to beginning of screen/display line") \
	X(ka_movement,                        CURSOR_SCREEN_LINE_DOWN,          .i = VIS_MOVE_SCREEN_LINE_DOWN,           "vis-motion-screenline-down",          "Move cursor screen/display line downwards") \
	X(ka_movement,                        CURSOR_SCREEN_LINE_END,           .i = VIS_MOVE_SCREEN_LINE_END,            "vis-motion-screenline-end",           "Move cursor to end of screen/display line") \
	X(ka_movement,                        CURSOR_SCREEN_LINE_MIDDLE,        .i = VIS_MOVE_SCREEN_LINE_MIDDLE,         "vis-motion-screenline-middle",        "Move cursor to middle of screen/display line") \
	X(ka_movement,                        CURSOR_SCREEN_LINE_UP,            .i = VIS_MOVE_SCREEN_LINE_UP,             "vis-motion-screenline-up",            "Move cursor screen/display line upwards") \
	X(ka_movement,                        CURSOR_SEARCH_REPEAT,             .i = VIS_MOVE_SEARCH_REPEAT,              "vis-motion-search-repeat",            "Move cursor to next match") \
	X(ka_movement,                        CURSOR_SEARCH_REPEAT_BACKWARD,    .i = VIS_MOVE_SEARCH_REPEAT_BACKWARD,     "vis-motion-search-repeat-backward",   "Move cursor to previous match in backward direction") \
	X(ka_movement,                        CURSOR_SEARCH_REPEAT_FORWARD,     .i = VIS_MOVE_SEARCH_REPEAT_FORWARD,      "vis-motion-search-repeat-forward",    "Move cursor to next match in forward direction") \
	X(ka_movement,                        CURSOR_SEARCH_REPEAT_REVERSE,     .i = VIS_MOVE_SEARCH_REPEAT_REVERSE,      "vis-motion-search-repeat-reverse",    "Move cursor to next match in opposite direction") \
	X(ka_movement,                        CURSOR_SEARCH_WORD_BACKWARD,      .i = VIS_MOVE_SEARCH_WORD_BACKWARD,       "vis-motion-search-word-backward",     "Move cursor to previous occurrence of the word under cursor") \
	X(ka_movement,                        CURSOR_SEARCH_WORD_FORWARD,       .i = VIS_MOVE_SEARCH_WORD_FORWARD,        "vis-motion-search-word-forward",      "Move cursor to next occurrence of the word under cursor") \
	X(ka_movement,                        CURSOR_SENTENCE_NEXT,             .i = VIS_MOVE_SENTENCE_NEXT,              "vis-motion-sentence-next",            "Move cursor sentence forward") \
	X(ka_movement,                        CURSOR_SENTENCE_PREV,             .i = VIS_MOVE_SENTENCE_PREV,              "vis-motion-sentence-prev",            "Move cursor sentence backward") \
	X(ka_movement,                        CURSOR_WINDOW_LINE_BOTTOM,        .i = VIS_MOVE_WINDOW_LINE_BOTTOM,         "vis-motion-window-line-bottom",       "Move cursor to bottom line of the window") \
	X(ka_movement,                        CURSOR_WINDOW_LINE_MIDDLE,        .i = VIS_MOVE_WINDOW_LINE_MIDDLE,         "vis-motion-window-line-middle",       "Move cursor to middle line of the window") \
	X(ka_movement,                        CURSOR_WINDOW_LINE_TOP,           .i = VIS_MOVE_WINDOW_LINE_TOP,            "vis-motion-window-line-top",          "Move cursor to top line of the window") \
	X(ka_movement,                        CURSOR_WORD_END_NEXT,             .i = VIS_MOVE_WORD_END_NEXT,              "vis-motion-word-end-next",            "Move cursor forward to the end of word") \
	X(ka_movement,                        CURSOR_WORD_END_PREV,             .i = VIS_MOVE_WORD_END_PREV,              "vis-motion-word-end-prev",            "Move cursor backwards to the end of word") \
	X(ka_movement,                        CURSOR_WORD_START_NEXT,           .i = VIS_MOVE_WORD_START_NEXT,            "vis-motion-word-start-next",          "Move cursor words forwards") \
	X(ka_movement,                        CURSOR_WORD_START_PREV,           .i = VIS_MOVE_WORD_START_PREV,            "vis-motion-word-start-prev",          "Move cursor words backwards") \
	X(ka_movement,                        TOTILL_REPEAT,                    .i = VIS_MOVE_TOTILL_REPEAT,              "vis-motion-totill-repeat",            "Repeat latest to/till motion") \
	X(ka_movement,                        TOTILL_REVERSE,                   .i = VIS_MOVE_TOTILL_REVERSE,             "vis-motion-totill-reverse",           "Repeat latest to/till motion but in opposite direction") \
	X(ka_movement_key,                    TILL_LEFT,                        .i = VIS_MOVE_TILL_LEFT,                  "vis-motion-till-left",                "Till after the occurrence of character to the left") \
	X(ka_movement_key,                    TILL_LINE_LEFT,                   .i = VIS_MOVE_TILL_LINE_LEFT,             "vis-motion-till-line-left",           "Till after the occurrence of character to the left on the current line") \
	X(ka_movement_key,                    TILL_LINE_RIGHT,                  .i = VIS_MOVE_TILL_LINE_RIGHT,            "vis-motion-till-line-right",          "Till before the occurrence of character to the right on the current line") \
	X(ka_movement_key,                    TILL_RIGHT,                       .i = VIS_MOVE_TILL_RIGHT,                 "vis-motion-till-right",               "Till before the occurrence of character to the right") \
	X(ka_movement_key,                    TO_LEFT,                          .i = VIS_MOVE_TO_LEFT,                    "vis-motion-to-left",                  "To the first occurrence of character to the left") \
	X(ka_movement_key,                    TO_LINE_LEFT,                     .i = VIS_MOVE_TO_LINE_LEFT,               "vis-motion-to-line-left",             "To the first occurrence of character to the left on the current line") \
	X(ka_movement_key,                    TO_LINE_RIGHT,                    .i = VIS_MOVE_TO_LINE_RIGHT,              "vis-motion-to-line-right",            "To the first occurrence of character to the right on the current line") \
	X(ka_movement_key,                    TO_RIGHT,                         .i = VIS_MOVE_TO_RIGHT,                   "vis-motion-to-right",                 "To the first occurrence of character to the right") \
	X(ka_nop,                             NOP,                              0,                                        "vis-nop",                             "Ignore key, do nothing") \
	X(ka_normalmode_escape,               MODE_NORMAL_ESCAPE,               0,                                        "vis-mode-normal-escape",              "Reset count or remove all non-primary selections") \
	X(ka_openline,                        OPEN_LINE_ABOVE,                  .i = -1,                                  "vis-open-line-above",                 "Begin a new line above the cursor") \
	X(ka_openline,                        OPEN_LINE_BELOW,                  .i = +1,                                  "vis-open-line-below",                 "Begin a new line below the cursor") \
	X(ka_operator,                        OPERATOR_CHANGE,                  .i = VIS_OP_CHANGE,                       "vis-operator-change",                 "Change operator") \
	X(ka_operator,                        OPERATOR_DELETE,                  .i = VIS_OP_DELETE,                       "vis-operator-delete",                 "Delete operator") \
	X(ka_operator,                        OPERATOR_SHIFT_LEFT,              .i = VIS_OP_SHIFT_LEFT,                   "vis-operator-shift-left",             "Shift left operator") \
	X(ka_operator,                        OPERATOR_SHIFT_RIGHT,             .i = VIS_OP_SHIFT_RIGHT,                  "vis-operator-shift-right",            "Shift right operator") \
	X(ka_operator,                        OPERATOR_YANK,                    .i = VIS_OP_YANK,                         "vis-operator-yank",                   "Yank operator") \
	X(ka_operator,                        PUT_AFTER,                        .i = VIS_OP_PUT_AFTER,                    "vis-put-after",                       "Put text after the cursor") \
	X(ka_operator,                        PUT_BEFORE,                       .i = VIS_OP_PUT_BEFORE,                   "vis-put-before",                      "Put text before the cursor") \
	X(ka_operator,                        SELECTIONS_NEW_LINES_BEGIN,       .i = VIS_OP_CURSOR_SOL,                   "vis-selection-new-lines-begin",       "Create a new selection at the start of every line covered by selection") \
	X(ka_operator,                        SELECTIONS_NEW_LINES_END,         .i = VIS_OP_CURSOR_EOL,                   "vis-selection-new-lines-end",         "Create a new selection at the end of every line covered by selection") \
	X(ka_percent,                         CURSOR_PERCENT,                   0,                                        "vis-motion-percent",                  "Move to count % of file or matching item") \
	X(ka_prompt_show,                     PROMPT_SEARCH_BACKWARD,           .s = "?",                                 "vis-search-backward",                 "Search backward") \
	X(ka_prompt_show,                     PROMPT_SEARCH_FORWARD,            .s = "/",                                 "vis-search-forward",                  "Search forward") \
	X(ka_prompt_show,                     PROMPT_SHOW,                      .s = ":",                                 "vis-prompt-show",                     "Show editor command line prompt") \
	X(ka_redo,                            REDO,                             0,                                        "vis-redo",                            "Redo last change") \
	X(ka_reg,                             REGISTER,                         0,                                        "vis-register",                        "Use given register for next operator") \
	X(ka_repeat,                          REPEAT,                           0,                                        "vis-repeat",                          "Repeat latest editor command") \
	X(ka_replace,                         REPLACE_CHAR,                     0,                                        "vis-replace-char",                    "Replace the character under the cursor") \
	X(ka_replacemode,                     MODE_REPLACE,                     .i = VIS_MOVE_NOP,                        "vis-mode-replace",                    "Enter replace mode") \
	X(ka_selection_end,                   SELECTION_FLIP,                   0,                                        "vis-selection-flip",                  "Flip selection, move cursor to other end") \
	X(ka_selections_align,                SELECTIONS_ALIGN,                 0,                                        "vis-selections-align",                "Try to align all selections on the same column") \
	X(ka_selections_align_indent,         SELECTIONS_ALIGN_INDENT_LEFT,     .i = -1,                                  "vis-selections-align-indent-left",    "Left-align all selections by inserting spaces") \
	X(ka_selections_align_indent,         SELECTIONS_ALIGN_INDENT_RIGHT,    .i = +1,                                  "vis-selections-align-indent-right",   "Right-align all selections by inserting spaces") \
	X(ka_selections_clear,                SELECTIONS_REMOVE_ALL,            0,                                        "vis-selections-remove-all",           "Remove all but the primary selection") \
	X(ka_selections_complement,           SELECTIONS_COMPLEMENT,            0,                                        "vis-selections-complement",           "Complement selections") \
	X(ka_selections_intersect,            SELECTIONS_INTERSECT,             0,                                        "vis-selections-intersect",            "Intersect with selections from mark") \
	X(ka_selections_match_next,           SELECTIONS_NEW_MATCH_ALL,         .b = true,                                "vis-selection-new-match-all",         "Select all regions matching the current selection") \
	X(ka_selections_match_next,           SELECTIONS_NEW_MATCH_NEXT,        0,                                        "vis-selection-new-match-next",        "Select the next region matching the current selection") \
	X(ka_selections_match_skip,           SELECTIONS_NEW_MATCH_SKIP,        0,                                        "vis-selection-new-match-skip",        "Clear current selection, but select next match") \
	X(ka_selections_minus,                SELECTIONS_MINUS,                 0,                                        "vis-selections-minus",                "Subtract selections from mark") \
	X(ka_selections_navigate,             SELECTIONS_NEXT,                  .i = +PAGE_HALF,                          "vis-selection-next",                  "Move to the next selection") \
	X(ka_selections_navigate,             SELECTIONS_PREV,                  .i = -PAGE_HALF,                          "vis-selection-prev",                  "Move to the previous selection") \
	X(ka_selections_new,                  SELECTIONS_NEW_LINE_ABOVE,        .i = -1,                                  "vis-selection-new-lines-above",       "Create a new selection on the line above") \
	X(ka_selections_new,                  SELECTIONS_NEW_LINE_ABOVE_FIRST,  .i = INT_MIN,                             "vis-selection-new-lines-above-first", "Create a new selection on the line above the first selection") \
	X(ka_selections_new,                  SELECTIONS_NEW_LINE_BELOW,        .i = +1,                                  "vis-selection-new-lines-below",       "Create a new selection on the line below") \
	X(ka_selections_new,                  SELECTIONS_NEW_LINE_BELOW_LAST,   .i = INT_MAX,                             "vis-selection-new-lines-below-last",  "Create a new selection on the line below the last selection") \
	X(ka_selections_remove,               SELECTIONS_REMOVE_LAST,           0,                                        "vis-selections-remove-last",          "Remove primary selection") \
	X(ka_selections_remove_column,        SELECTIONS_REMOVE_COLUMN,         .i = 1,                                   "vis-selections-remove-column",        "Remove count selection column") \
	X(ka_selections_remove_column_except, SELECTIONS_REMOVE_COLUMN_EXCEPT,  .i = 1,                                   "vis-selections-remove-column-except", "Remove all but the count selection column") \
	X(ka_selections_restore,              SELECTIONS_RESTORE,               0,                                        "vis-selections-restore",              "Restore selections from mark") \
	X(ka_selections_rotate,               SELECTIONS_ROTATE_LEFT,           .i = -1,                                  "vis-selections-rotate-left",          "Rotate selections left") \
	X(ka_selections_rotate,               SELECTIONS_ROTATE_RIGHT,          .i = +1,                                  "vis-selections-rotate-right",         "Rotate selections right") \
	X(ka_selections_save,                 SELECTIONS_SAVE,                  0,                                        "vis-selections-save",                 "Save currently active selections to mark") \
	X(ka_selections_trim,                 SELECTIONS_TRIM,                  0,                                        "vis-selections-trim",                 "Remove leading and trailing white space from selections") \
	X(ka_selections_union,                SELECTIONS_UNION,                 0,                                        "vis-selections-union",                "Add selections from mark") \
	X(ka_suspend,                         EDITOR_SUSPEND,                   0,                                        "vis-suspend",                         "Suspend the editor") \
	X(ka_switchmode,                      MODE_NORMAL,                      .i = VIS_MODE_NORMAL,                     "vis-mode-normal",                     "Enter normal mode") \
	X(ka_switchmode,                      MODE_VISUAL,                      .i = VIS_MODE_VISUAL,                     "vis-mode-visual-charwise",            "Enter characterwise visual mode") \
	X(ka_switchmode,                      MODE_VISUAL_LINE,                 .i = VIS_MODE_VISUAL_LINE,                "vis-mode-visual-linewise",            "Enter linewise visual mode") \
	X(ka_text_object,                     TEXT_OBJECT_ANGLE_BRACKET_INNER,  .i = VIS_TEXTOBJECT_INNER_ANGLE_BRACKET,  "vis-textobject-angle-bracket-inner",  "<> block (inner variant)") \
	X(ka_text_object,                     TEXT_OBJECT_ANGLE_BRACKET_OUTER,  .i = VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET,  "vis-textobject-angle-bracket-outer",  "<> block (outer variant)") \
	X(ka_text_object,                     TEXT_OBJECT_BACKTICK_INNER,       .i = VIS_TEXTOBJECT_INNER_BACKTICK,       "vis-textobject-backtick-inner",       "A backtick delimited string (inner variant)") \
	X(ka_text_object,                     TEXT_OBJECT_BACKTICK_OUTER,       .i = VIS_TEXTOBJECT_OUTER_BACKTICK,       "vis-textobject-backtick-outer",       "A backtick delimited string (outer variant)") \
	X(ka_text_object,                     TEXT_OBJECT_CURLY_BRACKET_INNER,  .i = VIS_TEXTOBJECT_INNER_CURLY_BRACKET,  "vis-textobject-curly-bracket-inner",  "{} block (inner variant)") \
	X(ka_text_object,                     TEXT_OBJECT_CURLY_BRACKET_OUTER,  .i = VIS_TEXTOBJECT_OUTER_CURLY_BRACKET,  "vis-textobject-curly-bracket-outer",  "{} block (outer variant)") \
	X(ka_text_object,                     TEXT_OBJECT_INDENTATION,          .i = VIS_TEXTOBJECT_INDENTATION,          "vis-textobject-indentation",          "All adjacent lines with the same indentation level as the current one") \
	X(ka_text_object,                     TEXT_OBJECT_LINE_INNER,           .i = VIS_TEXTOBJECT_INNER_LINE,           "vis-textobject-line-inner",           "The whole line, excluding leading and trailing whitespace") \
	X(ka_text_object,                     TEXT_OBJECT_LINE_OUTER,           .i = VIS_TEXTOBJECT_OUTER_LINE,           "vis-textobject-line-outer",           "The whole line") \
	X(ka_text_object,                     TEXT_OBJECT_LONGWORD_INNER,       .i = VIS_TEXTOBJECT_INNER_LONGWORD,       "vis-textobject-bigword-inner",        "A WORD leading and trailing whitespace excluded") \
	X(ka_text_object,                     TEXT_OBJECT_LONGWORD_OUTER,       .i = VIS_TEXTOBJECT_OUTER_LONGWORD,       "vis-textobject-bigword-outer",        "A WORD leading and trailing whitespace included") \
	X(ka_text_object,                     TEXT_OBJECT_PARAGRAPH,            .i = VIS_TEXTOBJECT_PARAGRAPH,            "vis-textobject-paragraph",            "A paragraph") \
	X(ka_text_object,                     TEXT_OBJECT_PARAGRAPH_OUTER,      .i = VIS_TEXTOBJECT_PARAGRAPH_OUTER,      "vis-textobject-paragraph-outer",      "A paragraph (outer variant)") \
	X(ka_text_object,                     TEXT_OBJECT_PARENTHESIS_INNER,    .i = VIS_TEXTOBJECT_INNER_PARENTHESIS,    "vis-textobject-parenthesis-inner",    "() block (inner variant)") \
	X(ka_text_object,                     TEXT_OBJECT_PARENTHESIS_OUTER,    .i = VIS_TEXTOBJECT_OUTER_PARENTHESIS,    "vis-textobject-parenthesis-outer",    "() block (outer variant)") \
	X(ka_text_object,                     TEXT_OBJECT_QUOTE_INNER,          .i = VIS_TEXTOBJECT_INNER_QUOTE,          "vis-textobject-quote-inner",          "A quoted string, excluding the quotation marks") \
	X(ka_text_object,                     TEXT_OBJECT_QUOTE_OUTER,          .i = VIS_TEXTOBJECT_OUTER_QUOTE,          "vis-textobject-quote-outer",          "A quoted string, including the quotation marks") \
	X(ka_text_object,                     TEXT_OBJECT_SEARCH_BACKWARD,      .i = VIS_TEXTOBJECT_SEARCH_BACKWARD,      "vis-textobject-search-backward",      "The next search match in backward direction") \
	X(ka_text_object,                     TEXT_OBJECT_SEARCH_FORWARD,       .i = VIS_TEXTOBJECT_SEARCH_FORWARD,       "vis-textobject-search-forward",       "The next search match in forward direction") \
	X(ka_text_object,                     TEXT_OBJECT_SENTENCE,             .i = VIS_TEXTOBJECT_SENTENCE,             "vis-textobject-sentence",             "A sentence") \
	X(ka_text_object,                     TEXT_OBJECT_SINGLE_QUOTE_INNER,   .i = VIS_TEXTOBJECT_INNER_SINGLE_QUOTE,   "vis-textobject-single-quote-inner",   "A single quoted string, excluding the quotation marks") \
	X(ka_text_object,                     TEXT_OBJECT_SINGLE_QUOTE_OUTER,   .i = VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE,   "vis-textobject-single-quote-outer",   "A single quoted string, including the quotation marks") \
	X(ka_text_object,                     TEXT_OBJECT_SQUARE_BRACKET_INNER, .i = VIS_TEXTOBJECT_INNER_SQUARE_BRACKET, "vis-textobject-square-bracket-inner", "[] block (inner variant)") \
	X(ka_text_object,                     TEXT_OBJECT_SQUARE_BRACKET_OUTER, .i = VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET, "vis-textobject-square-bracket-outer", "[] block (outer variant)") \
	X(ka_text_object,                     TEXT_OBJECT_WORD_INNER,           .i = VIS_TEXTOBJECT_INNER_WORD,           "vis-textobject-word-inner",           "A word leading and trailing whitespace excluded") \
	X(ka_text_object,                     TEXT_OBJECT_WORD_OUTER,           .i = VIS_TEXTOBJECT_OUTER_WORD,           "vis-textobject-word-outer",           "A word leading and trailing whitespace included") \
	X(ka_undo,                            UNDO,                             0,                                        "vis-undo",                            "Undo last change") \
	X(ka_unicode_info,                    UNICODE_INFO,                     .i = VIS_ACTION_UNICODE_INFO,             "vis-unicode-info",                    "Show Unicode codepoint(s) of character under cursor") \
	X(ka_unicode_info,                    UTF8_INFO,                        .i = VIS_ACTION_UTF8_INFO,                "vis-utf8-info",                       "Show UTF-8 encoded codepoint(s) of character under cursor") \
	X(ka_visualmode_escape,               MODE_VISUAL_ESCAPE,               0,                                        "vis-mode-visual-escape",              "Reset count or switch to normal mode") \
	X(ka_window,                          WINDOW_REDRAW_BOTTOM,             .w = view_redraw_bottom,                  "vis-window-redraw-bottom",            "Redraw cursor line at the bottom of the window") \
	X(ka_window,                          WINDOW_REDRAW_CENTER,             .w = view_redraw_center,                  "vis-window-redraw-center",            "Redraw cursor line at the center of the window") \
	X(ka_window,                          WINDOW_REDRAW_TOP,                .w = view_redraw_top,                     "vis-window-redraw-top",               "Redraw cursor line at the top of the window") \
	X(ka_wscroll,                         WINDOW_HALFPAGE_DOWN,             .i = +PAGE_HALF,                          "vis-window-halfpage-down",            "Scroll window half pages forwards (downwards)") \
	X(ka_wscroll,                         WINDOW_HALFPAGE_UP,               .i = -PAGE_HALF,                          "vis-window-halfpage-up",              "Scroll window half pages backwards (upwards)") \
	X(ka_wscroll,                         WINDOW_PAGE_DOWN,                 .i = +PAGE,                               "vis-window-page-down",                "Scroll window pages forwards (downwards)") \
	X(ka_wscroll,                         WINDOW_PAGE_UP,                   .i = -PAGE,                               "vis-window-page-up",                  "Scroll window pages backwards (upwards)") \
	X(ka_wslide,                          WINDOW_SLIDE_DOWN,                .i = +1,                                  "vis-window-slide-down",               "Slide window content downwards") \
	X(ka_wslide,                          WINDOW_SLIDE_UP,                  .i = -1,                                  "vis-window-slide-up",                 "Slide window content upwards") \

#define ENUM(_, e, ...) VIS_ACTION_##e,
typedef enum { KEY_ACTION_LIST(ENUM) } VisActionKind;
#undef ENUM

/* NOTE: must conform to the vis library signature, but we can rename the parameters */
#undef KEY_ACTION_FN
#define KEY_ACTION_FN(name) const char *name(Vis *_unused, const char *keys, const Arg *arg)

/** key bindings functions */

static KEY_ACTION_FN(ka_nop)
{
	return keys;
}

static KEY_ACTION_FN(ka_macro_record)
{
	if (!vis_macro_record_stop(vis)) {
		if (!keys[0])
			return NULL;
		const char *next = vis_keys_next(vis, keys);
		if (next - keys > 1)
			return next;
		enum VisRegister reg = vis_register_from(vis, keys[0]);
		vis_macro_record(vis, reg);
		keys++;
	}
	vis_draw(vis);
	return keys;
}

static KEY_ACTION_FN(ka_macro_replay)
{
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisRegister reg = vis_register_from(vis, keys[0]);
	vis_macro_replay(vis, reg);
	return keys+1;
}

static KEY_ACTION_FN(ka_suspend)
{
	ui_terminal_suspend(&vis->ui);
	return keys;
}

static KEY_ACTION_FN(ka_repeat)
{
	vis_repeat(vis);
	return keys;
}

static KEY_ACTION_FN(ka_selections_new)
{
	View *view = vis_view(vis);
	bool anchored = view_selections_primary_get(view)->anchored;
	VisCountIterator it = vis_count_iterator_get(vis, 1);
	while (vis_count_iterator_next(&it)) {
		Selection *sel = NULL;
		switch (arg->i) {
		case -1:
		case +1:
			sel = view_selections_primary_get(view);
			break;
		case INT_MIN:
			sel = view_selections(view);
			break;
		case INT_MAX:
			for (Selection *s = view_selections(view); s; s = view_selections_next(s))
				sel = s;
			break;
		}

		if (!sel)
			return keys;

		size_t oldpos = view_cursors_pos(sel);
		if (arg->i > 0)
			view_line_down(sel);
		else if (arg->i < 0)
			view_line_up(sel);
		size_t newpos = view_cursors_pos(sel);
		view_cursors_to(sel, oldpos);
		Selection *sel_new = view_selections_new(view, newpos);
		if (!sel_new) {
			if (arg->i == -1)
				sel_new = view_selections_prev(sel);
			else if (arg->i == +1)
				sel_new = view_selections_next(sel);
		}
		if (sel_new) {
			view_selections_primary_set(sel_new);
			sel_new->anchored = anchored;
		}
	}
	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_selections_align)
{
	View *view = vis_view(vis);
	Text *txt = vis_text(vis);
	int mincol = INT_MAX;
	for (Selection *s = view_selections(view); s; s = view_selections_next(s)) {
		if (!s->line)
			continue;
		if (s->col >= 0 && s->col < mincol)
			mincol = s->col;
	}
	for (Selection *s = view_selections(view); s; s = view_selections_next(s)) {
		if (view_cursors_cell_set(s, mincol) == -1) {
			size_t pos = view_cursors_pos(s);
			size_t col = text_line_width_set(txt, pos, mincol);
			view_cursors_to(s, col);
		}
	}
	return keys;
}

static KEY_ACTION_FN(ka_selections_align_indent)
{
	View *view = vis_view(vis);
	Text *txt = vis_text(vis);
	bool left_align = arg->i < 0;
	int columns = view_selections_column_count(view);

	for (int i = 0; i < columns; i++) {
		int mincol = INT_MAX, maxcol = 0;
		for (Selection *s = view_selections_column(view, i); s; s = view_selections_column_next(s, i)) {
			Filerange sel = view_selections_get(s);
			size_t pos = left_align ? sel.start : sel.end;
			int col = text_line_width_get(txt, pos);
			if (col < mincol)
				mincol = col;
			if (col > maxcol)
				maxcol = col;
		}

		size_t len = maxcol - mincol;
		char *buf = malloc(len+1);
		if (!buf)
			return keys;
		memset(buf, ' ', len);

		for (Selection *s = view_selections_column(view, i); s; s = view_selections_column_next(s, i)) {
			Filerange sel = view_selections_get(s);
			size_t pos = left_align ? sel.start : sel.end;
			size_t ipos = sel.start;
			int col = text_line_width_get(txt, pos);
			if (col < maxcol) {
				size_t off = maxcol - col;
				if (off <= len)
					text_insert(txt, ipos, buf, off);
			}
		}

		free(buf);
	}

	view_draw(view);
	return keys;
}

static KEY_ACTION_FN(ka_selections_clear)
{
	View *view = vis_view(vis);
	if (view->selection_count > 1)
		view_selections_dispose_all(view);
	else
		view_selection_clear(view_selections_primary_get(view));
	return keys;
}

static Selection *ka_selection_new(View *view, Filerange *r, bool isprimary) {
	Text *txt = view->text;
	size_t pos = text_char_prev(txt, r->end);
	Selection *s = view_selections_new(view, pos);
	if (!s)
		return NULL;
	view_selections_set(s, r);
	s->anchored = true;
	if (isprimary)
		view_selections_primary_set(s);
	return s;
}

static KEY_ACTION_FN(ka_selections_match_next)
{
	Text *txt = vis_text(vis);
	View *view = vis_view(vis);
	Selection *s = view_selections_primary_get(view);
	Filerange sel = view_selections_get(s);
	if (!text_range_valid(&sel))
		return keys;

	static bool match_word;

	if (view->selection_count == 1) {
		Filerange word = text_object_word(txt, view_cursors_pos(s));
		match_word = text_range_equal(&sel, &word);
	}

	Filerange (*find_next)(Text *, size_t, const char *) = text_object_word_find_next;
	Filerange (*find_prev)(Text *, size_t, const char *) = text_object_word_find_prev;
	if (!match_word) {
		find_next = text_object_find_next;
		find_prev = text_object_find_prev;
	}

	char *buf = text_bytes_alloc0(txt, sel.start, text_range_size(&sel));
	if (!buf)
		return keys;

	bool match_all = arg->b;
	Filerange primary = sel;

	for (;;) {
		sel = find_next(txt, sel.end, buf);
		if (!text_range_valid(&sel))
			break;
		if (ka_selection_new(view, &sel, !match_all) && !match_all)
			goto out;
	}

	sel = primary;

	for (;;) {
		sel = find_prev(txt, sel.start, buf);
		if (!text_range_valid(&sel))
			break;
		if (ka_selection_new(view, &sel, !match_all) && !match_all)
			break;
	}

out:
	free(buf);
	return keys;
}

static KEY_ACTION_FN(ka_selections_match_skip)
{
	View *view = vis_view(vis);
	Selection *sel = view_selections_primary_get(view);
	keys = ka_selections_match_next(vis, keys, arg);
	if (sel != view_selections_primary_get(view))
		view_selections_dispose(sel);
	return keys;
}

static KEY_ACTION_FN(ka_selections_remove)
{
	View *view = vis_view(vis);
	view_selections_dispose(view_selections_primary_get(view));
	view_cursors_to(view->selection, view_cursor_get(view));
	return keys;
}

static KEY_ACTION_FN(ka_selections_remove_column)
{
	View *view = vis_view(vis);
	int max = view_selections_column_count(view);
	int column = VIS_COUNT_DEFAULT(vis->action.count, arg->i) - 1;
	if (column >= max)
		column = max - 1;
	if (view->selection_count == 1) {
		vis_keys_feed(vis, "<Escape>");
		return keys;
	}

	for (Selection *s = view_selections_column(view, column), *next; s; s = next) {
		next = view_selections_column_next(s, column);
		view_selections_dispose(s);
	}

	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_selections_remove_column_except)
{
	View *view = vis_view(vis);
	int max = view_selections_column_count(view);
	int column = VIS_COUNT_DEFAULT(vis->action.count, arg->i) - 1;
	if (column >= max)
		column = max - 1;
	if (view->selection_count == 1) {
		vis_redraw(vis);
		return keys;
	}

	Selection *sel = view_selections(view);
	Selection *col = view_selections_column(view, column);
	for (Selection *next; sel; sel = next) {
		next = view_selections_next(sel);
		if (sel == col)
			col = view_selections_column_next(col, column);
		else
			view_selections_dispose(sel);
	}

	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_wscroll)
{
	View *view = vis_view(vis);
	int count = vis->action.count;
	switch (arg->i) {
	case -PAGE:
		view_scroll_page_up(view);
		break;
	case +PAGE:
		view_scroll_page_down(view);
		break;
	case -PAGE_HALF:
		view_scroll_halfpage_up(view);
		break;
	case +PAGE_HALF:
		view_scroll_halfpage_down(view);
		break;
	default:
		if (count == VIS_COUNT_UNKNOWN)
			count = arg->i < 0 ? -arg->i : arg->i;
		if (arg->i < 0)
			view_scroll_up(view, count);
		else
			view_scroll_down(view, count);
		break;
	}
	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_wslide)
{
	View *view = vis_view(vis);
	int count = vis->action.count;
	if (count == VIS_COUNT_UNKNOWN)
		count = arg->i < 0 ? -arg->i : arg->i;
	if (arg->i >= 0)
		view_slide_down(view, count);
	else
		view_slide_up(view, count);
	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_selections_navigate)
{
	View *view = vis_view(vis);
	if (view->selection_count == 1)
		return ka_wscroll(vis, keys, arg);
	Selection *s = view_selections_primary_get(view);
	VisCountIterator it = vis_count_iterator_get(vis, 1);
	while (vis_count_iterator_next(&it)) {
		if (arg->i > 0) {
			s = view_selections_next(s);
			if (!s)
				s = view_selections(view);
		} else {
			s = view_selections_prev(s);
			if (!s) {
				s = view_selections(view);
				for (Selection *n = s; n; n = view_selections_next(n))
					s = n;
			}
		}
	}
	view_selections_primary_set(s);
	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_selections_rotate)
{
	typedef struct {
		Selection *sel;
		char *data;
		size_t len;
	} Rotate;

	Array arr;
	Text *txt = vis_text(vis);
	View *view = vis_view(vis);
	int columns = view_selections_column_count(view);
	int selections = columns == 1 ? view->selection_count : columns;
	int count = VIS_COUNT_DEFAULT(vis->action.count, 1);
	array_init_sized(&arr, sizeof(Rotate));
	if (!array_reserve(&arr, selections))
		return keys;
	size_t line = 0;

	for (Selection *s = view_selections(view), *next; s; s = next) {
		next = view_selections_next(s);
		size_t line_next = 0;

		Filerange sel = view_selections_get(s);
		Rotate rot;
		rot.sel = s;
		rot.len = text_range_size(&sel);
		if ((rot.data = malloc(rot.len)))
			rot.len = text_bytes_get(txt, sel.start, rot.len, rot.data);
		else
			rot.len = 0;
		array_add(&arr, &rot);

		if (!line)
			line = text_lineno_by_pos(txt, view_cursors_pos(s));
		if (next)
			line_next = text_lineno_by_pos(txt, view_cursors_pos(next));
		if (!next || (columns > 1 && line != line_next)) {
			size_t len = arr.len;
			size_t off = arg->i > 0 ? count % len : len - (count % len);
			for (size_t i = 0; i < len; i++) {
				size_t j = (i + off) % len;
				Rotate *oldrot = array_get(&arr, i);
				Rotate *newrot = array_get(&arr, j);
				if (!oldrot || !newrot || oldrot == newrot)
					continue;
				Filerange newsel = view_selections_get(newrot->sel);
				if (!text_range_valid(&newsel))
					continue;
				if (!text_delete_range(txt, &newsel))
					continue;
				if (!text_insert(txt, newsel.start, oldrot->data, oldrot->len))
					continue;
				newsel.end = newsel.start + oldrot->len;
				view_selections_set(newrot->sel, &newsel);
				free(oldrot->data);
			}
			array_clear(&arr);
		}
		line = line_next;
	}

	array_release(&arr);
	vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_selections_trim)
{
	Text *txt = vis_text(vis);
	View *view = vis_view(vis);
	for (Selection *s = view_selections(view), *next; s; s = next) {
		next = view_selections_next(s);
		Filerange sel = view_selections_get(s);
		if (!text_range_valid(&sel))
			continue;
		for (char b; sel.start < sel.end && text_byte_get(txt, sel.end-1, &b)
			&& isspace((unsigned char)b); sel.end--);
		for (char b; sel.start <= sel.end && text_byte_get(txt, sel.start, &b)
			&& isspace((unsigned char)b); sel.start++);
		if (sel.start < sel.end) {
			view_selections_set(s, &sel);
		} else if (!view_selections_dispose(s)) {
			vis_mode_switch(vis, VIS_MODE_NORMAL);
		}
	}
	return keys;
}

static void selections_set(Vis *vis, View *view, Array *sel) {
	enum VisMode mode = vis->mode->id;
	bool anchored = mode == VIS_MODE_VISUAL || mode == VIS_MODE_VISUAL_LINE;
	view_selections_set_all(view, sel, anchored);
	if (!anchored)
		view_selections_clear_all(view);
}

static KEY_ACTION_FN(ka_selections_save)
{
	Win *win = vis->win;
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array sel = view_selections_get_all(view);
	vis_mark_set(win, mark, &sel);
	array_release(&sel);
	vis_cancel(vis);
	return keys;
}

static KEY_ACTION_FN(ka_selections_restore)
{
	Win *win = vis->win;
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array sel = vis_mark_get(win, mark);
	selections_set(vis, view, &sel);
	array_release(&sel);
	vis_cancel(vis);
	return keys;
}

static KEY_ACTION_FN(ka_selections_union)
{
	Win *win = vis->win;
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array a = vis_mark_get(win, mark);
	Array b = view_selections_get_all(view);
	Array sel;
	array_init_from(&sel, &a);

	size_t i = 0, j = 0;
	Filerange *r1 = array_get(&a, i), *r2 = array_get(&b, j), cur = text_range_empty();
	while (r1 || r2) {
		if (r1 && text_range_overlap(r1, &cur)) {
			cur = text_range_union(r1, &cur);
			r1 = array_get(&a, ++i);
		} else if (r2 && text_range_overlap(r2, &cur)) {
			cur = text_range_union(r2, &cur);
			r2 = array_get(&b, ++j);
		} else {
			if (text_range_valid(&cur))
				array_add(&sel, &cur);
			if (!r1) {
				cur = *r2;
				r2 = array_get(&b, ++j);
			} else if (!r2) {
				cur = *r1;
				r1 = array_get(&a, ++i);
			} else {
				if (r1->start < r2->start) {
					cur = *r1;
					r1 = array_get(&a, ++i);
				} else {
					cur = *r2;
					r2 = array_get(&b, ++j);
				}
			}
		}
	}

	if (text_range_valid(&cur))
		array_add(&sel, &cur);

	selections_set(vis, view, &sel);
	vis_cancel(vis);

	array_release(&a);
	array_release(&b);
	array_release(&sel);

	return keys;
}

static void intersect(Array *ret, Array *a, Array *b) {
	size_t i = 0, j = 0;
	Filerange *r1 = array_get(a, i), *r2 = array_get(b, j);
	while (r1 && r2) {
		if (text_range_overlap(r1, r2)) {
			Filerange new = text_range_intersect(r1, r2);
			array_add(ret, &new);
		}
		if (r1->end < r2->end)
			r1 = array_get(a, ++i);
		else
			r2 = array_get(b, ++j);
	}
}

static KEY_ACTION_FN(ka_selections_intersect)
{
	Win *win = vis->win;
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array a = vis_mark_get(win, mark);
	Array b = view_selections_get_all(view);
	Array sel;
	array_init_from(&sel, &a);

	intersect(&sel, &a, &b);
	selections_set(vis, view, &sel);
	vis_cancel(vis);

	array_release(&a);
	array_release(&b);
	array_release(&sel);

	return keys;
}

static void complement(Array *ret, Array *a, Filerange *universe) {
	size_t pos = universe->start;
	for (size_t i = 0, len = a->len; i < len; i++) {
		Filerange *r = array_get(a, i);
		if (pos < r->start) {
			Filerange new = text_range_new(pos, r->start);
			array_add(ret, &new);
		}
		pos = r->end;
	}
	if (pos < universe->end) {
		Filerange new = text_range_new(pos, universe->end);
		array_add(ret, &new);
	}
}

static KEY_ACTION_FN(ka_selections_complement)
{
	Text *txt = vis_text(vis);
	View *view = vis_view(vis);
	Filerange universe = text_object_entire(txt, 0);
	Array a = view_selections_get_all(view);
	Array sel;
	array_init_from(&sel, &a);

	complement(&sel, &a, &universe);

	selections_set(vis, view, &sel);
	array_release(&a);
	array_release(&sel);
	return keys;
}

static KEY_ACTION_FN(ka_selections_minus)
{
	Text *txt = vis_text(vis);
	Win *win = vis->win;
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array a = view_selections_get_all(view);
	Array b = vis_mark_get(win, mark);
	Array sel;
	array_init_from(&sel, &a);
	Array b_complement;
	array_init_from(&b_complement, &b);

	Filerange universe = text_object_entire(txt, 0);
	complement(&b_complement, &b, &universe);
	intersect(&sel, &a, &b_complement);

	selections_set(vis, view, &sel);
	vis_cancel(vis);

	array_release(&a);
	array_release(&b);
	array_release(&b_complement);
	array_release(&sel);

	return keys;
}

static KEY_ACTION_FN(ka_replace)
{
	if (!keys[0]) {
		vis_keymap_disable(vis);
		return NULL;
	}

	const char *next = vis_keys_next(vis, keys);
	if (!next)
		return NULL;

	char replacement[UTFmax+1];
	if (!vis_keys_utf8(vis, keys, replacement))
		return next;

	if (replacement[0] == 0x1b) /* <Escape> */
		return next;

	vis_operator(vis, VIS_OP_REPLACE, replacement);
	if (vis->mode->id == VIS_MODE_OPERATOR_PENDING)
		vis_motion(vis, VIS_MOVE_CHAR_NEXT);
	return next;
}

static KEY_ACTION_FN(ka_count)
{
	int digit = keys[-1] - '0';
	int count = VIS_COUNT_DEFAULT(vis->action.count, 0);
	if (0 <= digit && digit <= 9) {
		if (digit == 0 && count == 0)
			vis_motion(vis, VIS_MOVE_LINE_BEGIN);
		else
			vis->action.count = VIS_COUNT_NORMALIZE(count * 10 + digit);
	}
	return keys;
}

static KEY_ACTION_FN(ka_gotoline)
{
	if (vis->action.count != VIS_COUNT_UNKNOWN)
		vis_motion(vis, VIS_MOVE_LINE);
	else if (arg->i < 0)
		vis_motion(vis, VIS_MOVE_FILE_BEGIN);
	else
		vis_motion(vis, VIS_MOVE_FILE_END);
	return keys;
}

static KEY_ACTION_FN(ka_operator)
{
	vis_operator(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_movement_key)
{
	if (!keys[0]) {
		vis_keymap_disable(vis);
		return NULL;
	}

	const char *next = vis_keys_next(vis, keys);
	if (!next)
		return NULL;
	char utf8[UTFmax+1];
	if (vis_keys_utf8(vis, keys, utf8))
		vis_motion(vis, arg->i, utf8);
	return next;
}

static KEY_ACTION_FN(ka_movement)
{
	vis_motion(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_text_object)
{
	vis_textobject(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_selection_end)
{
	for (Selection *s = view_selections(vis_view(vis)); s; s = view_selections_next(s))
		view_selections_flip(s);
	return keys;
}

static KEY_ACTION_FN(ka_reg)
{
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisRegister reg = vis_register_from(vis, keys[0]);
	vis_register(vis, reg);
	return keys+1;
}

static KEY_ACTION_FN(ka_mark)
{
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisMark mark = vis_mark_from(vis, keys[0]);
	vis_mark(vis, mark);
	return keys+1;
}

static KEY_ACTION_FN(ka_undo)
{
	size_t pos = text_undo(vis_text(vis));
	if (pos != EPOS) {
		View *view = vis_view(vis);
		if (view->selection_count == 1)
			view_cursors_to(view->selection, pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static KEY_ACTION_FN(ka_redo)
{
	size_t pos = text_redo(vis_text(vis));
	if (pos != EPOS) {
		View *view = vis_view(vis);
		if (view->selection_count == 1)
			view_cursors_to(view->selection, pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static KEY_ACTION_FN(ka_earlier)
{
	size_t pos = EPOS;
	VisCountIterator it = vis_count_iterator_get(vis, 1);
	while (vis_count_iterator_next(&it))
		pos = text_earlier(vis_text(vis));
	if (pos != EPOS) {
		view_cursors_to(vis_view(vis)->selection, pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static KEY_ACTION_FN(ka_later)
{
	size_t pos = EPOS;
	VisCountIterator it = vis_count_iterator_get(vis, 1);
	while (vis_count_iterator_next(&it))
		pos = text_later(vis_text(vis));
	if (pos != EPOS) {
		view_cursors_to(vis_view(vis)->selection, pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static KEY_ACTION_FN(ka_delete)
{
	vis_operator(vis, VIS_OP_DELETE);
	vis_motion(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_insert_register)
{
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisRegister reg = vis_register_from(vis, keys[0]);
	if (reg != VIS_REG_INVALID) {
		vis_register(vis, reg);
		vis_operator(vis, VIS_OP_PUT_BEFORE_END);
	}
	return keys+1;
}

static KEY_ACTION_FN(ka_prompt_show)
{
	vis_prompt_show(vis, arg->s);
	return keys;
}

static KEY_ACTION_FN(ka_insert_verbatim)
{
	Rune rune = 0;
	char buf[4], type = keys[0];
	const char *data = NULL;
	int len = 0, count = 0, base = 0;
	switch (type) {
	case '\0':
		return NULL;
	case 'o':
	case 'O':
		count = 3;
		base = 8;
		break;
	case 'U':
		count = 4;
		/* fall through */
	case 'u':
		count += 4;
		base = 16;
		break;
	case 'x':
	case 'X':
		count = 2;
		base = 16;
		break;
	default:
		if ('0' <= type && type <= '9') {
			rune = type - '0';
			count = 2;
			base = 10;
		}
		break;
	}

	if (base) {
		for (keys++; keys[0] && count > 0; keys++, count--) {
			int v = 0;
			if (base == 8 && '0' <= keys[0] && keys[0] <= '7') {
				v = keys[0] - '0';
			} else if ((base == 10 || base == 16) && '0' <= keys[0] && keys[0] <= '9') {
				v = keys[0] - '0';
			} else if (base == 16 && 'a' <= keys[0] && keys[0] <= 'f') {
				v = 10 + keys[0] - 'a';
			} else if (base == 16 && 'A' <= keys[0] && keys[0] <= 'F') {
				v = 10 + keys[0] - 'A';
			} else {
				count = 0;
				break;
			}
			rune = rune * base + v;
		}

		if (count > 0)
			return NULL;
		if (type == 'u' || type == 'U') {
			len = runetochar(buf, &rune);
		} else {
			buf[0] = rune;
			len = 1;
		}

		data = buf;
	} else {
		const char *next = vis_keys_next(vis, keys);
		if (!next)
			return NULL;
		if ((rune = vis_keys_codepoint(vis, keys)) != (Rune)-1) {
			len = runetochar(buf, &rune);
			if (buf[0] == '\n')
				buf[0] = '\r';
			data = buf;
		} else {
			vis_info_show(vis, "Unknown key");
		}
		keys = next;
	}

	if (len > 0)
		vis_insert_key(vis, data, len);
	return keys;
}

static KEY_ACTION_FN(ka_call)
{
	arg->f(vis);
	return keys;
}

static KEY_ACTION_FN(ka_window)
{
	arg->w(vis_view(vis));
	return keys;
}

static KEY_ACTION_FN(ka_openline)
{
	vis_operator(vis, VIS_OP_MODESWITCH, VIS_MODE_INSERT);
	if (arg->i > 0) {
		vis_motion(vis, VIS_MOVE_LINE_END);
		vis_keys_feed(vis, "<Enter>");
	} else {
		if (vis->autoindent) {
			vis_motion(vis, VIS_MOVE_LINE_START);
			vis_keys_feed(vis, "<vis-motion-line-start>");
		} else {
			vis_motion(vis, VIS_MOVE_LINE_BEGIN);
			vis_keys_feed(vis, "<vis-motion-line-begin>");
		}
		vis_keys_feed(vis, "<Enter><vis-motion-line-up>");
	}
	return keys;
}

static KEY_ACTION_FN(ka_join)
{
	bool normal = (vis->mode->id == VIS_MODE_NORMAL);
	vis_operator(vis, VIS_OP_JOIN, arg->s);
	if (normal) {
		vis->action.count = VIS_COUNT_DEFAULT(vis->action.count, 0);
		if (vis->action.count > 0)
			vis->action.count -= 1;
		vis_motion(vis, VIS_MOVE_LINE_NEXT);
	}
	return keys;
}

static KEY_ACTION_FN(ka_normalmode_escape)
{
	if (vis->action.count == VIS_COUNT_UNKNOWN)
		ka_selections_clear(vis, keys, arg);
	else
		vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_visualmode_escape)
{
	if (vis->action.count == VIS_COUNT_UNKNOWN)
		vis_mode_switch(vis, VIS_MODE_NORMAL);
	else
		vis->action.count = VIS_COUNT_UNKNOWN;
	return keys;
}

static KEY_ACTION_FN(ka_switchmode)
{
	vis_mode_switch(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_insertmode)
{
	vis_operator(vis, VIS_OP_MODESWITCH, VIS_MODE_INSERT);
	vis_motion(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_replacemode)
{
	vis_operator(vis, VIS_OP_MODESWITCH, VIS_MODE_REPLACE);
	vis_motion(vis, arg->i);
	return keys;
}

static KEY_ACTION_FN(ka_unicode_info)
{
	View *view = vis_view(vis);
	Text *txt = vis_text(vis);
	size_t start = view_cursor_get(view);
	size_t end = text_char_next(txt, start);
	char *grapheme = text_bytes_alloc0(txt, start, end-start), *codepoint = grapheme;
	if (!grapheme)
		return keys;
	Buffer info = {0};
	mbstate_t ps = {0};
	Iterator it = text_iterator_get(txt, start);
	for (size_t pos = start; it.pos < end; pos = it.pos) {
		if (!text_iterator_codepoint_next(&it, NULL)) {
			vis_info_show(vis, "Failed to parse code point");
			goto err;
		}
		size_t len = it.pos - pos;
		wchar_t wc = 0xFFFD;
		size_t res = mbrtowc(&wc, codepoint, len, &ps);
		bool combining = false;
		if (res != (size_t)-1 && res != (size_t)-2)
			combining = (wc != L'\0' && wcwidth(wc) == 0);
		unsigned char ch = *codepoint;
		if (ch < 128 && !isprint(ch))
			buffer_appendf(&info, "<^%c> ", ch == 127 ? '?' : ch + 64);
		else
			buffer_appendf(&info, "<%s%.*s> ", combining ? " " : "", (int)len, codepoint);
		if (arg->i == VIS_ACTION_UNICODE_INFO) {
			buffer_appendf(&info, "U+%04"PRIX32" ", (uint32_t)wc);
		} else {
			for (size_t i = 0; i < len; i++)
				buffer_appendf(&info, "%02x ", (uint8_t)codepoint[i]);
		}
		codepoint += len;
	}
	vis_info_show(vis, "%s", buffer_content0(&info));
err:
	free(grapheme);
	buffer_release(&info);
	return keys;
}

static KEY_ACTION_FN(ka_percent)
{
	if (vis->action.count == VIS_COUNT_UNKNOWN)
		vis_motion(vis, VIS_MOVE_BRACKET_MATCH);
	else
		vis_motion(vis, VIS_MOVE_PERCENT);
	return keys;
}

static KEY_ACTION_FN(ka_jumplist)
{
	if (arg->i < 0)
		vis_jumplist_prev(vis);
	else if (arg->i > 0)
		vis_jumplist_next(vis);
	else
		vis_jumplist_save(vis);
	return keys;
}

static void signal_handler(int signum, siginfo_t *siginfo, void *context) {
	vis_signal_handler(vis, signum, siginfo, context);
}

#define KEY_ACTION_STRUCT(impl, _, arg, lua_name, help) {lua_name, VIS_HELP(help) impl, {arg}},
static const KeyAction vis_action[] = { KEY_ACTION_LIST(KEY_ACTION_STRUCT) };
#undef KEY_ACTION_STRUCT

#include "config.h"

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			continue;
		} else if (strcmp(argv[i], "-") == 0) {
			continue;
		} else if (strcmp(argv[i], "--") == 0) {
			break;
		} else if (strcmp(argv[i], "-v") == 0) {
			printf("vis %s%s%s%s%s%s%s\n", VERSION,
			       CONFIG_CURSES  ? " +curses"  : "",
			       CONFIG_LUA     ? " +lua"     : "",
			       CONFIG_LPEG    ? " +lpeg"    : "",
			       CONFIG_TRE     ? " +tre"     : "",
			       CONFIG_ACL     ? " +acl"     : "",
			       CONFIG_SELINUX ? " +selinux" : "");
			return 0;
		} else {
			fprintf(stderr, "Unknown command option: %s\n", argv[i]);
			return 1;
		}
	}

	if (!vis_init(vis))
		return EXIT_FAILURE;

	vis_event_emit(vis, VIS_EVENT_INIT);

	for (int i = 0; i < LENGTH(vis_action); i++) {
		if (!vis_action_register(vis, vis_action + i))
			vis_die(vis, "Could not register action: %s\n", vis_action[i].name);
	}

	for (int i = 0; i < LENGTH(default_bindings); i++) {
		for (const KeyBinding **binding = default_bindings[i]; binding && *binding; binding++) {
			for (const KeyBinding *kb = *binding; kb->key; kb++) {
				vis_mode_map(vis, i, false, kb->key, kb);
			}
		}
	}

	for (const char **k = keymaps; k[0]; k += 2)
		vis_keymap_add(vis, k[0], k[1]);

	/* install signal handlers etc. */
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = signal_handler;
	if (sigaction(SIGBUS, &sa, NULL) == -1 ||
	    sigaction(SIGINT, &sa, NULL) == -1 ||
	    sigaction(SIGCONT, &sa, NULL) == -1 ||
	    sigaction(SIGWINCH, &sa, NULL) == -1 ||
	    sigaction(SIGTERM, &sa, NULL) == -1 ||
	    sigaction(SIGHUP, &sa, NULL) == -1) {
		vis_die(vis, "Failed to set signal handler: %s\n", strerror(errno));
	}

	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1)
		vis_die(vis, "Failed to ignore signals\n");

	sigset_t blockset;
	sigemptyset(&blockset);
	sigaddset(&blockset, SIGBUS);
	sigaddset(&blockset, SIGCONT);
	sigaddset(&blockset, SIGWINCH);
	sigaddset(&blockset, SIGTERM);
	sigaddset(&blockset, SIGHUP);
	if (sigprocmask(SIG_BLOCK, &blockset, NULL) == -1)
		vis_die(vis, "Failed to block signals\n");

	char *cmd = NULL;
	bool end_of_options = false, win_created = false;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && !end_of_options) {
			if (strcmp(argv[i], "-") == 0) {
				if (!vis_window_new_fd(vis, STDOUT_FILENO))
					vis_die(vis, "Can not create empty buffer\n");
				ssize_t len = 0;
				char buf[PIPE_BUF];
				Text *txt = vis_text(vis);
				while ((len = read(STDIN_FILENO, buf, sizeof buf)) > 0)
					text_insert(txt, text_size(txt), buf, len);
				if (len == -1)
					vis_die(vis, "Can not read from stdin\n");
				text_snapshot(txt);
				int fd = open("/dev/tty", O_RDWR);
				if (fd == -1)
					vis_die(vis, "Can not reopen stdin\n");
				dup2(fd, STDIN_FILENO);
				close(fd);
			} else if (strcmp(argv[i], "--") == 0) {
				end_of_options = true;
				continue;
			}
		} else if (argv[i][0] == '+' && !end_of_options) {
			cmd = argv[i] + (argv[i][1] == '/' || argv[i][1] == '?');
			continue;
		} else if (!vis_window_new(vis, argv[i])) {
			vis_die(vis, "Can not load '%s': %s\n", argv[i], strerror(errno));
		}
		win_created = true;
		if (cmd) {
			vis_prompt_cmd(vis, cmd);
			cmd = NULL;
		}
	}

	if (!vis->win && !win_created) {
		if (!vis_window_new(vis, NULL))
			vis_die(vis, "Can not create empty buffer\n");
		if (cmd)
			vis_prompt_cmd(vis, cmd);
	}

	int status = vis_run(vis);
	vis_cleanup(vis);
	return status;
}
