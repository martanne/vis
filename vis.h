#ifndef VIS_H
#define VIS_H

#include <stddef.h>
#include <regex.h>
#include "text-motions.h"
#include "text-objects.h"
#include "window.h"
#include "register.h"

typedef struct Vis Vis;
typedef struct VisWin VisWin;

struct VisWin {
	Vis *vis;            /* editor instance to which this window belongs */
	Text *text;          /* underlying text management */
	Win *win;            /* vis window for the text area  */
	WINDOW *statuswin;   /* curses window for the statusbar */
	int width, height;   /* window size including the statusbar */
	VisWin *prev, *next; /* neighbouring windows */
};

typedef struct {
	VisWin *win;
	VisWin *editor; /* active editor window before prompt is shown */
	char *title;
	WINDOW *titlewin;
	bool active;
} Prompt;

typedef void (*vis_statusbar_t)(WINDOW *win, bool active, const char *filename, size_t line, size_t col);

enum Reg {
	REG_a,
	REG_b,
	REG_c,
	// ...
	REG_z,
	REG_DEFAULT,
	REG_LAST,
};

enum Mark {
	MARK_a,
	MARK_b,
	MARK_c,
	// ...
	MARK_z,
	MARK_LAST,
};

struct Vis {
	int width, height;             /* terminal size, available for all windows */
	VisWin *windows;               /* list of windows */
	VisWin *win;                   /* currently active window */
	Syntax *syntaxes;              /* NULL terminated array of syntax definitions */
	Register registers[REG_LAST];
	Prompt *prompt;
	Regex *search_pattern;
	void (*windows_arrange)(Vis*); /* current layout which places the windows */
	vis_statusbar_t statusbar;     /* configurable user hook to draw statusbar */
};


typedef struct {
	short fg, bg;   /* fore and background color */
	int attr;       /* curses attributes */
} Color;

typedef struct {
	char *rule;     /* regex to search for */
	int cflags;     /* compilation flags (REG_*) used when compiling */
	Color color;    /* settings to apply in case of a match */
	regex_t regex;  /* compiled form of the above rule */
} SyntaxRule;

#define SYNTAX_REGEX_RULES 10

typedef struct Syntax Syntax;

struct Syntax {                              /* a syntax definition */
	char *name;                           /* syntax name */
	char *file;                           /* apply to files matching this regex */
	regex_t file_regex;                   /* compiled file name regex */
	SyntaxRule rules[SYNTAX_REGEX_RULES]; /* all rules for this file type */
};

Vis *vis_new(int width, int height);
void vis_free(Vis*); 
void vis_resize(Vis*, int width, int height); 
void vis_draw(Vis*);
void vis_update(Vis*);
void vis_insert_key(Vis*, const char *c, size_t len);
void vis_replace_key(Vis*, const char *c, size_t len);
void vis_backspace_key(Vis*);
void vis_delete_key(Vis*);
void vis_insert(Vis*, size_t pos, const char *data, size_t len);
void vis_delete(Vis*, size_t pos, size_t len);

// TODO comment
bool vis_syntax_load(Vis*, Syntax *syntaxes, Color *colors);
void vis_syntax_unload(Vis*);

bool vis_window_new(Vis*, const char *filename);
void vis_window_split(Vis*, const char *filename);
void vis_window_vsplit(Vis*, const char *filename);
void vis_window_next(Vis*);
void vis_window_prev(Vis*);

char *vis_prompt_get(Vis *vis);
void vis_prompt_set(Vis *vis, const char *line);
void vis_prompt_show(Vis *vis, const char *title);
void vis_prompt_hide(Vis *vis);


void vis_statusbar_set(Vis*, vis_statusbar_t);

/* library initialization code, should be run at startup */
void vis_init(void);
short vis_color_reserve(short fg, short bg);
short vis_color_get(short fg, short bg);

#endif
