#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stdarg.h>
#include <termkey.h>

typedef struct Ui Ui;
typedef struct UiWin UiWin;

enum UiLayout {
	UI_LAYOUT_HORIZONTAL,
	UI_LAYOUT_VERTICAL,
};

enum UiOption {
	UI_OPTION_NONE = 0,
	UI_OPTION_LINE_NUMBERS_ABSOLUTE = 1 << 0,
	UI_OPTION_LINE_NUMBERS_RELATIVE = 1 << 1,
	UI_OPTION_SYMBOL_SPACE = 1 << 2,
	UI_OPTION_SYMBOL_TAB = 1 << 3,
	UI_OPTION_SYMBOL_TAB_FILL = 1 << 4,
	UI_OPTION_SYMBOL_EOL = 1 << 5,
	UI_OPTION_SYMBOL_EOF = 1 << 6,
	UI_OPTION_CURSOR_LINE = 1 << 7,
	UI_OPTION_STATUSBAR = 1 << 8,
	UI_OPTION_ONELINE = 1 << 9,
	UI_OPTION_LARGE_FILE = 1 << 10,
};

enum UiStyles {
	UI_STYLE_LEXER_MAX = 64,
	UI_STYLE_DEFAULT,
	UI_STYLE_CURSOR,
	UI_STYLE_CURSOR_PRIMARY,
	UI_STYLE_CURSOR_LINE,
	UI_STYLE_SELECTION,
	UI_STYLE_LINENUMBER,
	UI_STYLE_COLOR_COLUMN,
	UI_STYLE_MAX,
};

#include "vis.h"
#include "text.h"
#include "view.h"

struct Ui {
	bool (*init)(Ui*, Vis*);
	bool (*start)(Ui*);
	void (*free)(Ui*);
	void (*resize)(Ui*);
	UiWin* (*window_new)(Ui*, View*, File*, enum UiOption);
	void (*window_free)(UiWin*);
	void (*window_focus)(UiWin*);
	void (*die)(Ui*, const char *msg, va_list ap) __attribute__((noreturn));
	void (*info)(Ui*, const char *msg, va_list ap);
	void (*info_hide)(Ui*);
	void (*arrange)(Ui*, enum UiLayout);
	void (*draw)(Ui*);
	void (*redraw)(Ui*);
	void (*update)(Ui*);
	void (*suspend)(Ui*);
	const char* (*getkey)(Ui*);
	bool (*haskey)(Ui*);
	void (*terminal_save)(Ui*);
	void (*terminal_restore)(Ui*);
	TermKey* (*termkey_get)(Ui*);
};

struct UiWin {
	void (*draw)(UiWin*);
	void (*draw_status)(UiWin*);
	void (*reload)(UiWin*, File*);
	void (*options_set)(UiWin*, enum UiOption);
	enum UiOption (*options_get)(UiWin*);
	bool (*syntax_style)(UiWin*, int id, const char *style);
};

#endif
