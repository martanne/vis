#define _POSIX_SOURCE
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "vis.h"
#include "util.h"

#ifdef PDCURSES
int ESCDELAY;
#endif
#ifndef NCURSES_REENTRANT
# define set_escdelay(d) (ESCDELAY = (d))
#endif

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
	bool common_prefix;
	void (*enter)(Mode *old);
	void (*leave)(Mode *new);
	bool (*unknown)(Key *key0, Key *key1);        /* unknown key for this mode, return value determines whether parent modes will be checked */ 
	void (*input)(const char *str, size_t len);   /* unknown key for this an all parent modes */
	void (*idle)(void);
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
	bool linewise;
} OperatorContext;

typedef struct {
	void (*func)(OperatorContext*); /* function implementing the operator logic */
	bool selfcontained;             /* is this operator followed by movements/text-objects */
} Operator;

typedef struct {
	size_t (*cmd)(const Arg*);
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
	Mark mark;
	Key key;
	Arg arg;
} Action;

typedef struct {
	const char *name;
	bool (*cmd)(const char *argv[]);
	regex_t regex;
} Command;

static Key getkey(void);
static void cursor(const Arg *arg);
static void call(const Arg *arg);

#include "config.h"

static Config *config = &editors[0];

static void cursor(const Arg *arg) {
	arg->m(vis->win->win);
}

static void call(const Arg *arg) {
	arg->f(vis);
}

typedef struct Screen Screen;
static struct Screen {
	int w, h;
	bool need_resize;
} screen = { .need_resize = true };

static void sigwinch_handler(int sig) {
	screen.need_resize = true;
}

static void resize_screen(Screen *screen) {
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) == -1) {
		getmaxyx(stdscr, screen->h, screen->w);
	} else {
		screen->w = ws.ws_col;
		screen->h = ws.ws_row;
	}

	resizeterm(screen->h, screen->w);
	wresize(stdscr, screen->h, screen->w);
	screen->need_resize = false;
}

static void setup() {
	setlocale(LC_CTYPE, "");
	if (!getenv("ESCDELAY"))
		set_escdelay(50);
	initscr();
	start_color();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	resize_screen(&screen);
	/* needed because we use getch() which implicitly calls refresh() which
	   would clear the screen (overwrite it with an empty / unused stdscr */
	refresh();

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);
}

static void cleanup() {
	endwin();
	//delscreen(set_term(NULL));
}

static bool keymatch(Key *key0, Key *key1) {
	return (key0->str[0] && memcmp(key0->str, key1->str, sizeof(key1->str)) == 0) ||
	       (key0->code && key0->code == key1->code);
}

static KeyBinding *keybinding(Mode *mode, Key *key0, Key *key1) {
	for (; mode; mode = mode->parent) {
		if (mode->common_prefix && !keymatch(key0, &mode->bindings->key[0]))
			continue;
		for (KeyBinding *kb = mode->bindings; kb && (kb->key[0].code || kb->key[0].str[0]); kb++) {
			if (keymatch(key0, &kb->key[0]) && (!key1 || keymatch(key1, &kb->key[1])))
				return kb;
		}
		if (mode->unknown && !mode->unknown(key0, key1))
			break;
	}
	return NULL;
}

static Key getkey(void) {
	Key key = { .str = "\0\0\0\0\0\0", .code = 0 };
	int keycode = getch();
	if (keycode == ERR)
		return key;

	// TODO verbatim insert mode
	int len = 0;
	if (keycode >= KEY_MIN) {
		key.code = keycode;
	} else {
		char keychar = keycode;
		key.str[len++] = keychar;

		if (!ISASCII(keychar) || keychar == '\e') {
			nodelay(stdscr, TRUE);
			for (int t; len < LENGTH(key.str) && (t = getch()) != ERR; len++)
				key.str[len] = t;
			nodelay(stdscr, FALSE);
		}
	}

	return key;
}

int main(int argc, char *argv[]) {
	/* decide which key configuration to use based on argv[0] */
	char *arg0 = argv[0];
	while (*arg0 && (*arg0 == '.' || *arg0 == '/'))
		arg0++;
	for (int i = 0; i < LENGTH(editors); i++) {
		if (editors[i].name[0] == arg0[0]) {
			config = &editors[i];
			break;
		}
	}

	mode_prev = mode = config->mode;
	setup();

	if (!(vis = vis_new(screen.w, screen.h)))
		return 1;
	if (!vis_syntax_load(vis, syntaxes, colors))
		return 1;
	vis_statusbar_set(vis, config->statusbar);

	if (!vis_window_new(vis, argc > 1 ? argv[1] : NULL))
		return 1;
	for (int i = 2; i < argc; i++) {
		if (!vis_window_new(vis, argv[i]))
			return 1;
	}

	struct timeval idle = { .tv_usec = 0 }, *timeout = NULL;
	Key key, key_prev, *key_mod = NULL;

	while (vis->running) {
		if (screen.need_resize) {
			resize_screen(&screen);
			vis_resize(vis, screen.w, screen.h);
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		vis_update(vis);
		doupdate();
		idle.tv_sec = 3;
		int r = select(1, &fds, NULL, NULL, timeout);
		if (r == -1 && errno == EINTR)
			continue;

		if (r < 0) {
			perror("select()");
			exit(EXIT_FAILURE);
		}

		if (!FD_ISSET(STDIN_FILENO, &fds)) {
			if (mode->idle)
				mode->idle();
			timeout = NULL;
			continue;
		}

		key = getkey();
		KeyBinding *action = keybinding(mode, key_mod ? key_mod : &key, key_mod ? &key : NULL);

		if (!action && key_mod) {
			/* second char of a combination was invalid, search again without the prefix */ 
			action = keybinding(mode, &key, NULL);
			key_mod = NULL;
		}
		if (action) {
			/* check if it is the first part of a combination */
			if (!key_mod && (action->key[1].code || action->key[1].str[0])) {
				key_prev = key;
				key_mod = &key_prev;
				continue;
			}
			action->func(&action->arg);
			key_mod = NULL;
			continue;
		}

		if (key.code)
			continue;
		
		if (mode->input)
			mode->input(key.str, strlen(key.str));
		if (mode->idle)
			timeout = &idle;
	}

	vis_free(vis);
	cleanup();
	return 0;
}
