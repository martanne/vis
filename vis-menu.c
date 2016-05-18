/*
 * MIT/X Consortium License
 *
 * © 2011 Rafael Garcia Gallego <rafael.garcia.gallego@gmail.com>
 *
 * Based on dmenu:
 * © 2010-2011 Connor Lane Smith <cls@lubutu.com>
 * © 2006-2011 Anselm R Garbe <anselm@garbe.us>
 * © 2009 Gottox <gottox@s01.de>
 * © 2009 Markus Schnalke <meillo@marmaro.de>
 * © 2009 Evan Gates <evan.gates@gmail.com>
 * © 2006-2008 Sander van Dijk <a dot h dot vandijk at gmail dot com>
 * © 2006-2007 Michał Janeczek <janeczek at gmail dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define CONTROL(ch)   (ch ^ 0x40)
#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))
#define FALSE 0
#define TRUE  1

enum Color {
	C_Normal,
	C_Reverse
};
typedef enum Color Color;

typedef struct Item Item;
struct Item {
	char *text;
	Item *left, *right;
};

static void   appenditem(Item*, Item**, Item**);
static void   calcoffsets(void);
static void   cleanup(void);
static void   die(const char*);
static void   drawtext(const char*, size_t, Color);
static void   drawmenu(void);
static char  *fstrstr(const char*, const char*);
static void   insert(const char*, ssize_t);
static void   match(int);
static size_t nextrune(int);
static void   readstdin(void);
static int    run(void);
static void   resetline(void);
static void   setup(void);
static size_t textw(const char*);
static size_t textwn(const char*, int);

static char   text[BUFSIZ] = "";
static int    barpos = 0;
static int    mw, mh;
static int    lines = 0;
static int    inputw, promptw;
static size_t cursor;
static char  *prompt = NULL;
static Item  *items = NULL;
static Item  *matches, *matchend;
static Item  *prev, *curr, *next, *sel;
static struct termios tio_old, tio_new;
static int  (*fstrncmp)(const char *, const char *, size_t) = strncmp;

void
appenditem(Item *item, Item **list, Item **last) {
	if(!*last)
		*list = item;
	else
		(*last)->right = item;
	item->left = *last;
	item->right = NULL;
	*last = item;
}

void
calcoffsets(void) {
        int i, n;

	if(lines>0)
		n = lines;
	else
		n = mw - (promptw + inputw + textw("<") + textw(">"));

        for(i = 0, next = curr; next; next = next->right)
                if((i += (lines>0 ? 1 : MIN(textw(next->text), n))) > n)
                        break;
        for(i = 0, prev = curr; prev && prev->left; prev = prev->left)
                if((i += (lines>0 ? 1 : MIN(textw(prev->left->text), n))) > n)
                        break;
}

void
cleanup() {
	if(barpos==0) fprintf(stderr, "\n");
	else fprintf(stderr, "\033[G\033[K");
	tcsetattr(0, TCSANOW, &tio_old);
}

void
die(const char *s) {
	tcsetattr(0, TCSANOW, &tio_old);
	fprintf(stderr, "%s\n", s);
	exit(1);
}

void
drawtext(const char *t, size_t w, Color col) {
	const char *prestr, *poststr;
	int i, tw;
	char *buf;

	if(w<5) return; /* This is the minimum size needed to write a label: 1 char + 4 padding spaces */
	tw=w-4; /* This is the text width, without the padding */
	if((buf=calloc(1, (tw+1))) == NULL) die("Can't calloc.");
	switch(col) {
	case C_Reverse:
		prestr="\033[7m";
		poststr="\033[0m";
		break;
	case C_Normal:
	default:
		prestr=poststr="";
	}

	memset(buf, ' ', tw);
	buf[tw]='\0';
	memcpy(buf, t, MIN(strlen(t), tw));
	if(textw(t)>w) /* Remember textw returns the width WITH padding */
		for(i=MAX((tw-4), 0); i<tw; i++) buf[i]='.';

	fprintf(stderr, "%s  %s  %s", prestr, buf, poststr);
	free(buf);
}

void
drawmenu(void) {
	Item *item;
	int rw;

	/* use default colors */
	fprintf(stderr, "\033[0m");

	/* place cursor in first column, clear it */
	fprintf(stderr, "\033[0G");
	fprintf(stderr, "\033[K");

	if(prompt)
		drawtext(prompt, promptw, C_Reverse);

	drawtext(text, ((lines==0 && matches)?inputw:mw-promptw), C_Normal);

	if(lines>0) {
		if(barpos!=0) resetline();
		for(rw=0, item = curr; item != next; rw++, item = item->right) {
			fprintf(stderr, "\n");
			drawtext(item->text, mw, (item == sel) ? C_Reverse : C_Normal);
		}
		for(; rw<lines; rw++)
			fprintf(stderr, "\n\033[K");
		resetline();
	} else if(matches) {
		rw=mw-(4+promptw+inputw);
		if(curr->left)
			drawtext("<", 5 /*textw("<")*/, C_Normal);
		for(item = curr; item != next; item = item->right) {
			drawtext(item->text, MIN(textw(item->text), rw), (item == sel) ? C_Reverse : C_Normal);
			if((rw-= textw(item->text)) <= 0) break;
		}
		if(next) {
			fprintf(stderr, "\033[%iG", mw-5);
			drawtext(">", 5 /*textw(">")*/, C_Normal);
		}

	}
	fprintf(stderr, "\033[%iG", (int)(promptw+textwn(text, cursor)-1));
}

char*
fstrstr(const char *s, const char *sub) {
	size_t len;

	for(len = strlen(sub); *s; s++)
		if(!fstrncmp(s, sub, len))
			return (char *)s;
	return NULL;
}

void
insert(const char *str, ssize_t n) {
	if(strlen(text) + n > sizeof text - 1)
		return;
	memmove(&text[cursor + n], &text[cursor], sizeof text - cursor - MAX(n, 0));
	if(n > 0)
		memcpy(&text[cursor], str, n);
	cursor += n;
	match(n > 0 && text[cursor] == '\0');
}

void
match(int sub) {
	size_t len = strlen(text);
	Item *lexact, *lprefix, *lsubstr, *exactend, *prefixend, *substrend;
	Item *item, *lnext;

	lexact = lprefix = lsubstr = exactend = prefixend = substrend = NULL;
	for(item = sub ? matches : items; item && item->text; item = lnext) {
		lnext = sub ? item->right : item + 1;
		if(!fstrncmp(text, item->text, len + 1))
			appenditem(item, &lexact, &exactend);
		else if(!fstrncmp(text, item->text, len))
			appenditem(item, &lprefix, &prefixend);
		else if(fstrstr(item->text, text))
			appenditem(item, &lsubstr, &substrend);
	}
	matches = lexact;
	matchend = exactend;

	if(lprefix) {
		if(matchend) {
			matchend->right = lprefix;
			lprefix->left = matchend;
		}
		else
			matches = lprefix;
		matchend = prefixend;
	}
	if(lsubstr) {
		if(matchend) {
			matchend->right = lsubstr;
			lsubstr->left = matchend;
		}
		else
			matches = lsubstr;
		matchend = substrend;
	}
	curr = sel = matches;
	calcoffsets();
}

size_t
nextrune(int inc) {
	ssize_t n;

	for(n = cursor + inc; n + inc >= 0 && (text[n] & 0xc0) == 0x80; n += inc);
	return n;
}

void
readstdin() {
	char buf[sizeof text], *p, *maxstr = NULL;
	size_t i, max = 0, size = 0;

	for(i = 0; fgets(buf, sizeof buf, stdin); i++) {
		if(i+1 >= size / sizeof *items)
			if(!(items = realloc(items, (size += BUFSIZ))))
				die("Can't realloc.");
		if((p = strchr(buf, '\n')))
			*p = '\0';
		if(!(items[i].text = strdup(buf)))
			die("Can't strdup.");
		if(strlen(items[i].text) > max)
			max = textw(maxstr = items[i].text);
	}
	if(items)
		items[i].text = NULL;
	inputw = textw(maxstr);
}

void
resetline(void) {
	if(barpos!=0) fprintf(stderr, "\033[%iH", (barpos>0)?0:(mh-lines));
	else fprintf(stderr, "\033[%iF", lines);
}

int
run(void) {
	char buf[32];
	char c;
	FILE *f;
	int n;

	while(1) {
		read(0, &c, 1);
		memset(buf, '\0', sizeof buf);
		buf[0]=c;
		switch_top:
		switch(c) {
		case CONTROL('['):
			read(0, &c, 1);
			esc_switch_top:
			switch(c) {
				case CONTROL('['): /* ESC, need to press twice due to console limitations */
					c=CONTROL('C');
					goto switch_top;
				case '[':
					read(0, &c, 1);
					switch(c) {
						case '1': /* Home */
						case '7':
						case 'H':
							if(c!='H') read(0, &c, 1); /* Remove trailing '~' from stdin */
							c=CONTROL('A');
							goto switch_top;
						case '2': /* Insert */
							read(0, &c, 1); /* Remove trailing '~' from stdin */
							c=CONTROL('Y');
							goto switch_top;
						case '3': /* Delete */
							read(0, &c, 1); /* Remove trailing '~' from stdin */
							c=CONTROL('D');
							goto switch_top;
						case '4': /* End */
						case '8':
						case 'F':
							if(c!='F') read(0, &c, 1); /* Remove trailing '~' from stdin */
							c=CONTROL('E');
							goto switch_top;
						case '5': /* PageUp */
							read(0, &c, 1); /* Remove trailing '~' from stdin */
							c=CONTROL('V');
							goto switch_top;
						case '6': /* PageDown */
							read(0, &c, 1); /* Remove trailing '~' from stdin */
							c='v';
							goto esc_switch_top;
						case 'A': /* Up arrow */
							c=CONTROL('P');
							goto switch_top;
						case 'B': /* Down arrow */
							c=CONTROL('N');
							goto switch_top;
						case 'C': /* Right arrow */
							c=CONTROL('F');
							goto switch_top;
						case 'D': /* Left arrow */
							c=CONTROL('B');
							goto switch_top;
					}
					break;
				case 'b':
					while(cursor > 0 && text[nextrune(-1)] == ' ')
						cursor = nextrune(-1);
					while(cursor > 0 && text[nextrune(-1)] != ' ')
						cursor = nextrune(-1);
					break;
				case 'f':
					while(text[cursor] != '\0' && text[nextrune(+1)] == ' ')
						cursor = nextrune(+1);
					if(text[cursor] != '\0') do
						cursor = nextrune(+1);
					while(text[cursor] != '\0' && text[cursor] != ' ');
					break;
				case 'd':
					while(text[cursor] != '\0' && text[nextrune(+1)] == ' ') {
						cursor = nextrune(+1);
						insert(NULL, nextrune(-1) - cursor);
					}
					if(text[cursor] != '\0') do {
						cursor = nextrune(+1);
						insert(NULL, nextrune(-1) - cursor);
					} while(text[cursor] != '\0' && text[cursor] != ' ');
					break;
				case 'v':
					if(!next)
						break;
					sel=curr=next;
					calcoffsets();
					break;
				default:
					break;
			}
			break;
		case CONTROL('C'):
			return EXIT_FAILURE;
		case CONTROL('M'): /* Return */
		case CONTROL('J'):
			if(sel) strncpy(text, sel->text, sizeof text); /* Complete the input first, when hitting return */
			cursor = strlen(text);
			match(TRUE);
			drawmenu();
			/* fallthrough */
		case CONTROL(']'):
		case CONTROL('\\'): /* These are usually close enough to RET to replace Shift+RET, again due to console limitations */
			puts(text);
			return EXIT_SUCCESS;
		case CONTROL('A'):
			if(sel == matches) {
				cursor=0;
				break;
			}
			sel=curr=matches;
			calcoffsets();
			break;
		case CONTROL('E'):
			if(text[cursor] != '\0') {
				cursor = strlen(text);
				break;
			}
			if(next) {
				curr = matchend;
				calcoffsets();
				curr = prev;
				calcoffsets();
				while(next && (curr = curr->right))
					calcoffsets();
			}
			sel = matchend;
			break;
		case CONTROL('B'):
			if(cursor > 0 && (!sel || !sel->left || lines > 0)) {
				cursor = nextrune(-1);
				break;
			}
			/* fallthrough */
		case CONTROL('P'):
			if(sel && sel->left && (sel = sel->left)->right == curr) {
				curr = prev;
				calcoffsets();
			}
			break;
		case CONTROL('F'):
			if(text[cursor] != '\0') {
				cursor = nextrune(+1);
				break;
			}
			/* fallthrough */
		case CONTROL('N'):
			if(sel && sel->right && (sel = sel->right) == next) {
				curr = next;
				calcoffsets();
			}
			break;
		case CONTROL('D'):
			if(text[cursor] == '\0')
				break;
			cursor = nextrune(+1);
			/* fallthrough */
		case CONTROL('H'):
		case CONTROL('?'): /* Backspace */
			if(cursor == 0)
				break;
			insert(NULL, nextrune(-1) - cursor);
			break;
		case CONTROL('I'): /* TAB */
			if(!sel)
				break;
			strncpy(text, sel->text, sizeof text);
			cursor = strlen(text);
			match(TRUE);
			break;
		case CONTROL('K'):
			text[cursor] = '\0';
			match(FALSE);
			break;
		case CONTROL('U'):
			insert(NULL, 0 - cursor);
			break;
		case CONTROL('W'):
			while(cursor > 0 && text[nextrune(-1)] == ' ')
				insert(NULL, nextrune(-1) - cursor);
			while(cursor > 0 && text[nextrune(-1)] != ' ')
				insert(NULL, nextrune(-1) - cursor);
			break;
		case CONTROL('V'):
			if(!prev)
				break;
			sel = curr = prev;
			calcoffsets();
			break;
		default:
			if(!iscntrl(*buf))
				insert(buf, strlen(buf));
			break;
		}
		drawmenu();
	}
}

void
setup(void) {
	int fd, result=-1;
	struct winsize ws;

	/* re-open stdin to read keyboard */
	if(freopen("/dev/tty", "r", stdin) == NULL) die("Can't reopen tty.");

	/* ioctl() the tty to get size */
	fd = open("/dev/tty", O_RDWR);
	if(fd == -1) {
		mh=24;
		mw=80;
	} else {
		result = ioctl(fd, TIOCGWINSZ, &ws);
		close(fd);
		if(result<0) {
			mw=80;
			mh=24;
		} else {
			mw=ws.ws_col;
			mh=ws.ws_row;
		}
	}

	/* change terminal attributes, save old */
	tcgetattr(0, &tio_old);
	memcpy ((char *)&tio_new, (char *)&tio_old, sizeof(struct termios));
	tio_new.c_iflag &= ~(BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	tio_new.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	tio_new.c_cflag &= ~(CSIZE|PARENB);
	tio_new.c_cflag |= CS8;
	tio_new.c_cc[VMIN]=1;
	tcsetattr(0, TCSANOW, &tio_new);

	lines=MIN(MAX(lines, 0), mh);
	promptw=(prompt?textw(prompt):0);
	inputw=MIN(inputw, mw/3);
	match(FALSE);
	if(barpos!=0) resetline();
	drawmenu();
}

size_t
textw(const char *s) {
	return textwn(s, -1);
}

size_t
textwn(const char *s, int l) {
	int b, c; /* bytes and UTF-8 characters */

	for(b=c=0; s && s[b] && (l<0 || b<l); b++) if((s[b] & 0xc0) != 0x80) c++;
	return c+4; /* Accomodate for the leading and trailing spaces */
}

int
main(int argc, char **argv) {
	int i;

	for(i=1; i<argc; i++)
		/* single flags */
		if(!strcmp(argv[i], "-v")) {
			puts("vis-menu " VERSION);
			exit(EXIT_SUCCESS);
		}
		else if(!strcmp(argv[i], "-i"))
			fstrncmp = strncasecmp;
		else if(!strcmp(argv[i], "-t"))
			barpos=1;
		else if(!strcmp(argv[i], "-b"))
			barpos=-1;
		/* double flags */
		else if(!strcmp(argv[i], "-p"))
			prompt=argv[++i];
		else if(!strcmp(argv[i], "-l"))
			lines = atoi(argv[++i]);
		else {
			strncpy(text, argv[i], sizeof(text)-1);
			cursor = strlen(text);
		}

	readstdin();
	setup();
	i = run();
	cleanup();
	return i;
}

