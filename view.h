#ifndef VIEW_H
#define VIEW_H

#include <stddef.h>
#include <stdbool.h>

#include "ui.h"
#include "text.h"
#include "array.h"

enum {
	SYNTAX_SYMBOL_SPACE,
	SYNTAX_SYMBOL_TAB,
	SYNTAX_SYMBOL_TAB_FILL,
	SYNTAX_SYMBOL_EOL,
	SYNTAX_SYMBOL_EOF,
	SYNTAX_SYMBOL_LAST,
};

typedef struct {
	Mark anchor;
	Mark cursor;
} SelectionRegion;

typedef struct Line Line;
struct Line {               /* a line on the screen, *not* in the file */
	Line *prev, *next;  /* pointer to neighbouring screen lines */
	size_t len;         /* line length in terms of bytes */
	size_t lineno;      /* line number from start of file */
	int width;          /* zero based position of last used column cell */
	Cell cells[];       /* win->width cells storing information about the displayed characters */
};

struct View;
typedef struct Selection {
	Mark cursor;            /* other selection endpoint where it changes */
	Mark anchor;            /* position where the selection was created */
	bool anchored;          /* whether anchor remains fixed */
	size_t pos;             /* in bytes from the start of the file */
	int row, col;           /* in terms of zero based screen coordinates */
	int lastcol;            /* remembered column used when moving across lines */
	Line *line;             /* screen line on which cursor currently resides */
	int generation;         /* used to filter out newly created cursors during iteration */
	int number;             /* how many cursors are located before this one */
	struct View *view;      /* associated view to which this cursor belongs */
	struct Selection *prev, *next; /* previous/next cursors ordered by location at creation time */
} Selection;

typedef struct View {
	Text *text;         /* underlying text management */
	char *textbuf;      /* scratch buffer used for drawing */
	int width, height;  /* size of display area */
	size_t start, end;  /* currently displayed area [start, end] in bytes from the start of the file */
	size_t start_last;  /* previously used start of visible area, used to update the mark */
	Mark start_mark;    /* mark to keep track of the start of the visible area */
	size_t lines_size;  /* number of allocated bytes for lines (grows only) */
	Line *lines;        /* view->height number of lines representing view content */
	Line *topline;      /* top of the view, first line currently shown */
	Line *lastline;     /* last currently used line, always <= bottomline */
	Line *bottomline;   /* bottom of view, might be unused if lastline < bottomline */
	Selection *selection;    /* primary selection, always placed within the visible viewport */
	Selection *selection_latest; /* most recently created cursor */
	Selection *selection_dead;   /* primary cursor which was disposed, will be removed when another cursor is created */
	int selection_count;   /* how many cursors do currently exist */
	Line *line;         /* used while drawing view content, line where next char will be drawn */
	int col;            /* used while drawing view content, column where next char will be drawn */
	const char *symbols[SYNTAX_SYMBOL_LAST]; /* symbols to use for white spaces etc */
	int tabwidth;       /* how many spaces should be used to display a tab character */
	Selection *selections;    /* all cursors currently active */
	int selection_generation; /* used to filter out newly created cursors during iteration */
	bool need_update;   /* whether view has been redrawn */
	bool large_file;    /* optimize for displaying large files */
	int colorcolumn;
	char *breakat;  /* characters which might cause a word wrap */
	int wrapcolumn; /* wrap lines at minimum of window width and wrapcolumn (if != 0) */
	int wrapcol;    /* used while drawing view content, column where word wrap might happen */
	bool prevch_breakat; /* used while drawing view content, previous char is part of breakat */
} View;

/**
 * @defgroup view_life
 * @{
 */
bool view_init(struct Win*, Text*);
void view_free(View*);
void view_reload(View*, Text*);
/**
 * @}
 * @defgroup view_viewport
 * @{
 */
/** Get the currently displayed text range. */
#define VIEW_VIEWPORT_GET(v) (Filerange){ .start = v.start, .end = v.end }
/**
 * Get window coordinate of text position.
 * @param pos The position to query.
 * @param line Will be updated with screen line on which ``pos`` resides.
 * @param row Will be updated with zero based window row on which ``pos`` resides.
 * @param col Will be updated with zero based window column on which ``pos`` resides.
 * @return Whether ``pos`` is visible. If not, the pointer arguments are left unmodified.
 */
bool view_coord_get(View*, size_t pos, Line **line, int *row, int *col);
/** Get position at the start of the ``n``-th window line, counting from 1. */
size_t view_screenline_goto(View*, int n);
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
/**
 * @}
 * @defgroup view_draw
 * @{
 */
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
 *
 *           If primary position was not visible before, we attempt to show
 *           the surrounding context. The viewport will be adjusted such
 *           that the line holding the primary cursor is shown in the middle
 *           of the window.
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
/** Get cursor position of primary selection. */
size_t view_cursor_get(View*);
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
void win_options_set(struct Win *, enum UiOption);
bool view_breakat_set(View*, const char *breakat);

/** Set how many spaces are used to display a tab `\t` character. */
void view_tabwidth_set(View*, int tabwidth);
/** Apply a style to a text range. */
void win_style(struct Win*, enum UiStyle, size_t start, size_t end);

/** @} */

#endif
