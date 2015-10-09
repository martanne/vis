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
	UI_OPTION_LINE_NUMBERS_NONE = 0,
	UI_OPTION_LINE_NUMBERS_ABSOLUTE = 1 << 0,
	UI_OPTION_LINE_NUMBERS_RELATIVE = 1 << 1,
};

#define UI_STYLES_MAX 64

#include "text.h"
#include "view.h"
#include "editor.h"

struct Ui {
	bool (*init)(Ui*, Editor*);
	void (*free)(Ui*);
	void (*resize)(Ui*);
	UiWin* (*window_new)(Ui*, View*, File*);
	void (*window_free)(UiWin*);
	void (*window_focus)(UiWin*);
	UiWin* (*prompt_new)(Ui*, View*, File*);
	void (*prompt)(Ui*, const char *title, const char *value);
	char* (*prompt_input)(Ui*);
	void (*prompt_hide)(Ui*);
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
	void (*draw_text)(UiWin*, const Line*);
	void (*draw_status)(UiWin*);
	void (*reload)(UiWin*, File*);
	void (*options)(UiWin*, enum UiOption);
	bool (*syntax_style)(UiWin*, int id, const char *style);
};

#endif
