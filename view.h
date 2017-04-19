#ifndef VIEW_H
#define VIEW_H

#include <stddef.h>
#include <stdbool.h>

typedef struct View View;
typedef struct Cursor Cursor;
typedef struct Selection Selection;

#include "text.h"
#include "ui.h"
#include "register.h"

typedef struct {
	char data[16];      /* utf8 encoded character displayed in this cell (might be more than
	                       one Unicode codepoint. might also not be the same as in the
	                       underlying text, for example tabs get expanded */
	size_t len;         /* number of bytes the character displayed in this cell uses, for
	                       characters which use more than 1 column to display, their length
	                       is stored in the leftmost cell whereas all following cells
	                       occupied by the same character have a length of 0. */
	int width;          /* display width i.e. number of columns occupied by this character */
	CellStyle style;    /* colors and attributes used to display this cell */
} Cell;

typedef struct Line Line;
struct Line {               /* a line on the screen, *not* in the file */
	Line *prev, *next;  /* pointer to neighbouring screen lines */
	size_t len;         /* line length in terms of bytes */
	size_t lineno;      /* line number from start of file */
	int width;          /* zero based position of last used column cell */
	Cell cells[];       /* win->width cells storing information about the displayed characters */
};

View *view_new(Text*);
void view_ui(View*, UiWin*);
/* change associated text displayed in this window */
void view_reload(View*, Text*);
void view_free(View*);

bool view_resize(View*, int width, int height);
int view_height_get(View*);
int view_width_get(View*);
void view_invalidate(View*);
void view_draw(View*);
bool view_update(View*);
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
size_t view_scroll_page_up(View*);
size_t view_scroll_page_down(View*);
size_t view_scroll_halfpage_up(View*);
size_t view_scroll_halfpage_down(View*);
/* place the cursor at the start ot the n-th window line, counting from 1 */
size_t view_screenline_goto(View*, int n);

Line *view_lines_first(View*);
Line *view_lines_last(View*);
Line *view_cursors_line_get(Cursor*);
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

void view_options_set(View*, enum UiOption options);
enum UiOption view_options_get(View*);
void view_colorcolumn_set(View*, int col);
int view_colorcolumn_get(View*);

bool view_coord_get(View*, size_t pos, Line **retline, int *retrow, int *retcol);
/* A view can manage multiple cursors, one of which (the main cursor) is always
 * placed within the visible viewport. All functions named view_cursor_* operate
 * on this cursor. Additional cursor can be created and manipulated using the
 * functions named view_cursors_* */

/* get main cursor position in bytes from start of the file */
size_t view_cursor_get(View*);
/* get selection associated with primary cursor */
Filerange view_selection_get(View*);
/* moves window viewport in direction until pos is visible. should only be
 * used for short distances between current cursor position and destination */
void view_scroll_to(View*, size_t pos);
/* move cursor to a given position. changes the viewport to make sure that
 * position is visible. if the position is in the middle of a line, try to
 * adjust the viewport in such a way that the whole line is displayed */
void view_cursor_to(View*, size_t pos);
/* create a new cursor at given position, fails if there already
 * exists a cursor at the same position */
Cursor *view_cursors_new(View*, size_t pos);
/* create a new cursor even if there already is one located at the
 * same position, this should only be used if the one of the two
 * cursors will later be disposed */
Cursor *view_cursors_new_force(View*, size_t pos);
/* get number of active cursors */
int view_cursors_count(View*);
/* get index/relative order at time of creation of a cursor [0,count-1] */
int view_cursors_number(Cursor*);
/* exist there more than 1 cursor */
bool view_cursors_multiple(View*);
/* dispose an existing cursor with its associated selection (if any),
 * not applicaple for the last existing cursor */
bool view_cursors_dispose(Cursor*);
/* if called for the last existing cursor its selection is destroyed
 * and the cursor is marked for destruction and will be disposed as
 * soon as a new cursor is created. */
bool view_cursors_dispose_force(Cursor*);
/* if the primary cursor was marked for destruction (by means of
 * view_cursors_dispose_force) return it and clear descruction flag */
Cursor *view_cursor_disposed(View*);
/* only keep the main cursor, release all others together with their
 * selections (if any) */
void view_cursors_clear(View*);
/* get the first cursor */
Cursor *view_cursors(View*);
/* get other cursors, no ordering is guaranteed */
Cursor *view_cursors_prev(Cursor*);
Cursor *view_cursors_next(Cursor*);
/* get the primary cursor which is always in the visible viewport */
Cursor *view_cursors_primary_get(View*);
void view_cursors_primary_set(Cursor*);
/* get current position of cursor in bytes from the start of the file */
size_t view_cursors_pos(Cursor*);
/* get 1-based line number of cursor location */
size_t view_cursors_line(Cursor*);
/* get 1-based column (number of graphemes on line) cursor postion */
size_t view_cursors_col(Cursor*);
/* get/set zero based index of cell on which cursor currently resides,
 * -1 if cursor is currently not visible */
int view_cursors_cell_get(Cursor*);
int view_cursors_cell_set(Cursor*, int cell);
/* place cursor at `pos' which should be in the interval [0, text-size] */
void view_cursors_to(Cursor*, size_t pos);
void view_cursors_scroll_to(Cursor*, size_t pos);
/* place cursor on given (line, column) pair, both values are 1-based */
void view_cursors_place(Cursor*, size_t line, size_t col);
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
void view_cursors_selection_set(Cursor*, const Filerange*);

Selection *view_selections_new(View*, Cursor*);
void view_selections_free(Selection*);
void view_selections_clear(View*);
void view_selections_swap(Selection*);
Selection *view_selections(View*);
Selection *view_selections_prev(Selection*);
Selection *view_selections_next(Selection*);
Filerange view_selections_get(Selection*);
void view_selections_set(Selection*, const Filerange*);
Text *view_text(View*);

/* get number of columns, that is maximal number of cursors on a line */
int view_cursors_column_count(View*);
/* get first cursor in zero based column */
Cursor *view_cursors_column(View*, int column);
/* get next cursor (i.e. on another line) in zero based column */
Cursor *view_cursors_column_next(Cursor*, int column);

bool view_style_define(View*, enum UiStyle, const char *style);
void view_style(View*, enum UiStyle, size_t start, size_t end);

#endif
