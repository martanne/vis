#ifndef UI_CURSES_H
#define UI_CURSES_H

#include <curses.h>
#include "ui.h"
#include "syntax.h"

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
