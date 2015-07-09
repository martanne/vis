#ifndef UI_CURSES_H
#define UI_CURSES_H

#include <curses.h>
#include "ui.h"

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
	UI_KEY_ENTER       = KEY_ENTER,
	UI_KEY_END         = KEY_END,
	UI_KEY_SHIFT_LEFT  = KEY_SLEFT,
	UI_KEY_SHIFT_RIGHT = KEY_SRIGHT,
};

Ui *ui_curses_new(void);
void ui_curses_free(Ui*);

#endif
