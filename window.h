#ifndef WINDOW_H
#define WINDOW_H

#include <curses.h>
#include "text.h"

typedef struct Syntax Syntax;
typedef struct Win Win;
typedef size_t Filepos;

Win *window_new(Text*);
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

/* cursor movements, also updates selection if one is active, returns new cursor postion */
size_t window_page_down(Win*);
size_t window_page_up(Win*);
size_t window_char_next(Win*);
size_t window_char_prev(Win*);
size_t window_line_down(Win*);
size_t window_line_up(Win*);

size_t window_cursor_get(Win*);
void window_cursor_getxy(Win*, size_t *lineno, size_t *col); 
void window_scroll_to(Win*, size_t pos);
void window_cursor_to(Win*, size_t pos);
void window_selection_start(Win*);
void window_selection_end(Win*);
Filerange window_selection_get(Win*);
void window_selection_clear(Win*);
Filerange window_viewport_get(Win*);
void window_syntax_set(Win*, Syntax*);
Syntax *window_syntax_get(Win*); 
void window_cursor_watch(Win *win, void (*cursor_moved)(Win*, void*), void *data); 

#endif
