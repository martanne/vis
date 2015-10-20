/*
 * Copyright (c) 2014 Marc André Tanner <mat at brain-dump.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <regex.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pwd.h>
#include <lauxlib.h>
#include <lualib.h>
#include <lua.h>

#include <termkey.h>
#include "vis.h"
#include "text-util.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-regex.h"
#include "util.h"
#include "map.h"
#include "ring-buffer.h"
#include "macro.h"


/* a mode contains a set of key bindings which are currently valid.
 *
 * each mode can specify one parent mode which is consultated if a given key
 * is not found in the current mode. hence the modes form a tree which is
 * searched from the current mode up towards the root mode until a valid binding
 * is found.
 *
 * if no binding is found, mode->input(...) is called and the user entered
 * keys are passed as argument. this is used to change the document content.
 */
typedef struct Mode Mode;
struct Mode {
	Mode *parent;                       /* if no match is found in this mode, search will continue there */
	Map *bindings;                      
	const char *name;                   /* descriptive, user facing name of the mode */
	const char *status;                 /* name displayed in the window status bar */
	const char *help;                   /* short description used by :help */
	bool isuser;                        /* whether this is a user or internal mode */
	void (*enter)(Vis*, Mode *old);           /* called right before the mode becomes active */
	void (*leave)(Vis*, Mode *new);           /* called right before the mode becomes inactive */
	void (*input)(Vis*, const char*, size_t); /* called whenever a key is not found in this mode and all its parent modes */
	void (*idle)(Vis*);                 /* called whenever a certain idle time i.e. without any user input elapsed */
	time_t idle_timeout;                /* idle time in seconds after which the registered function will be called */
	bool visual;                        /* whether text selection is possible in this mode */
};



typedef struct {
	int count;        /* how many times should the command be executed? */
	Register *reg;    /* always non-NULL, set to a default register */
	Filerange range;  /* which part of the file should be affected by the operator */
	size_t pos;       /* at which byte from the start of the file should the operation start? */
	bool linewise;    /* should the changes always affect whole lines? */
	const Arg *arg;   /* arbitrary arguments */
} OperatorContext;

typedef struct {
	size_t (*func)(Vis*, Text*, OperatorContext*); /* operator logic, returns new cursor position */
} Operator;

typedef struct {
	/* TODO: merge types / use union to save space */
	size_t (*cur)(Cursor*);            /* a movement based on current window content from view.h */
	size_t (*txt)(Text*, size_t pos); /* a movement form text-motions.h */
	size_t (*file)(Vis*, File*, size_t pos);
	size_t (*vis)(Vis*, Text*, size_t pos);
	size_t (*view)(Vis*, View*);
	size_t (*win)(Vis*, Win*, size_t pos);
	enum {
		LINEWISE  = 1 << 0,
		CHARWISE  = 1 << 1,
		INCLUSIVE = 1 << 2,
		EXCLUSIVE = 1 << 3,
		IDEMPOTENT = 1 << 4,
		JUMP = 1 << 5,
	} type;
	int count;
} Movement;

typedef struct {
	Filerange (*range)(Text*, size_t pos); /* a text object from text-objects.h */
	enum {
		INNER,
		OUTER,
	} type;
} TextObject;

typedef struct {             /** collects all information until an operator is executed */
	int count;
	enum VisMotionType type;
	const Operator *op;
	const Movement *movement;
	const TextObject *textobj;
	Register *reg;
	enum VisMark mark;
	Arg arg;
} Action;

struct File {
	Text *text;
	const char *name;
	volatile sig_atomic_t truncated;
	bool is_stdin;
	struct stat stat;
	int refcount;
	Mark marks[VIS_MARK_INVALID];
	File *next, *prev;
};

typedef struct {
	time_t state;           /* state of the text, used to invalidate change list */
	size_t index;           /* #number of changes */
	size_t pos;             /* where the current change occured */
} ChangeList;

struct Win {
	Vis *editor;         /* editor instance to which this window belongs */
	UiWin *ui;
	File *file;             /* file being displayed in this window */
	View *view;             /* currently displayed part of underlying text */
	ViewEvent events;
	RingBuffer *jumplist;   /* LRU jump management */
	ChangeList changelist;  /* state for iterating through least recently changes */
	Win *prev, *next;       /* neighbouring windows */
};

struct Vis {
	Ui *ui;
	File *files;
	Win *windows;                     /* list of windows */
	Win *win;                         /* currently active window */
	Register registers[VIS_REGISTER_INVALID];     /* register used for copy and paste */
	Macro macros[VIS_MACRO_INVALID];         /* recorded macros */
	Macro *recording, *last_recording;/* currently and least recently recorded macro */
	Win *prompt;                      /* 1-line height window to get user input */
	Win *prompt_window;               /* window which was focused before prompt was shown */
	char prompt_type;                 /* command ':' or search '/','?' prompt */
	Regex *search_pattern;            /* last used search pattern */
	char search_char[8];              /* last used character to search for via 'f', 'F', 't', 'T' */
	int last_totill;                  /* last to/till movement used for ';' and ',' */
	int tabwidth;                     /* how many spaces should be used to display a tab */
	bool expandtab;                   /* whether typed tabs should be converted to spaces */
	bool autoindent;                  /* whether indentation should be copied from previous line on newline */
	Map *cmds;                        /* ":"-commands, used for unique prefix queries */
	Map *options;                     /* ":set"-options */
	Buffer buffer_repeat;             /* holds data to repeat last insertion/replacement */
	Buffer input_queue;               /* holds pending input keys */
	
	Action action;       /* current action which is in progress */
	Action action_prev;  /* last operator action used by the repeat '.' key */
	Mode *mode;          /* currently active mode, used to search for keybindings */
	Mode *mode_prev;     /* previsouly active user mode */
	Mode *mode_before_prompt; /* user mode which was active before entering prompt */
	volatile bool running; /* exit main loop once this becomes false */
	int exit_status;
	volatile sig_atomic_t cancel_filter; /* abort external command */
	volatile sig_atomic_t sigbus;
	sigjmp_buf sigbus_jmpbuf;
	Map *actions;          /* built in special editor keys / commands */
	lua_State *lua;
};

/* TODO make part of struct Vis? */
static Mode vis_modes[VIS_MODE_LAST];

enum CmdOpt {          /* option flags for command definitions */
	CMD_OPT_NONE,  /* no option (default value) */
	CMD_OPT_FORCE, /* whether the command can be forced by appending '!' */
	CMD_OPT_ARGS,  /* whether the command line should be parsed in to space
	                * separated arguments to placed into argv, otherwise argv[1]
	                * will contain the  remaining command line unmodified */
};

typedef struct {             /* command definitions for the ':'-prompt */
	const char *name[3]; /* name and optional alias for the command */
	/* command logic called with a NULL terminated array of arguments.
	 * argv[0] will be the command name */
	bool (*cmd)(Vis*, Filerange*, enum CmdOpt opt, const char *argv[]);
	enum CmdOpt opt;     /* command option flags */
} Command;

static void mode_set(Vis *vis, Mode *new_mode);
/** window / file handling */

static void file_free(Vis *vis, File *file) {
	if (!file)
		return;
	if (--file->refcount > 0)
		return;
	
	text_free(file->text);
	free((char*)file->name);
	
	if (file->prev)
		file->prev->next = file->next;
	if (file->next)
		file->next->prev = file->prev;
	if (vis->files == file)
		vis->files = file->next;
	free(file);
}

static File *file_new_text(Vis *vis, Text *text) {
	File *file = calloc(1, sizeof(*file));
	if (!file)
		return NULL;
	file->text = text;
	file->stat = text_stat(text);
	file->refcount++;
	if (vis->files)
		vis->files->prev = file;
	file->next = vis->files;
	vis->files = file;
	return file;
}

static File *file_new(Vis *vis, const char *filename) {
	if (filename) {
		/* try to detect whether the same file is already open in another window
		 * TODO: do this based on inodes */
		for (File *file = vis->files; file; file = file->next) {
			if (file->name && strcmp(file->name, filename) == 0) {
				file->refcount++;
				return file;
			}
		}
	}

	Text *text = text_load(filename);
	if (!text && filename && errno == ENOENT)
		text = text_load(NULL);
	if (!text)
		return NULL;
	
	File *file = file_new_text(vis, text);
	if (!file) {
		text_free(text);
		return NULL;
	}

	if (filename)
		file->name = strdup(filename);
	return file;
}

static void window_name(Win *win, const char *filename) {
	lua_State *L = win->editor->lua;
	File *file = win->file;
	if (filename != file->name) {
		free((char*)file->name);
		file->name = filename ? strdup(filename) : NULL;
	}

	if (filename && L) {
		lua_getglobal(L, "lexers");
		lua_getfield(L, -1, "lexer_name");
		lua_pushstring(L, filename);
		lua_pcall(L, 1, 1, 0);
		if (lua_isstring(L, -1)) {
			const char *lexer_name = lua_tostring(L, -1);
			if (lexer_name)
				view_syntax_set(win->view, lexer_name);
		}
		lua_pop(L, 2); /* return value: lexer name, lexers variable */
	}
}

static void windows_invalidate(Vis *vis, size_t start, size_t end) {
	for (Win *win = vis->windows; win; win = win->next) {
		if (vis->win != win && vis->win->file == win->file) {
			Filerange view = view_viewport_get(win->view);
			if ((view.start <= start && start <= view.end) ||
			    (view.start <= end && end <= view.end))
				view_draw(win->view);
		}
	}
	view_draw(vis->win->view);
}

static void window_selection_changed(void *win, Filerange *sel) {
	File *file = ((Win*)win)->file;
	if (text_range_valid(sel)) {
		file->marks[MARK_SELECTION_START] = text_mark_set(file->text, sel->start);
		file->marks[MARK_SELECTION_END] = text_mark_set(file->text, sel->end);
	}
}

static void windows_arrange(Vis *vis, enum UiLayout layout) {
	vis->ui->arrange(vis->ui, layout);
}

static void window_free(Win *win) {
	if (!win)
		return;
	Vis *vis = win->editor;
	if (vis && vis->ui)
		vis->ui->window_free(win->ui);
	view_free(win->view);
	ringbuf_free(win->jumplist);
	free(win);
}

static Win *window_new_file(Vis *vis, File *file) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->editor = vis;
	win->file = file;
	win->events = (ViewEvent) {
		.data = win,
		.selection = window_selection_changed,
	};
	win->jumplist = ringbuf_alloc(31);
	win->view = view_new(file->text, vis->lua, &win->events);
	win->ui = vis->ui->window_new(vis->ui, win->view, file);
	if (!win->jumplist || !win->view || !win->ui) {
		window_free(win);
		return NULL;
	}
	view_tabwidth_set(win->view, vis->tabwidth);
	if (vis->windows)
		vis->windows->prev = win;
	win->next = vis->windows;
	vis->windows = win;
	vis->win = win;
	vis->ui->window_focus(win->ui);
	return win;
}

bool vis_window_reload(Win *win) {
	const char *name = win->file->name;
	if (!name)
		return false; /* can't reload unsaved file */
	/* temporarily unset file name, otherwise file_new returns the same File */
	win->file->name = NULL;
	File *file = file_new(win->editor, name);
	win->file->name = name;
	if (!file)
		return false;
	file_free(win->editor, win->file);
	win->file = file;
	win->ui->reload(win->ui, file);
	return true;
}

bool vis_window_split(Win *original) {
	Win *win = window_new_file(original->editor, original->file);
	if (!win)
		return false;
	win->file = original->file;
	win->file->refcount++;
	view_syntax_set(win->view, view_syntax_get(original->view));
	view_options_set(win->view, view_options_get(original->view));
	view_cursor_to(win->view, view_cursor_get(original->view));
	vis_draw(win->editor);
	return true;
}

void vis_resize(Vis *vis) {
	vis->ui->resize(vis->ui);
}

void vis_window_next(Vis *vis) {
	Win *sel = vis->win;
	if (!sel)
		return;
	vis->win = vis->win->next;
	if (!vis->win)
		vis->win = vis->windows;
	vis->ui->window_focus(vis->win->ui);
}

void vis_window_prev(Vis *vis) {
	Win *sel = vis->win;
	if (!sel)
		return;
	vis->win = vis->win->prev;
	if (!vis->win)
		for (vis->win = vis->windows; vis->win->next; vis->win = vis->win->next);
	vis->ui->window_focus(vis->win->ui);
}

static int tabwidth_get(Vis *vis) {
	return vis->tabwidth;
}

static void tabwidth_set(Vis *vis, int tabwidth) {
	if (tabwidth < 1 || tabwidth > 8)
		return;
	for (Win *win = vis->windows; win; win = win->next)
		view_tabwidth_set(win->view, tabwidth);
	vis->tabwidth = tabwidth;
}

void vis_draw(Vis *vis) {
	vis->ui->draw(vis->ui);
}

void vis_update(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next)
		view_update(win->view);
	view_update(vis->win->view);
	vis->ui->update(vis->ui);
}

void vis_suspend(Vis *vis) {
	vis->ui->suspend(vis->ui);
}

bool vis_window_new(Vis *vis, const char *filename) {
	File *file = file_new(vis, filename);
	if (!file)
		return false;
	Win *win = window_new_file(vis, file);
	if (!win) {
		file_free(vis, file);
		return false;
	}

	window_name(win, filename);
	vis_draw(vis);

	return true;
}

void vis_window_close(Win *win) {
	Vis *vis = win->editor;
	file_free(vis, win->file);
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (vis->windows == win)
		vis->windows = win->next;
	if (vis->win == win)
		vis->win = win->next ? win->next : win->prev;
	if (vis->prompt_window == win)
		vis->prompt_window = NULL;
	window_free(win);
	if (vis->win)
		vis->ui->window_focus(vis->win->ui);
	vis_draw(vis);
}

Vis *vis_new(Ui *ui) {
	if (!ui)
		return NULL;
	Vis *vis = calloc(1, sizeof(Vis));
	if (!vis)
		return NULL;
	lua_State *L = luaL_newstate();
	if (!(vis->lua = L))
		goto err;
	luaL_openlibs(L);

	/* try to get users home directory */
	const char *home = getenv("HOME");
	if (!home || !*home) {
		struct passwd *pw = getpwuid(getuid());
		if (pw)
			home = pw->pw_dir;
	}

	/* extends lua's package.path with:
	 * - $VIS_PATH/lexers
	 * - $HOME/.vis/lexers
	 * - /usr/share/vis/lexers
	 * - package.path (standard lua search path)
	 */
	int paths = 2;
	lua_getglobal(L, "package");

	const char *vis_path = getenv("VIS_PATH");
	if (vis_path) {
		lua_pushstring(L, vis_path);
		lua_pushstring(L, "/lexers/?.lua;");
		lua_concat(L, 2);
		paths++;
	}

	if (home && *home) {
		lua_pushstring(L, home);
		lua_pushstring(L, "/.vis/lexers/?.lua;");
		lua_concat(L, 2);
		paths++;
	}

	lua_pushstring(L, "/usr/share/vis/lexers/?.lua;");
	lua_getfield(L, -paths, "path");
	lua_concat(L, paths);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1); /* package */

	/* try to load the lexer module */
	lua_getglobal(L, "require");
	lua_pushstring(L, "lexer");
	if (lua_pcall(L, 1, 1, 0)) {
		lua_close(L);
		vis->lua = L = NULL;
	} else {
		lua_setglobal(L, "lexers");
		vis_theme_load(vis, "default");
	}

	vis->ui = ui;
	vis->ui->init(vis->ui, vis);
	vis->tabwidth = 8;
	vis->expandtab = false;
	for (int i = 0; i < VIS_MODE_LAST; i++) {
		Mode *mode = &vis_modes[i];
		if (!(mode->bindings = map_new()))
			goto err;
	}
	if (!(vis->prompt = calloc(1, sizeof(Win))))
		goto err;
	if (!(vis->prompt->file = calloc(1, sizeof(File))))
		goto err;
	if (!(vis->prompt->file->text = text_load(NULL)))
		goto err;
	if (!(vis->prompt->view = view_new(vis->prompt->file->text, NULL, NULL)))
		goto err;
	if (!(vis->prompt->ui = vis->ui->prompt_new(vis->ui, vis->prompt->view, vis->prompt->file)))
		goto err;
	if (!(vis->search_pattern = text_regex_new()))
		goto err;
	vis->mode_prev = vis->mode = &vis_modes[VIS_MODE_NORMAL];
	return vis;
err:
	vis_free(vis);
	return NULL;
}

void vis_free(Vis *vis) {
	if (!vis)
		return;
	if (vis->lua)
		lua_close(vis->lua);
	while (vis->windows)
		vis_window_close(vis->windows);
	file_free(vis, vis->prompt->file);
	window_free(vis->prompt);
	text_regex_free(vis->search_pattern);
	for (int i = 0; i < LENGTH(vis->registers); i++)
		register_release(&vis->registers[i]);
	for (int i = 0; i < LENGTH(vis->macros); i++)
		macro_release(&vis->macros[i]);
	vis->ui->free(vis->ui);
	map_free(vis->cmds);
	map_free(vis->options);
	map_free(vis->actions);
	buffer_release(&vis->buffer_repeat);
	for (int i = 0; i < VIS_MODE_LAST; i++) {
		Mode *mode = &vis_modes[i];
		map_free(mode->bindings);
	}
	free(vis);
}

void vis_insert(Vis *vis, size_t pos, const char *data, size_t len) {
	text_insert(vis->win->file->text, pos, data, len);
	windows_invalidate(vis, pos, pos + len);
}

void vis_insert_key(Vis *vis, const char *data, size_t len) {
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		vis_insert(vis, pos, data, len);
		view_cursors_scroll_to(c, pos + len);
	}
}

void vis_replace(Vis *vis, size_t pos, const char *data, size_t len) {
	size_t chars = 0;
	for (size_t i = 0; i < len; i++) {
		if (ISUTF8(data[i]))
			chars++;
	}

	Text *txt = vis->win->file->text;
	Iterator it = text_iterator_get(txt, pos);
	for (char c; chars-- > 0 && text_iterator_byte_get(&it, &c) && c != '\r' && c != '\n'; )
		text_iterator_char_next(&it, NULL);

	text_delete(txt, pos, it.pos - pos);
	vis_insert(vis, pos, data, len);
}

void vis_replace_key(Vis *vis, const char *data, size_t len) {
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		vis_replace(vis, pos, data, len);
		view_cursors_scroll_to(c, pos + len);
	}
}

void vis_delete(Vis *vis, size_t pos, size_t len) {
	text_delete(vis->win->file->text, pos, len);
	windows_invalidate(vis, pos, pos + len);
}

void vis_prompt_show(Vis *vis, const char *title, const char *text) {
	if (vis->prompt_window)
		return;
	vis->prompt_window = vis->win;
	vis->win = vis->prompt;
	vis->prompt_type = title[0];
	vis->ui->prompt(vis->ui, title, text);
}

void vis_prompt_hide(Vis *vis) {
	if (!vis->prompt_window)
		return;
	vis->ui->prompt_hide(vis->ui);
	vis->win = vis->prompt_window;
	vis->prompt_window = NULL;
}

char *vis_prompt_get(Vis *vis) {
	return vis->ui->prompt_input(vis->ui);
}

void vis_info_show(Vis *vis, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vis->ui->info(vis->ui, msg, ap);
	va_end(ap);
}

void vis_info_hide(Vis *vis) {
	vis->ui->info_hide(vis->ui);
}

/** operators */
static size_t op_change(Vis*, Text*, OperatorContext *c);
static size_t op_yank(Vis*, Text*, OperatorContext *c);
static size_t op_put(Vis*, Text*, OperatorContext *c);
static size_t op_delete(Vis*, Text*, OperatorContext *c);
static size_t op_shift_right(Vis*, Text*, OperatorContext *c);
static size_t op_shift_left(Vis*, Text*, OperatorContext *c);
static size_t op_case_change(Vis*, Text*, OperatorContext *c);
static size_t op_join(Vis*, Text*, OperatorContext *c);
static size_t op_repeat_insert(Vis*, Text*, OperatorContext *c);
static size_t op_repeat_replace(Vis*, Text*, OperatorContext *c);
static size_t op_cursor(Vis*, Text*, OperatorContext *c);

static Operator ops[] = {
	[OP_DELETE]      = { op_delete      },
	[OP_CHANGE]      = { op_change      },
	[OP_YANK]        = { op_yank        },
	[OP_PUT_AFTER]   = { op_put         },
	[OP_SHIFT_RIGHT] = { op_shift_right },
	[OP_SHIFT_LEFT]  = { op_shift_left  },
	[OP_CASE_SWAP] = { op_case_change },
	[OP_JOIN]          = { op_join          },
	[OP_REPEAT_INSERT]  = { op_repeat_insert  },
	[OP_REPEAT_REPLACE] = { op_repeat_replace },
	[OP_CURSOR_SOL]         = { op_cursor         },
};

/** movements which can be used besides the one in text-motions.h and view.h */

/* search in forward direction for the word under the cursor */
static size_t search_word_forward(Vis*, Text *txt, size_t pos);
/* search in backward direction for the word under the cursor */
static size_t search_word_backward(Vis*, Text *txt, size_t pos);
/* search again for the last used search pattern */
static size_t search_forward(Vis*, Text *txt, size_t pos);
static size_t search_backward(Vis*, Text *txt, size_t pos);
/* goto action.mark */
static size_t mark_goto(Vis*, File *txt, size_t pos);
/* goto first non-blank char on line pointed by action.mark */
static size_t mark_line_goto(Vis*, File *txt, size_t pos);
/* goto to next occurence of action.key to the right */
static size_t to(Vis*, Text *txt, size_t pos);
/* goto to position before next occurence of action.key to the right */
static size_t till(Vis*, Text *txt, size_t pos);
/* goto to next occurence of action.key to the left */
static size_t to_left(Vis*, Text *txt, size_t pos);
/* goto to position after next occurence of action.key to the left */
static size_t till_left(Vis*, Text *txt, size_t pos);
/* goto line number action.count */
static size_t line(Vis*, Text *txt, size_t pos);
/* goto to byte action.count on current line */
static size_t column(Vis*, Text *txt, size_t pos);
/* goto the action.count-th line from top of the focused window */
static size_t view_lines_top(Vis*, View*);
/* goto the start of middle line of the focused window */
static size_t view_lines_middle(Vis*, View*);
/* goto the action.count-th line from bottom of the focused window */
static size_t view_lines_bottom(Vis*, View*);
/* navigate the change list */
static size_t window_changelist_next(Vis*, Win*, size_t pos);
static size_t window_changelist_prev(Vis*, Win*, size_t pos);
/* navigate the jump list */
static size_t window_jumplist_next(Vis*, Win*, size_t pos);
static size_t window_jumplist_prev(Vis*, Win*, size_t pos);
static size_t window_nop(Vis*, Win*, size_t pos);

static Movement moves[] = {
	[MOVE_LINE_UP]             = { .cur = view_line_up,            .type = LINEWISE           },
	[MOVE_LINE_DOWN]           = { .cur = view_line_down,          .type = LINEWISE           },
	[MOVE_SCREEN_LINE_UP]      = { .cur = view_screenline_up,                                 },
	[MOVE_SCREEN_LINE_DOWN]    = { .cur = view_screenline_down,                               },
	[MOVE_SCREEN_LINE_BEGIN]   = { .cur = view_screenline_begin,   .type = CHARWISE           },
	[MOVE_SCREEN_LINE_MIDDLE]  = { .cur = view_screenline_middle,  .type = CHARWISE           },
	[MOVE_SCREEN_LINE_END]     = { .cur = view_screenline_end,     .type = CHARWISE|INCLUSIVE },
	[MOVE_LINE_PREV]           = { .txt = text_line_prev,                                      },
	[MOVE_LINE_BEGIN]          = { .txt = text_line_begin,                                     },
	[MOVE_LINE_START]          = { .txt = text_line_start,                                     },
	[MOVE_LINE_FINISH]         = { .txt = text_line_finish,         .type = INCLUSIVE          },
	[MOVE_LINE_LASTCHAR]       = { .txt = text_line_lastchar,       .type = INCLUSIVE          },
	[MOVE_LINE_END]            = { .txt = text_line_end,                                       },
	[MOVE_LINE_NEXT]           = { .txt = text_line_next,                                      },
	[MOVE_LINE]                = { .vis = line,                     .type = LINEWISE|IDEMPOTENT|JUMP},
	[MOVE_COLUMN]              = { .vis = column,                   .type = CHARWISE|IDEMPOTENT},
	[MOVE_CHAR_PREV]           = { .txt = text_char_prev,           .type = CHARWISE           },
	[MOVE_CHAR_NEXT]           = { .txt = text_char_next,           .type = CHARWISE           },
	[MOVE_LINE_CHAR_PREV]      = { .txt = text_line_char_prev,      .type = CHARWISE           },
	[MOVE_LINE_CHAR_NEXT]      = { .txt = text_line_char_next,      .type = CHARWISE           },
	[MOVE_WORD_START_PREV]     = { .txt = text_word_start_prev,     .type = CHARWISE           },
	[MOVE_WORD_START_NEXT]     = { .txt = text_word_start_next,     .type = CHARWISE           },
	[MOVE_WORD_END_PREV]       = { .txt = text_word_end_prev,       .type = CHARWISE|INCLUSIVE },
	[MOVE_WORD_END_NEXT]       = { .txt = text_word_end_next,       .type = CHARWISE|INCLUSIVE },
	[MOVE_LONGWORD_START_PREV] = { .txt = text_longword_start_prev, .type = CHARWISE           },
	[MOVE_LONGWORD_START_NEXT] = { .txt = text_longword_start_next, .type = CHARWISE           },
	[MOVE_LONGWORD_END_PREV]   = { .txt = text_longword_end_prev,   .type = CHARWISE|INCLUSIVE },
	[MOVE_LONGWORD_END_NEXT]   = { .txt = text_longword_end_next,   .type = CHARWISE|INCLUSIVE },
	[MOVE_SENTENCE_PREV]       = { .txt = text_sentence_prev,       .type = LINEWISE           },
	[MOVE_SENTENCE_NEXT]       = { .txt = text_sentence_next,       .type = LINEWISE           },
	[MOVE_PARAGRAPH_PREV]      = { .txt = text_paragraph_prev,      .type = LINEWISE|JUMP      },
	[MOVE_PARAGRAPH_NEXT]      = { .txt = text_paragraph_next,      .type = LINEWISE|JUMP      },
	[MOVE_FUNCTION_START_PREV] = { .txt = text_function_start_prev, .type = LINEWISE|JUMP      },
	[MOVE_FUNCTION_START_NEXT] = { .txt = text_function_start_next, .type = LINEWISE|JUMP      },
	[MOVE_FUNCTION_END_PREV]   = { .txt = text_function_end_prev,   .type = LINEWISE|JUMP      },
	[MOVE_FUNCTION_END_NEXT]   = { .txt = text_function_end_next,   .type = LINEWISE|JUMP      },
	[MOVE_BRACKET_MATCH]       = { .txt = text_bracket_match,       .type = INCLUSIVE|JUMP     },
	[MOVE_FILE_BEGIN]          = { .txt = text_begin,               .type = LINEWISE|JUMP      },
	[MOVE_FILE_END]            = { .txt = text_end,                 .type = LINEWISE|JUMP      },
	[MOVE_LEFT_TO]             = { .vis = to_left,                                             },
	[MOVE_RIGHT_TO]            = { .vis = to,                       .type = INCLUSIVE          },
	[MOVE_LEFT_TILL]           = { .vis = till_left,                                           },
	[MOVE_RIGHT_TILL]          = { .vis = till,                     .type = INCLUSIVE          },
	[MOVE_MARK]                = { .file = mark_goto,               .type = JUMP|IDEMPOTENT    },
	[MOVE_MARK_LINE]           = { .file = mark_line_goto,          .type = LINEWISE|JUMP|IDEMPOTENT},
	[MOVE_SEARCH_WORD_FORWARD] = { .vis = search_word_forward,      .type = JUMP                   },
	[MOVE_SEARCH_WORD_BACKWARD]= { .vis = search_word_backward,     .type = JUMP                   },
	[MOVE_SEARCH_NEXT]         = { .vis = search_forward,           .type = JUMP                   },
	[MOVE_SEARCH_PREV]         = { .vis = search_backward,          .type = JUMP                   },
	[MOVE_WINDOW_LINE_TOP]     = { .view = view_lines_top,         .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_WINDOW_LINE_MIDDLE]  = { .view = view_lines_middle,      .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_WINDOW_LINE_BOTTOM]  = { .view = view_lines_bottom,      .type = LINEWISE|JUMP|IDEMPOTENT },
	[MOVE_CHANGELIST_NEXT]     = { .win = window_changelist_next,  .type = INCLUSIVE               },
	[MOVE_CHANGELIST_PREV]     = { .win = window_changelist_prev,  .type = INCLUSIVE               },
	[MOVE_JUMPLIST_NEXT]       = { .win = window_jumplist_next,    .type = INCLUSIVE               },
	[MOVE_JUMPLIST_PREV]       = { .win = window_jumplist_prev,    .type = INCLUSIVE               },
	[MOVE_NOP]                 = { .win = window_nop,              .type = IDEMPOTENT              },
};

static TextObject textobjs[] = {
	[TEXT_OBJ_INNER_WORD]           = { text_object_word                  },
	[TEXT_OBJ_OUTER_WORD]           = { text_object_word_outer            },
	[TEXT_OBJ_INNER_LONGWORD]       = { text_object_longword              },
	[TEXT_OBJ_OUTER_LONGWORD]       = { text_object_longword_outer        },
	[TEXT_OBJ_SENTENCE]             = { text_object_sentence              },
	[TEXT_OBJ_PARAGRAPH]            = { text_object_paragraph             },
	[TEXT_OBJ_OUTER_SQUARE_BRACKET] = { text_object_square_bracket, OUTER },
	[TEXT_OBJ_INNER_SQUARE_BRACKET] = { text_object_square_bracket, INNER },
	[TEXT_OBJ_OUTER_CURLY_BRACKET]  = { text_object_curly_bracket,  OUTER },
	[TEXT_OBJ_INNER_CURLY_BRACKET]  = { text_object_curly_bracket,  INNER },
	[TEXT_OBJ_OUTER_ANGLE_BRACKET]  = { text_object_angle_bracket,  OUTER },
	[TEXT_OBJ_INNER_ANGLE_BRACKET]  = { text_object_angle_bracket,  INNER },
	[TEXT_OBJ_OUTER_PARANTHESE]     = { text_object_paranthese,     OUTER },
	[TEXT_OBJ_INNER_PARANTHESE]     = { text_object_paranthese,     INNER },
	[TEXT_OBJ_OUTER_QUOTE]          = { text_object_quote,          OUTER },
	[TEXT_OBJ_INNER_QUOTE]          = { text_object_quote,          INNER },
	[TEXT_OBJ_OUTER_SINGLE_QUOTE]   = { text_object_single_quote,   OUTER },
	[TEXT_OBJ_INNER_SINGLE_QUOTE]   = { text_object_single_quote,   INNER },
	[TEXT_OBJ_OUTER_BACKTICK]       = { text_object_backtick,       OUTER },
	[TEXT_OBJ_INNER_BACKTICK]       = { text_object_backtick,       INNER },
	[TEXT_OBJ_OUTER_ENTIRE]         = { text_object_entire,               },
	[TEXT_OBJ_INNER_ENTIRE]         = { text_object_entire_inner,         },
	[TEXT_OBJ_OUTER_FUNCTION]       = { text_object_function,             },
	[TEXT_OBJ_INNER_FUNCTION]       = { text_object_function_inner,       },
	[TEXT_OBJ_OUTER_LINE]           = { text_object_line,                 },
	[TEXT_OBJ_INNER_LINE]           = { text_object_line_inner,           },
};


/** commands to enter at the ':'-prompt */
/* set various runtime options */
static bool cmd_set(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* for each argument create a new window and open the corresponding file */
static bool cmd_open(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* close current window (discard modifications if forced ) and open argv[1],
 * if no argv[1] is given re-read to current file from disk */
static bool cmd_edit(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* close the current window, discard modifications if forced */
static bool cmd_quit(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* close all windows which show current file, discard modifications if forced  */
static bool cmd_bdelete(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* close all windows, exit editor, discard modifications if forced */
static bool cmd_qall(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* for each argument try to insert the file content at current cursor postion */
static bool cmd_read(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
static bool cmd_substitute(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* if no argument are given, split the current window horizontally,
 * otherwise open the file */
static bool cmd_split(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* if no argument are given, split the current window vertically,
 * otherwise open the file */
static bool cmd_vsplit(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* create a new empty window and arrange all windows either horizontally or vertically */
static bool cmd_new(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
static bool cmd_vnew(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window and close it */
static bool cmd_wq(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window if it was changvis, then close the window */
static bool cmd_xit(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window to the name given.
 * do not change internal filname association. further :w commands
 * without arguments will still write to the old filename */
static bool cmd_write(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* save the file displayed in the current window to the name given,
 * associate the new name with the buffer. further :w commands
 * without arguments will write to the new filename */
static bool cmd_saveas(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* filter range through external program argv[1] */
static bool cmd_filter(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* switch to the previous/next saved state of the text, chronologically */
static bool cmd_earlier_later(Vis*, Filerange*, enum CmdOpt, const char *argv[]);
/* dump current key bindings */
static bool cmd_help(Vis*, Filerange*, enum CmdOpt, const char *argv[]);

static void action_reset(Vis*, Action *a);

static void vis_mode_operator_enter(Vis *vis, Mode *old) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_OPERATOR_OPTION];
}

static void vis_mode_operator_leave(Vis *vis, Mode *new) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
}

static void vis_mode_operator_input(Vis *vis, const char *str, size_t len) {
	/* invalid operator */
	action_reset(vis, &vis->action);
	mode_set(vis, vis->mode_prev);
}

static void vis_mode_visual_enter(Vis *vis, Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
}

static void vis_mode_visual_line_enter(Vis *vis, Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
	vis_motion(vis, MOVE_LINE_END);
}

static void vis_mode_visual_line_leave(Vis *vis, Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	} else {
		view_cursor_to(vis->win->view, view_cursor_get(vis->win->view));
	}
}

static void vis_mode_visual_leave(Vis *vis, Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	}
}

static void vis_mode_prompt_input(Vis *vis, const char *str, size_t len) {
	vis_insert_key(vis, str, len);
}

static void vis_mode_prompt_enter(Vis *vis, Mode *old) {
	if (old->isuser && old != &vis_modes[VIS_MODE_PROMPT])
		vis->mode_before_prompt = old;
}

static void vis_mode_prompt_leave(Vis *vis, Mode *new) {
	if (new->isuser)
		vis_prompt_hide(vis);
}

static void vis_mode_insert_leave(Vis *vis, Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_idle(Vis *vis) {
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_input(Vis *vis, const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = view_cursor_get(vis->win->view);
	if (pos != oldpos)
		buffer_truncate(&vis->buffer_repeat);
	buffer_append(&vis->buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(vis, &vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_INSERT];
	vis_insert_key(vis, str, len);
}

static void vis_mode_replace_leave(Vis *vis, Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
}

static void vis_mode_replace_input(Vis *vis, const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = view_cursor_get(vis->win->view);
	if (pos != oldpos)
		buffer_truncate(&vis->buffer_repeat);
	buffer_append(&vis->buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(vis, &vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_REPLACE];
	vis_replace_key(vis, str, len);
}

/*
 * the tree of modes currently looks like this. the double line between OPERATOR-OPTION
 * and OPERATOR is only in effect once an operator is detected. that is when entering the
 * OPERATOR mode its parent is set to OPERATOR-OPTION which makes {INNER-,}TEXTOBJ
 * reachable. once the operator is processed (i.e. the OPERATOR mode is left) its parent
 * mode is reset back to MOVE.
 *
 * Similarly the +-ed line between OPERATOR and TEXTOBJ is only active within the visual
 * modes.
 *
 *
 *                                         BASIC
 *                                    (arrow keys etc.)
 *                                    /      |
 *               /-------------------/       |
 *            READLINE                      MOVE
 *            /       \                 (h,j,k,l ...)
 *           /         \                     |       \-----------------\
 *          /           \                    |                         |
 *       INSERT       PROMPT             OPERATOR ++++           INNER-TEXTOBJ
 *          |      (history etc)       (d,c,y,p ..)   +      (i [wsp[]()b<>{}B"'`] )
 *          |                                |     \\  +               |
 *          |                                |      \\  +              |
 *       REPLACE                           NORMAL    \\  +          TEXTOBJ
 *                                           |        \\  +     (a [wsp[]()b<>{}B"'`] )
 *                                           |         \\  +   +       |
 *                                           |          \\  + +        |
 *                                         VISUAL        \\     OPERATOR-OPTION
 *                                           |            \\        (v,V)
 *                                           |             \\        //
 *                                           |              \\======//
 *                                      VISUAL-LINE
 */

static Mode vis_modes[] = {
	[VIS_MODE_BASIC] = {
		.name = "BASIC",
		.parent = NULL,
	},
	[VIS_MODE_MOVE] = {
		.name = "MOVE",
		.parent = &vis_modes[VIS_MODE_BASIC],
	},
	[VIS_MODE_TEXTOBJ] = {
		.name = "TEXT-OBJECTS",
		.parent = &vis_modes[VIS_MODE_MOVE],
	},
	[VIS_MODE_OPERATOR_OPTION] = {
		.name = "OPERATOR-OPTION",
		.parent = &vis_modes[VIS_MODE_TEXTOBJ],
	},
	[VIS_MODE_OPERATOR] = {
		.name = "OPERATOR",
		.parent = &vis_modes[VIS_MODE_MOVE],
		.enter = vis_mode_operator_enter,
		.leave = vis_mode_operator_leave,
		.input = vis_mode_operator_input,
	},
	[VIS_MODE_NORMAL] = {
		.name = "NORMAL",
		.status = "",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
	},
	[VIS_MODE_VISUAL] = {
		.name = "VISUAL",
		.status = "--VISUAL--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.enter = vis_mode_visual_enter,
		.leave = vis_mode_visual_leave,
		.visual = true,
	},
	[VIS_MODE_VISUAL_LINE] = {
		.name = "VISUAL LINE",
		.status = "--VISUAL LINE--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_VISUAL],
		.enter = vis_mode_visual_line_enter,
		.leave = vis_mode_visual_line_leave,
		.visual = true,
	},
	[VIS_MODE_READLINE] = {
		.name = "READLINE",
		.parent = &vis_modes[VIS_MODE_BASIC],
	},
	[VIS_MODE_PROMPT] = {
		.name = "PROMPT",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.input = vis_mode_prompt_input,
		.enter = vis_mode_prompt_enter,
		.leave = vis_mode_prompt_leave,
	},
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.status = "--INSERT--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.leave = vis_mode_insert_leave,
		.input = vis_mode_insert_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.status = "--REPLACE--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_INSERT],
		.leave = vis_mode_replace_leave,
		.input = vis_mode_replace_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
};


static Mode *mode_get(Vis *vis, enum VisMode mode) {
	if (mode < LENGTH(vis_modes))
		return &vis_modes[mode];
	return NULL;
}

static bool mode_map(Mode *mode, const char *name, KeyBinding *binding) {
	return map_put(mode->bindings, name, binding);
} 

bool vis_mode_map(Vis *vis, enum VisMode modeid, const char *name, KeyBinding *binding) {
	Mode *mode = mode_get(vis, modeid);
	return mode && map_put(mode->bindings, name, binding);
}

bool vis_mode_bindings(Vis *vis, enum VisMode modeid, KeyBinding **bindings) {
	Mode *mode = mode_get(vis, modeid);
	if (!mode)
		return false;
	bool success = true;
	for (KeyBinding *kb = *bindings; kb->key; kb++) {
		if (!mode_map(mode, kb->key, kb))
			success = false;
	}
	return success;
}

bool vis_mode_unmap(Vis *vis, enum VisMode modeid, const char *name) {
	Mode *mode = mode_get(vis, modeid);
	return mode && map_delete(mode->bindings, name);
}

bool vis_action_register(Vis *vis, KeyAction *action) {
	if (!vis->actions)
		vis->actions = map_new();
	if (!vis->actions)
		return false;
	return map_put(vis->actions, action->name, action);
}

static const char *getkey(Vis*);
static void action_do(Vis*, Action *a);

/** operator implementations of type: void (*op)(OperatorContext*) */

static size_t op_delete(Vis *vis, Text *txt, OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put(c->reg, txt, &c->range);
	text_delete_range(txt, &c->range);
	size_t pos = c->range.start;
	if (c->linewise && pos == text_size(txt))
		pos = text_line_begin(txt, text_line_prev(txt, pos));
	return pos;
}

static size_t op_change(Vis *vis, Text *txt, OperatorContext *c) {
	op_delete(vis, txt, c);
	return c->range.start;
}

static size_t op_yank(Vis *vis, Text *txt, OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put(c->reg, txt, &c->range);
	return c->pos;
}

static size_t op_put(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = c->pos;
	switch (c->arg->i) {
	case OP_PUT_AFTER:
	case OP_PUT_AFTER_END:
		if (c->reg->linewise)
			pos = text_line_next(txt, pos);
		else
			pos = text_char_next(txt, pos);
		break;
	case OP_PUT_BEFORE:
	case OP_PUT_BEFORE_END:
		if (c->reg->linewise)
			pos = text_line_begin(txt, pos);
		break;
	}

	for (int i = 0; i < c->count; i++) {
		text_insert(txt, pos, c->reg->data, c->reg->len);
		pos += c->reg->len;
	}

	if (c->reg->linewise) {
		switch (c->arg->i) {
		case OP_PUT_BEFORE_END:
		case OP_PUT_AFTER_END:
			pos = text_line_start(txt, pos);
			break;
		case OP_PUT_AFTER:
			pos = text_line_start(txt, text_line_next(txt, c->pos));
			break;
		case OP_PUT_BEFORE:
			pos = text_line_start(txt, c->pos);
			break;
		}
	}

	return pos;
}

static const char *expandtab(Vis *vis) {
	static char spaces[9];
	int tabwidth = tabwidth_get(vis);
	tabwidth = MIN(tabwidth, LENGTH(spaces) - 1);
	for (int i = 0; i < tabwidth; i++)
		spaces[i] = ' ';
	spaces[tabwidth] = '\0';
	return vis->expandtab ? spaces : "\t";
}

static size_t op_shift_right(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	const char *tab = expandtab(vis);
	size_t tablen = strlen(tab);

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		prev_pos = pos = text_line_begin(txt, pos);
		text_insert(txt, pos, tab, tablen);
		pos = text_line_prev(txt, pos);
	}  while (pos >= c->range.start && pos != prev_pos);

	return c->pos + tablen;
}

static size_t op_shift_left(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	size_t tabwidth = tabwidth_get(vis), tablen;

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		char c;
		size_t len = 0;
		prev_pos = pos = text_line_begin(txt, pos);
		Iterator it = text_iterator_get(txt, pos);
		if (text_iterator_byte_get(&it, &c) && c == '\t') {
			len = 1;
		} else {
			for (len = 0; text_iterator_byte_get(&it, &c) && c == ' '; len++)
				text_iterator_byte_next(&it, NULL);
		}
		tablen = MIN(len, tabwidth);
		text_delete(txt, pos, tablen);
		pos = text_line_prev(txt, pos);
	}  while (pos >= c->range.start && pos != prev_pos);

	return c->pos - tablen;
}

static size_t op_case_change(Vis *vis, Text *txt, OperatorContext *c) {
	size_t len = text_range_size(&c->range);
	char *buf = malloc(len);
	if (!buf)
		return c->pos;
	len = text_bytes_get(txt, c->range.start, len, buf);
	size_t rem = len;
	for (char *cur = buf; rem > 0; cur++, rem--) {
		int ch = (unsigned char)*cur;
		if (isascii(ch)) {
			if (c->arg->i == OP_CASE_SWAP)
				*cur = islower(ch) ? toupper(ch) : tolower(ch);
			else if (c->arg->i == OP_CASE_UPPER)
				*cur = toupper(ch);
			else
				*cur = tolower(ch);
		}
	}

	text_delete(txt, c->range.start, len);
	text_insert(txt, c->range.start, buf, len);
	free(buf);
	return c->pos;
}

static size_t op_cursor(Vis *vis, Text *txt, OperatorContext *c) {
	View *view = vis->win->view;
	Filerange r = text_range_linewise(txt, &c->range);
	for (size_t line = text_range_line_first(txt, &r); line != EPOS; line = text_range_line_next(txt, &r, line)) {
		Cursor *cursor = view_cursors_new(view);
		if (cursor) {
			size_t pos;
			if (c->arg->i == OP_CURSOR_EOL)
				pos = text_line_finish(txt, line);
			else
				pos = text_line_start(txt, line);
			view_cursors_to(cursor, pos);
		}
	}
	return EPOS;
}

static size_t op_join(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;

	/* if operator and range are both linewise, skip last line break */
	if (c->linewise && text_range_is_linewise(txt, &c->range)) {
		size_t line_prev = text_line_prev(txt, pos);
		size_t line_prev_prev = text_line_prev(txt, line_prev);
		if (line_prev_prev >= c->range.start)
			pos = line_prev;
	}

	do {
		prev_pos = pos;
		size_t end = text_line_start(txt, pos);
		pos = text_char_next(txt, text_line_finish(txt, text_line_prev(txt, end)));
		if (pos >= c->range.start && end > pos) {
			text_delete(txt, pos, end - pos);
			text_insert(txt, pos, " ", 1);
		} else {
			break;
		}
	} while (pos != prev_pos);

	return c->range.start;
}

static size_t op_repeat_insert(Vis *vis, Text *txt, OperatorContext *c) {
	size_t len = vis->buffer_repeat.len;
	if (!len)
		return c->pos;
	text_insert(txt, c->pos, vis->buffer_repeat.data, len);
	return c->pos + len;
}

static size_t op_repeat_replace(Vis *vis, Text *txt, OperatorContext *c) {
	const char *data = vis->buffer_repeat.data;
	size_t len = vis->buffer_repeat.len;
	vis_replace(vis, c->pos, data, len);
	return c->pos + len;
}

/** movement implementations of type: size_t (*move)(const Arg*) */

static char *get_word_at(Text *txt, size_t pos) {
	Filerange word = text_object_word(txt, pos);
	if (!text_range_valid(&word))
		return NULL;
	size_t len = word.end - word.start;
	char *buf = malloc(len+1);
	if (!buf)
		return NULL;
	len = text_bytes_get(txt, word.start, len, buf);
	buf[len] = '\0';
	return buf;
}

static size_t search_word_forward(Vis *vis, Text *txt, size_t pos) {
	char *word = get_word_at(txt, pos);
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_forward(txt, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_word_backward(Vis *vis, Text *txt, size_t pos) {
	char *word = get_word_at(txt, pos);
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_backward(txt, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_forward(Vis *vis, Text *txt, size_t pos) {
	return text_search_forward(txt, pos, vis->search_pattern);
}

static size_t search_backward(Vis *vis, Text *txt, size_t pos) {
	return text_search_backward(txt, pos, vis->search_pattern);
}

static size_t mark_goto(Vis *vis, File *file, size_t pos) {
	return text_mark_get(file->text, file->marks[vis->action.mark]);
}

static size_t mark_line_goto(Vis *vis, File *file, size_t pos) {
	return text_line_start(file->text, mark_goto(vis, file, pos));
}

static size_t to(Vis *vis, Text *txt, size_t pos) {
	char c;
	size_t hit = text_line_find_next(txt, pos+1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to(vis, txt, pos);
	if (hit != pos)
		return text_char_prev(txt, hit);
	return pos;
}

static size_t to_left(Vis *vis, Text *txt, size_t pos) {
	char c;
	if (pos == 0)
		return pos;
	size_t hit = text_line_find_prev(txt, pos-1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till_left(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to_left(vis, txt, pos);
	if (hit != pos)
		return text_char_next(txt, hit);
	return pos;
}

static size_t line(Vis *vis, Text *txt, size_t pos) {
	return text_pos_by_lineno(txt, vis->action.count);
}

static size_t column(Vis *vis, Text *txt, size_t pos) {
	return text_line_offset(txt, pos, vis->action.count);
}

static size_t view_lines_top(Vis *vis, View *view) {
	return view_screenline_goto(view, vis->action.count);
}

static size_t view_lines_middle(Vis *vis, View *view) {
	int h = view_height_get(view);
	return view_screenline_goto(view, h/2);
}

static size_t view_lines_bottom(Vis *vis, View *view) {
	int h = view_height_get(vis->win->view);
	return view_screenline_goto(vis->win->view, h - vis->action.count);
}

static size_t window_changelist_next(Vis *vis, Win *win, size_t pos) {
	ChangeList *cl = &win->changelist;
	Text *txt = win->file->text;
	time_t state = text_state(txt);
	if (cl->state != state)
		cl->index = 0;
	else if (cl->index > 0 && pos == cl->pos)
		cl->index--;
	size_t newpos = text_history_get(txt, cl->index);
	if (newpos == EPOS)
		cl->index++;
	else
		cl->pos = newpos;
	cl->state = state;
	return cl->pos;
}

static size_t window_changelist_prev(Vis *vis, Win *win, size_t pos) {
	ChangeList *cl = &win->changelist;
	Text *txt = win->file->text;
	time_t state = text_state(txt);
	if (cl->state != state)
		cl->index = 0;
	else if (pos == cl->pos)
		win->changelist.index++;
	size_t newpos = text_history_get(txt, cl->index);
	if (newpos == EPOS)
		cl->index--;
	else
		cl->pos = newpos;
	cl->state = state;
	return cl->pos;
}

static size_t window_jumplist_next(Vis *vis, Win *win, size_t cur) {
	while (win->jumplist) {
		Mark mark = ringbuf_next(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->file->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

static size_t window_jumplist_prev(Vis *vis, Win *win, size_t cur) {
	while (win->jumplist) {
		Mark mark = ringbuf_prev(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->file->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

static size_t window_nop(Vis *vis, Win *win, size_t pos) {
	return pos;
}

/** action processing: execut the operator / movement / text object */

static void window_jumplist_add(Win *win, size_t pos) {
	Mark mark = text_mark_set(win->file->text, pos);
	if (mark && win->jumplist)
		ringbuf_add(win->jumplist, mark);
}

static void window_jumplist_invalidate(Win *win) {
	if (win->jumplist)
		ringbuf_invalidate(win->jumplist);
}

static void action_do(Vis *vis, Action *a) {
	Win *win = vis->win;
	Text *txt = win->file->text;
	View *view = win->view;
	if (a->count < 1)
		a->count = 1;
	bool multiple_cursors = view_cursors_count(view) > 1;
	bool linewise = !(a->type & CHARWISE) && (
		a->type & LINEWISE || (a->movement && a->movement->type & LINEWISE) ||
		vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE]);

	for (Cursor *cursor = view_cursors(view), *next; cursor; cursor = next) {

		next = view_cursors_next(cursor);
		size_t pos = view_cursors_pos(cursor);
		Register *reg = a->reg ? a->reg : &vis->registers[REG_DEFAULT];
		if (multiple_cursors)
			reg = view_cursors_register(cursor);

		OperatorContext c = {
			.count = a->count,
			.pos = pos,
			.range = text_range_empty(),
			.reg = reg,
			.linewise = linewise,
			.arg = &a->arg,
		};

		if (a->movement) {
			size_t start = pos;
			for (int i = 0; i < a->count; i++) {
				if (a->movement->txt)
					pos = a->movement->txt(txt, pos);
				else if (a->movement->cur)
					pos = a->movement->cur(cursor);
				else if (a->movement->file)
					pos = a->movement->file(vis, vis->win->file, pos);
				else if (a->movement->vis)
					pos = a->movement->vis(vis, txt, pos);
				else if (a->movement->view)
					pos = a->movement->view(vis, view);
				else if (a->movement->win)
					pos = a->movement->win(vis, win, pos);
				if (pos == EPOS || a->movement->type & IDEMPOTENT)
					break;
			}

			if (pos == EPOS) {
				c.range.start = start;
				c.range.end = start;
				pos = start;
			} else {
				c.range = text_range_new(start, pos);
			}

			if (!a->op) {
				if (a->movement->type & CHARWISE)
					view_cursors_scroll_to(cursor, pos);
				else
					view_cursors_to(cursor, pos);
				if (vis->mode->visual)
					c.range = view_cursors_selection_get(cursor);
				if (a->movement->type & JUMP)
					window_jumplist_add(win, pos);
				else
					window_jumplist_invalidate(win);
			} else if (a->movement->type & INCLUSIVE) {
				c.range.end = text_char_next(txt, c.range.end);
			}
		} else if (a->textobj) {
			if (vis->mode->visual)
				c.range = view_cursors_selection_get(cursor);
			else
				c.range.start = c.range.end = pos;
			for (int i = 0; i < a->count; i++) {
				Filerange r = a->textobj->range(txt, pos);
				if (!text_range_valid(&r))
					break;
				if (a->textobj->type == OUTER) {
					r.start--;
					r.end++;
				}

				c.range = text_range_union(&c.range, &r);

				if (i < a->count - 1)
					pos = c.range.end + 1;
			}
		} else if (vis->mode->visual) {
			c.range = view_cursors_selection_get(cursor);
			if (!text_range_valid(&c.range))
				c.range.start = c.range.end = pos;
		}

		if (linewise && vis->mode != &vis_modes[VIS_MODE_VISUAL])
			c.range = text_range_linewise(txt, &c.range);
		if (vis->mode->visual) {
			view_cursors_selection_set(cursor, &c.range);
			if (vis->mode == &vis_modes[VIS_MODE_VISUAL] || a->textobj)
				view_cursors_selection_sync(cursor);
		}

		if (a->op) {
			size_t pos = a->op->func(vis, txt, &c);
			if (pos != EPOS) {
				view_cursors_to(cursor, pos);
			} else {
				view_cursors_dispose(cursor);
			}
		}
	}

	if (a->op) {
		if (a->op == &ops[OP_CHANGE])
			vis_mode_switch(vis, VIS_MODE_INSERT);
		else if (vis->mode == &vis_modes[VIS_MODE_OPERATOR])
			mode_set(vis, vis->mode_prev);
		else if (vis->mode->visual)
			vis_mode_switch(vis, VIS_MODE_NORMAL);
		text_snapshot(txt);
		vis_draw(vis);
	}

	if (a != &vis->action_prev) {
		if (a->op)
			vis->action_prev = *a;
		action_reset(vis, a);
	}
}

static void action_reset(Vis *vis, Action *a) {
	a->count = 0;
	a->type = 0;
	a->op = NULL;
	a->movement = NULL;
	a->textobj = NULL;
	a->reg = NULL;
}

static void mode_set(Vis *vis, Mode *new_mode) {
	if (vis->mode == new_mode)
		return;
	if (vis->mode->leave)
		vis->mode->leave(vis, new_mode);
	if (vis->mode->isuser)
		vis->mode_prev = vis->mode;
	vis->mode = new_mode;
	if (new_mode->enter)
		new_mode->enter(vis, vis->mode_prev);
	vis->win->ui->draw_status(vis->win->ui);
}

/** ':'-command implementations */


/* command recognized at the ':'-prompt. commands are found using a unique
 * prefix match. that is if a command should be available under an abbreviation
 * which is a prefix for another command it has to be added as an alias. the
 * long human readable name should always come first */
static Command cmds[] = {
	/* command name / optional alias, function,       options */
	{ { "bdelete"                  }, cmd_bdelete,    CMD_OPT_FORCE },
	{ { "edit"                     }, cmd_edit,       CMD_OPT_FORCE },
	{ { "help"                     }, cmd_help,       CMD_OPT_NONE  },
	{ { "new"                      }, cmd_new,        CMD_OPT_NONE  },
	{ { "open"                     }, cmd_open,       CMD_OPT_NONE  },
	{ { "qall"                     }, cmd_qall,       CMD_OPT_FORCE },
	{ { "quit", "q"                }, cmd_quit,       CMD_OPT_FORCE },
	{ { "read",                    }, cmd_read,       CMD_OPT_FORCE },
	{ { "saveas"                   }, cmd_saveas,     CMD_OPT_FORCE },
	{ { "set",                     }, cmd_set,        CMD_OPT_ARGS  },
	{ { "split"                    }, cmd_split,      CMD_OPT_NONE  },
	{ { "substitute", "s"          }, cmd_substitute, CMD_OPT_NONE  },
	{ { "vnew"                     }, cmd_vnew,       CMD_OPT_NONE  },
	{ { "vsplit",                  }, cmd_vsplit,     CMD_OPT_NONE  },
	{ { "wq",                      }, cmd_wq,         CMD_OPT_FORCE },
	{ { "write", "w"               }, cmd_write,      CMD_OPT_FORCE },
	{ { "xit",                     }, cmd_xit,        CMD_OPT_FORCE },
	{ { "earlier"                  }, cmd_earlier_later, CMD_OPT_NONE },
	{ { "later"                    }, cmd_earlier_later, CMD_OPT_NONE },
	{ { "!",                       }, cmd_filter,     CMD_OPT_NONE  },
	{ /* array terminator */                                        },
};

/* parse human-readable boolean value in s. If successful, store the result in
 * outval and return true. Else return false and leave outval alone. */
static bool parse_bool(const char *s, bool *outval) {
	for (const char **t = (const char*[]){"1", "true", "yes", "on", NULL}; *t; t++) {
		if (!strcasecmp(s, *t)) {
			*outval = true;
			return true;
		}
	}
	for (const char **f = (const char*[]){"0", "false", "no", "off", NULL}; *f; f++) {
		if (!strcasecmp(s, *f)) {
			*outval = false;
			return true;
		}
	}
	return false;
}

static bool cmd_set(Vis *vis, Filerange *range, enum CmdOpt cmdopt, const char *argv[]) {

	typedef struct {
		const char *names[3];
		enum {
			OPTION_TYPE_STRING,
			OPTION_TYPE_BOOL,
			OPTION_TYPE_NUMBER,
		} type;
		bool optional;
		int index;
	} OptionDef;

	enum {
		OPTION_AUTOINDENT,
		OPTION_EXPANDTAB,
		OPTION_TABWIDTH,
		OPTION_SYNTAX,
		OPTION_SHOW,
		OPTION_NUMBER,
		OPTION_NUMBER_RELATIVE,
		OPTION_CURSOR_LINE,
		OPTION_THEME,
		OPTION_COLOR_COLUMN,
	};

	/* definitions have to be in the same order as the enum above */
	static OptionDef options[] = {
		[OPTION_AUTOINDENT]      = { { "autoindent", "ai"       }, OPTION_TYPE_BOOL   },
		[OPTION_EXPANDTAB]       = { { "expandtab", "et"        }, OPTION_TYPE_BOOL   },
		[OPTION_TABWIDTH]        = { { "tabwidth", "tw"         }, OPTION_TYPE_NUMBER },
		[OPTION_SYNTAX]          = { { "syntax"                 }, OPTION_TYPE_STRING, true },
		[OPTION_SHOW]            = { { "show"                   }, OPTION_TYPE_STRING },
		[OPTION_NUMBER]          = { { "numbers", "nu"          }, OPTION_TYPE_BOOL   },
		[OPTION_NUMBER_RELATIVE] = { { "relativenumbers", "rnu" }, OPTION_TYPE_BOOL   },
		[OPTION_CURSOR_LINE]     = { { "cursorline", "cul"      }, OPTION_TYPE_BOOL   },
		[OPTION_THEME]           = { { "theme"                  }, OPTION_TYPE_STRING },
		[OPTION_COLOR_COLUMN]    = { { "colorcolumn", "cc"      }, OPTION_TYPE_NUMBER },
	};

	if (!vis->options) {
		if (!(vis->options = map_new()))
			return false;
		for (int i = 0; i < LENGTH(options); i++) {
			options[i].index = i;
			for (const char **name = options[i].names; *name; name++) {
				if (!map_put(vis->options, *name, &options[i]))
					return false;
			}
		}
	}

	if (!argv[1]) {
		vis_info_show(vis, "Expecting: set option [value]");
		return false;
	}

	Arg arg;
	bool invert = false;
	OptionDef *opt = NULL;

	if (!strncasecmp(argv[1], "no", 2)) {
		opt = map_closest(vis->options, argv[1]+2);
		if (opt && opt->type == OPTION_TYPE_BOOL)
			invert = true;
		else
			opt = NULL;
	}

	if (!opt)
		opt = map_closest(vis->options, argv[1]);
	if (!opt) {
		vis_info_show(vis, "Unknown option: `%s'", argv[1]);
		return false;
	}

	switch (opt->type) {
	case OPTION_TYPE_STRING:
		if (!opt->optional && !argv[2]) {
			vis_info_show(vis, "Expecting string option value");
			return false;
		}
		arg.s = argv[2];
		break;
	case OPTION_TYPE_BOOL:
		if (!argv[2]) {
			arg.b = true;
		} else if (!parse_bool(argv[2], &arg.b)) {
			vis_info_show(vis, "Expecting boolean option value not: `%s'", argv[2]);
			return false;
		}
		if (invert)
			arg.b = !arg.b;
		break;
	case OPTION_TYPE_NUMBER:
		if (!argv[2]) {
			vis_info_show(vis, "Expecting number");
			return false;
		}
		/* TODO: error checking? long type */
		arg.i = strtoul(argv[2], NULL, 10);
		break;
	}

	switch (opt->index) {
	case OPTION_EXPANDTAB:
		vis->expandtab = arg.b;
		break;
	case OPTION_AUTOINDENT:
		vis->autoindent = arg.b;
		break;
	case OPTION_TABWIDTH:
		tabwidth_set(vis, arg.i);
		break;
	case OPTION_SYNTAX:
		if (!argv[2]) {
			const char *syntax = view_syntax_get(vis->win->view);
			if (syntax)
				vis_info_show(vis, "Syntax definition in use: `%s'", syntax);
			else
				vis_info_show(vis, "No syntax definition in use");
			return true;
		}

		if (parse_bool(argv[2], &arg.b) && !arg.b)
			return view_syntax_set(vis->win->view, NULL);
		if (!view_syntax_set(vis->win->view, argv[2])) {
			vis_info_show(vis, "Unknown syntax definition: `%s'", argv[2]);
			return false;
		}
		break;
	case OPTION_SHOW:
		if (!argv[2]) {
			vis_info_show(vis, "Expecting: spaces, tabs, newlines");
			return false;
		}
		char *keys[] = { "spaces", "tabs", "newlines" };
		int values[] = {
			UI_OPTION_SYMBOL_SPACE,
			UI_OPTION_SYMBOL_TAB|UI_OPTION_SYMBOL_TAB_FILL,
			UI_OPTION_SYMBOL_EOL,
		};
		int flags = view_options_get(vis->win->view);
		for (const char **args = &argv[2]; *args; args++) {
			for (int i = 0; i < LENGTH(keys); i++) {
				if (strcmp(*args, keys[i]) == 0) {
					flags |= values[i];
				} else if (strstr(*args, keys[i]) == *args) {
					bool show;
					const char *v = *args + strlen(keys[i]);
					if (*v == '=' && parse_bool(v+1, &show)) {
						if (show)
							flags |= values[i];
						else
							flags &= ~values[i];
					}
				}
			}
		}
		view_options_set(vis->win->view, flags);
		break;
	case OPTION_NUMBER: {
		enum UiOption opt = view_options_get(vis->win->view);
		if (arg.b) {
			opt &= ~UI_OPTION_LINE_NUMBERS_RELATIVE;
			opt |=  UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		} else {
			opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		}
		view_options_set(vis->win->view, opt); 
		break;
	}
	case OPTION_NUMBER_RELATIVE: {
		enum UiOption opt = view_options_get(vis->win->view);
		if (arg.b) {
			opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
			opt |=  UI_OPTION_LINE_NUMBERS_RELATIVE;
		} else {
			opt &= ~UI_OPTION_LINE_NUMBERS_RELATIVE;
		}
		view_options_set(vis->win->view, opt); 
		break;
	}
	case OPTION_CURSOR_LINE: {
		enum UiOption opt = view_options_get(vis->win->view);
		if (arg.b)
			opt |= UI_OPTION_CURSOR_LINE;
		else
			opt &= ~UI_OPTION_CURSOR_LINE;
		view_options_set(vis->win->view, opt);
		break;
	}
	case OPTION_THEME:
		if (!vis_theme_load(vis, arg.s)) {
			vis_info_show(vis, "Failed to load theme: `%s'", arg.s);
			return false;
		}
		break;
	case OPTION_COLOR_COLUMN:
		view_colorcolumn_set(vis->win->view, arg.i);
		break;
	}

	return true;
}

static bool is_file_pattern(const char *pattern) {
	return pattern && (strcmp(pattern, ".") == 0 || strchr(pattern, '*') ||
	       strchr(pattern, '[') || strchr(pattern, '{'));
}

static const char *file_open_dialog(Vis *vis, const char *pattern) {
	if (!is_file_pattern(pattern))
		return pattern;
	/* this is a bit of a hack, we temporarily replace the text/view of the active
	 * window such that we can use cmd_filter as is */
	char vis_open[512];
	static char filename[PATH_MAX];
	Filerange range = text_range_empty();
	Win *win = vis->win;
	File *file = win->file;
	Text *txt_orig = file->text;
	View *view_orig = win->view;
	Text *txt = text_load(NULL);
	View *view = view_new(txt, NULL, NULL);
	filename[0] = '\0';
	snprintf(vis_open, sizeof(vis_open)-1, "vis-open %s", pattern ? pattern : "");

	if (!txt || !view)
		goto out;
	win->view = view;
	file->text = txt;

	if (cmd_filter(vis, &range, CMD_OPT_NONE, (const char *[]){ "open", vis_open, NULL })) {
		size_t len = text_size(txt);
		if (len >= sizeof(filename))
			len = 0;
		if (len > 0)
			text_bytes_get(txt, 0, --len, filename);
		filename[len] = '\0';
	}

out:
	view_free(view);
	text_free(txt);
	win->view = view_orig;
	file->text = txt_orig;
	return filename[0] ? filename : NULL;
}

static bool openfiles(Vis *vis, const char **files) {
	for (; *files; files++) {
		const char *file = file_open_dialog(vis, *files);
		if (!file)
			return false;
		errno = 0;
		if (!vis_window_new(vis, file)) {
			vis_info_show(vis, "Could not open `%s' %s", file,
			                 errno ? strerror(errno) : "");
			return false;
		}
	}
	return true;
}

static bool cmd_open(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (!argv[1])
		return vis_window_new(vis, NULL);
	return openfiles(vis, &argv[1]);
}

static bool is_view_closeable(Win *win) {
	if (!text_modified(win->file->text))
		return true;
	return win->file->refcount > 1;
}

static void info_unsaved_changes(Vis *vis) {
	vis_info_show(vis, "No write since last change (add ! to override)");
}

static bool cmd_edit(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	Win *oldwin = vis->win;
	if (!(opt & CMD_OPT_FORCE) && !is_view_closeable(oldwin)) {
		info_unsaved_changes(vis);
		return false;
	}
	if (!argv[1])
		return vis_window_reload(oldwin);
	if (!openfiles(vis, &argv[1]))
		return false;
	if (vis->win != oldwin)
		vis_window_close(oldwin);
	return vis->win != oldwin;
}

static bool cmd_quit(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (!(opt & CMD_OPT_FORCE) && !is_view_closeable(vis->win)) {
		info_unsaved_changes(vis);
		return false;
	}
	vis_window_close(vis->win);
	if (!vis->windows)
		vis_exit(vis, EXIT_SUCCESS);
	return true;
}

static bool cmd_xit(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (text_modified(vis->win->file->text) && !cmd_write(vis, range, opt, argv)) {
		if (!(opt & CMD_OPT_FORCE))
			return false;
	}
	return cmd_quit(vis, range, opt, argv);
}

static bool cmd_bdelete(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	Text *txt = vis->win->file->text;
	if (text_modified(txt) && !(opt & CMD_OPT_FORCE)) {
		info_unsaved_changes(vis);
		return false;
	}
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (win->file->text == txt)
			vis_window_close(win);
	}
	if (!vis->windows)
		vis_exit(vis, EXIT_SUCCESS);
	return true;
}

static bool cmd_qall(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (!text_modified(vis->win->file->text) || (opt & CMD_OPT_FORCE))
			vis_window_close(win);
	}
	if (!vis->windows)
		vis_exit(vis, EXIT_SUCCESS);
	else
		info_unsaved_changes(vis);
	return vis->windows == NULL;
}

static bool cmd_read(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	char cmd[255];

	if (!argv[1]) {
		vis_info_show(vis, "Filename or command expected");
		return false;
	}

	bool iscmd = (opt & CMD_OPT_FORCE) || argv[1][0] == '!';
	const char *arg = argv[1]+(argv[1][0] == '!');
	snprintf(cmd, sizeof cmd, "%s%s", iscmd ? "" : "cat ", arg);

	size_t pos = view_cursor_get(vis->win->view);
	if (!text_range_valid(range))
		*range = (Filerange){ .start = pos, .end = pos };
	Filerange delete = *range;
	range->start = range->end;

	bool ret = cmd_filter(vis, range, opt, (const char*[]){ argv[0], "sh", "-c", cmd, NULL});
	if (ret)
		text_delete_range(vis->win->file->text, &delete);
	return ret;
}

static bool cmd_substitute(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	char pattern[255];
	if (!text_range_valid(range))
		*range = (Filerange){ .start = 0, .end = text_size(vis->win->file->text) };
	snprintf(pattern, sizeof pattern, "s%s", argv[1]);
	return cmd_filter(vis, range, opt, (const char*[]){ argv[0], "sed", pattern, NULL});
}

static bool cmd_split(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	enum UiOption options = view_options_get(vis->win->view);
	windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	if (!argv[1])
		return vis_window_split(vis->win);
	bool ret = openfiles(vis, &argv[1]);
	view_options_set(vis->win->view, options);
	return ret;
}

static bool cmd_vsplit(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	enum UiOption options = view_options_get(vis->win->view);
	windows_arrange(vis, UI_LAYOUT_VERTICAL);
	if (!argv[1])
		return vis_window_split(vis->win);
	bool ret = openfiles(vis, &argv[1]);
	view_options_set(vis->win->view, options);
	return ret;
}

static bool cmd_new(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_vnew(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	windows_arrange(vis, UI_LAYOUT_VERTICAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_wq(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (cmd_write(vis, range, opt, argv))
		return cmd_quit(vis, range, opt, argv);
	return false;
}

static bool cmd_write(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	File *file = vis->win->file;
	Text *text = file->text;
	if (!text_range_valid(range))
		*range = (Filerange){ .start = 0, .end = text_size(text) };
	if (!argv[1])
		argv[1] = file->name;
	if (!argv[1]) {
		if (file->is_stdin) {
			if (strchr(argv[0], 'q')) {
				ssize_t written = text_write_range(text, range, STDOUT_FILENO);
				if (written == -1 || (size_t)written != text_range_size(range)) {
					vis_info_show(vis, "Can not write to stdout");
					return false;
				}
				/* make sure the file is marked as saved i.e. not modified */
				text_save_range(text, range, NULL);
				return true;
			}
			vis_info_show(vis, "No filename given, use 'wq' to write to stdout");
			return false;
		}
		vis_info_show(vis, "Filename expected");
		return false;
	}
	for (const char **name = &argv[1]; *name; name++) {
		struct stat meta;
		if (!(opt & CMD_OPT_FORCE) && file->stat.st_mtime && stat(*name, &meta) == 0 &&
		    file->stat.st_mtime < meta.st_mtime) {
			vis_info_show(vis, "WARNING: file has been changed since reading it");
			return false;
		}
		if (!text_save_range(text, range, *name)) {
			vis_info_show(vis, "Can't write `%s'", *name);
			return false;
		}
		if (!file->name) {
			window_name(vis->win, *name);
			file->name = vis->win->file->name;
		}
		if (strcmp(file->name, *name) == 0)
			file->stat = text_stat(text);
	}
	return true;
}

static bool cmd_saveas(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (cmd_write(vis, range, opt, argv)) {
		window_name(vis->win, argv[1]);
		vis->win->file->stat = text_stat(vis->win->file->text);
		return true;
	}
	return false;
}

static bool cmd_filter(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	/* if an invalid range was given, stdin (i.e. key board input) is passed
	 * through the external command. */
	Text *text = vis->win->file->text;
	View *view = vis->win->view;
	int pin[2], pout[2], perr[2], status = -1;
	bool interactive = !text_range_valid(range);
	size_t pos = view_cursor_get(view);

	if (pipe(pin) == -1)
		return false;
	if (pipe(pout) == -1) {
		close(pin[0]);
		close(pin[1]);
		return false;
	}

	if (pipe(perr) == -1) {
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		return false;
	}

	vis->ui->terminal_save(vis->ui);
	pid_t pid = fork();

	if (pid == -1) {
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		close(perr[0]);
		close(perr[1]);
		vis_info_show(vis, "fork failure: %s", strerror(errno));
		return false;
	} else if (pid == 0) { /* child i.e filter */
		if (!interactive)
			dup2(pin[0], STDIN_FILENO);
		close(pin[0]);
		close(pin[1]);
		dup2(pout[1], STDOUT_FILENO);
		close(pout[1]);
		close(pout[0]);
		if (!interactive)
			dup2(perr[1], STDERR_FILENO);
		close(perr[0]);
		close(perr[1]);
		if (!argv[2])
			execl("/bin/sh", "sh", "-c", argv[1], NULL);
		else
			execvp(argv[1], (char**)argv+1);
		vis_info_show(vis, "exec failure: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	vis->cancel_filter = false;

	close(pin[0]);
	close(pout[1]);
	close(perr[1]);

	fcntl(pout[0], F_SETFL, O_NONBLOCK);
	fcntl(perr[0], F_SETFL, O_NONBLOCK);

	if (interactive)
		*range = (Filerange){ .start = pos, .end = pos };

	/* ranges which are written to the filter and read back in */
	Filerange rout = *range;
	Filerange rin = (Filerange){ .start = range->end, .end = range->end };

	/* The general idea is the following:
	 *
	 *  1) take a snapshot
	 *  2) write [range.start, range.end] to exteneral command
	 *  3) read the output of the external command and insert it after the range
	 *  4) depending on the exit status of the external command
	 *     - on success: delete original range
	 *     - on failure: revert to previous snapshot
	 *
	 *  2) and 3) happend in small junks
	 */

	text_snapshot(text);

	fd_set rfds, wfds;
	Buffer errmsg;
	buffer_init(&errmsg);

	do {
		if (vis->cancel_filter) {
			kill(-pid, SIGTERM);
			vis_info_show(vis, "Command cancelled");
			break;
		}

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if (pin[1] != -1)
			FD_SET(pin[1], &wfds);
		if (pout[0] != -1)
			FD_SET(pout[0], &rfds);
		if (perr[0] != -1)
			FD_SET(perr[0], &rfds);

		if (select(FD_SETSIZE, &rfds, &wfds, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			vis_info_show(vis, "Select failure");
			break;
		}

		if (pin[1] != -1 && FD_ISSET(pin[1], &wfds)) {
			Filerange junk = *range;
			if (junk.end > junk.start + PIPE_BUF)
				junk.end = junk.start + PIPE_BUF;
			ssize_t len = text_write_range(text, &junk, pin[1]);
			if (len > 0) {
				range->start += len;
				if (text_range_size(range) == 0) {
					close(pout[1]);
					pout[1] = -1;
				}
			} else {
				close(pin[1]);
				pin[1] = -1;
				if (len == -1)
					vis_info_show(vis, "Error writing to external command");
			}
		}

		if (pout[0] != -1 && FD_ISSET(pout[0], &rfds)) {
			char buf[BUFSIZ];
			ssize_t len = read(pout[0], buf, sizeof buf);
			if (len > 0) {
				text_insert(text, rin.end, buf, len);
				rin.end += len;
			} else if (len == 0) {
				close(pout[0]);
				pout[0] = -1;
			} else if (errno != EINTR && errno != EWOULDBLOCK) {
				vis_info_show(vis, "Error reading from filter stdout");
				close(pout[0]);
				pout[0] = -1;
			}
		}

		if (perr[0] != -1 && FD_ISSET(perr[0], &rfds)) {
			char buf[BUFSIZ];
			ssize_t len = read(perr[0], buf, sizeof buf);
			if (len > 0) {
				buffer_append(&errmsg, buf, len);
			} else if (len == 0) {
				close(perr[0]);
				perr[0] = -1;
			} else if (errno != EINTR && errno != EWOULDBLOCK) {
				vis_info_show(vis, "Error reading from filter stderr");
				close(pout[0]);
				pout[0] = -1;
			}
		}

	} while (pin[1] != -1 || pout[0] != -1 || perr[0] != -1);

	if (pin[1] != -1)
		close(pin[1]);
	if (pout[0] != -1)
		close(pout[0]);
	if (perr[0] != -1)
		close(perr[0]);

	if (waitpid(pid, &status, 0) == pid && status == 0) {
		text_delete_range(text, &rout);
		text_snapshot(text);
	} else {
		/* make sure we have somehting to undo */
		text_insert(text, pos, " ", 1);
		text_undo(text);
	}

	view_cursor_to(view, rout.start);

	if (!vis->cancel_filter) {
		if (status == 0)
			vis_info_show(vis, "Command succeded");
		else if (errmsg.len > 0)
			vis_info_show(vis, "Command failed: %s", errmsg.data);
		else
			vis_info_show(vis, "Command failed");
	}

	vis->ui->terminal_restore(vis->ui);
	return status == 0;
}

static bool cmd_earlier_later(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	Text *txt = vis->win->file->text;
	char *unit = "";
	long count = 1;
	size_t pos = EPOS;
	if (argv[1]) {
		errno = 0;
		count = strtol(argv[1], &unit, 10);
		if (errno || unit == argv[1] || count < 0) {
			vis_info_show(vis, "Invalid number");
			return false;
		}

		if (*unit) {
			while (*unit && isspace((unsigned char)*unit))
				unit++;
			switch (*unit) {
			case 'd': count *= 24; /* fall through */
			case 'h': count *= 60; /* fall through */
			case 'm': count *= 60; /* fall through */
			case 's': break;
			default:
				vis_info_show(vis, "Unknown time specifier (use: s,m,h or d)");
				return false;
			}

			if (argv[0][0] == 'e')
				count = -count; /* earlier, move back in time */

			pos = text_restore(txt, text_state(txt) + count);
		}
	}

	if (!*unit) {
		if (argv[0][0] == 'e')
			pos = text_earlier(txt, count);
		else
			pos = text_later(txt, count);
	}

	time_t state = text_state(txt);
	char buf[32];
	strftime(buf, sizeof buf, "State from %H:%M", localtime(&state));
	vis_info_show(vis, "%s", buf);

	return pos != EPOS;
}

bool print_keybinding(const char *key, void *value, void *data) {
	Text *txt = (Text*)data;
	KeyBinding *binding = (KeyBinding*)value;
	const char *desc = binding->alias;
	if (!desc && binding->action)
		desc = binding->action->help;
	return text_appendf(txt, "  %-15s\t%s\n", key, desc ? desc : "");
}

static void print_mode(Mode *mode, Text *txt, bool recursive) {
	if (recursive && mode->parent)
		print_mode(mode->parent, txt, recursive);
	map_iterate(mode->bindings, print_keybinding, txt);
}

static bool cmd_help(Vis *vis, Filerange *range, enum CmdOpt opt, const char *argv[]) {
	if (!vis_window_new(vis, NULL))
		return false;
	
	Text *txt = vis->win->file->text;

	text_appendf(txt, "vis %s, compiled " __DATE__ " " __TIME__ "\n\n", VERSION);

	text_appendf(txt, " Modes\n\n");
	for (int i = 0; i < LENGTH(vis_modes); i++) {
		Mode *mode = &vis_modes[i];
		if (mode->help)
			text_appendf(txt, "  %-15s\t%s\n", mode->name, mode->help);
	}

	for (int i = 0; i < LENGTH(vis_modes); i++) {
		Mode *mode = &vis_modes[i];
		if (mode->isuser && !map_empty(mode->bindings)) {
			text_appendf(txt, "\n %s\n\n", mode->name);
			print_mode(mode, txt, i == VIS_MODE_NORMAL ||
				i == VIS_MODE_INSERT);
		}
	}
	
	text_appendf(txt, "\n Text objects\n\n");
	print_mode(&vis_modes[VIS_MODE_TEXTOBJ], txt, false);

	text_appendf(txt, "\n Motions\n\n");
	print_mode(&vis_modes[VIS_MODE_MOVE], txt, false);

	text_appendf(txt, "\n :-Commands\n\n");
	for (Command *cmd = cmds; cmd && cmd->name[0]; cmd++)
		text_appendf(txt, "  %s\n", cmd->name[0]);

	text_save(txt, NULL);
	return true;
}

static Filepos parse_pos(Win *win, char **cmd) {
	size_t pos = EPOS;
	View *view = win->view;
	Text *txt = win->file->text;
	Mark *marks = win->file->marks;
	switch (**cmd) {
	case '.':
		pos = text_line_begin(txt, view_cursor_get(view));
		(*cmd)++;
		break;
	case '$':
		pos = text_size(txt);
		(*cmd)++;
		break;
	case '\'':
		(*cmd)++;
		if ('a' <= **cmd && **cmd <= 'z')
			pos = text_mark_get(txt, marks[**cmd - 'a']);
		else if (**cmd == '<')
			pos = text_mark_get(txt, marks[MARK_SELECTION_START]);
		else if (**cmd == '>')
			pos = text_mark_get(txt, marks[MARK_SELECTION_END]);
		(*cmd)++;
		break;
	case '/':
		(*cmd)++;
		char *pattern_end = strchr(*cmd, '/');
		if (!pattern_end)
			return EPOS;
		*pattern_end++ = '\0';
		Regex *regex = text_regex_new();
		if (!regex)
			return EPOS;
		if (!text_regex_compile(regex, *cmd, 0)) {
			*cmd = pattern_end;
			pos = text_search_forward(txt, view_cursor_get(view), regex);
		}
		text_regex_free(regex);
		break;
	case '+':
	case '-':
	{
		CursorPos curspos = view_cursor_getpos(view);
		long long line = curspos.line + strtoll(*cmd, cmd, 10);
		if (line < 0)
			line = 0;
		pos = text_pos_by_lineno(txt, line);
		break;
	}
	default:
		if ('0' <= **cmd && **cmd <= '9')
			pos = text_pos_by_lineno(txt, strtoul(*cmd, cmd, 10));
		break;
	}

	return pos;
}

static Filerange parse_range(Win *win, char **cmd) {
	Text *txt = win->file->text;
	Filerange r = text_range_empty();
	Mark *marks = win->file->marks;
	char start = **cmd;
	switch (**cmd) {
	case '%':
		r.start = 0;
		r.end = text_size(txt);
		(*cmd)++;
		break;
	case '*':
		r.start = text_mark_get(txt, marks[MARK_SELECTION_START]);
		r.end = text_mark_get(txt, marks[MARK_SELECTION_END]);
		(*cmd)++;
		break;
	default:
		r.start = parse_pos(win, cmd);
		if (**cmd != ',') {
			if (start == '.')
				r.end = text_line_next(txt, r.start);
			return r;
		}
		(*cmd)++;
		r.end = parse_pos(win, cmd);
		break;
	}
	return r;
}

static Command *lookup_cmd(Vis *vis, const char *name) {
	if (!vis->cmds) {
		if (!(vis->cmds = map_new()))
			return NULL;
	
		for (Command *cmd = cmds; cmd && cmd->name[0]; cmd++) {
			for (const char **name = cmd->name; *name; name++)
				map_put(vis->cmds, *name, cmd);
		}
	}
	return map_closest(vis->cmds, name);
}

bool vis_cmd(Vis *vis, const char *cmdline) {
	enum CmdOpt opt = CMD_OPT_NONE;
	size_t len = strlen(cmdline);
	char *line = malloc(len+2);
	if (!line)
		return false;
	line = strncpy(line, cmdline, len+1);
	char *name = line;

	Filerange range = parse_range(vis->win, &name);
	if (!text_range_valid(&range)) {
		/* if only one position was given, jump to it */
		if (range.start != EPOS && !*name) {
			view_cursor_to(vis->win->view, range.start);
			free(line);
			return true;
		}

		if (name != line) {
			vis_info_show(vis, "Invalid range\n");
			free(line);
			return false;
		}
	}
	/* skip leading white space */
	while (*name == ' ')
		name++;
	char *param = name;
	while (*param && isalpha(*param))
		param++;

	if (*param == '!') {
		if (param != name) {
			opt |= CMD_OPT_FORCE;
			*param = ' ';
		} else {
			param++;
		}
	}

	memmove(param+1, param, strlen(param)+1);
	*param++ = '\0'; /* separate command name from parameters */

	Command *cmd = lookup_cmd(vis, name);
	if (!cmd) {
		vis_info_show(vis, "Not an editor command");
		free(line);
		return false;
	}

	char *s = param;
	const char *argv[32] = { name };
	for (int i = 1; i < LENGTH(argv); i++) {
		while (s && *s && *s == ' ')
			s++;
		if (s && !*s)
			s = NULL;
		argv[i] = s;
		if (!(cmd->opt & CMD_OPT_ARGS)) {
			/* remove trailing spaces */
			if (s) {
				while (*s) s++;
				while (*(--s) == ' ') *s = '\0';
			}
			s = NULL;
		}
		if (s && (s = strchr(s, ' ')))
			*s++ = '\0';
		/* strip out a single '!' argument to make ":q !" work */
		if (argv[i] && !strcmp(argv[i], "!")) {
			opt |= CMD_OPT_FORCE;
			i--;
		}
	}

	cmd->cmd(vis, &range, opt, argv);
	free(line);
	return true;
}

static bool prompt_cmd(Vis *vis, char type, const char *cmd) {
	if (!cmd || !cmd[0])
		return true;
	switch (type) {
	case '/':
		return vis_motion(vis, MOVE_SEARCH_FORWARD, cmd);
	case '?':
		return vis_motion(vis, MOVE_SEARCH_BACKWARD, cmd);
	case '+':
	case ':':
		return vis_cmd(vis, cmd);
	}
	return false;
}

void vis_die(Vis *vis, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vis->ui->die(vis->ui, msg, ap);
	va_end(ap);
}

const char *vis_key_next(Vis *vis, const char *keys) {
	TermKeyKey key;
	TermKey *termkey = vis->ui->termkey_get(vis->ui);
	const char *next = NULL;
	if (!keys)
		return NULL;
	/* first try to parse a special key of the form <Key> */
	if (*keys == '<' && (next = termkey_strpkey(termkey, keys+1, &key, TERMKEY_FORMAT_VIM)) && *next == '>')
		return next+1;
	if (*keys == '<') {
		const char *start = keys + 1, *end = start;
		while (*end && *end != '>')
			end++;
		if (end > start && end - start - 1 < 64 && *end == '>') {
			char key[64];
			memcpy(key, start, end - start);
			key[end - start] = '\0';
			if (map_get(vis->actions, key))
				return end + 1;
		}
	}
	while (!ISUTF8(*keys))
		keys++;
	return termkey_strpkey(termkey, keys, &key, TERMKEY_FORMAT_VIM);
}

static const char *vis_keys_raw(Vis *vis, Buffer *buf, const char *input) {
	char *keys = buf->data, *start = keys, *cur = keys, *end;
	bool prefix = false;
	KeyBinding *binding = NULL;
	
	while (cur && *cur) {

		if (!(end = (char*)vis_key_next(vis, cur))) {
			// XXX: can't parse key this should never happen, throw away remaining input
			buffer_truncate(buf);
			return input + strlen(input);
		}

		char tmp = *end;
		*end = '\0';
		prefix = false;
		binding = NULL;

		for (Mode *mode = vis->mode; mode && !binding && !prefix; mode = mode->parent) {
			binding = map_get(mode->bindings, start);
			/* "<" is never treated as a prefix because it is used to denote
			 * special key symbols */
			if (strcmp(cur, "<"))
				prefix = !binding && map_contains(mode->bindings, start);
		}

		*end = tmp;
		
		if (binding) { /* exact match */
			if (binding->action) {
				end = (char*)binding->action->func(vis, end, &binding->action->arg);
				if (!end)
					break;
				start = cur = end;
			} else if (binding->alias) {
				buffer_put0(buf, end);
				buffer_prepend0(buf, binding->alias);
				start = cur = buf->data;
			}
		} else if (prefix) { /* incomplete key binding? */
			cur = end;
		} else { /* no keybinding */
			KeyAction *action = NULL;
			if (start[0] == '<' && end[-1] == '>') {
				/* test for special editor key command */
				char tmp = end[-1];
				end[-1] = '\0';
				action = map_get(vis->actions, start+1);
				end[-1] = tmp;
				if (action) {
					end = (char*)action->func(vis, end, &action->arg);
					if (!end)
						break;
				}
			}
			if (!action && vis->mode->input)
				vis->mode->input(vis, start, end - start);
			start = cur = end;
		}
	}

	buffer_put0(buf, start);
	return input + (start - keys);
}

const char *vis_keys(Vis *vis, const char *input) {
	if (!input)
		return NULL;

	if (!buffer_append0(&vis->input_queue, input)) {
		buffer_truncate(&vis->input_queue);
		return NULL;
	}

	return vis_keys_raw(vis, &vis->input_queue, input);
}

static const char *getkey(Vis *vis) {
	const char *key = vis->ui->getkey(vis->ui);
	if (!key)
		return NULL;
	vis_info_hide(vis);
	if (vis->recording)
		macro_append(vis->recording, key);
	return key;
}

bool vis_signal_handler(Vis *vis, int signum, const siginfo_t *siginfo, const void *context) {
	switch (signum) {
	case SIGBUS:
		for (File *file = vis->files; file; file = file->next) {
			if (text_sigbus(file->text, siginfo->si_addr))
				file->truncated = true;
		}
		vis->sigbus = true;
		if (vis->running)
			siglongjmp(vis->sigbus_jmpbuf, 1);
		return true;
	case SIGINT:
		vis->cancel_filter = true;
		return true;
	}
	return false;
}

static void vis_args(Vis *vis, int argc, char *argv[]) {
	char *cmd = NULL;
	bool end_of_options = false;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && !end_of_options) {
			switch (argv[i][1]) {
			case '-':
				end_of_options = true;
				break;
			case 'v':
				vis_die(vis, "vis %s, compiled " __DATE__ " " __TIME__ "\n", VERSION);
				break;
			case '\0':
				break;
			default:
				vis_die(vis, "Unknown command option: %s\n", argv[i]);
				break;
			}
		} else if (argv[i][0] == '+') {
			cmd = argv[i] + (argv[i][1] == '/' || argv[i][1] == '?');
		} else if (!vis_window_new(vis, argv[i])) {
			vis_die(vis, "Can not load `%s': %s\n", argv[i], strerror(errno));
		} else if (cmd) {
			prompt_cmd(vis, cmd[0], cmd+1);
			cmd = NULL;
		}
	}

	if (!vis->windows) {
		if (!strcmp(argv[argc-1], "-")) {
			if (!vis_window_new(vis, NULL))
				vis_die(vis, "Can not create empty buffer\n");
			ssize_t len = 0;
			char buf[PIPE_BUF];
			File *file = vis->win->file;
			Text *txt = file->text;
			file->is_stdin = true;
			while ((len = read(STDIN_FILENO, buf, sizeof buf)) > 0)
				text_insert(txt, text_size(txt), buf, len);
			if (len == -1)
				vis_die(vis, "Can not read from stdin\n");
			text_snapshot(txt);
			int fd = open("/dev/tty", O_RDONLY);
			if (fd == -1)
				vis_die(vis, "Can not reopen stdin\n");
			dup2(fd, STDIN_FILENO);
			close(fd);
		} else if (!vis_window_new(vis, NULL)) {
			vis_die(vis, "Can not create empty buffer\n");
		}
		if (cmd)
			prompt_cmd(vis, cmd[0], cmd+1);
	}
}

int vis_run(Vis *vis, int argc, char *argv[]) {
	vis_args(vis, argc, argv);

	struct timespec idle = { .tv_nsec = 0 }, *timeout = NULL;

	sigset_t emptyset;
	sigemptyset(&emptyset);
	vis_draw(vis);
	vis->running = true;
	vis->exit_status = EXIT_SUCCESS;

	sigsetjmp(vis->sigbus_jmpbuf, 1);

	while (vis->running) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		if (vis->sigbus) {
			char *name = NULL;
			for (Win *next, *win = vis->windows; win; win = next) {
				next = win->next;
				if (win->file->truncated) {
					free(name);
					name = strdup(win->file->name);
					vis_window_close(win);
				}
			}
			if (!vis->windows)
				vis_die(vis, "WARNING: file `%s' truncated!\n", name ? name : "-");
			else
				vis_info_show(vis, "WARNING: file `%s' truncated!\n", name ? name : "-");
			vis->sigbus = false;
			free(name);
		}

		vis_update(vis);
		idle.tv_sec = vis->mode->idle_timeout;
		int r = pselect(1, &fds, NULL, NULL, timeout, &emptyset);
		if (r == -1 && errno == EINTR)
			continue;

		if (r < 0) {
			/* TODO save all pending changes to a ~suffixed file */
			vis_die(vis, "Error in mainloop: %s\n", strerror(errno));
		}

		if (!FD_ISSET(STDIN_FILENO, &fds)) {
			if (vis->mode->idle)
				vis->mode->idle(vis);
			timeout = NULL;
			continue;
		}

		TermKey *termkey = vis->ui->termkey_get(vis->ui);
		termkey_advisereadable(termkey);
		const char *key;

		while ((key = getkey(vis)))
			vis_keys(vis, key);

		if (vis->mode->idle)
			timeout = &idle;
	}
	return vis->exit_status;
}

bool vis_operator(Vis *vis, enum VisOperator id) {
	switch (id) {
	case OP_CASE_LOWER:
	case OP_CASE_UPPER:
	case OP_CASE_SWAP:
		vis->action.arg.i = id;
		id = OP_CASE_SWAP;
		break;
	case OP_CURSOR_SOL:
	case OP_CURSOR_EOL:
		vis->action.arg.i = id;
		id = OP_CURSOR_SOL;
		break;
	case OP_PUT_AFTER:
	case OP_PUT_AFTER_END:
	case OP_PUT_BEFORE:
	case OP_PUT_BEFORE_END:
		vis->action.arg.i = id;
		id = OP_PUT_AFTER;
		break;
	default:
		break;
	}
	if (id >= LENGTH(ops))
		return false;
	Operator *op = &ops[id];
	if (vis->mode->visual) {
		vis->action.op = op;
		action_do(vis, &vis->action);
		return true;
	}
	/* switch to operator mode inorder to make operator options and
	 * text-object available */
	vis_mode_switch(vis, VIS_MODE_OPERATOR);
	if (vis->action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		vis->action.type = LINEWISE;
		vis_motion(vis, MOVE_LINE_NEXT);
	} else {
		vis->action.op = op;
	}

	/* put is not a real operator, does not need a range to operate on */
	if (id == OP_PUT_AFTER)
		vis_motion(vis, MOVE_NOP);

	return true;
}

void vis_mode_switch(Vis *vis, enum VisMode mode) {
	mode_set(vis, &vis_modes[mode]);
}

bool vis_motion(Vis *vis, enum VisMotion motion, ...) {
	va_list ap;
	va_start(ap, motion);

	switch (motion) {
	case MOVE_WORD_START_NEXT:
		if (vis->action.op == &ops[OP_CHANGE])
			motion = MOVE_WORD_END_NEXT;
		break;
	case MOVE_LONGWORD_START_NEXT:
		if (vis->action.op == &ops[OP_CHANGE])
			motion = MOVE_LONGWORD_END_NEXT;
		break;
	case MOVE_SEARCH_FORWARD:
	case MOVE_SEARCH_BACKWARD:
	{
		const char *pattern = va_arg(ap, char*);
		if (text_regex_compile(vis->search_pattern, pattern, REG_EXTENDED)) {
			action_reset(vis, &vis->action);
			goto err;
		}
		if (motion == MOVE_SEARCH_FORWARD)
			motion = MOVE_SEARCH_NEXT;
		else
			motion = MOVE_SEARCH_PREV;
		break;
	}
	case MOVE_RIGHT_TO:
	case MOVE_LEFT_TO:
	case MOVE_RIGHT_TILL:
	case MOVE_LEFT_TILL:
	{
		const char *key = va_arg(ap, char*);
		if (!key)
			goto err;
		strncpy(vis->search_char, key, sizeof(vis->search_char));
		vis->search_char[sizeof(vis->search_char)-1] = '\0';
		vis->last_totill = motion;
		break;
	}
	case MOVE_TOTILL_REPEAT:
		if (!vis->last_totill)
			goto err;
		motion = vis->last_totill;
		break;
	case MOVE_TOTILL_REVERSE:
		switch (vis->last_totill) {
		case MOVE_RIGHT_TO:
			motion = MOVE_LEFT_TO;
			break;
		case MOVE_LEFT_TO:
			motion = MOVE_RIGHT_TO;
			break;
		case MOVE_RIGHT_TILL:
			motion = MOVE_LEFT_TILL;
			break;
		case MOVE_LEFT_TILL:
			motion = MOVE_RIGHT_TILL;
			break;
		default:
			goto err;
		}
		break;
	case MOVE_MARK:
	case MOVE_MARK_LINE:
	{
		int mark = va_arg(ap, int);
		if (MARK_a <= mark && mark < VIS_MARK_INVALID)
			vis->action.mark = mark;
		else
			goto err;
		break;
	}
	default:
		break;
	}

	vis->action.movement = &moves[motion];
	va_end(ap);
	action_do(vis, &vis->action);
	return true;
err:
	va_end(ap);
	return false;
}

void vis_textobject(Vis *vis, enum VisTextObject textobj) {
	if (textobj < LENGTH(textobjs)) {
		vis->action.textobj = &textobjs[textobj];
		action_do(vis, &vis->action);
	}
}

static Macro *macro_get(Vis *vis, enum VisMacro m) {
	if (m == VIS_MACRO_LAST_RECORDED)
		return vis->last_recording;
	if (m < LENGTH(vis->macros))
		return &vis->macros[m];
	return NULL;
}

bool vis_macro_record(Vis *vis, enum VisMacro id) {
	Macro *macro = macro_get(vis, id);
	if (vis->recording || !macro)
		return false;
	macro_reset(macro);
	vis->recording = macro;
	return true;
}

bool vis_macro_record_stop(Vis *vis) {
	if (!vis->recording)
		return false;
	/* XXX: hack to remove last recorded key, otherwise upon replay
	 * we would start another recording */
	if (vis->recording->len > 1) {
		vis->recording->len--;
		vis->recording->data[vis->recording->len-1] = '\0';
	}
	vis->last_recording = vis->recording;
	vis->recording = NULL;
	return true;
}

bool vis_macro_recording(Vis *vis) {
	return vis->recording;
}

bool vis_macro_replay(Vis *vis, enum VisMacro id) {
	Macro *macro = macro_get(vis, id);
	if (!macro || macro == vis->recording)
		return false;
	Buffer buf;
	buffer_init(&buf);
	buffer_put(&buf, macro->data, macro->len);
	vis_keys_raw(vis, &buf, macro->data);
	buffer_release(&buf);
	return true;
}

void vis_repeat(Vis *vis) {
	int count = vis->action.count;
	vis->action = vis->action_prev;
	if (count)
		vis->action.count = count;
	action_do(vis, &vis->action);
}

void vis_mark_set(Vis *vis, enum VisMark mark, size_t pos) {
	File *file = vis->win->file;
	if (mark < LENGTH(file->marks))
		file->marks[mark] = text_mark_set(file->text, pos);
}

void vis_motion_type(Vis *vis, enum VisMotionType type) {
	vis->action.type = type;
}

int vis_count_get(Vis *vis) {
	return vis->action.count;
}

void vis_count_set(Vis *vis, int count) {
	vis->action.count = count;
}

void vis_register_set(Vis *vis, enum VisRegister reg) {
	if (reg < LENGTH(vis->registers))
		vis->action.reg = &vis->registers[reg];
}

Register *vis_register_get(Vis *vis, enum VisRegister reg) {
	if (reg < LENGTH(vis->registers))
		return &vis->registers[reg];
	return NULL;
}

void vis_exit(Vis *vis, int status) {
	vis->running = false;
	vis->exit_status = status;
}

const char *vis_mode_status(Vis *vis) {
	return vis->mode->status;
}

void vis_insert_tab(Vis *vis) {
	const char *tab = expandtab(vis);
	vis_insert_key(vis, tab, strlen(tab));
}

static void copy_indent_from_previous_line(Win *win) {
	View *view = win->view;
	Text *text = win->file->text;
	size_t pos = view_cursor_get(view);
	size_t prev_line = text_line_prev(text, pos);
	if (pos == prev_line)
		return;
	size_t begin = text_line_begin(text, prev_line);
	size_t start = text_line_start(text, begin);
	size_t len = start-begin;
	char *buf = malloc(len);
	if (!buf)
		return;
	len = text_bytes_get(text, begin, len, buf);
	vis_insert_key(win->editor, buf, len);
	free(buf);
}

void vis_insert_nl(Vis *vis) {
	const char *nl;
	switch (text_newline_type(vis->win->file->text)) {
	case TEXT_NEWLINE_CRNL:
		nl = "\r\n";
		break;
	default:
		nl = "\n";
		break;
	}

	vis_insert_key(vis, nl, strlen(nl));

	if (vis->autoindent)
		copy_indent_from_previous_line(vis->win);
}

void vis_prompt_enter(Vis *vis) {
	char *s = vis_prompt_get(vis);
	/* it is important to switch back to the previous mode, which hides
	 * the prompt and more importantly resets vis->win to the currently
	 * focused editor window *before* anything is executed which depends
	 * on vis->win.
	 */
	mode_set(vis, vis->mode_before_prompt);
	if (s && *s && prompt_cmd(vis, vis->prompt_type, s) && vis->running)
		vis_mode_switch(vis, VIS_MODE_NORMAL);
	free(s);
	vis_draw(vis);
}

Text *vis_text(Vis *vis) {
	return vis->win->file->text;
}

View *vis_view(Vis *vis) {
	return vis->win->view;
}

Text *vis_file_text(File *file) {
	return file->text;
}

const char *vis_file_name(File *file) {
	return file->name;
}

bool vis_theme_load(Vis *vis, const char *name) {
	lua_State *L = vis->lua;
	if (!L)
		return false;
	/* package.loaded['themes/'..name] = nil
	 * require 'themes/'..name */
	lua_pushstring(L, "themes/");
	lua_pushstring(L, name);
	lua_concat(L, 2);
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");
	lua_pushvalue(L, -3);
	lua_pushnil(L);
	lua_settable(L, -3);
	lua_pop(L, 2);
	lua_getglobal(L, "require");
	lua_pushvalue(L, -2);
	if (lua_pcall(L, 1, 0, 0))
		return false;
	for (Win *win = vis->windows; win; win = win->next)
		view_syntax_set(win->view, view_syntax_get(win->view));
	return true;
}
