#ifndef EDITOR_H
#define EDITOR_H

#include <curses.h>
#include <stddef.h>
#include <stdbool.h>
#include "window.h"
#include "register.h"
#include "syntax.h"

typedef struct Editor Editor;
typedef struct EditorWin EditorWin;

struct EditorWin {
	Editor *editor;         /* editor instance to which this window belongs */
	Text *text;             /* underlying text management */
	Win *win;               /* vis window for the text area  */
	WINDOW *statuswin;      /* curses window for the statusbar */
	int width, height;      /* window size including the statusbar */
	EditorWin *prev, *next; /* neighbouring windows */
};

typedef struct {
	EditorWin *win;         /* 1-line height editor window used for the prompt */
	EditorWin *editor;      /* active editor window before prompt is shown */
	char *title;            /* title displayed to the left of the prompt */
	WINDOW *titlewin;       /* the curses window holding the prompt title */
	bool active;            /* whether the prompt is currently shown or not */
} Prompt;

typedef void (*editor_statusbar_t)(WINDOW *win, bool active, const char *filename, size_t line, size_t col);

enum Reg {
	REG_a,
	REG_b,
	REG_c,
	REG_d,
	REG_e,
	REG_f,
	REG_g,
	REG_h,
	REG_i,
	REG_j,
	REG_k,
	REG_l,
	REG_m,
	REG_n,
	REG_o,
	REG_p,
	REG_q,
	REG_r,
	REG_s,
	REG_t,
	REG_u,
	REG_v,
	REG_w,
	REG_x,
	REG_y,
	REG_z,
	REG_DEFAULT,
	REG_LAST,
};

enum Mark {
	MARK_a,
	MARK_b,
	MARK_c,
	MARK_d,
	MARK_e,
	MARK_f,
	MARK_g,
	MARK_h,
	MARK_i,
	MARK_j,
	MARK_k,
	MARK_l,
	MARK_m,
	MARK_n,
	MARK_o,
	MARK_p,
	MARK_q,
	MARK_r,
	MARK_s,
	MARK_t,
	MARK_u,
	MARK_v,
	MARK_w,
	MARK_x,
	MARK_y,
	MARK_z,
};

struct Editor {
	int width, height;                /* terminal size, available for all windows */
	EditorWin *windows;               /* list of windows */
	EditorWin *win;                   /* currently active window */
	Syntax *syntaxes;                 /* NULL terminated array of syntax definitions */
	Register registers[REG_LAST];     /* register used for copy and paste */
	Prompt *prompt;                   /* used to get user input */
	Regex *search_pattern;            /* last used search pattern */
	void (*windows_arrange)(Editor*); /* current layout which places the windows */
	editor_statusbar_t statusbar;     /* configurable user hook to draw statusbar */
	bool running;                     /* (TODO move elsewhere?) */
};

Editor *editor_new(int width, int height);
void editor_free(Editor*);
void editor_resize(Editor*, int width, int height);
void editor_draw(Editor*);
void editor_update(Editor*);
void editor_insert_key(Editor*, const char *c, size_t len);
void editor_replace_key(Editor*, const char *c, size_t len);
void editor_backspace_key(Editor*);
void editor_delete_key(Editor*);
void editor_insert(Editor*, size_t pos, const char *data, size_t len);
void editor_delete(Editor*, size_t pos, size_t len);

// TODO comment
bool editor_syntax_load(Editor*, Syntax *syntaxes, Color *colors);
void editor_syntax_unload(Editor*);

bool editor_window_new(Editor*, const char *filename);
void editor_window_close(Editor *vis);
void editor_window_split(Editor*, const char *filename);
void editor_window_vsplit(Editor*, const char *filename);
void editor_window_next(Editor*);
void editor_window_prev(Editor*);

char *editor_prompt_get(Editor *vis);
void editor_prompt_set(Editor *vis, const char *line);
void editor_prompt_show(Editor *vis, const char *title);
void editor_prompt_hide(Editor *vis);


void editor_statusbar_set(Editor*, editor_statusbar_t);

/* library initialization code, should be run at startup */
void editor_init(void);
short editor_color_reserve(short fg, short bg);
short editor_color_get(short fg, short bg);

#endif
