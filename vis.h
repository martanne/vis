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

struct Vis {
	int width, height;             /* terminal size, available for all windows */
	VisWin *windows;               /* list of windows */
	VisWin *win;                   /* currently active window */
	Syntax *syntaxes;              /* NULL terminated array of syntax definitions */
	Register registers[REG_LAST];
	void (*windows_arrange)(Vis*); /* current layout which places the windows */
	vis_statusbar_t statusbar;     /* configurable user hook to draw statusbar */
};

typedef union {
	bool b;
	size_t i;
	const char *s;
	size_t (*m)(Win*);
	void (*f)(Vis*);
} Arg;

typedef struct {
	char str[6];
	int code;
} Key;

typedef struct {
	Key key[2];
	void (*func)(const Arg *arg);
	const Arg arg;
} KeyBinding;

typedef struct Mode Mode;
struct Mode {
	Mode *parent;
	KeyBinding *bindings;
	const char *name;
	void (*enter)(void);
	void (*leave)(void);
	bool (*unknown)(Key *key0, Key *key1);        /* unknown key for this mode, return value determines whether parent modes will be checked */ 
	bool (*input)(const char *str, size_t len);   /* unknown key for this an all parent modes */
};

typedef struct {
	char *name;
	Mode *mode;
	vis_statusbar_t statusbar;
} Config;

typedef struct {
	int count;
	Register *reg;
	Filerange range;
	size_t pos;
} OperatorContext;

typedef struct {
	void (*func)(OperatorContext*); /* function implementing the operator logic */
	bool selfcontained;             /* is this operator followed by movements/text-objects */
} Operator;

typedef struct {
	size_t (*win)(Win*);
	size_t (*txt)(Text*, size_t pos);
	enum {
		LINEWISE  = 1 << 0,
		CHARWISE  = 1 << 1,
		INCLUSIVE = 1 << 2,
		EXCLUSIVE = 1 << 3,
	} type;
	int count;
} Movement;

typedef struct {
	Filerange (*range)(Text*, size_t pos);
	enum {
		INNER,
		OUTER,
	} type;
} TextObject;

typedef struct {
	int count;
	bool linewise;
	Operator *op;
	Movement *movement;
	TextObject *textobj;
	Register *reg;
} Action;

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
void vis_snapshot(Vis*);
void vis_undo(Vis*);
void vis_redo(Vis*);
void vis_draw(Vis*);
void vis_update(Vis*);
void vis_insert_key(Vis*, const char *c, size_t len);
void vis_replace_key(Vis*, const char *c, size_t len);
void vis_backspace_key(Vis*);
void vis_delete_key(Vis*);
void vis_insert(Vis*, size_t pos, const char *data, size_t len);
void vis_delete(Vis*, size_t pos, size_t len);

// mark handling
typedef int Mark;
void vis_mark_set(Vis*, Mark, size_t pos);
void vis_mark_goto(Vis*, Mark);
void vis_mark_clear(Vis*, Mark);

// TODO comment
bool vis_syntax_load(Vis*, Syntax *syntaxes, Color *colors);
void vis_syntax_unload(Vis*);

void vis_search(Vis *ed, const char *regex, int direction);
size_t vis_line_goto(Vis *vis, size_t lineno);

bool vis_window_new(Vis *vis, const char *filename);
void vis_window_split(Vis *ed, const char *filename);
void vis_window_vsplit(Vis *ed, const char *filename);
void vis_window_next(Vis *ed);
void vis_window_prev(Vis *ed);

void vis_statusbar_set(Vis*, vis_statusbar_t);

/* library initialization code, should be run at startup */
void vis_init(void);
short vis_color_reserve(short fg, short bg);
short vis_color_get(short fg, short bg);

#endif
