#ifndef VIEW_H
#define VIEW_H

#include <stddef.h>
#include <stdbool.h>

typedef struct View View;
typedef struct Selection Selection;

#include "text.h"
#include "ui.h"
#include "array.h"

typedef struct {
	Mark anchor;
	Mark cursor;
} SelectionRegion;

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

/**
 * @defgroup view_life
 * @{
 */
View *view_new(Text*);
void view_free(View*);
void view_ui(View*, UiWin*);
Text *view_text(View*);
void view_reload(View*, Text*);
/**
 * @}
 * @defgroup view_viewport
 * @{
 */
/** Get the currently displayed text range. */
Filerange view_viewport_get(View*);
/**
 * Get window coordinate of text position.
 * @param pos The position to query.
 * @param line Will be updated with screen line on which ``pos`` resides.
 * @param row Will be updaded with zero based window row on which ``pos`` resides.
 * @param col Will be updated with zero based window column on which ``pos`` resides.
 * @return Whether ``pos`` is visible. If not, the pointer arguments are left unmodified.
 */
bool view_coord_get(View*, size_t pos, Line **line, int *row, int *col);
/** Get position at the start of the ``n``-th window line, counting from 1. */
size_t view_screenline_goto(View*, int n);
/** Get first screen line. */
Line *view_lines_first(View*);
/** Get last non-empty screen line. */
Line *view_lines_last(View*);
size_t view_slide_up(View*, int lines);
size_t view_slide_down(View*, int lines);
size_t view_scroll_up(View*, int lines);
size_t view_scroll_down(View*, int lines);
size_t view_scroll_page_up(View*);
size_t view_scroll_page_down(View*);
size_t view_scroll_halfpage_up(View*);
size_t view_scroll_halfpage_down(View*);
void view_redraw_top(View*);
void view_redraw_center(View*);
void view_redraw_bottom(View*);
void view_scroll_to(View*, size_t pos);
/**
 * @}
 * @defgroup view_size
 * @{
 */
bool view_resize(View*, int width, int height);
int view_height_get(View*);
int view_width_get(View*);
/**
 * @}
 * @defgroup view_draw
 * @{
 */
void view_invalidate(View*);
void view_draw(View*);
bool view_update(View*);

/**
 * @}
 * @defgroup view_selnew
 * @{
 */
/**
 * Create a new singleton selection at the given position.
 * @rst
 * .. note:: New selections are created non-anchored.
 * .. warning:: Fails if position is already covered by a selection.
 * @endrst
 */
Selection *view_selections_new(View*, size_t pos);
/**
 * Create a new selection even if position is already covered by an
 * existing selection. 
 * @rst
 * .. note:: This should only be used if the old selection is eventually
 *           disposed.
 * @endrst
 */
Selection *view_selections_new_force(View*, size_t pos);
/**
 * Dispose an existing selection.
 * @rst
 * .. warning:: Not applicable for the last existing selection.
 * @endrst
 */
bool view_selections_dispose(Selection*);
/**
 * Forcefully dispose an existing selection.
 *
 * If called for the last existing selection, it will be reduced and
 * marked for destruction. As soon as a new selection is created this one
 * will be disposed.
 */
bool view_selections_dispose_force(Selection*);
/**
 * Query state of primary selection.
 *
 * If the primary selection was marked for destruction, return it and
 * clear destruction flag.
 */
Selection *view_selection_disposed(View*);
/** Dispose all but the primary selection. */
void view_selections_dispose_all(View*);
/** Dispose all invalid and merge all overlapping selections. */
void view_selections_normalize(View*);
/**
 * Replace currently active selections.
 * @param array The array of ``Filerange`` objects.
 * @param anchored Whether *all* selection should be anchored.
 */
void view_selections_set_all(View*, Array*, bool anchored);
/** Get array containing a ``Fileranges`` for each selection. */
Array view_selections_get_all(View*);
/**
 * @}
 * @defgroup view_navigate
 * @{
 */
Selection *view_selections_primary_get(View*);
void view_selections_primary_set(Selection*);
/** Get first selection. */
Selection *view_selections(View*);
/** Get immediate predecessor of selection. */
Selection *view_selections_prev(Selection*);
/** Get immediate successor of selection. */
Selection *view_selections_next(Selection*);
/**
 * Get number of existing selections.
 * @rst
 * .. note:: Is always at least 1.
 * @endrst
 */
int view_selections_count(View*);
/**
 * Get selection index.
 * @rst
 * .. note:: Is always in range ``[0, count-1]``.
 * .. warning: The relative order is determined during creation and assumed
 *             to remain the same.
 * @endrst
 */
int view_selections_number(Selection*);
/** Get maximal number of selections on a single line. */
int view_selections_column_count(View*);
/**
 * Starting from the start of the text, get the `column`-th selection on a line.
 * @param column The zero based column index.
 */
Selection *view_selections_column(View*, int column);
/**
 * Get the next `column`-th selection on a line.
 * @param column The zero based column index.
 */
Selection *view_selections_column_next(Selection*, int column);
/**
 * @}
 * @defgroup view_cover
 * @{
 */
/** Get an inclusive range of the selection cover. */
Filerange view_selections_get(Selection*);
/** Set selection cover. Updates both cursor and anchor. */
bool view_selections_set(Selection*, const Filerange*);
/**
 * Reduce selection to character currently covered by the cursor.
 * @rst
 * .. note:: Sets selection to non-anchored mode.
 * @endrst
 */
void view_selection_clear(Selection*);
/** Reduce *all* currently active selections. */
void view_selections_clear_all(View*);
/**
 * Flip selection orientation. Swap cursor and anchor.
 * @rst
 * .. note:: Has no effect on singleton selections.
 * @endrst
 */
void view_selections_flip(Selection*);
/**
 * @}
 * @defgroup view_anchor
 * @{
 */
/**
 * Anchor selection.
 * Further updates will only update the cursor, the anchor will remain fixed.
 */
void view_selections_anchor(Selection*, bool anchored);
/** Check whether selection is anchored. */
bool view_selections_anchored(Selection*);
/** Get position of selection cursor. */
/**
 * @}
 * @defgroup view_props
 * @{
 */
/** Get position of selection cursor. */
size_t view_cursors_pos(Selection*);
/** Get 1-based line number of selection cursor. */
size_t view_cursors_line(Selection*);
/**
 * Get 1-based column of selection cursor.
 * @rst
 * .. note:: Counts the number of graphemes on the logical line up to the cursor
 *           position.
 * @endrst
 */
size_t view_cursors_col(Selection*);
/**
 * Get screen line of selection cursor.
 * @rst
 * .. warning: Is `NULL` for non-visible selections.
 * @endrst
 */
Line *view_cursors_line_get(Selection*);
/**
 * Get zero based index of screen cell on which selection cursor currently resides.
 * @rst
 * .. warning:: Returns ``-1`` if the selection cursor is currently not visible.
 * @endrst
 */
int view_cursors_cell_get(Selection*);
/**
 * @}
 * @defgroup view_place
 * @{
 */
/**
 * Place cursor of selection at `pos`.
 * @rst
 * .. note:: If the selection is not anchored, both selection endpoints
 *           will be adjusted to form a singleton selection covering one
 *           character starting at `pos`. Otherwise only the selection
 *           cursor will be changed while the anchor remains fixed.
 * @endrst
 */
void view_cursors_to(Selection*, size_t pos);
/**
 * Adjusts window viewport until the requested position becomes visible.
 * @rst
 * .. note:: For all but the primary selection this is equivalent to
 *           ``view_selection_to``.
 * .. warning:: Repeatedly redraws the window content. Should only be used for
 *              short distances between current cursor position and destination.
 * @endrst
 */
void view_cursors_scroll_to(Selection*, size_t pos);
/**
 * Place cursor on given (line, column) pair.
 * @param line the 1-based line number
 * @param col the 1 based column
 * @rst
 * .. note:: Except for the different addressing format this is equivalent to
 *           `view_selection_to`.
 * @endrst
 */
void view_cursors_place(Selection*, size_t line, size_t col);
/**
 * Place selection cursor on zero based window cell index.
 * @rst
 * .. warning:: Fails if the selection cursor is currently not visible.
 * @endrst
 */
int view_cursors_cell_set(Selection*, int cell);
/**
 * @}
 * @defgroup view_motions
 * @{
 */
size_t view_line_down(Selection*);
size_t view_line_up(Selection*);
size_t view_screenline_down(Selection*);
size_t view_screenline_up(Selection*);
size_t view_screenline_begin(Selection*);
size_t view_screenline_middle(Selection*);
size_t view_screenline_end(Selection*);
/**
 * @}
 * @defgroup view_primary
 * @{
 */
/**
 * Move primary selection cursor to the given position.
 * Makes sure that position is visible.
 * @rst
 * .. note:: If position was not visible before, we attempt to show
 *           surrounding context. The viewport will be adjusted such
 *           that the line holding the cursor is shown in the middle
 *           of the window.
 * @endrst
 */
void view_cursor_to(View*, size_t pos);
/** Get cursor position of primary selection. */
size_t view_cursor_get(View*);
/**
 * Get primary selection.
 * @rst
 * .. note:: Is always a non-empty range.
 * @endrst
 */
Filerange view_selection_get(View*);
/**
 * @}
 * @defgroup view_save
 * @{
 */
Filerange view_regions_restore(View*, SelectionRegion*);
bool view_regions_save(View*, Filerange*, SelectionRegion*);
/**
 * @}
 * @defgroup view_style
 * @{
 */
void view_options_set(View*, enum UiOption options);
enum UiOption view_options_get(View*);
void view_colorcolumn_set(View*, int col);
int view_colorcolumn_get(View*);

/** Set how many spaces are used to display a tab `\t` character. */
void view_tabwidth_set(View*, int tabwidth);
/** Define a display style. */
bool view_style_define(View*, enum UiStyle, const char *style);
/** Apply a style to a text range. */
void view_style(View*, enum UiStyle, size_t start, size_t end);

char *view_symbol_eof_get(View*);

/** @} */

#endif
