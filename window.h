#ifndef WINDOW_H
#define WINDOW_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"
#include "syntax.h"

typedef struct Win Win;

typedef struct {
	size_t line;
	size_t col;
} CursorPos;

Win *window_new(Text*);
/* change associated text displayed in this window */
void window_reload(Win*, Text*);
void window_free(Win*);

/* keyboard input at cursor position */
size_t window_insert_key(Win*, const char *c, size_t len);
size_t window_replace_key(Win*, const char *c, size_t len);
size_t window_backspace_key(Win*);
size_t window_delete_key(Win*);

bool window_resize(Win*, int width, int height);
void window_move(Win*, int x, int y);
void window_draw(Win*);
/* flush all changes made to the ncurses windows to the screen */
void window_update(Win*);
/* changes how many spaces are used for one tab (must be >0), redraws the window */
void window_tabwidth_set(Win*, int tabwidth);

/* cursor movements which also update selection if one is active.
 * they return new cursor postion */
size_t window_char_next(Win*);
size_t window_char_prev(Win*);
size_t window_line_down(Win*);
size_t window_line_up(Win*);
size_t window_line_begin(Win*);
size_t window_line_middle(Win*);
size_t window_line_end(Win*);
/* move window content up/down, but keep cursor position unchanged unless it is
 * on a now invisible line in which case we try to preserve the column position */
size_t window_slide_up(Win*, int lines);
size_t window_slide_down(Win*, int lines);
/* scroll window contents up/down by lines, place the cursor on the newly
 * visible line, try to preserve the column position */
size_t window_scroll_up(Win*, int lines);
size_t window_scroll_down(Win*, int lines);
/* place the cursor at the start ot the n-th window line, counting from 1 */
size_t window_line_goto(Win*, int n);

/* get cursor position in bytes from start of the file */
size_t window_cursor_get(Win*);
/* get cursor position in terms of screen coordinates */
CursorPos window_cursor_getpos(Win*);
/* moves window viewport in direction until pos is visible. should only be
 * used for short distances between current cursor position and destination */
void window_scroll_to(Win*, size_t pos);
/* move cursor to a given position. changes the viewport to make sure that
 * position is visible. if the position is in the middle of a line, try to
 * adjust the viewport in such a way that the whole line is displayed */
void window_cursor_to(Win*, size_t pos);
/* redraw current cursor line at top/center/bottom of window */
void window_redraw_top(Win*);
void window_redraw_center(Win*);
void window_redraw_bottom(Win*);
/* start selected area at current cursor position. further cursor movements will
 * affect the selected region. */
void window_selection_start(Win*);
/* returns the currently selected text region, is either empty or well defined,
 * i.e. sel.start <= sel.end */
Filerange window_selection_get(Win*);
void window_selection_set(Win*, Filerange *sel);
/* clear selection and redraw window */
void window_selection_clear(Win*);
/* get the currently displayed area in bytes from the start of the file */
Filerange window_viewport_get(Win*);
/* associate a set of syntax highlighting rules to this window. */
void window_syntax_set(Win*, Syntax*);
Syntax *window_syntax_get(Win*);
/* whether to show line numbers to the left of the text content */
void window_line_numbers_show(Win*, bool show);
/* register a user defined function which will be called whenever the cursor has moved */
void window_cursor_watch(Win *win, void (*cursor_moved)(Win*, void*), void *data);

#endif
