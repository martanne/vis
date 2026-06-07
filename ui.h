#ifndef UI_H
#define UI_H

/* enable large file optimization for files larger than: */
#define UI_LARGE_FILE_SIZE (1 << 25)
/* enable large file optimization for files containing lines longer than: */
#define UI_LARGE_FILE_LINE_SIZE (1 << 16)

#define UI_MAX_WIDTH  1024
#define UI_MAX_HEIGHT 1024

enum UiLayout {
	UI_LAYOUT_HORIZONTAL,
	UI_LAYOUT_VERTICAL,
	UI_LAYOUT_COUNT,
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

typedef enum {
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
	UI_STYLE_WHITESPACE,
	UI_STYLE_LAST = UI_STYLE_WHITESPACE,
	/* NOTE: user/lexer styles */
	UI_STYLE_MAX = 512,
} VisUiStyle;

typedef enum {
	VisCellAttribute_Normal    = 0,
	VisCellAttribute_Underline = 1u << 0,
	VisCellAttribute_Reverse   = 1u << 1,
	VisCellAttribute_Blink     = 1u << 2,
	VisCellAttribute_Bold      = 1u << 3,
	VisCellAttribute_Italic    = 1u << 4,
	VisCellAttribute_Dim       = 1u << 5,
} VisCellAttribute;

typedef enum {
	VisCellProperty_None          = 0,
	VisCellProperty_FGSet         = 1u << 0, // user provided FG (if not keep cell's existing FG)
	VisCellProperty_BGSet         = 1u << 1, // user provided BG (if not keep cell's existing BG)
	VisCellProperty_IndexedFG     = 1u << 2, // FG is a color table index
	VisCellProperty_IndexedBG     = 1u << 3, // BG is a color table index
	VisCellProperty_KeepAttribute = 1u << 4, // or attribute onto cell's existing attribute
} VisCellProperties;

// NOTE(rnp): FG/BG may be indexed, as indicated by the cell properties. For
// alignment they are not defined as unions. The FG index is stored in the
// fg_r and fg_g u8s and the BG index is strored in the bg_g and bg_b u8s.
// Use the VisCellStyleBGIndex*() and VisCellStyleFGIndex*() macros to extract.
typedef struct {
	u8 attributes; // VisCellAttributes
	u8 properties; // VisCellProperties
	u8 fg_r, fg_g, fg_b;
	u8 bg_r, bg_g, bg_b;
} VisCellStyle;
#define VisCellStyleFGIndexGet(v)        (((u16)(v)->fg_r << 8u) | (v)->fg_g)
#define VisCellStyleBGIndexGet(v)        (((u16)(v)->bg_g << 8u) | (v)->bg_b)
#define VisCellStyleFGIndexSet(v, index) ((v)->fg_r = ((index >> 8u) & 0xFFu), ((v)->fg_g = (index) & 0xFFu))
#define VisCellStyleBGIndexSet(v, index) ((v)->bg_g = ((index >> 8u) & 0xFFu), ((v)->bg_b = (index) & 0xFFu))

// TODO(rnp): refactor: should be possible to pack Cell down to 16 bytes; check how tab expansion is working
typedef struct {
	char data[16];      /* utf8 encoded character displayed in this cell (might be more than
	                       one Unicode codepoint. might also not be the same as in the
	                       underlying text, for example tabs get expanded */
	u32 len;            /* number of bytes the character displayed in this cell uses, for
	                       characters which use more than 1 column to display, their length
	                       is stored in the leftmost cell whereas all following cells
	                       occupied by the same character have a length of 0. */
	int width;          /* display width i.e. number of columns occupied by this character */
	VisCellStyle style; /* colors and attributes used to display this cell */
} Cell;

typedef struct {
	Cell *cells;
	u64   size;
} VisCellBuffer;

typedef struct {
	s16  *palette;
	s16   color_pairs_max;
	s16   color_pair_current;
	s16   default_fg;
	s16   default_bg;
	s8    change_colors;
} VisCursesUI;

typedef struct {
	// NOTE(rnp): cell front buffer, updated on draw to match Ui::cell_buffer back buffer
	VisCellBuffer cell_buffer;
	Buffer output_buffer;

	/* Indicates that the terminal contents may have changed externally */
	bool flush_terminal;
} VisVT100UI;

// TODO(rnp): flatten UI into vis, only one exists and it must be in a vis context
typedef struct {
	char info[UI_MAX_WIDTH];   /* info message displayed at the bottom of the screen */
	TermKey termkey;           /* libtermkey instance to handle keyboard input (stdin or /dev/tty) */
	VisCellBuffer cell_buffer; /* 2D grid of cells, at least as large as current terminal size */
	int width, height;         /* terminal dimensions available for all windows */
	int cur_row, cur_col;      /* active cursor's (0-based) position on the terminal */
	enum UiLayout layout;      /* whether windows are displayed horizontally or vertically */
	// TODO(rnp): cleanup usage of this
	bool doupdate;             /* Whether to update the screen after refreshing contents */

	str8 term;                 /* selected value for TERM (0 terminated) */

#if CONFIG_CURSES
	VisCursesUI curses;
#else
	VisVT100UI  vt100;
#endif

	// static_assert(U16_MAX <= UI_STYLE_MAX)
	u16          style_count;  /* count of styles currently in use */
	VisCellStyle styles[UI_STYLE_MAX];
} Ui;

#include "view.h"
#include "vis.h"
#include "text.h"

VIS_INTERNAL int  ui_terminal_colors(void);
VIS_INTERNAL void ui_terminal_free(Ui*);
VIS_INTERNAL void ui_terminal_restore(Ui*);
VIS_INTERNAL void ui_terminal_resume(Ui*);
VIS_INTERNAL void ui_terminal_save(Ui*, bool fscr);
VIS_INTERNAL void ui_terminal_suspend(Ui*);

VIS_INTERNAL __attribute__((noreturn)) void ui_die(Ui *, const char *, va_list);
VIS_INTERNAL bool ui_init(Ui *);
VIS_INTERNAL void ui_arrange(Vis *, enum UiLayout);
VIS_INTERNAL void ui_draw(Vis *);
VIS_INTERNAL void ui_info_hide(Ui *);
VIS_INTERNAL void ui_info_show(Ui *, const char *, va_list);
VIS_INTERNAL void ui_resize(Ui*);

VIS_INTERNAL bool ui_window_init(Ui *, Win *, enum UiOption);

VIS_INTERNAL bool vis_ui_getkey(Vis *, TermKeyKey *);

VIS_INTERNAL u16  vis_ui_style_push(Vis *);
VIS_INTERNAL bool vis_ui_style_define(Vis *, u16 style_id, str8 style);
VIS_INTERNAL void vis_ui_window_style_set(Ui *ui, Cell *cell, u16 style_id);
VIS_INTERNAL bool vis_ui_window_style_set_pos(Win *win, int x, int y, u16 style_id);

VIS_INTERNAL void ui_window_options_set(Win *win, enum UiOption options);
VIS_INTERNAL void ui_window_status(Vis *vis, Win *win, const char *status);

#endif
