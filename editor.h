#ifndef EDITOR_H
#define EDITOR_H

#include <curses.h>
#include <stddef.h>
#include <stdbool.h>
#include "window.h"
#include "register.h"
#include "macro.h"
#include "syntax.h"
#include "ring-buffer.h"

typedef struct Editor Editor;
typedef struct EditorWin EditorWin;

typedef struct {
	size_t index;           /* #number of changes */
	size_t pos;             /* where the current change occured */
} ChangeList;

struct EditorWin {
	Editor *editor;         /* editor instance to which this window belongs */
	Text *text;             /* underlying text management */
	Win *win;               /* window for the text area  */
	RingBuffer *jumplist;   /* LRU jump management */
	ChangeList changelist;  /* state for iterating through least recently changes */
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
	MARK_SELECTION_START,
	MARK_SELECTION_END,
};

struct Editor {
	int width, height;                /* terminal size, available for all windows */
	EditorWin *windows;               /* list of windows */
	EditorWin *win;                   /* currently active window */
	Syntax *syntaxes;                 /* NULL terminated array of syntax definitions */
	Register registers[REG_LAST];     /* register used for copy and paste */
	Macro macros[26];                 /* recorded macros */
	Macro *recording, *last_recording;/* currently and least recently recorded macro */
	Prompt *prompt;                   /* used to get user input */
	char info[255];                   /* a user message currently being displayed */
	Regex *search_pattern;            /* last used search pattern */
	void (*windows_arrange)(Editor*); /* current layout which places the windows */
	void (*statusbar)(EditorWin*);    /* configurable user hook to draw statusbar */
	int tabwidth;                     /* how many spaces should be used to display a tab */
	bool expandtab;                   /* whether typed tabs should be converted to spaces */
	bool autoindent;                  /* whether indentation should be copied from previous line on newline */
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

/* set tabwidth (must be in range [1, 8], affects all windows */
void editor_tabwidth_set(Editor*, int tabwidth);
int editor_tabwidth_get(Editor*);

/* load a set of syntax highlighting definitions which will be associated
 * to the underlying window based on the file type loaded.
 *
 * both *syntaxes and *colors must point to a NULL terminated arrays.
 */
bool editor_syntax_load(Editor*, Syntax *syntaxes, Color *colors);
void editor_syntax_unload(Editor*);

/* creates a new window, and loads the given file. if filename is NULL
 * an unamed / empty buffer is created. If the given file is already opened
 * in another window, share the underlying text that is changes will be
 * visible in both windows */
bool editor_window_new(Editor*, const char *filename);
bool editor_window_new_fd(Editor*, int fd);
/* reload the file currently displayed in the window from disk */
bool editor_window_reload(EditorWin*);
void editor_window_close(EditorWin*);
/* split the given window. changes to the displayed text will be reflected
 * in both windows */
bool editor_window_split(EditorWin*);
/* focus the next / previous window */
void editor_window_next(Editor*);
void editor_window_prev(Editor*);

void editor_window_jumplist_add(EditorWin*, size_t pos);
size_t editor_window_jumplist_prev(EditorWin*);
size_t editor_window_jumplist_next(EditorWin*);
void editor_window_jumplist_invalidate(EditorWin*);

size_t editor_window_changelist_prev(EditorWin*);
size_t editor_window_changelist_next(EditorWin*);
/* rearrange all windows either vertically or horizontally */
void editor_windows_arrange_vertical(Editor*);
void editor_windows_arrange_horizontal(Editor*);
/* display a user prompt with a certain title and default text */
void editor_prompt_show(Editor*, const char *title, const char *text);
/* hide the user prompt if it is currently shown */
void editor_prompt_hide(Editor*);
/* return the content of the command prompt in a malloc(3)-ed string
 * which the call site has to free. */
char *editor_prompt_get(Editor*);
/* replace the current command line content with the one given */
void editor_prompt_set(Editor*, const char *line);

/* display a message to the user */
void editor_info_show(Editor*, const char *msg, ...);
void editor_info_hide(Editor*);

/* register a callback which is called whenever the statusbar needs to
 * be redrawn */
void editor_statusbar_set(Editor*, void (*statusbar)(EditorWin*));

/* look up a curses color pair for the given combination of fore and
 * background color */
short editor_color_get(short fg, short bg);

#endif
