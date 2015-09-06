#ifndef UI_CURSES_H
#define UI_CURSES_H

#include <curses.h>
#include "ui.h"
#include "syntax.h"

enum {
	UI_KEY_DOWN        = KEY_DOWN,
	UI_KEY_UP          = KEY_UP,
	UI_KEY_LEFT        = KEY_LEFT,
	UI_KEY_RIGHT       = KEY_RIGHT,
	UI_KEY_HOME        = KEY_HOME,
	UI_KEY_BACKSPACE   = KEY_BACKSPACE,
	UI_KEY_DELETE      = KEY_DC,
	UI_KEY_PAGE_DOWN   = KEY_NPAGE,
	UI_KEY_PAGE_UP     = KEY_PPAGE,
	UI_KEY_END         = KEY_END,
	UI_KEY_SHIFT_LEFT  = KEY_SLEFT,
	UI_KEY_SHIFT_RIGHT = KEY_SRIGHT,
};

enum {
	UI_COLOR_DEFAULT   = -1,
	UI_COLOR_BLACK     = COLOR_BLACK,
	UI_COLOR_RED       = COLOR_RED,
	UI_COLOR_GREEN     = COLOR_GREEN,
	UI_COLOR_YELLOW    = COLOR_YELLOW,
	UI_COLOR_BLUE      = COLOR_BLUE,
	UI_COLOR_MAGENTA   = COLOR_MAGENTA,
	UI_COLOR_CYAN      = COLOR_CYAN,
	UI_COLOR_WHITE     = COLOR_WHITE,
};

enum {
	UI_ATTR_NORMAL     = A_NORMAL,
	UI_ATTR_UNDERLINE  = A_UNDERLINE,
	UI_ATTR_REVERSE    = A_REVERSE,
	UI_ATTR_BOLD       = A_BOLD,
};

Ui *ui_curses_new(Color *colors);
void ui_curses_free(Ui*);

#endif
