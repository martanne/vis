#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stdarg.h>
#include <termkey.h>

/* enable large file optimization for files larger than: */
#define UI_LARGE_FILE_SIZE (1 << 25)
/* enable large file optimization for files containing lines longer than: */
#define UI_LARGE_FILE_LINE_SIZE (1 << 16)

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

enum UiStyle {
	UI_STYLE_LEXER_MAX = 64,
	UI_STYLE_DEFAULT,
	UI_STYLE_CURSOR,
	UI_STYLE_CURSOR_PRIMARY,
	UI_STYLE_CURSOR_LINE,
	UI_STYLE_SELECTION,
	UI_STYLE_LINENUMBER,
	UI_STYLE_LINENUMBER_CURSOR,
	UI_STYLE_COLOR_COLUMN,
	UI_STYLE_STATUS,
	UI_STYLE_STATUS_FOCUSED,
	UI_STYLE_SEPARATOR,
	UI_STYLE_INFO,
	UI_STYLE_EOF,
	UI_STYLE_MAX,
};

#if CONFIG_CURSES
typedef uint64_t CellAttr;
typedef short CellColor;

static inline bool cell_color_equal(CellColor c1, CellColor c2) {
	return c1 == c2;
}
#else
typedef uint8_t CellAttr;
typedef struct {
	uint8_t r, g, b;
	uint8_t index;
} CellColor;

static inline bool cell_color_equal(CellColor c1, CellColor c2) {
	if (c1.index != (uint8_t)-1 || c2.index != (uint8_t)-1)
		return c1.index == c2.index;
	return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

#endif

typedef struct {
	CellAttr attr;
	CellColor fg, bg;
} CellStyle;

#include "vis.h"
#include "text.h"
#include "view.h"

struct Ui {
	bool (*init)(Ui*, Vis*);
	void (*free)(Ui*);
	void (*resize)(Ui*);
	UiWin* (*window_new)(Ui*, Win*, enum UiOption);
	void (*window_free)(UiWin*);
	void (*window_focus)(UiWin*);
	void (*window_swap)(UiWin*, UiWin*);
	void (*die)(Ui*, const char *msg, va_list ap) __attribute__((noreturn));
	void (*info)(Ui*, const char *msg, va_list ap);
	void (*info_hide)(Ui*);
	void (*arrange)(Ui*, enum UiLayout);
	void (*draw)(Ui*);
	void (*redraw)(Ui*);
	void (*suspend)(Ui*);
	void (*resume)(Ui*);
	bool (*getkey)(Ui*, TermKeyKey*);
	void (*terminal_save)(Ui*);
	void (*terminal_restore)(Ui*);
	TermKey* (*termkey_get)(Ui*);
	int (*colors)(Ui*);
};

struct UiWin {
	CellStyle (*style_get)(UiWin*, enum UiStyle);
	void (*status)(UiWin*, const char *txt);
	void (*options_set)(UiWin*, enum UiOption);
	enum UiOption (*options_get)(UiWin*);
	bool (*style_define)(UiWin*, int id, const char *style);
	int (*window_width)(UiWin*);
	int (*window_height)(UiWin*);
};

bool is_default_color(CellColor c);

#endif
