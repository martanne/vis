#define _POSIX_SOURCE
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "editor.h"
#include "util.h"

#ifdef PDCURSES
int ESCDELAY;
#endif
#ifndef NCURSES_REENTRANT
# define set_escdelay(d) (ESCDELAY = (d))
#endif

typedef union {
	size_t i;
	const char *s;
	size_t (*m)(Editor*);
	void (*f)(Editor*);
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
	bool (*input)(const char *str, size_t len);
};

typedef struct {
	char *name;
	Mode *mode;
} Config;

static void cursor(const Arg *arg);
static void call(const Arg *arg);
static void insert(const Arg *arg);
static void line(const Arg *arg);
static void find_forward(const Arg *arg);
static void find_backward(const Arg *arg);

static Mode *mode;
static Editor *editor;

#include "config.h"

static Config *config = &editors[0];

static void cursor(const Arg *arg) {
	arg->m(editor);
}

static void call(const Arg *arg) {
	arg->f(editor);
}

static void line(const Arg *arg) {
	editor_line_goto(editor, arg->i);
}

static void find_forward(const Arg *arg) {
	editor_search(editor, arg->s, 1);
}

static void find_backward(const Arg *arg) {
	editor_search(editor, arg->s, -1);
}

static void insert(const Arg *arg) {
	editor_insert(editor, arg->s, strlen(arg->s));
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
	editor_free(editor);
	endwin();
}

static bool keymatch(Key *key0, Key *key1) {
	return (key0->str[0] && memcmp(key0->str, key1->str, sizeof(key1->str)) == 0) ||
	       (key0->code && key0->code == key1->code);
}

static KeyBinding *keybinding(Mode *mode, Key *key0, Key *key1) {
	for (; mode; mode = mode->parent) {
		for (KeyBinding *kb = mode->bindings; kb && (kb->key[0].code || kb->key[0].str[0]); kb++) {
			if (keymatch(key0, &kb->key[0]) && (!key1 || keymatch(key1, &kb->key[1])))
				return kb;
		}
	}
	return NULL;
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
	mode = config->mode;

	setup();
	if (!(editor = editor_new(screen.w, screen.h, statusbar)))
		return 1;
	if (!editor_syntax_load(editor, syntaxes, colors))
		return 1;
	char *filename = argc > 1 ? argv[1] : NULL;
	if (!editor_load(editor, filename))
		return 1;

	struct timeval idle = { .tv_usec = 0 }, *timeout = NULL;
	Key key, key_prev, *key_mod = NULL;

	for (;;) {
		if (screen.need_resize) {
			resize_screen(&screen);
			editor_resize(editor, screen.w, screen.h);
		}

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		editor_update(editor);
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
			editor_snapshot(editor);
			timeout = NULL;
			continue;
		}

		int keycode = getch();
		if (keycode == ERR)
			continue;

		// TODO verbatim insert mode
		int len = 0;
		if (keycode >= KEY_MIN) {
			key.code = keycode;
			key.str[0] = '\0';
		} else {
			char keychar = keycode;
			key.str[len++] = keychar;
			key.code = 0;

			if (!ISASCII(keychar) || keychar == '\e') {
				nodelay(stdscr, TRUE);
				for (int t; len < LENGTH(key.str) && (t = getch()) != ERR; len++)
					key.str[len] = t;
				nodelay(stdscr, FALSE);
			}
		}

		for (size_t i = len; i < LENGTH(key.str); i++)
			key.str[i] = '\0';

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

		if (keycode >= KEY_MIN)
			continue;
		
		if (mode->input && mode->input(key.str, len))
			timeout = &idle;
	}

	cleanup();
	return 0;
}
