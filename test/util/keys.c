#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <termkey.h>

static TermKey *termkey;

static void die(const char *errstr, ...) {
        va_list ap;
        va_start(ap, errstr);
        vfprintf(stderr, errstr, ap);
        va_end(ap);
        exit(EXIT_FAILURE);
}

static void print(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stdout, fmt, ap);
	fflush(stdout);
        va_end(ap);
}

static void wait(void) {
	usleep(termkey_get_waittime(termkey)*10000);
}

static void printkey(TermKeyKey *key) {
	switch (key->type) {
	case TERMKEY_TYPE_UNICODE:
		if (key->modifiers & TERMKEY_KEYMOD_SHIFT)
			;
		if (key->modifiers & TERMKEY_KEYMOD_CTRL)
			key->utf8[0] &= 0x1f;
		if (key->modifiers & TERMKEY_KEYMOD_ALT)
			;
		print("%s", key->utf8);
		break;
	case TERMKEY_TYPE_FUNCTION:
		break;
	case TERMKEY_TYPE_KEYSYM:
		switch (key->code.sym) {
		case TERMKEY_SYM_UNKNOWN:
		case TERMKEY_SYM_NONE:
			die("Unknown key sym\n");
		case TERMKEY_SYM_BACKSPACE:
			print("\b");
			break;
		case TERMKEY_SYM_TAB:
			print("\t");
			break;
		case TERMKEY_SYM_ENTER:
			print("\n");
			break;
		case TERMKEY_SYM_ESCAPE:
			print("\033");
			wait();
			break;
		case TERMKEY_SYM_SPACE:
			print(" ");
			break;
		case TERMKEY_SYM_UP:
			print("\033OA");
			break;
		case TERMKEY_SYM_DOWN:
			print("\033OB");
			break;
		case TERMKEY_SYM_RIGHT:
			print("\033OC");
			break;
		case TERMKEY_SYM_LEFT:
			print("\033OD");
			break;
		case TERMKEY_SYM_DEL:
		case TERMKEY_SYM_BEGIN:
		case TERMKEY_SYM_FIND:
		case TERMKEY_SYM_INSERT:
		case TERMKEY_SYM_DELETE:
		case TERMKEY_SYM_SELECT:
		case TERMKEY_SYM_PAGEUP:
		case TERMKEY_SYM_PAGEDOWN:
		case TERMKEY_SYM_HOME:
		case TERMKEY_SYM_END:
		case TERMKEY_SYM_CANCEL:
		case TERMKEY_SYM_CLEAR:
		case TERMKEY_SYM_CLOSE:
		case TERMKEY_SYM_COMMAND:
		case TERMKEY_SYM_COPY:
		case TERMKEY_SYM_EXIT:
		case TERMKEY_SYM_HELP:
		case TERMKEY_SYM_MARK:
		case TERMKEY_SYM_MESSAGE:
		case TERMKEY_SYM_MOVE:
		case TERMKEY_SYM_OPEN:
		case TERMKEY_SYM_OPTIONS:
		case TERMKEY_SYM_PRINT:
		case TERMKEY_SYM_REDO:
		case TERMKEY_SYM_REFERENCE:
		case TERMKEY_SYM_REFRESH:
		case TERMKEY_SYM_REPLACE:
		case TERMKEY_SYM_RESTART:
		case TERMKEY_SYM_RESUME:
		case TERMKEY_SYM_SAVE:
		case TERMKEY_SYM_SUSPEND:
		case TERMKEY_SYM_UNDO:
		case TERMKEY_SYM_KP0:
		case TERMKEY_SYM_KP1:
		case TERMKEY_SYM_KP2:
		case TERMKEY_SYM_KP3:
		case TERMKEY_SYM_KP4:
		case TERMKEY_SYM_KP5:
		case TERMKEY_SYM_KP6:
		case TERMKEY_SYM_KP7:
		case TERMKEY_SYM_KP8:
		case TERMKEY_SYM_KP9:
		case TERMKEY_SYM_KPENTER:
		case TERMKEY_SYM_KPPLUS:
		case TERMKEY_SYM_KPMINUS:
		case TERMKEY_SYM_KPMULT:
		case TERMKEY_SYM_KPDIV:
		case TERMKEY_SYM_KPCOMMA:
		case TERMKEY_SYM_KPPERIOD:
		case TERMKEY_SYM_KPEQUALS:
			break;
		}
		break;
	}
}

int main(int argc, char *argv[]) {
	char buf[1024];
	FILE *file = stdin;
	char *term = getenv("TERM");
	if (!term)
		term = "xterm";
	if (!(termkey = termkey_new_abstract(term, TERMKEY_FLAG_UTF8)))
		die("Failed to initialize libtermkey\n");
	while (fgets(buf, sizeof buf, file)) {
		TermKeyKey key;
		const char *keys = buf, *next;
		while (*keys) {
			if (*keys == '\n') {
				keys++;
			} else if (*keys == '<' && (next = termkey_strpkey(termkey, keys+1, &key, TERMKEY_FORMAT_VIM)) && *next == '>') {
				printkey(&key);
				keys = next+1;
			} else if ((next = termkey_strpkey(termkey, keys, &key, TERMKEY_FORMAT_VIM))) {
				printkey(&key);
				keys = next;
			} else {
				die("Failed to parse keys: %s\n", keys);
			}
		}
	}

	return 0;
}
