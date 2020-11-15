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

#include "ui-terminal.h"
#include "vis.h"
#include "vis-lua.h"
#include "text-util.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"
#include "libutf.h"
#include "array.h"
#include "buffer.h"

#define PAGE      INT_MAX
#define PAGE_HALF (INT_MAX-1)

/** functions to be called from keybindings */
/* ignore key, do nothing */
static const char *nop(Vis*, const char *keys, const Arg *arg);
/* record/replay macro indicated by keys */
static const char *macro_record(Vis*, const char *keys, const Arg *arg);
static const char *macro_replay(Vis*, const char *keys, const Arg *arg);
/* temporarily suspend the editor and return to the shell, type 'fg' to get back */
static const char *suspend(Vis*, const char *keys, const Arg *arg);
/* reset count if set, otherwise remove all but the primary selection */
static const char *normalmode_escape(Vis*, const char *keys, const Arg *arg);
/* reset count if set, otherwise switch to normal mode */
static const char *visualmode_escape(Vis*, const char *keys, const Arg *arg);
/* switch to mode indicated by arg->i */
static const char *switchmode(Vis*, const char *keys, const Arg *arg);
/* switch to insert mode after performing movement indicated by arg->i */
static const char *insertmode(Vis*, const char *keys, const Arg *arg);
/* switch to replace mode after performing movement indicated by arg->i */
static const char *replacemode(Vis*, const char *keys, const Arg *arg);
/* add a new line either before or after the one where the cursor currently is */
static const char *openline(Vis*, const char *keys, const Arg *arg);
/* join lines from current cursor position to movement indicated by arg */
static const char *join(Vis*, const char *keys, const Arg *arg);
/* perform last action i.e. action_prev again */
static const char *repeat(Vis*, const char *keys, const Arg *arg);
/* replace character at cursor with one from keys */
static const char *replace(Vis*, const char *keys, const Arg *arg);
/* create a new cursor on the previous (arg->i < 0) or next (arg->i > 0) line */
static const char *selections_new(Vis*, const char *keys, const Arg *arg);
/* try to align all selections on the same column */
static const char *selections_align(Vis*, const char *keys, const Arg *arg);
/* try to align all selections by inserting the correct amount of white spaces */
static const char *selections_align_indent(Vis*, const char *keys, const Arg *arg);
/* remove all but the primary cursor and their selections */
static const char *selections_clear(Vis*, const char *keys, const Arg *arg);
/* remove the least recently added selection */
static const char *selections_remove(Vis*, const char *keys, const Arg *arg);
/* remove count (or arg->i)-th selection column */
static const char *selections_remove_column(Vis*, const char *keys, const Arg *arg);
/* remove all but the count (or arg->i)-th selection column */
static const char *selections_remove_column_except(Vis*, const char *keys, const Arg *arg);
/* move to the previous (arg->i < 0) or next (arg->i > 0) selection */
static const char *selections_navigate(Vis*, const char *keys, const Arg *arg);
/* select the next region matching the current selection */
static const char *selections_match_next(Vis*, const char *keys, const Arg *arg);
/* clear current selection but select next match */
static const char *selections_match_skip(Vis*, const char *keys, const Arg *arg);
/* rotate selection content count times left (arg->i < 0) or right (arg->i > 0) */
static const char *selections_rotate(Vis*, const char *keys, const Arg *arg);
/* remove leading and trailing white spaces from selections */
static const char *selections_trim(Vis*, const char *keys, const Arg *arg);
/* save active selections to mark */
static const char *selections_save(Vis*, const char *keys, const Arg *arg);
/* restore selections from mark */
static const char *selections_restore(Vis*, const char *keys, const Arg *arg);
/* union selections from mark */
static const char *selections_union(Vis*, const char *keys, const Arg *arg);
/* intersect selections from mark */
static const char *selections_intersect(Vis*, const char *keys, const Arg *arg);
/* perform complement of current active selections */
static const char *selections_complement(Vis*, const char *keys, const Arg *arg);
/* subtract selections from mark */
static const char *selections_minus(Vis*, const char *keys, const Arg *arg);
/* adjust current used count according to keys */
static const char *count(Vis*, const char *keys, const Arg *arg);
/* move to the count-th line or if not given either to the first (arg->i < 0)
 *  or last (arg->i > 0) line of file */
static const char *gotoline(Vis*, const char *keys, const Arg *arg);
/* make the current action use the operator indicated by arg->i */
static const char *operator(Vis*, const char *keys, const Arg *arg);
/* blocks to read a key and performs movement indicated by arg->i which
 * should be one of VIS_MOVE_{RIGHT,LEFT}_{TO,TILL} */
static const char *movement_key(Vis*, const char *keys, const Arg *arg);
/* perform the movement as indicated by arg->i */
static const char *movement(Vis*, const char *keys, const Arg *arg);
/* let the current operator affect the range indicated by the text object arg->i */
static const char *textobj(Vis*, const char *keys, const Arg *arg);
/* move to the other end of selected text */
static const char *selection_end(Vis*, const char *keys, const Arg *arg);
/* use register indicated by keys for the current operator */
static const char *reg(Vis*, const char *keys, const Arg *arg);
/* use mark indicated by keys for the current action */
static const char *mark(Vis*, const char *keys, const Arg *arg);
/* {un,re}do last action, redraw window */
static const char *undo(Vis*, const char *keys, const Arg *arg);
static const char *redo(Vis*, const char *keys, const Arg *arg);
/* earlier, later action chronologically, redraw window */
static const char *earlier(Vis*, const char *keys, const Arg *arg);
static const char *later(Vis*, const char *keys, const Arg *arg);
/* delete from the current cursor position to the end of
 * movement as indicated by arg->i */
static const char *delete(Vis*, const char *keys, const Arg *arg);
/* insert register content indicated by keys at current cursor position */
static const char *insert_register(Vis*, const char *keys, const Arg *arg);
/* show a user prompt to get input with title arg->s */
static const char *prompt_show(Vis*, const char *keys, const Arg *arg);
/* blocks to read 3 consecutive digits and inserts the corresponding byte value */
static const char *insert_verbatim(Vis*, const char *keys, const Arg *arg);
/* scroll window content according to arg->i which can be either PAGE, PAGE_HALF,
 * or an arbitrary number of lines. a multiplier overrides what is given in arg->i.
 * negative values scroll back, positive forward. */
static const char *wscroll(Vis*, const char *keys, const Arg *arg);
/* similar to scroll, but do only move window content not cursor position */
static const char *wslide(Vis*, const char *keys, const Arg *arg);
/* call editor function as indicated by arg->f */
static const char *call(Vis*, const char *keys, const Arg *arg);
/* call window function as indicated by arg->w */
static const char *window(Vis*, const char *keys, const Arg *arg);
/* show info about Unicode character at cursor position */
static const char *unicode_info(Vis*, const char *keys, const Arg *arg);
/* either go to count % of file or to matching item */
static const char *percent(Vis*, const char *keys, const Arg *arg);
/* navigate jumplist next (arg->i > 0), prev (arg->i < 0), save (arg->i = 0) */
static const char *jumplist(Vis*, const char *keys, const Arg *arg);

enum {
	VIS_ACTION_EDITOR_SUSPEND,
	VIS_ACTION_CURSOR_CHAR_PREV,
	VIS_ACTION_CURSOR_CHAR_NEXT,
	VIS_ACTION_CURSOR_LINE_CHAR_PREV,
	VIS_ACTION_CURSOR_LINE_CHAR_NEXT,
	VIS_ACTION_CURSOR_CODEPOINT_PREV,
	VIS_ACTION_CURSOR_CODEPOINT_NEXT,
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
	VIS_ACTION_CURSOR_PERCENT,
	VIS_ACTION_CURSOR_BYTE,
	VIS_ACTION_CURSOR_BYTE_LEFT,
	VIS_ACTION_CURSOR_BYTE_RIGHT,
	VIS_ACTION_CURSOR_PARAGRAPH_PREV,
	VIS_ACTION_CURSOR_PARAGRAPH_NEXT,
	VIS_ACTION_CURSOR_SENTENCE_PREV,
	VIS_ACTION_CURSOR_SENTENCE_NEXT,
	VIS_ACTION_CURSOR_BLOCK_START,
	VIS_ACTION_CURSOR_BLOCK_END,
	VIS_ACTION_CURSOR_PARENTHESIS_START,
	VIS_ACTION_CURSOR_PARENTHESIS_END,
	VIS_ACTION_CURSOR_COLUMN,
	VIS_ACTION_CURSOR_LINE_FIRST,
	VIS_ACTION_CURSOR_LINE_LAST,
	VIS_ACTION_CURSOR_WINDOW_LINE_TOP,
	VIS_ACTION_CURSOR_WINDOW_LINE_MIDDLE,
	VIS_ACTION_CURSOR_WINDOW_LINE_BOTTOM,
	VIS_ACTION_CURSOR_SEARCH_REPEAT_FORWARD,
	VIS_ACTION_CURSOR_SEARCH_REPEAT_BACKWARD,
	VIS_ACTION_CURSOR_SEARCH_REPEAT,
	VIS_ACTION_CURSOR_SEARCH_REPEAT_REVERSE,
	VIS_ACTION_CURSOR_SEARCH_WORD_FORWARD,
	VIS_ACTION_CURSOR_SEARCH_WORD_BACKWARD,
	VIS_ACTION_WINDOW_PAGE_UP,
	VIS_ACTION_WINDOW_PAGE_DOWN,
	VIS_ACTION_WINDOW_HALFPAGE_UP,
	VIS_ACTION_WINDOW_HALFPAGE_DOWN,
	VIS_ACTION_MODE_NORMAL,
	VIS_ACTION_MODE_NORMAL_ESCAPE,
	VIS_ACTION_MODE_VISUAL,
	VIS_ACTION_MODE_VISUAL_ESCAPE,
	VIS_ACTION_MODE_VISUAL_LINE,
	VIS_ACTION_MODE_INSERT,
	VIS_ACTION_MODE_REPLACE,
	VIS_ACTION_DELETE_CHAR_PREV,
	VIS_ACTION_DELETE_CHAR_NEXT,
	VIS_ACTION_DELETE_LINE_BEGIN,
	VIS_ACTION_DELETE_WORD_PREV,
	VIS_ACTION_JUMPLIST_PREV,
	VIS_ACTION_JUMPLIST_NEXT,
	VIS_ACTION_JUMPLIST_SAVE,
	VIS_ACTION_UNDO,
	VIS_ACTION_REDO,
	VIS_ACTION_EARLIER,
	VIS_ACTION_LATER,
	VIS_ACTION_MACRO_RECORD,
	VIS_ACTION_MACRO_REPLAY,
	VIS_ACTION_MARK,
	VIS_ACTION_REDRAW,
	VIS_ACTION_REPLACE_CHAR,
	VIS_ACTION_TOTILL_REPEAT,
	VIS_ACTION_TOTILL_REVERSE,
	VIS_ACTION_PROMPT_SEARCH_FORWARD,
	VIS_ACTION_PROMPT_SEARCH_BACKWARD,
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
	VIS_ACTION_COUNT,
	VIS_ACTION_INSERT_NEWLINE,
	VIS_ACTION_INSERT_TAB,
	VIS_ACTION_INSERT_VERBATIM,
	VIS_ACTION_INSERT_REGISTER,
	VIS_ACTION_WINDOW_NEXT,
	VIS_ACTION_WINDOW_PREV,
	VIS_ACTION_APPEND_CHAR_NEXT,
	VIS_ACTION_APPEND_LINE_END,
	VIS_ACTION_INSERT_LINE_START,
	VIS_ACTION_OPEN_LINE_ABOVE,
	VIS_ACTION_OPEN_LINE_BELOW,
	VIS_ACTION_JOIN_LINES,
	VIS_ACTION_JOIN_LINES_TRIM,
	VIS_ACTION_PROMPT_SHOW,
	VIS_ACTION_REPEAT,
	VIS_ACTION_SELECTION_FLIP,
	VIS_ACTION_WINDOW_REDRAW_TOP,
	VIS_ACTION_WINDOW_REDRAW_CENTER,
	VIS_ACTION_WINDOW_REDRAW_BOTTOM,
	VIS_ACTION_WINDOW_SLIDE_UP,
	VIS_ACTION_WINDOW_SLIDE_DOWN,
	VIS_ACTION_PUT_AFTER,
	VIS_ACTION_PUT_BEFORE,
	VIS_ACTION_SELECTIONS_NEW_LINE_ABOVE,
	VIS_ACTION_SELECTIONS_NEW_LINE_ABOVE_FIRST,
	VIS_ACTION_SELECTIONS_NEW_LINE_BELOW,
	VIS_ACTION_SELECTIONS_NEW_LINE_BELOW_LAST,
	VIS_ACTION_SELECTIONS_NEW_LINES_BEGIN,
	VIS_ACTION_SELECTIONS_NEW_LINES_END,
	VIS_ACTION_SELECTIONS_NEW_MATCH_ALL,
	VIS_ACTION_SELECTIONS_NEW_MATCH_NEXT,
	VIS_ACTION_SELECTIONS_NEW_MATCH_SKIP,
	VIS_ACTION_SELECTIONS_ALIGN,
	VIS_ACTION_SELECTIONS_ALIGN_INDENT_LEFT,
	VIS_ACTION_SELECTIONS_ALIGN_INDENT_RIGHT,
	VIS_ACTION_SELECTIONS_REMOVE_ALL,
	VIS_ACTION_SELECTIONS_REMOVE_LAST,
	VIS_ACTION_SELECTIONS_REMOVE_COLUMN,
	VIS_ACTION_SELECTIONS_REMOVE_COLUMN_EXCEPT,
	VIS_ACTION_SELECTIONS_PREV,
	VIS_ACTION_SELECTIONS_NEXT,
	VIS_ACTION_SELECTIONS_ROTATE_LEFT,
	VIS_ACTION_SELECTIONS_ROTATE_RIGHT,
	VIS_ACTION_SELECTIONS_TRIM,
	VIS_ACTION_SELECTIONS_SAVE,
	VIS_ACTION_SELECTIONS_RESTORE,
	VIS_ACTION_SELECTIONS_UNION,
	VIS_ACTION_SELECTIONS_INTERSECT,
	VIS_ACTION_SELECTIONS_COMPLEMENT,
	VIS_ACTION_SELECTIONS_MINUS,
	VIS_ACTION_TEXT_OBJECT_WORD_OUTER,
	VIS_ACTION_TEXT_OBJECT_WORD_INNER,
	VIS_ACTION_TEXT_OBJECT_LONGWORD_OUTER,
	VIS_ACTION_TEXT_OBJECT_LONGWORD_INNER,
	VIS_ACTION_TEXT_OBJECT_SENTENCE,
	VIS_ACTION_TEXT_OBJECT_PARAGRAPH,
	VIS_ACTION_TEXT_OBJECT_PARAGRAPH_OUTER,
	VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_OUTER,
	VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_INNER,
	VIS_ACTION_TEXT_OBJECT_PARENTHESIS_OUTER,
	VIS_ACTION_TEXT_OBJECT_PARENTHESIS_INNER,
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
	VIS_ACTION_TEXT_OBJECT_LINE_OUTER,
	VIS_ACTION_TEXT_OBJECT_LINE_INNER,
	VIS_ACTION_TEXT_OBJECT_INDENTATION,
	VIS_ACTION_TEXT_OBJECT_SEARCH_FORWARD,
	VIS_ACTION_TEXT_OBJECT_SEARCH_BACKWARD,
	VIS_ACTION_UNICODE_INFO,
	VIS_ACTION_UTF8_INFO,
	VIS_ACTION_NOP,
};

static const KeyAction vis_action[] = {
	[VIS_ACTION_EDITOR_SUSPEND] = {
		"vis-suspend",
		VIS_HELP("Suspend the editor")
		suspend,
	},
	[VIS_ACTION_CURSOR_CHAR_PREV] = {
		"vis-motion-char-prev",
		VIS_HELP("Move cursor left, to the previous character")
		movement, { .i = VIS_MOVE_CHAR_PREV }
	},
	[VIS_ACTION_CURSOR_CHAR_NEXT] = {
		"vis-motion-char-next",
		VIS_HELP("Move cursor right, to the next character")
		movement, { .i = VIS_MOVE_CHAR_NEXT }
	},
	[VIS_ACTION_CURSOR_LINE_CHAR_PREV] = {
		"vis-motion-line-char-prev",
		VIS_HELP("Move cursor left, to the previous character on the same line")
		movement, { .i = VIS_MOVE_LINE_CHAR_PREV }
	},
	[VIS_ACTION_CURSOR_LINE_CHAR_NEXT] = {
		"vis-motion-line-char-next",
		VIS_HELP("Move cursor right, to the next character on the same line")
		movement, { .i = VIS_MOVE_LINE_CHAR_NEXT }
	},
	[VIS_ACTION_CURSOR_CODEPOINT_PREV] = {
		"vis-motion-codepoint-prev",
		VIS_HELP("Move to the previous Unicode codepoint")
		movement, { .i = VIS_MOVE_CODEPOINT_PREV }
	},
	[VIS_ACTION_CURSOR_CODEPOINT_NEXT] = {
		"vis-motion-codepoint-next",
		VIS_HELP("Move to the next Unicode codepoint")
		movement, { .i = VIS_MOVE_CODEPOINT_NEXT }
	},
	[VIS_ACTION_CURSOR_WORD_START_PREV] = {
		"vis-motion-word-start-prev",
		VIS_HELP("Move cursor words backwards")
		movement, { .i = VIS_MOVE_WORD_START_PREV }
	},
	[VIS_ACTION_CURSOR_WORD_START_NEXT] = {
		"vis-motion-word-start-next",
		VIS_HELP("Move cursor words forwards")
		movement, { .i = VIS_MOVE_WORD_START_NEXT }
	},
	[VIS_ACTION_CURSOR_WORD_END_PREV] = {
		"vis-motion-word-end-prev",
		VIS_HELP("Move cursor backwards to the end of word")
		movement, { .i = VIS_MOVE_WORD_END_PREV }
	},
	[VIS_ACTION_CURSOR_WORD_END_NEXT] = {
		"vis-motion-word-end-next",
		VIS_HELP("Move cursor forward to the end of word")
		movement, { .i = VIS_MOVE_WORD_END_NEXT }
	},
	[VIS_ACTION_CURSOR_LONGWORD_START_PREV] = {
		"vis-motion-bigword-start-prev",
		VIS_HELP("Move cursor WORDS backwards")
		movement, { .i = VIS_MOVE_LONGWORD_START_PREV }
	},
	[VIS_ACTION_CURSOR_LONGWORD_START_NEXT] = {
		"vis-motion-bigword-start-next",
		VIS_HELP("Move cursor WORDS forwards")
		movement, { .i = VIS_MOVE_LONGWORD_START_NEXT }
	},
	[VIS_ACTION_CURSOR_LONGWORD_END_PREV] = {
		"vis-motion-bigword-end-prev",
		VIS_HELP("Move cursor backwards to the end of WORD")
		movement, { .i = VIS_MOVE_LONGWORD_END_PREV }
	},
	[VIS_ACTION_CURSOR_LONGWORD_END_NEXT] = {
		"vis-motion-bigword-end-next",
		VIS_HELP("Move cursor forward to the end of WORD")
		movement, { .i = VIS_MOVE_LONGWORD_END_NEXT }
	},
	[VIS_ACTION_CURSOR_LINE_UP] = {
		"vis-motion-line-up",
		VIS_HELP("Move cursor line upwards")
		movement, { .i = VIS_MOVE_LINE_UP }
	},
	[VIS_ACTION_CURSOR_LINE_DOWN] = {
		"vis-motion-line-down",
		VIS_HELP("Move cursor line downwards")
		movement, { .i = VIS_MOVE_LINE_DOWN }
	},
	[VIS_ACTION_CURSOR_LINE_START] = {
		"vis-motion-line-start",
		VIS_HELP("Move cursor to first non-blank character of the line")
		movement, { .i = VIS_MOVE_LINE_START }
	},
	[VIS_ACTION_CURSOR_LINE_FINISH] = {
		"vis-motion-line-finish",
		VIS_HELP("Move cursor to last non-blank character of the line")
		movement, { .i = VIS_MOVE_LINE_FINISH }
	},
	[VIS_ACTION_CURSOR_LINE_BEGIN] = {
		"vis-motion-line-begin",
		VIS_HELP("Move cursor to first character of the line")
		movement, { .i = VIS_MOVE_LINE_BEGIN }
	},
	[VIS_ACTION_CURSOR_LINE_END] = {
		"vis-motion-line-end",
		VIS_HELP("Move cursor to end of the line")
		movement, { .i = VIS_MOVE_LINE_END }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_UP] = {
		"vis-motion-screenline-up",
		VIS_HELP("Move cursor screen/display line upwards")
		movement, { .i = VIS_MOVE_SCREEN_LINE_UP }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_DOWN] = {
		"vis-motion-screenline-down",
		VIS_HELP("Move cursor screen/display line downwards")
		movement, { .i = VIS_MOVE_SCREEN_LINE_DOWN }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_BEGIN] = {
		"vis-motion-screenline-begin",
		VIS_HELP("Move cursor to beginning of screen/display line")
		movement, { .i = VIS_MOVE_SCREEN_LINE_BEGIN }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_MIDDLE] = {
		"vis-motion-screenline-middle",
		VIS_HELP("Move cursor to middle of screen/display line")
		movement, { .i = VIS_MOVE_SCREEN_LINE_MIDDLE }
	},
	[VIS_ACTION_CURSOR_SCREEN_LINE_END] = {
		"vis-motion-screenline-end",
		VIS_HELP("Move cursor to end of screen/display line")
		movement, { .i = VIS_MOVE_SCREEN_LINE_END }
	},
	[VIS_ACTION_CURSOR_PERCENT] = {
		"vis-motion-percent",
		VIS_HELP("Move to count % of file or matching item")
		percent
	},
	[VIS_ACTION_CURSOR_BYTE] = {
		"vis-motion-byte",
		VIS_HELP("Move to absolute byte position")
		movement, { .i = VIS_MOVE_BYTE }
	},
	[VIS_ACTION_CURSOR_BYTE_LEFT] = {
		"vis-motion-byte-left",
		VIS_HELP("Move count bytes to the left")
		movement, { .i = VIS_MOVE_BYTE_LEFT }
	},
	[VIS_ACTION_CURSOR_BYTE_RIGHT] = {
		"vis-motion-byte-right",
		VIS_HELP("Move count bytes to the right")
		movement, { .i = VIS_MOVE_BYTE_RIGHT }
	},
	[VIS_ACTION_CURSOR_PARAGRAPH_PREV] = {
		"vis-motion-paragraph-prev",
		VIS_HELP("Move cursor paragraph backward")
		movement, { .i = VIS_MOVE_PARAGRAPH_PREV }
	},
	[VIS_ACTION_CURSOR_PARAGRAPH_NEXT] = {
		"vis-motion-paragraph-next",
		VIS_HELP("Move cursor paragraph forward")
		movement, { .i = VIS_MOVE_PARAGRAPH_NEXT }
	},
	[VIS_ACTION_CURSOR_SENTENCE_PREV] = {
		"vis-motion-sentence-prev",
		VIS_HELP("Move cursor sentence backward")
		movement, { .i = VIS_MOVE_SENTENCE_PREV }
	},
	[VIS_ACTION_CURSOR_SENTENCE_NEXT] = {
		"vis-motion-sentence-next",
		VIS_HELP("Move cursor sentence forward")
		movement, { .i = VIS_MOVE_SENTENCE_NEXT }
	},
	[VIS_ACTION_CURSOR_BLOCK_START] = {
		"vis-motion-block-start",
		VIS_HELP("Move cursor to the opening curly brace in a block")
		movement, { .i = VIS_MOVE_BLOCK_START }
	},
	[VIS_ACTION_CURSOR_BLOCK_END] = {
		"vis-motion-block-end",
		VIS_HELP("Move cursor to the closing curly brace in a block")
		movement, { .i = VIS_MOVE_BLOCK_END }
	},
	[VIS_ACTION_CURSOR_PARENTHESIS_START] = {
		"vis-motion-parenthesis-start",
		VIS_HELP("Move cursor to the opening parenthesis inside a pair of parentheses")
		movement, { .i = VIS_MOVE_PARENTHESIS_START }
	},
	[VIS_ACTION_CURSOR_PARENTHESIS_END] = {
		"vis-motion-parenthesis-end",
		VIS_HELP("Move cursor to the closing parenthesis inside a pair of parentheses")
		movement, { .i = VIS_MOVE_PARENTHESIS_END }
	},
	[VIS_ACTION_CURSOR_COLUMN] = {
		"vis-motion-column",
		VIS_HELP("Move cursor to given column of current line")
		movement, { .i = VIS_MOVE_COLUMN }
	},
	[VIS_ACTION_CURSOR_LINE_FIRST] = {
		"vis-motion-line-first",
		VIS_HELP("Move cursor to given line (defaults to first)")
		gotoline, { .i = -1 }
	},
	[VIS_ACTION_CURSOR_LINE_LAST] = {
		"vis-motion-line-last",
		VIS_HELP("Move cursor to given line (defaults to last)")
		gotoline, { .i = +1 }
	},
	[VIS_ACTION_CURSOR_WINDOW_LINE_TOP] = {
		"vis-motion-window-line-top",
		VIS_HELP("Move cursor to top line of the window")
		movement, { .i = VIS_MOVE_WINDOW_LINE_TOP }
	},
	[VIS_ACTION_CURSOR_WINDOW_LINE_MIDDLE] = {
		"vis-motion-window-line-middle",
		VIS_HELP("Move cursor to middle line of the window")
		movement, { .i = VIS_MOVE_WINDOW_LINE_MIDDLE }
	},
	[VIS_ACTION_CURSOR_WINDOW_LINE_BOTTOM] = {
		"vis-motion-window-line-bottom",
		VIS_HELP("Move cursor to bottom line of the window")
		movement, { .i = VIS_MOVE_WINDOW_LINE_BOTTOM }
	},
	[VIS_ACTION_CURSOR_SEARCH_REPEAT_FORWARD] = {
		"vis-motion-search-repeat-forward",
		VIS_HELP("Move cursor to next match in forward direction")
		movement, { .i = VIS_MOVE_SEARCH_REPEAT_FORWARD }
	},
	[VIS_ACTION_CURSOR_SEARCH_REPEAT_BACKWARD] = {
		"vis-motion-search-repeat-backward",
		VIS_HELP("Move cursor to previous match in backward direction")
		movement, { .i = VIS_MOVE_SEARCH_REPEAT_BACKWARD }
	},
	[VIS_ACTION_CURSOR_SEARCH_REPEAT] = {
		"vis-motion-search-repeat",
		VIS_HELP("Move cursor to next match")
		movement, { .i = VIS_MOVE_SEARCH_REPEAT }
	},
	[VIS_ACTION_CURSOR_SEARCH_REPEAT_REVERSE] = {
		"vis-motion-search-repeat-reverse",
		VIS_HELP("Move cursor to next match in opposite direction")
		movement, { .i = VIS_MOVE_SEARCH_REPEAT_REVERSE }
	},
	[VIS_ACTION_CURSOR_SEARCH_WORD_FORWARD] = {
		"vis-motion-search-word-forward",
		VIS_HELP("Move cursor to next occurrence of the word under cursor")
		movement, { .i = VIS_MOVE_SEARCH_WORD_FORWARD }
	},
	[VIS_ACTION_CURSOR_SEARCH_WORD_BACKWARD] = {
		"vis-motion-search-word-backward",
		VIS_HELP("Move cursor to previous occurrence of the word under cursor")
		movement, { .i = VIS_MOVE_SEARCH_WORD_BACKWARD }
	},
	[VIS_ACTION_WINDOW_PAGE_UP] = {
		"vis-window-page-up",
		VIS_HELP("Scroll window pages backwards (upwards)")
		wscroll, { .i = -PAGE }
	},
	[VIS_ACTION_WINDOW_HALFPAGE_UP] = {
		"vis-window-halfpage-up",
		VIS_HELP("Scroll window half pages backwards (upwards)")
		wscroll, { .i = -PAGE_HALF }
	},
	[VIS_ACTION_WINDOW_PAGE_DOWN] = {
		"vis-window-page-down",
		VIS_HELP("Scroll window pages forwards (downwards)")
		wscroll, { .i = +PAGE }
	},
	[VIS_ACTION_WINDOW_HALFPAGE_DOWN] = {
		"vis-window-halfpage-down",
		VIS_HELP("Scroll window half pages forwards (downwards)")
		wscroll, { .i = +PAGE_HALF }
	},
	[VIS_ACTION_MODE_NORMAL] = {
		"vis-mode-normal",
		VIS_HELP("Enter normal mode")
		switchmode, { .i = VIS_MODE_NORMAL }
	},
	[VIS_ACTION_MODE_NORMAL_ESCAPE] = {
		"vis-mode-normal-escape",
		VIS_HELP("Reset count or remove all non-primary selections")
		normalmode_escape,
	},
	[VIS_ACTION_MODE_VISUAL] = {
		"vis-mode-visual-charwise",
		VIS_HELP("Enter characterwise visual mode")
		switchmode, { .i = VIS_MODE_VISUAL }
	},
	[VIS_ACTION_MODE_VISUAL_ESCAPE] = {
		"vis-mode-visual-escape",
		VIS_HELP("Reset count or switch to normal mode")
		visualmode_escape,
	},
	[VIS_ACTION_MODE_VISUAL_LINE] = {
		"vis-mode-visual-linewise",
		VIS_HELP("Enter linewise visual mode")
		switchmode, { .i = VIS_MODE_VISUAL_LINE }
	},
	[VIS_ACTION_MODE_INSERT] = {
		"vis-mode-insert",
		VIS_HELP("Enter insert mode")
		insertmode, { .i = VIS_MOVE_NOP }
	},
	[VIS_ACTION_MODE_REPLACE] = {
		"vis-mode-replace",
		VIS_HELP("Enter replace mode")
		replacemode, { .i = VIS_MOVE_NOP }
	},
	[VIS_ACTION_DELETE_CHAR_PREV] = {
		"vis-delete-char-prev",
		VIS_HELP("Delete the previous character")
		delete, { .i = VIS_MOVE_CHAR_PREV }
	},
	[VIS_ACTION_DELETE_CHAR_NEXT] = {
		"vis-delete-char-next",
		VIS_HELP("Delete the next character")
		delete, { .i = VIS_MOVE_CHAR_NEXT }
	},
	[VIS_ACTION_DELETE_LINE_BEGIN] = {
		"vis-delete-line-begin",
		VIS_HELP("Delete until the start of the current line")
		delete, { .i = VIS_MOVE_LINE_BEGIN }
	},
	[VIS_ACTION_DELETE_WORD_PREV] = {
		"vis-delete-word-prev",
		VIS_HELP("Delete the previous WORD")
		delete, { .i = VIS_MOVE_WORD_START_PREV }
	},
	[VIS_ACTION_JUMPLIST_PREV] = {
		"vis-jumplist-prev",
		VIS_HELP("Go to older cursor position in jump list")
		jumplist, { .i = -1 }
	},
	[VIS_ACTION_JUMPLIST_NEXT] = {
		"vis-jumplist-next",
		VIS_HELP("Go to newer cursor position in jump list")
		jumplist, { .i = +1 }
	},
	[VIS_ACTION_JUMPLIST_SAVE] = {
		"vis-jumplist-save",
		VIS_HELP("Save current selections in jump list")
		jumplist, { .i = 0 }
	},
	[VIS_ACTION_UNDO] = {
		"vis-undo",
		VIS_HELP("Undo last change")
		undo,
	},
	[VIS_ACTION_REDO] = {
		"vis-redo",
		VIS_HELP("Redo last change")
		redo,
	},
	[VIS_ACTION_EARLIER] = {
		"vis-earlier",
		VIS_HELP("Goto older text state")
		earlier,
	},
	[VIS_ACTION_LATER] = {
		"vis-later",
		VIS_HELP("Goto newer text state")
		later,
	},
	[VIS_ACTION_MACRO_RECORD] = {
		"vis-macro-record",
		VIS_HELP("Record macro into given register")
		macro_record,
	},
	[VIS_ACTION_MACRO_REPLAY] = {
		"vis-macro-replay",
		VIS_HELP("Replay macro, execute the content of the given register")
		macro_replay,
	},
	[VIS_ACTION_MARK] = {
		"vis-mark",
		VIS_HELP("Use given mark for next action")
		mark,
	},
	[VIS_ACTION_REDRAW] = {
		"vis-redraw",
		VIS_HELP("Redraw current editor content")
		call, { .f = vis_redraw }
	},
	[VIS_ACTION_REPLACE_CHAR] = {
		"vis-replace-char",
		VIS_HELP("Replace the character under the cursor")
		replace,
	},
	[VIS_ACTION_TOTILL_REPEAT] = {
		"vis-motion-totill-repeat",
		VIS_HELP("Repeat latest to/till motion")
		movement, { .i = VIS_MOVE_TOTILL_REPEAT }
	},
	[VIS_ACTION_TOTILL_REVERSE] = {
		"vis-motion-totill-reverse",
		VIS_HELP("Repeat latest to/till motion but in opposite direction")
		movement, { .i = VIS_MOVE_TOTILL_REVERSE }
	},
	[VIS_ACTION_PROMPT_SEARCH_FORWARD] = {
		"vis-search-forward",
		VIS_HELP("Search forward")
		prompt_show, { .s = "/" }
	},
	[VIS_ACTION_PROMPT_SEARCH_BACKWARD] = {
		"vis-search-backward",
		VIS_HELP("Search backward")
		prompt_show, { .s = "?" }
	},
	[VIS_ACTION_TILL_LEFT] = {
		"vis-motion-till-left",
		VIS_HELP("Till after the occurrence of character to the left")
		movement_key, { .i = VIS_MOVE_LEFT_TILL }
	},
	[VIS_ACTION_TILL_RIGHT] = {
		"vis-motion-till-right",
		VIS_HELP("Till before the occurrence of character to the right")
		movement_key, { .i = VIS_MOVE_RIGHT_TILL }
	},
	[VIS_ACTION_TO_LEFT] = {
		"vis-motion-to-left",
		VIS_HELP("To the first occurrence of character to the left")
		movement_key, { .i = VIS_MOVE_LEFT_TO }
	},
	[VIS_ACTION_TO_RIGHT] = {
		"vis-motion-to-right",
		VIS_HELP("To the first occurrence of character to the right")
		movement_key, { .i = VIS_MOVE_RIGHT_TO }
	},
	[VIS_ACTION_REGISTER] = {
		"vis-register",
		VIS_HELP("Use given register for next operator")
		reg,
	},
	[VIS_ACTION_OPERATOR_CHANGE] = {
		"vis-operator-change",
		VIS_HELP("Change operator")
		operator, { .i = VIS_OP_CHANGE }
	},
	[VIS_ACTION_OPERATOR_DELETE] = {
		"vis-operator-delete",
		VIS_HELP("Delete operator")
		operator, { .i = VIS_OP_DELETE }
	},
	[VIS_ACTION_OPERATOR_YANK] = {
		"vis-operator-yank",
		VIS_HELP("Yank operator")
		operator, { .i = VIS_OP_YANK }
	},
	[VIS_ACTION_OPERATOR_SHIFT_LEFT] = {
		"vis-operator-shift-left",
		VIS_HELP("Shift left operator")
		operator, { .i = VIS_OP_SHIFT_LEFT }
	},
	[VIS_ACTION_OPERATOR_SHIFT_RIGHT] = {
		"vis-operator-shift-right",
		VIS_HELP("Shift right operator")
		operator, { .i = VIS_OP_SHIFT_RIGHT }
	},
	[VIS_ACTION_COUNT] = {
		"vis-count",
		VIS_HELP("Count specifier")
		count,
	},
	[VIS_ACTION_INSERT_NEWLINE] = {
		"vis-insert-newline",
		VIS_HELP("Insert a line break (depending on file type)")
		call, { .f = vis_insert_nl }
	},
	[VIS_ACTION_INSERT_TAB] = {
		"vis-insert-tab",
		VIS_HELP("Insert a tab (might be converted to spaces)")
		call, { .f = vis_insert_tab }
	},
	[VIS_ACTION_INSERT_VERBATIM] = {
		"vis-insert-verbatim",
		VIS_HELP("Insert Unicode character based on code point")
		insert_verbatim,
	},
	[VIS_ACTION_INSERT_REGISTER] = {
		"vis-insert-register",
		VIS_HELP("Insert specified register content")
		insert_register,
	},
	[VIS_ACTION_WINDOW_NEXT] = {
		"vis-window-next",
		VIS_HELP("Focus next window")
		call, { .f = vis_window_next }
	},
	[VIS_ACTION_WINDOW_PREV] = {
		"vis-window-prev",
		VIS_HELP("Focus previous window")
		call, { .f = vis_window_prev }
	},
	[VIS_ACTION_APPEND_CHAR_NEXT] = {
		"vis-append-char-next",
		VIS_HELP("Append text after the cursor")
		insertmode, { .i = VIS_MOVE_LINE_CHAR_NEXT }
	},
	[VIS_ACTION_APPEND_LINE_END] = {
		"vis-append-line-end",
		VIS_HELP("Append text after the end of the line")
		insertmode, { .i = VIS_MOVE_LINE_END },
	},
	[VIS_ACTION_INSERT_LINE_START] = {
		"vis-insert-line-start",
		VIS_HELP("Insert text before the first non-blank in the line")
		insertmode, { .i = VIS_MOVE_LINE_START },
	},
	[VIS_ACTION_OPEN_LINE_ABOVE] = {
		"vis-open-line-above",
		VIS_HELP("Begin a new line above the cursor")
		openline, { .i = -1 }
	},
	[VIS_ACTION_OPEN_LINE_BELOW] = {
		"vis-open-line-below",
		VIS_HELP("Begin a new line below the cursor")
		openline, { .i = +1 }
	},
	[VIS_ACTION_JOIN_LINES] = {
		"vis-join-lines",
		VIS_HELP("Join selected lines")
		join, { .s = " " }
	},
	[VIS_ACTION_JOIN_LINES_TRIM] = {
		"vis-join-lines-trim",
		VIS_HELP("Join selected lines, remove white space")
		join, { .s = "" }
	},
	[VIS_ACTION_PROMPT_SHOW] = {
		"vis-prompt-show",
		VIS_HELP("Show editor command line prompt")
		prompt_show, { .s = ":" }
	},
	[VIS_ACTION_REPEAT] = {
		"vis-repeat",
		VIS_HELP("Repeat latest editor command")
		repeat
	},
	[VIS_ACTION_SELECTION_FLIP] = {
		"vis-selection-flip",
		VIS_HELP("Flip selection, move cursor to other end")
		selection_end,
	},
	[VIS_ACTION_WINDOW_REDRAW_TOP] = {
		"vis-window-redraw-top",
		VIS_HELP("Redraw cursor line at the top of the window")
		window, { .w = view_redraw_top }
	},
	[VIS_ACTION_WINDOW_REDRAW_CENTER] = {
		"vis-window-redraw-center",
		VIS_HELP("Redraw cursor line at the center of the window")
		window, { .w = view_redraw_center }
	},
	[VIS_ACTION_WINDOW_REDRAW_BOTTOM] = {
		"vis-window-redraw-bottom",
		VIS_HELP("Redraw cursor line at the bottom of the window")
		window, { .w = view_redraw_bottom }
	},
	[VIS_ACTION_WINDOW_SLIDE_UP] = {
		"vis-window-slide-up",
		VIS_HELP("Slide window content upwards")
		wslide, { .i = -1 }
	},
	[VIS_ACTION_WINDOW_SLIDE_DOWN] = {
		"vis-window-slide-down",
		VIS_HELP("Slide window content downwards")
		wslide, { .i = +1 }
	},
	[VIS_ACTION_PUT_AFTER] = {
		"vis-put-after",
		VIS_HELP("Put text after the cursor")
		operator, { .i = VIS_OP_PUT_AFTER }
	},
	[VIS_ACTION_PUT_BEFORE] = {
		"vis-put-before",
		VIS_HELP("Put text before the cursor")
		operator, { .i = VIS_OP_PUT_BEFORE }
	},
	[VIS_ACTION_SELECTIONS_NEW_LINE_ABOVE] = {
		"vis-selection-new-lines-above",
		VIS_HELP("Create a new selection on the line above")
		selections_new, { .i = -1 }
	},
	[VIS_ACTION_SELECTIONS_NEW_LINE_ABOVE_FIRST] = {
		"vis-selection-new-lines-above-first",
		VIS_HELP("Create a new selection on the line above the first selection")
		selections_new, { .i = INT_MIN }
	},
	[VIS_ACTION_SELECTIONS_NEW_LINE_BELOW] = {
		"vis-selection-new-lines-below",
		VIS_HELP("Create a new selection on the line below")
		selections_new, { .i = +1 }
	},
	[VIS_ACTION_SELECTIONS_NEW_LINE_BELOW_LAST] = {
		"vis-selection-new-lines-below-last",
		VIS_HELP("Create a new selection on the line below the last selection")
		selections_new, { .i = INT_MAX }
	},
	[VIS_ACTION_SELECTIONS_NEW_LINES_BEGIN] = {
		"vis-selection-new-lines-begin",
		VIS_HELP("Create a new selection at the start of every line covered by selection")
		operator, { .i = VIS_OP_CURSOR_SOL }
	},
	[VIS_ACTION_SELECTIONS_NEW_LINES_END] = {
		"vis-selection-new-lines-end",
		VIS_HELP("Create a new selection at the end of every line covered by selection")
		operator, { .i = VIS_OP_CURSOR_EOL }
	},
	[VIS_ACTION_SELECTIONS_NEW_MATCH_ALL] = {
		"vis-selection-new-match-all",
		VIS_HELP("Select all regions matching the current selection")
		selections_match_next, { .b = true }
	},
	[VIS_ACTION_SELECTIONS_NEW_MATCH_NEXT] = {
		"vis-selection-new-match-next",
		VIS_HELP("Select the next region matching the current selection")
		selections_match_next,
	},
	[VIS_ACTION_SELECTIONS_NEW_MATCH_SKIP] = {
		"vis-selection-new-match-skip",
		VIS_HELP("Clear current selection, but select next match")
		selections_match_skip,
	},
	[VIS_ACTION_SELECTIONS_ALIGN] = {
		"vis-selections-align",
		VIS_HELP("Try to align all selections on the same column")
		selections_align,
	},
	[VIS_ACTION_SELECTIONS_ALIGN_INDENT_LEFT] = {
		"vis-selections-align-indent-left",
		VIS_HELP("Left-align all selections by inserting spaces")
		selections_align_indent, { .i = -1 }
	},
	[VIS_ACTION_SELECTIONS_ALIGN_INDENT_RIGHT] = {
		"vis-selections-align-indent-right",
		VIS_HELP("Right-align all selections by inserting spaces")
		selections_align_indent, { .i = +1 }
	},
	[VIS_ACTION_SELECTIONS_REMOVE_ALL] = {
		"vis-selections-remove-all",
		VIS_HELP("Remove all but the primary selection")
		selections_clear,
	},
	[VIS_ACTION_SELECTIONS_REMOVE_LAST] = {
		"vis-selections-remove-last",
		VIS_HELP("Remove primary selection")
		selections_remove,
	},
	[VIS_ACTION_SELECTIONS_REMOVE_COLUMN] = {
		"vis-selections-remove-column",
		VIS_HELP("Remove count selection column")
		selections_remove_column, { .i = 1 }
	},
	[VIS_ACTION_SELECTIONS_REMOVE_COLUMN_EXCEPT] = {
		"vis-selections-remove-column-except",
		VIS_HELP("Remove all but the count selection column")
		selections_remove_column_except, { .i = 1 }
	},
	[VIS_ACTION_SELECTIONS_PREV] = {
		"vis-selection-prev",
		VIS_HELP("Move to the previous selection")
		selections_navigate, { .i = -PAGE_HALF }
	},
	[VIS_ACTION_SELECTIONS_NEXT] = {
		"vis-selection-next",
		VIS_HELP("Move to the next selection")
		selections_navigate, { .i = +PAGE_HALF }
	},
	[VIS_ACTION_SELECTIONS_ROTATE_LEFT] = {
		"vis-selections-rotate-left",
		VIS_HELP("Rotate selections left")
		selections_rotate, { .i = -1 }
	},
	[VIS_ACTION_SELECTIONS_ROTATE_RIGHT] = {
		"vis-selections-rotate-right",
		VIS_HELP("Rotate selections right")
		selections_rotate, { .i = +1 }
	},
	[VIS_ACTION_SELECTIONS_TRIM] = {
		"vis-selections-trim",
		VIS_HELP("Remove leading and trailing white space from selections")
		selections_trim
	},
	[VIS_ACTION_SELECTIONS_SAVE] = {
		"vis-selections-save",
		VIS_HELP("Save currently active selections to mark")
		selections_save
	},
	[VIS_ACTION_SELECTIONS_RESTORE] = {
		"vis-selections-restore",
		VIS_HELP("Restore selections from mark")
		selections_restore
	},
	[VIS_ACTION_SELECTIONS_UNION] = {
		"vis-selections-union",
		VIS_HELP("Add selections from mark")
		selections_union
	},
	[VIS_ACTION_SELECTIONS_INTERSECT] = {
		"vis-selections-intersect",
		VIS_HELP("Intersect with selections from mark")
		selections_intersect
	},
	[VIS_ACTION_SELECTIONS_COMPLEMENT] = {
		"vis-selections-complement",
		VIS_HELP("Complement selections")
		selections_complement
	},
	[VIS_ACTION_SELECTIONS_MINUS] = {
		"vis-selections-minus",
		VIS_HELP("Subtract selections from mark")
		selections_minus
	},
	[VIS_ACTION_TEXT_OBJECT_WORD_OUTER] = {
		"vis-textobject-word-outer",
		VIS_HELP("A word leading and trailing whitespace included")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_WORD }
	},
	[VIS_ACTION_TEXT_OBJECT_WORD_INNER] = {
		"vis-textobject-word-inner",
		VIS_HELP("A word leading and trailing whitespace excluded")
		textobj, { .i = VIS_TEXTOBJECT_INNER_WORD }
	},
	[VIS_ACTION_TEXT_OBJECT_LONGWORD_OUTER] = {
		"vis-textobject-bigword-outer",
		VIS_HELP("A WORD leading and trailing whitespace included")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_LONGWORD }
	},
	[VIS_ACTION_TEXT_OBJECT_LONGWORD_INNER] = {
		"vis-textobject-bigword-inner",
		VIS_HELP("A WORD leading and trailing whitespace excluded")
		textobj, { .i = VIS_TEXTOBJECT_INNER_LONGWORD }
	},
	[VIS_ACTION_TEXT_OBJECT_SENTENCE] = {
		"vis-textobject-sentence",
		VIS_HELP("A sentence")
		textobj, { .i = VIS_TEXTOBJECT_SENTENCE }
	},
	[VIS_ACTION_TEXT_OBJECT_PARAGRAPH] = {
		"vis-textobject-paragraph",
		VIS_HELP("A paragraph")
		textobj, { .i = VIS_TEXTOBJECT_PARAGRAPH }
	},
	[VIS_ACTION_TEXT_OBJECT_PARAGRAPH_OUTER] = {
		"vis-textobject-paragraph-outer",
		VIS_HELP("A paragraph (outer variant)")
		textobj, { .i = VIS_TEXTOBJECT_PARAGRAPH_OUTER }
	},
	[VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_OUTER] = {
		"vis-textobject-square-bracket-outer",
		VIS_HELP("[] block (outer variant)")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_SQUARE_BRACKET_INNER] = {
		"vis-textobject-square-bracket-inner",
		VIS_HELP("[] block (inner variant)")
		textobj, { .i = VIS_TEXTOBJECT_INNER_SQUARE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_PARENTHESIS_OUTER] = {
		"vis-textobject-parenthesis-outer",
		VIS_HELP("() block (outer variant)")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_PARENTHESIS }
	},
	[VIS_ACTION_TEXT_OBJECT_PARENTHESIS_INNER] = {
		"vis-textobject-parenthesis-inner",
		VIS_HELP("() block (inner variant)")
		textobj, { .i = VIS_TEXTOBJECT_INNER_PARENTHESIS }
	},
	[VIS_ACTION_TEXT_OBJECT_ANGLE_BRACKET_OUTER] = {
		"vis-textobject-angle-bracket-outer",
		VIS_HELP("<> block (outer variant)")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_ANGLE_BRACKET_INNER] = {
		"vis-textobject-angle-bracket-inner",
		VIS_HELP("<> block (inner variant)")
		textobj, { .i = VIS_TEXTOBJECT_INNER_ANGLE_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_CURLY_BRACKET_OUTER] = {
		"vis-textobject-curly-bracket-outer",
		VIS_HELP("{} block (outer variant)")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_CURLY_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_CURLY_BRACKET_INNER] = {
		"vis-textobject-curly-bracket-inner",
		VIS_HELP("{} block (inner variant)")
		textobj, { .i = VIS_TEXTOBJECT_INNER_CURLY_BRACKET }
	},
	[VIS_ACTION_TEXT_OBJECT_QUOTE_OUTER] = {
		"vis-textobject-quote-outer",
		VIS_HELP("A quoted string, including the quotation marks")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_QUOTE_INNER] = {
		"vis-textobject-quote-inner",
		VIS_HELP("A quoted string, excluding the quotation marks")
		textobj, { .i = VIS_TEXTOBJECT_INNER_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_SINGLE_QUOTE_OUTER] = {
		"vis-textobject-single-quote-outer",
		VIS_HELP("A single quoted string, including the quotation marks")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_SINGLE_QUOTE_INNER] = {
		"vis-textobject-single-quote-inner",
		VIS_HELP("A single quoted string, excluding the quotation marks")
		textobj, { .i = VIS_TEXTOBJECT_INNER_SINGLE_QUOTE }
	},
	[VIS_ACTION_TEXT_OBJECT_BACKTICK_OUTER] = {
		"vis-textobject-backtick-outer",
		VIS_HELP("A backtick delimited string (outer variant)")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_BACKTICK }
	},
	[VIS_ACTION_TEXT_OBJECT_BACKTICK_INNER] = {
		"vis-textobject-backtick-inner",
		VIS_HELP("A backtick delimited string (inner variant)")
		textobj, { .i = VIS_TEXTOBJECT_INNER_BACKTICK }
	},
	[VIS_ACTION_TEXT_OBJECT_LINE_OUTER] = {
		"vis-textobject-line-outer",
		VIS_HELP("The whole line")
		textobj, { .i = VIS_TEXTOBJECT_OUTER_LINE }
	},
	[VIS_ACTION_TEXT_OBJECT_LINE_INNER] = {
		"vis-textobject-line-inner",
		VIS_HELP("The whole line, excluding leading and trailing whitespace")
		textobj, { .i = VIS_TEXTOBJECT_INNER_LINE }
	},
	[VIS_ACTION_TEXT_OBJECT_INDENTATION] = {
		"vis-textobject-indentation",
		VIS_HELP("All adjacent lines with the same indentation level as the current one")
		textobj, { .i = VIS_TEXTOBJECT_INDENTATION }
	},
	[VIS_ACTION_TEXT_OBJECT_SEARCH_FORWARD] = {
		"vis-textobject-search-forward",
		VIS_HELP("The next search match in forward direction")
		textobj, { .i = VIS_TEXTOBJECT_SEARCH_FORWARD }
	},
	[VIS_ACTION_TEXT_OBJECT_SEARCH_BACKWARD] = {
		"vis-textobject-search-backward",
		VIS_HELP("The next search match in backward direction")
		textobj, { .i = VIS_TEXTOBJECT_SEARCH_BACKWARD }
	},
	[VIS_ACTION_UNICODE_INFO] = {
		"vis-unicode-info",
		VIS_HELP("Show Unicode codepoint(s) of character under cursor")
		unicode_info, { .i = VIS_ACTION_UNICODE_INFO  }
	},
	[VIS_ACTION_UTF8_INFO] = {
		"vis-utf8-info",
		VIS_HELP("Show UTF-8 encoded codepoint(s) of character under cursor")
		unicode_info, { .i = VIS_ACTION_UTF8_INFO }
	},
	[VIS_ACTION_NOP] = {
		"vis-nop",
		VIS_HELP("Ignore key, do nothing")
		nop,
	},
};

#include "config.h"

/** key bindings functions */

static const char *nop(Vis *vis, const char *keys, const Arg *arg) {
	return keys;
}

static const char *macro_record(Vis *vis, const char *keys, const Arg *arg) {
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

static const char *macro_replay(Vis *vis, const char *keys, const Arg *arg) {
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisRegister reg = vis_register_from(vis, keys[0]);
	vis_macro_replay(vis, reg);
	return keys+1;
}

static const char *suspend(Vis *vis, const char *keys, const Arg *arg) {
	vis_suspend(vis);
	return keys;
}

static const char *repeat(Vis *vis, const char *keys, const Arg *arg) {
	vis_repeat(vis);
	return keys;
}

static const char *selections_new(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	bool anchored = view_selections_anchored(view_selections_primary_get(view));
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
			view_selections_anchor(sel_new, anchored);
		}
	}
	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *selections_align(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	Text *txt = vis_text(vis);
	int mincol = INT_MAX;
	for (Selection *s = view_selections(view); s; s = view_selections_next(s)) {
		int col = view_cursors_cell_get(s);
		if (col >= 0 && col < mincol)
			mincol = col;
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

static const char *selections_align_indent(Vis *vis, const char *keys, const Arg *arg) {
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

static const char *selections_clear(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	if (view_selections_count(view) > 1)
		view_selections_dispose_all(view);
	else
		view_selection_clear(view_selections_primary_get(view));
	return keys;
}

static Selection *selection_new(View *view, Filerange *r, bool isprimary) {
	Text *txt = view_text(view);
	size_t pos = text_char_prev(txt, r->end);
	Selection *s = view_selections_new(view, pos);
	if (!s)
		return NULL;
	view_selections_set(s, r);
	view_selections_anchor(s, true);
	if (isprimary)
		view_selections_primary_set(s);
	return s;
}

static const char *selections_match_next(Vis *vis, const char *keys, const Arg *arg) {
	Text *txt = vis_text(vis);
	View *view = vis_view(vis);
	Selection *s = view_selections_primary_get(view);
	Filerange sel = view_selections_get(s);
	if (!text_range_valid(&sel))
		return keys;

	static bool match_word;

	if (view_selections_count(view) == 1) {
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
		if (selection_new(view, &sel, !match_all) && !match_all)
			goto out;
	}

	sel = primary;

	for (;;) {
		sel = find_prev(txt, sel.start, buf);
		if (!text_range_valid(&sel))
			break;
		if (selection_new(view, &sel, !match_all) && !match_all)
			break;
	}

out:
	free(buf);
	return keys;
}

static const char *selections_match_skip(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	Selection *sel = view_selections_primary_get(view);
	keys = selections_match_next(vis, keys, arg);
	if (sel != view_selections_primary_get(view))
		view_selections_dispose(sel);
	return keys;
}

static const char *selections_remove(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	view_selections_dispose(view_selections_primary_get(view));
	view_cursor_to(view, view_cursor_get(view));
	return keys;
}

static const char *selections_remove_column(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	int max = view_selections_column_count(view);
	int column = vis_count_get_default(vis, arg->i) - 1;
	if (column >= max)
		column = max - 1;
	if (view_selections_count(view) == 1) {
		vis_keys_feed(vis, "<Escape>");
		return keys;
	}

	for (Selection *s = view_selections_column(view, column), *next; s; s = next) {
		next = view_selections_column_next(s, column);
		view_selections_dispose(s);
	}

	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *selections_remove_column_except(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	int max = view_selections_column_count(view);
	int column = vis_count_get_default(vis, arg->i) - 1;
	if (column >= max)
		column = max - 1;
	if (view_selections_count(view) == 1) {
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

	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *selections_navigate(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	if (view_selections_count(view) == 1)
		return wscroll(vis, keys, arg);
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
	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *selections_rotate(Vis *vis, const char *keys, const Arg *arg) {

	typedef struct {
		Selection *sel;
		char *data;
		size_t len;
	} Rotate;

	Array arr;
	Text *txt = vis_text(vis);
	View *view = vis_view(vis);
	int columns = view_selections_column_count(view);
	int selections = columns == 1 ? view_selections_count(view) : columns;
	int count = vis_count_get_default(vis, 1);
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
			size_t len = array_length(&arr);
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
	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *selections_trim(Vis *vis, const char *keys, const Arg *arg) {
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
	enum VisMode mode = vis_mode_get(vis);
	bool anchored = mode == VIS_MODE_VISUAL || mode == VIS_MODE_VISUAL_LINE;
	view_selections_set_all(view, sel, anchored);
	if (!anchored)
		view_selections_clear_all(view);
}

static const char *selections_save(Vis *vis, const char *keys, const Arg *arg) {
	Win *win = vis_window(vis);
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array sel = view_selections_get_all(view);
	vis_mark_set(win, mark, &sel);
	array_release(&sel);
	vis_cancel(vis);
	return keys;
}

static const char *selections_restore(Vis *vis, const char *keys, const Arg *arg) {
	Win *win = vis_window(vis);
	View *view = vis_view(vis);
	enum VisMark mark = vis_mark_used(vis);
	Array sel = vis_mark_get(win, mark);
	selections_set(vis, view, &sel);
	array_release(&sel);
	vis_cancel(vis);
	return keys;
}

static const char *selections_union(Vis *vis, const char *keys, const Arg *arg) {
	Win *win = vis_window(vis);
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

static const char *selections_intersect(Vis *vis, const char *keys, const Arg *arg) {
	Win *win = vis_window(vis);
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
	for (size_t i = 0, len = array_length(a); i < len; i++) {
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

static const char *selections_complement(Vis *vis, const char *keys, const Arg *arg) {
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

static const char *selections_minus(Vis *vis, const char *keys, const Arg *arg) {
	Text *txt = vis_text(vis);
	Win *win = vis_window(vis);
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

static const char *replace(Vis *vis, const char *keys, const Arg *arg) {
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
	if (vis_mode_get(vis) == VIS_MODE_OPERATOR_PENDING)
		vis_motion(vis, VIS_MOVE_CHAR_NEXT);
	return next;
}

static const char *count(Vis *vis, const char *keys, const Arg *arg) {
	int digit = keys[-1] - '0';
	int count = vis_count_get_default(vis, 0);
	if (0 <= digit && digit <= 9) {
		if (digit == 0 && count == 0)
			vis_motion(vis, VIS_MOVE_LINE_BEGIN);
		else
			vis_count_set(vis, count * 10 + digit);
	}
	return keys;
}

static const char *gotoline(Vis *vis, const char *keys, const Arg *arg) {
	if (vis_count_get(vis) != VIS_COUNT_UNKNOWN)
		vis_motion(vis, VIS_MOVE_LINE);
	else if (arg->i < 0)
		vis_motion(vis, VIS_MOVE_FILE_BEGIN);
	else
		vis_motion(vis, VIS_MOVE_FILE_END);
	return keys;
}

static const char *operator(Vis *vis, const char *keys, const Arg *arg) {
	vis_operator(vis, arg->i);
	return keys;
}

static const char *movement_key(Vis *vis, const char *keys, const Arg *arg) {
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

static const char *movement(Vis *vis, const char *keys, const Arg *arg) {
	vis_motion(vis, arg->i);
	return keys;
}

static const char *textobj(Vis *vis, const char *keys, const Arg *arg) {
	vis_textobject(vis, arg->i);
	return keys;
}

static const char *selection_end(Vis *vis, const char *keys, const Arg *arg) {
	for (Selection *s = view_selections(vis_view(vis)); s; s = view_selections_next(s))
		view_selections_flip(s);
	return keys;
}

static const char *reg(Vis *vis, const char *keys, const Arg *arg) {
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisRegister reg = vis_register_from(vis, keys[0]);
	vis_register(vis, reg);
	return keys+1;
}

static const char *mark(Vis *vis, const char *keys, const Arg *arg) {
	if (!keys[0])
		return NULL;
	const char *next = vis_keys_next(vis, keys);
	if (next - keys > 1)
		return next;
	enum VisMark mark = vis_mark_from(vis, keys[0]);
	vis_mark(vis, mark);
	return keys+1;
}

static const char *undo(Vis *vis, const char *keys, const Arg *arg) {
	size_t pos = text_undo(vis_text(vis));
	if (pos != EPOS) {
		View *view = vis_view(vis);
		if (view_selections_count(view) == 1)
			view_cursor_to(view, pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static const char *redo(Vis *vis, const char *keys, const Arg *arg) {
	size_t pos = text_redo(vis_text(vis));
	if (pos != EPOS) {
		View *view = vis_view(vis);
		if (view_selections_count(view) == 1)
			view_cursor_to(view, pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static const char *earlier(Vis *vis, const char *keys, const Arg *arg) {
	size_t pos = EPOS;
	VisCountIterator it = vis_count_iterator_get(vis, 1);
	while (vis_count_iterator_next(&it))
		pos = text_earlier(vis_text(vis));
	if (pos != EPOS) {
		view_cursor_to(vis_view(vis), pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static const char *later(Vis *vis, const char *keys, const Arg *arg) {
	size_t pos = EPOS;
	VisCountIterator it = vis_count_iterator_get(vis, 1);
	while (vis_count_iterator_next(&it))
		pos = text_later(vis_text(vis));
	if (pos != EPOS) {
		view_cursor_to(vis_view(vis), pos);
		/* redraw all windows in case some display the same file */
		vis_draw(vis);
	}
	return keys;
}

static const char *delete(Vis *vis, const char *keys, const Arg *arg) {
	vis_operator(vis, VIS_OP_DELETE);
	vis_motion(vis, arg->i);
	return keys;
}

static const char *insert_register(Vis *vis, const char *keys, const Arg *arg) {
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

static const char *prompt_show(Vis *vis, const char *keys, const Arg *arg) {
	vis_prompt_show(vis, arg->s);
	return keys;
}

static const char *insert_verbatim(Vis *vis, const char *keys, const Arg *arg) {
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

static const char *wscroll(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	int count = vis_count_get(vis);
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
	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *wslide(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	int count = vis_count_get(vis);
	if (count == VIS_COUNT_UNKNOWN)
		count = arg->i < 0 ? -arg->i : arg->i;
	if (arg->i >= 0)
		view_slide_down(view, count);
	else
		view_slide_up(view, count);
	vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *call(Vis *vis, const char *keys, const Arg *arg) {
	arg->f(vis);
	return keys;
}

static const char *window(Vis *vis, const char *keys, const Arg *arg) {
	arg->w(vis_view(vis));
	return keys;
}

static const char *openline(Vis *vis, const char *keys, const Arg *arg) {
	vis_operator(vis, VIS_OP_MODESWITCH, VIS_MODE_INSERT);
	if (arg->i > 0) {
		vis_motion(vis, VIS_MOVE_LINE_END);
		vis_keys_feed(vis, "<Enter>");
	} else {
		if (vis_get_autoindent(vis)) {
			vis_motion(vis, VIS_MOVE_LINE_START);
			vis_keys_feed(vis, "<vis-motion-line-start>");
		} else {
			vis_motion(vis, VIS_MOVE_LINE_BEGIN);
			vis_keys_feed(vis, "<vis-motion-line-begin>");
		}
		vis_keys_feed(vis, "<Enter><Up>");
	}
	return keys;
}

static const char *join(Vis *vis, const char *keys, const Arg *arg) {
	bool normal = (vis_mode_get(vis) == VIS_MODE_NORMAL);
	vis_operator(vis, VIS_OP_JOIN, arg->s);
	if (normal) {
		int count = vis_count_get_default(vis, 0);
		if (count)
			vis_count_set(vis, count-1);
		vis_motion(vis, VIS_MOVE_LINE_NEXT);
	}
	return keys;
}

static const char *normalmode_escape(Vis *vis, const char *keys, const Arg *arg) {
	if (vis_count_get(vis) == VIS_COUNT_UNKNOWN)
		selections_clear(vis, keys, arg);
	else
		vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *visualmode_escape(Vis *vis, const char *keys, const Arg *arg) {
	if (vis_count_get(vis) == VIS_COUNT_UNKNOWN)
		vis_mode_switch(vis, VIS_MODE_NORMAL);
	else
		vis_count_set(vis, VIS_COUNT_UNKNOWN);
	return keys;
}

static const char *switchmode(Vis *vis, const char *keys, const Arg *arg) {
	vis_mode_switch(vis, arg->i);
	return keys;
}

static const char *insertmode(Vis *vis, const char *keys, const Arg *arg) {
	vis_operator(vis, VIS_OP_MODESWITCH, VIS_MODE_INSERT);
	vis_motion(vis, arg->i);
	return keys;
}

static const char *replacemode(Vis *vis, const char *keys, const Arg *arg) {
	vis_operator(vis, VIS_OP_MODESWITCH, VIS_MODE_REPLACE);
	vis_motion(vis, arg->i);
	return keys;
}

static const char *unicode_info(Vis *vis, const char *keys, const Arg *arg) {
	View *view = vis_view(vis);
	Text *txt = vis_text(vis);
	size_t start = view_cursor_get(view);
	size_t end = text_char_next(txt, start);
	char *grapheme = text_bytes_alloc0(txt, start, end-start), *codepoint = grapheme;
	if (!grapheme)
		return keys;
	Buffer info;
	buffer_init(&info);
	mbstate_t ps = { 0 };
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

static const char *percent(Vis *vis, const char *keys, const Arg *arg) {
	if (vis_count_get(vis) == VIS_COUNT_UNKNOWN)
		vis_motion(vis, VIS_MOVE_BRACKET_MATCH);
	else
		vis_motion(vis, VIS_MOVE_PERCENT);
	return keys;
}

static const char *jumplist(Vis *vis, const char *keys, const Arg *arg) {
	if (arg->i < 0)
		vis_jumplist_prev(vis);
	else if (arg->i > 0)
		vis_jumplist_next(vis);
	else
		vis_jumplist_save(vis);
	return keys;
}

static Vis *vis;

static void signal_handler(int signum, siginfo_t *siginfo, void *context) {
	vis_signal_handler(vis, signum, siginfo, context);
}

int main(int argc, char *argv[]) {

	VisEvent event = {
		.init = vis_lua_init,
		.start = vis_lua_start,
		.quit = vis_lua_quit,
		.mode_insert_input = vis_lua_mode_insert_input,
		.mode_replace_input = vis_lua_mode_replace_input,
		.file_open = vis_lua_file_open,
		.file_save_pre = vis_lua_file_save_pre,
		.file_save_post = vis_lua_file_save_post,
		.file_close = vis_lua_file_close,
		.win_open = vis_lua_win_open,
		.win_close = vis_lua_win_close,
		.win_highlight = vis_lua_win_highlight,
		.win_status = vis_lua_win_status,
		.term_csi = vis_lua_term_csi,
	};

	vis = vis_new(ui_term_new(), &event);
	if (!vis)
		return EXIT_FAILURE;

	for (int i = 0; i < LENGTH(vis_action); i++) {
		const KeyAction *action = &vis_action[i];
		if (!vis_action_register(vis, action))
			vis_die(vis, "Could not register action: %s\n", action->name);
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

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			continue;
		} else if (strcmp(argv[i], "-") == 0) {
			continue;
		} else if (strcmp(argv[i], "--") == 0) {
			break;
		} else if (strcmp(argv[i], "-v") == 0) {
			printf("vis %s%s%s%s%s%s%s\n", VERSION,
			       CONFIG_CURSES ? " +curses" : "",
			       CONFIG_LUA ? " +lua" : "",
			       CONFIG_LPEG ? " +lpeg" : "",
			       CONFIG_TRE ? " +tre" : "",
			       CONFIG_ACL ? " +acl" : "",
			       CONFIG_SELINUX ? " +selinux" : "");
			return 0;
		} else {
			fprintf(stderr, "Unknown command option: %s\n", argv[i]);
			return 1;
		}
	}

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
			vis_die(vis, "Can not load `%s': %s\n", argv[i], strerror(errno));
		}
		win_created = true;
		if (cmd) {
			vis_prompt_cmd(vis, cmd);
			cmd = NULL;
		}
	}

	if (!vis_window(vis) && !win_created) {
		if (!vis_window_new(vis, NULL))
			vis_die(vis, "Can not create empty buffer\n");
		if (cmd)
			vis_prompt_cmd(vis, cmd);
	}

	int status = vis_run(vis);
	vis_free(vis);
	return status;
}
