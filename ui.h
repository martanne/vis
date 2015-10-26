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
	UI_OPTION_LINE_NUMBERS_ABSOLUTE = 1 << 0,
	UI_OPTION_LINE_NUMBERS_RELATIVE = 1 << 1,
	UI_OPTION_SYMBOL_SPACE = 1 << 2,
	UI_OPTION_SYMBOL_TAB = 1 << 3,
	UI_OPTION_SYMBOL_TAB_FILL = 1 << 4,
	UI_OPTION_SYMBOL_EOL = 1 << 5,
	UI_OPTION_SYMBOL_EOF = 1 << 6,
};

#define UI_STYLES_MAX 64

#include "text.h"
#include "view.h"
#include "vis.h"

struct Ui {
	bool (*init)(Ui*, Vis*);
	void (*free)(Ui*);
	void (*resize)(Ui*);
	UiWin* (*window_new)(Ui*, View*, File*);
	void (*window_free)(UiWin*);
	void (*window_focus)(UiWin*);
	UiWin* (*prompt_new)(Ui*, View*, File*);
	void (*prompt)(Ui*, const char *title, const char *value);
	char* (*prompt_input)(Ui*);
	void (*prompt_hide)(Ui*);
	void (*die)(Ui*, const char *msg, va_list ap);
	void (*info)(Ui*, const char *msg, va_list ap);
	void (*info_hide)(Ui*);
	void (*arrange)(Ui*, enum UiLayout);
	void (*draw)(Ui*);
	void (*update)(Ui*);
	void (*suspend)(Ui*);
	void (*resume)(Ui*);
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
