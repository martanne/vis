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
	void (*statusbar)(EditorWin*);    /* configurable user hook to draw statusbar */
};

Editor *editor_new(int width, int height);
void editor_free(Editor*);
void editor_resize(Editor*, int width, int height);
void editor_draw(Editor*);
void editor_update(Editor*);

/* these function operate on the currently focused window but make sure
 * that all windows which show the affected region are redrawn too. */
void editor_insert_key(Editor*, const char *c, size_t len);
void editor_replace_key(Editor*, const char *c, size_t len);
void editor_backspace_key(Editor*);
void editor_delete_key(Editor*);
void editor_insert(Editor*, size_t pos, const char *data, size_t len);
void editor_delete(Editor*, size_t pos, size_t len);

/* load a set of syntax highlighting definitions which will be associated
 * to the underlying window based on the file type loaded.
 *
 * both *syntaxes and *colors must point to a NULL terminated array.
 * it the i-th syntax definition does not specifiy a fore ground color
 * then the i-th entry of the color array will be used instead
 */
bool editor_syntax_load(Editor*, Syntax *syntaxes, Color *colors);
void editor_syntax_unload(Editor*);

/* creates a new window, and loads the given file. if filename is NULL
 * an unamed / empty buffer is created */
bool editor_window_new(Editor*, const char *filename);
/* reload the file currently displayed in the window from disk */
bool editor_window_reload(EditorWin*);
void editor_window_close(EditorWin*);
/* if filename is non NULL it is equivalent to window_new call above.
 * if however filename is NULL a new window is created and linked to the
 * same underlying text as the currently selected one. changes will
 * thus be visible in both windows. */
void editor_window_split(Editor*, const char *filename);
void editor_window_vsplit(Editor*, const char *filename);
/* focus the next / previous window */
void editor_window_next(Editor*);
void editor_window_prev(Editor*);
/* return the content of the command prompt in a malloc(3)-ed string
 * which the call site has to free. */
char *editor_prompt_get(Editor *vis);
/* replace the current command line content with the one given */
void editor_prompt_set(Editor *vis, const char *line);
void editor_prompt_show(Editor *vis, const char *title);
void editor_prompt_hide(Editor *vis);

void editor_statusbar_set(Editor*, void (*statusbar)(EditorWin*));

/* look up a curses color pair for the given combination of fore and
 * background color */
short editor_color_get(short fg, short bg);

#endif
