#ifndef VIEW_H
#define VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include "register.h"
#include "text.h"
#include "ui.h"
#include "syntax.h"

typedef struct View View;
typedef struct Cursor Cursor;
typedef struct Selection Selection;

typedef struct {
	void *data;
	void (*selection)(void *data, Filerange*);
} ViewEvent;

typedef struct {
	int width;          /* display width i.e. number of columns ocupied by this character */
	size_t len;         /* number of bytes the character displayed in this cell uses, for
	                       character which use more than 1 column to display, their lenght
	                       is stored in the leftmost cell wheras all following cells
	                       occupied by the same character have a length of 0. */
	char data[8];       /* utf8 encoded character displayed in this cell might not be the
			       the same as in the underlying text, for example tabs get expanded */
	unsigned int attr;
	bool istab;
	bool selected;      /* whether this cell is part of a selected region */
	bool cursor;        /* whether a cursor is currently locaated on the cell */
} Cell;

typedef struct Line Line;
struct Line {               /* a line on the screen, *not* in the file */
	Line *prev, *next;  /* pointer to neighbouring screen lines */
	size_t len;         /* line length in terms of bytes */
	size_t lineno;      /* line number from start of file */
	int width;          /* zero based position of last used column cell */
	Cell cells[];       /* win->width cells storing information about the displayed characters */
};

typedef struct {
	size_t line;
	size_t col;
} CursorPos;

View *view_new(Text*, ViewEvent*);
void view_ui(View*, UiWin*);
/* change associated text displayed in this window */
void view_reload(View*, Text*);
void view_free(View*);

bool view_resize(View*, int width, int height);
int view_height_get(View*);
int view_width_get(View*);
void view_draw(View*);
/* changes how many spaces are used for one tab (must be >0), redraws the window */
void view_tabwidth_set(View*, int tabwidth);

/* cursor movements which also update selection if one is active.
 * they return new cursor postion */
size_t view_line_down(Cursor*);
size_t view_line_up(Cursor*);
size_t view_screenline_down(Cursor*);
size_t view_screenline_up(Cursor*);
size_t view_screenline_begin(Cursor*);
size_t view_screenline_middle(Cursor*);
size_t view_screenline_end(Cursor*);
/* move window content up/down, but keep cursor position unchanged unless it is
 * on a now invisible line in which case we try to preserve the column position */
size_t view_slide_up(View*, int lines);
size_t view_slide_down(View*, int lines);
/* scroll window contents up/down by lines, place the cursor on the newly
 * visible line, try to preserve the column position */
size_t view_scroll_up(View*, int lines);
size_t view_scroll_down(View*, int lines);
/* place the cursor at the start ot the n-th window line, counting from 1 */
size_t view_screenline_goto(View*, int n);

const Line *view_lines_get(View*);
/* redraw current cursor line at top/center/bottom of window */
void view_redraw_top(View*);
void view_redraw_center(View*);
void view_redraw_bottom(View*);
/* get the currently displayed area in bytes from the start of the file */
Filerange view_viewport_get(View*);
/* move visible viewport n-lines up/down, redraws the view but does not change
 * cursor position which becomes invalid and should be corrected by calling
 * view_cursor_to. the return value indicates wether the visible area changed.
 */
bool view_viewport_up(View *view, int n);
bool view_viewport_down(View *view, int n);
/* associate a set of syntax highlighting rules to this window. */
void view_syntax_set(View*, Syntax*);
Syntax *view_syntax_get(View*);
void view_symbols_set(View*, int flags);
int view_symbols_get(View*);

/* A view can manage multiple cursors, one of which (the main cursor) is always
 * placed within the visible viewport. All functions named view_cursor_* operate
 * on this cursor. Additional cursor can be created and manipulated using the
 * functions named view_cursors_* */

/* get main cursor position in terms of screen coordinates */
CursorPos view_cursor_getpos(View*);
/* get main cursor position in bytes from start of the file */
size_t view_cursor_get(View*);
/* moves window viewport in direction until pos is visible. should only be
 * used for short distances between current cursor position and destination */
void view_scroll_to(View*, size_t pos);
/* move cursor to a given position. changes the viewport to make sure that
 * position is visible. if the position is in the middle of a line, try to
 * adjust the viewport in such a way that the whole line is displayed */
void view_cursor_to(View*, size_t pos);
/* create a new cursor */
Cursor *view_cursors_new(View*);
/* get number of active cursors */
int view_cursors_count(View*);
/* dispose an existing cursor with its associated selection (if any),
 * not applicaple for the last existing cursor */
void view_cursors_dispose(Cursor*);
/* only keep the main cursor, release all others together with their
 * selections (if any) */
void view_cursors_clear(View*);
/* get the main cursor which is always in the visible viewport */
Cursor *view_cursor(View*);
/* get the first cursor */
Cursor *view_cursors(View*);
Cursor *view_cursors_prev(Cursor*);
Cursor *view_cursors_next(Cursor*);
/* get current position of cursor in bytes from the start of the file */
size_t view_cursors_pos(Cursor*);
/* place cursor at `pos' which should be in the interval [0, text-size] */
void view_cursors_to(Cursor*, size_t pos);
void view_cursors_scroll_to(Cursor*, size_t pos);
/* get register associated with this register */
Register *view_cursors_register(Cursor*);
/* start selected area at current cursor position. further cursor movements
 * will affect the selected region. */
void view_cursors_selection_start(Cursor*);
/* detach cursor from selection, further cursor movements will not affect
 * the selected region. */ 
void view_cursors_selection_stop(Cursor*);
/* clear selection associated with this cursor (if any) */
void view_cursors_selection_clear(Cursor*);
/* move cursor position from one end of the selection to the other */
void view_cursors_selection_swap(Cursor*);
/* move cursor to the end/boundary of the associated selection */
void view_cursors_selection_sync(Cursor*);
/* restore previous used selection of this cursor */
void view_cursors_selection_restore(Cursor*);
/* get/set the selected region associated with this cursor */
Filerange view_cursors_selection_get(Cursor*);
void view_cursors_selection_set(Cursor*, Filerange*);

Selection *view_selections_new(View*);
void view_selections_free(Selection*);
void view_selections_clear(View*);
void view_selections_swap(Selection*);
Selection *view_selections(View*);
Selection *view_selections_prev(Selection*);
Selection *view_selections_next(Selection*);
Filerange view_selections_get(Selection*);
void view_selections_set(Selection*, Filerange*);

#endif
