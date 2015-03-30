#ifndef UI_CURSES_H
#define UI_CURSES_H

#include "ui.h"

Ui *ui_curses_new(void);
void ui_curses_free(Ui*);

#endif
