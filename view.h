#ifndef VIEW_H
#define VIEW_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"
#include "ui.h"
#include "syntax.h"

typedef struct View View;

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
	bool istab;
	unsigned int attr;
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

/* keyboard input at cursor position */
size_t view_insert_key(View*, const char *c, size_t len);
size_t view_replace_key(View*, const char *c, size_t len);
size_t view_backspace_key(View*);
size_t view_delete_key(View*);

bool view_resize(View*, int width, int height);
int view_height_get(View*);
void view_draw(View*);
/* changes how many spaces are used for one tab (must be >0), redraws the window */
void view_tabwidth_set(View*, int tabwidth);

/* cursor movements which also update selection if one is active.
 * they return new cursor postion */
size_t view_char_next(View*);
size_t view_char_prev(View*);
size_t view_line_down(View*);
size_t view_line_up(View*);
size_t view_screenline_down(View*);
size_t view_screenline_up(View*);
size_t view_screenline_begin(View*);
size_t view_screenline_middle(View*);
size_t view_screenline_end(View*);
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

/* get cursor position in bytes from start of the file */
size_t view_cursor_get(View*);

const Line *view_lines_get(View*);
/* get cursor position in terms of screen coordinates */
CursorPos view_cursor_getpos(View*);
/* moves window viewport in direction until pos is visible. should only be
 * used for short distances between current cursor position and destination */
void view_scroll_to(View*, size_t pos);
/* move cursor to a given position. changes the viewport to make sure that
 * position is visible. if the position is in the middle of a line, try to
 * adjust the viewport in such a way that the whole line is displayed */
void view_cursor_to(View*, size_t pos);
/* redraw current cursor line at top/center/bottom of window */
void view_redraw_top(View*);
void view_redraw_center(View*);
void view_redraw_bottom(View*);
/* start selected area at current cursor position. further cursor movements will
 * affect the selected region. */
void view_selection_start(View*);
/* returns the currently selected text region, is either empty or well defined,
 * i.e. sel.start <= sel.end */
Filerange view_selection_get(View*);
void view_selection_set(View*, Filerange *sel);
/* clear selection and redraw window */
void view_selection_clear(View*);
/* get the currently displayed area in bytes from the start of the file */
Filerange view_viewport_get(View*);
/* associate a set of syntax highlighting rules to this window. */
void view_syntax_set(View*, Syntax*);
Syntax *view_syntax_get(View*);

#endif
