/* parts of the color handling code originates from tmux/colour.c and is
 * Copyright (c) 2008 Nicholas Marriott <nicm@users.sourceforge.net>
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include <locale.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "ui-curses.h"
#include "vis.h"
#include "text.h"
#include "util.h"
#include "text-util.h"

#ifdef NCURSES_VERSION
# ifndef NCURSES_EXT_COLORS
#  define NCURSES_EXT_COLORS 0
# endif
# if !NCURSES_EXT_COLORS
#  define MAX_COLOR_PAIRS 256
# endif
#endif
#ifndef MAX_COLOR_PAIRS
# define MAX_COLOR_PAIRS COLOR_PAIRS
#endif

#ifdef PDCURSES
int ESCDELAY;
#endif
#ifndef NCURSES_REENTRANT
# define set_escdelay(d) (ESCDELAY = (d))
#endif

#define CONTROL(k) ((k)&0x1F)

#if 0
#define wresize(win, y, x) do { \
	if (wresize(win, y, x) == ERR) { \
		printf("ERROR resizing: %d x %d\n", x, y); \
	} else { \
		printf("OK resizing: %d x %d\n", x, y); \
	} \
	fflush(stdout); \
} while (0);

#define mvwin(win, y, x) do { \
	if (mvwin(win, y, x) == ERR) { \
		printf("ERROR moving: %d x %d\n", x, y); \
	} else { \
		printf("OK moving: %d x %d\n", x, y); \
	} \
	fflush(stdout); \
} while (0);
#endif

typedef struct {
	attr_t attr;
	short fg, bg;
} CellStyle;

typedef struct UiCursesWin UiCursesWin;

typedef struct {
	Ui ui;                    /* generic ui interface, has to be the first struct member */
	Vis *vis;              /* editor instance to which this ui belongs */
	UiCursesWin *windows;     /* all windows managed by this ui */
	UiCursesWin *selwin;      /* the currently selected layout */
	char info[255];           /* info message displayed at the bottom of the screen */
	int width, height;        /* terminal dimensions available for all windows */
	enum UiLayout layout;     /* whether windows are displayed horizontally or vertically */
	TermKey *termkey;         /* libtermkey instance to handle keyboard input */
	char key[64];             /* string representation of last pressed key */
} UiCurses;

struct UiCursesWin {
	UiWin uiwin;              /* generic interface, has to be the first struct member */
	UiCurses *ui;             /* ui which manages this window */
	File *file;               /* file being displayed in this window */
	View *view;               /* current viewport */
	WINDOW *win;              /* curses window for the text area */
	WINDOW *winstatus;        /* curses window for the status bar */
	WINDOW *winside;          /* curses window for the side bar (line numbers) */
	int width, height;        /* window dimension including status bar */
	int x, y;                 /* window position */
	int sidebar_width;        /* width of the sidebar showing line numbers etc. */
	UiCursesWin *next, *prev; /* pointers to neighbouring windows */
	enum UiOption options;    /* display settings for this window */
	CellStyle styles[UI_STYLE_MAX];
};

static volatile sig_atomic_t need_resize; /* TODO */

static void sigwinch_handler(int sig) {
	need_resize = true;
}

typedef struct {
	unsigned char i;
	unsigned char r;
	unsigned char g;
	unsigned char b;
} Color;

static int color_compare(const void *lhs0, const void *rhs0) {
	const Color *lhs = lhs0, *rhs = rhs0;

	if (lhs->r < rhs->r)
		return -1;
	if (lhs->r > rhs->r)
		return 1;

	if (lhs->g < rhs->g)
		return -1;
	if (lhs->g > rhs->g)
		return 1;

	if (lhs->b < rhs->b)
		return -1;
	if (lhs->b > rhs->b)
		return 1;

	return 0;
}

/* Work out the nearest color from the 256 color set. */
static int color_find_rgb(unsigned char r, unsigned char g, unsigned char b)
{
	static const Color color_from_256[] = {
		{   0, 0x00, 0x00, 0x00 }, {   1, 0x00, 0x00, 0x5f },
		{   2, 0x00, 0x00, 0x87 }, {   3, 0x00, 0x00, 0xaf },
		{   4, 0x00, 0x00, 0xd7 }, {   5, 0x00, 0x00, 0xff },
		{   6, 0x00, 0x5f, 0x00 }, {   7, 0x00, 0x5f, 0x5f },
		{   8, 0x00, 0x5f, 0x87 }, {   9, 0x00, 0x5f, 0xaf },
		{  10, 0x00, 0x5f, 0xd7 }, {  11, 0x00, 0x5f, 0xff },
		{  12, 0x00, 0x87, 0x00 }, {  13, 0x00, 0x87, 0x5f },
		{  14, 0x00, 0x87, 0x87 }, {  15, 0x00, 0x87, 0xaf },
		{  16, 0x00, 0x87, 0xd7 }, {  17, 0x00, 0x87, 0xff },
		{  18, 0x00, 0xaf, 0x00 }, {  19, 0x00, 0xaf, 0x5f },
		{  20, 0x00, 0xaf, 0x87 }, {  21, 0x00, 0xaf, 0xaf },
		{  22, 0x00, 0xaf, 0xd7 }, {  23, 0x00, 0xaf, 0xff },
		{  24, 0x00, 0xd7, 0x00 }, {  25, 0x00, 0xd7, 0x5f },
		{  26, 0x00, 0xd7, 0x87 }, {  27, 0x00, 0xd7, 0xaf },
		{  28, 0x00, 0xd7, 0xd7 }, {  29, 0x00, 0xd7, 0xff },
		{  30, 0x00, 0xff, 0x00 }, {  31, 0x00, 0xff, 0x5f },
		{  32, 0x00, 0xff, 0x87 }, {  33, 0x00, 0xff, 0xaf },
		{  34, 0x00, 0xff, 0xd7 }, {  35, 0x00, 0xff, 0xff },
		{  36, 0x5f, 0x00, 0x00 }, {  37, 0x5f, 0x00, 0x5f },
		{  38, 0x5f, 0x00, 0x87 }, {  39, 0x5f, 0x00, 0xaf },
		{  40, 0x5f, 0x00, 0xd7 }, {  41, 0x5f, 0x00, 0xff },
		{  42, 0x5f, 0x5f, 0x00 }, {  43, 0x5f, 0x5f, 0x5f },
		{  44, 0x5f, 0x5f, 0x87 }, {  45, 0x5f, 0x5f, 0xaf },
		{  46, 0x5f, 0x5f, 0xd7 }, {  47, 0x5f, 0x5f, 0xff },
		{  48, 0x5f, 0x87, 0x00 }, {  49, 0x5f, 0x87, 0x5f },
		{  50, 0x5f, 0x87, 0x87 }, {  51, 0x5f, 0x87, 0xaf },
		{  52, 0x5f, 0x87, 0xd7 }, {  53, 0x5f, 0x87, 0xff },
		{  54, 0x5f, 0xaf, 0x00 }, {  55, 0x5f, 0xaf, 0x5f },
		{  56, 0x5f, 0xaf, 0x87 }, {  57, 0x5f, 0xaf, 0xaf },
		{  58, 0x5f, 0xaf, 0xd7 }, {  59, 0x5f, 0xaf, 0xff },
		{  60, 0x5f, 0xd7, 0x00 }, {  61, 0x5f, 0xd7, 0x5f },
		{  62, 0x5f, 0xd7, 0x87 }, {  63, 0x5f, 0xd7, 0xaf },
		{  64, 0x5f, 0xd7, 0xd7 }, {  65, 0x5f, 0xd7, 0xff },
		{  66, 0x5f, 0xff, 0x00 }, {  67, 0x5f, 0xff, 0x5f },
		{  68, 0x5f, 0xff, 0x87 }, {  69, 0x5f, 0xff, 0xaf },
		{  70, 0x5f, 0xff, 0xd7 }, {  71, 0x5f, 0xff, 0xff },
		{  72, 0x87, 0x00, 0x00 }, {  73, 0x87, 0x00, 0x5f },
		{  74, 0x87, 0x00, 0x87 }, {  75, 0x87, 0x00, 0xaf },
		{  76, 0x87, 0x00, 0xd7 }, {  77, 0x87, 0x00, 0xff },
		{  78, 0x87, 0x5f, 0x00 }, {  79, 0x87, 0x5f, 0x5f },
		{  80, 0x87, 0x5f, 0x87 }, {  81, 0x87, 0x5f, 0xaf },
		{  82, 0x87, 0x5f, 0xd7 }, {  83, 0x87, 0x5f, 0xff },
		{  84, 0x87, 0x87, 0x00 }, {  85, 0x87, 0x87, 0x5f },
		{  86, 0x87, 0x87, 0x87 }, {  87, 0x87, 0x87, 0xaf },
		{  88, 0x87, 0x87, 0xd7 }, {  89, 0x87, 0x87, 0xff },
		{  90, 0x87, 0xaf, 0x00 }, {  91, 0x87, 0xaf, 0x5f },
		{  92, 0x87, 0xaf, 0x87 }, {  93, 0x87, 0xaf, 0xaf },
		{  94, 0x87, 0xaf, 0xd7 }, {  95, 0x87, 0xaf, 0xff },
		{  96, 0x87, 0xd7, 0x00 }, {  97, 0x87, 0xd7, 0x5f },
		{  98, 0x87, 0xd7, 0x87 }, {  99, 0x87, 0xd7, 0xaf },
		{ 100, 0x87, 0xd7, 0xd7 }, { 101, 0x87, 0xd7, 0xff },
		{ 102, 0x87, 0xff, 0x00 }, { 103, 0x87, 0xff, 0x5f },
		{ 104, 0x87, 0xff, 0x87 }, { 105, 0x87, 0xff, 0xaf },
		{ 106, 0x87, 0xff, 0xd7 }, { 107, 0x87, 0xff, 0xff },
		{ 108, 0xaf, 0x00, 0x00 }, { 109, 0xaf, 0x00, 0x5f },
		{ 110, 0xaf, 0x00, 0x87 }, { 111, 0xaf, 0x00, 0xaf },
		{ 112, 0xaf, 0x00, 0xd7 }, { 113, 0xaf, 0x00, 0xff },
		{ 114, 0xaf, 0x5f, 0x00 }, { 115, 0xaf, 0x5f, 0x5f },
		{ 116, 0xaf, 0x5f, 0x87 }, { 117, 0xaf, 0x5f, 0xaf },
		{ 118, 0xaf, 0x5f, 0xd7 }, { 119, 0xaf, 0x5f, 0xff },
		{ 120, 0xaf, 0x87, 0x00 }, { 121, 0xaf, 0x87, 0x5f },
		{ 122, 0xaf, 0x87, 0x87 }, { 123, 0xaf, 0x87, 0xaf },
		{ 124, 0xaf, 0x87, 0xd7 }, { 125, 0xaf, 0x87, 0xff },
		{ 126, 0xaf, 0xaf, 0x00 }, { 127, 0xaf, 0xaf, 0x5f },
		{ 128, 0xaf, 0xaf, 0x87 }, { 129, 0xaf, 0xaf, 0xaf },
		{ 130, 0xaf, 0xaf, 0xd7 }, { 131, 0xaf, 0xaf, 0xff },
		{ 132, 0xaf, 0xd7, 0x00 }, { 133, 0xaf, 0xd7, 0x5f },
		{ 134, 0xaf, 0xd7, 0x87 }, { 135, 0xaf, 0xd7, 0xaf },
		{ 136, 0xaf, 0xd7, 0xd7 }, { 137, 0xaf, 0xd7, 0xff },
		{ 138, 0xaf, 0xff, 0x00 }, { 139, 0xaf, 0xff, 0x5f },
		{ 140, 0xaf, 0xff, 0x87 }, { 141, 0xaf, 0xff, 0xaf },
		{ 142, 0xaf, 0xff, 0xd7 }, { 143, 0xaf, 0xff, 0xff },
		{ 144, 0xd7, 0x00, 0x00 }, { 145, 0xd7, 0x00, 0x5f },
		{ 146, 0xd7, 0x00, 0x87 }, { 147, 0xd7, 0x00, 0xaf },
		{ 148, 0xd7, 0x00, 0xd7 }, { 149, 0xd7, 0x00, 0xff },
		{ 150, 0xd7, 0x5f, 0x00 }, { 151, 0xd7, 0x5f, 0x5f },
		{ 152, 0xd7, 0x5f, 0x87 }, { 153, 0xd7, 0x5f, 0xaf },
		{ 154, 0xd7, 0x5f, 0xd7 }, { 155, 0xd7, 0x5f, 0xff },
		{ 156, 0xd7, 0x87, 0x00 }, { 157, 0xd7, 0x87, 0x5f },
		{ 158, 0xd7, 0x87, 0x87 }, { 159, 0xd7, 0x87, 0xaf },
		{ 160, 0xd7, 0x87, 0xd7 }, { 161, 0xd7, 0x87, 0xff },
		{ 162, 0xd7, 0xaf, 0x00 }, { 163, 0xd7, 0xaf, 0x5f },
		{ 164, 0xd7, 0xaf, 0x87 }, { 165, 0xd7, 0xaf, 0xaf },
		{ 166, 0xd7, 0xaf, 0xd7 }, { 167, 0xd7, 0xaf, 0xff },
		{ 168, 0xd7, 0xd7, 0x00 }, { 169, 0xd7, 0xd7, 0x5f },
		{ 170, 0xd7, 0xd7, 0x87 }, { 171, 0xd7, 0xd7, 0xaf },
		{ 172, 0xd7, 0xd7, 0xd7 }, { 173, 0xd7, 0xd7, 0xff },
		{ 174, 0xd7, 0xff, 0x00 }, { 175, 0xd7, 0xff, 0x5f },
		{ 176, 0xd7, 0xff, 0x87 }, { 177, 0xd7, 0xff, 0xaf },
		{ 178, 0xd7, 0xff, 0xd7 }, { 179, 0xd7, 0xff, 0xff },
		{ 180, 0xff, 0x00, 0x00 }, { 181, 0xff, 0x00, 0x5f },
		{ 182, 0xff, 0x00, 0x87 }, { 183, 0xff, 0x00, 0xaf },
		{ 184, 0xff, 0x00, 0xd7 }, { 185, 0xff, 0x00, 0xff },
		{ 186, 0xff, 0x5f, 0x00 }, { 187, 0xff, 0x5f, 0x5f },
		{ 188, 0xff, 0x5f, 0x87 }, { 189, 0xff, 0x5f, 0xaf },
		{ 190, 0xff, 0x5f, 0xd7 }, { 191, 0xff, 0x5f, 0xff },
		{ 192, 0xff, 0x87, 0x00 }, { 193, 0xff, 0x87, 0x5f },
		{ 194, 0xff, 0x87, 0x87 }, { 195, 0xff, 0x87, 0xaf },
		{ 196, 0xff, 0x87, 0xd7 }, { 197, 0xff, 0x87, 0xff },
		{ 198, 0xff, 0xaf, 0x00 }, { 199, 0xff, 0xaf, 0x5f },
		{ 200, 0xff, 0xaf, 0x87 }, { 201, 0xff, 0xaf, 0xaf },
		{ 202, 0xff, 0xaf, 0xd7 }, { 203, 0xff, 0xaf, 0xff },
		{ 204, 0xff, 0xd7, 0x00 }, { 205, 0xff, 0xd7, 0x5f },
		{ 206, 0xff, 0xd7, 0x87 }, { 207, 0xff, 0xd7, 0xaf },
		{ 208, 0xff, 0xd7, 0xd7 }, { 209, 0xff, 0xd7, 0xff },
		{ 210, 0xff, 0xff, 0x00 }, { 211, 0xff, 0xff, 0x5f },
		{ 212, 0xff, 0xff, 0x87 }, { 213, 0xff, 0xff, 0xaf },
		{ 214, 0xff, 0xff, 0xd7 }, { 215, 0xff, 0xff, 0xff },
		{ 216, 0x08, 0x08, 0x08 }, { 217, 0x12, 0x12, 0x12 },
		{ 218, 0x1c, 0x1c, 0x1c }, { 219, 0x26, 0x26, 0x26 },
		{ 220, 0x30, 0x30, 0x30 }, { 221, 0x3a, 0x3a, 0x3a },
		{ 222, 0x44, 0x44, 0x44 }, { 223, 0x4e, 0x4e, 0x4e },
		{ 224, 0x58, 0x58, 0x58 }, { 225, 0x62, 0x62, 0x62 },
		{ 226, 0x6c, 0x6c, 0x6c }, { 227, 0x76, 0x76, 0x76 },
		{ 228, 0x80, 0x80, 0x80 }, { 229, 0x8a, 0x8a, 0x8a },
		{ 230, 0x94, 0x94, 0x94 }, { 231, 0x9e, 0x9e, 0x9e },
		{ 232, 0xa8, 0xa8, 0xa8 }, { 233, 0xb2, 0xb2, 0xb2 },
		{ 234, 0xbc, 0xbc, 0xbc }, { 235, 0xc6, 0xc6, 0xc6 },
		{ 236, 0xd0, 0xd0, 0xd0 }, { 237, 0xda, 0xda, 0xda },
		{ 238, 0xe4, 0xe4, 0xe4 }, { 239, 0xee, 0xee, 0xee },
	};

	static const Color color_to_256[] = {
		{   0, 0x00, 0x00, 0x00 }, {   1, 0x00, 0x00, 0x5f },
		{   2, 0x00, 0x00, 0x87 }, {   3, 0x00, 0x00, 0xaf },
		{   4, 0x00, 0x00, 0xd7 }, {   5, 0x00, 0x00, 0xff },
		{   6, 0x00, 0x5f, 0x00 }, {   7, 0x00, 0x5f, 0x5f },
		{   8, 0x00, 0x5f, 0x87 }, {   9, 0x00, 0x5f, 0xaf },
		{  10, 0x00, 0x5f, 0xd7 }, {  11, 0x00, 0x5f, 0xff },
		{  12, 0x00, 0x87, 0x00 }, {  13, 0x00, 0x87, 0x5f },
		{  14, 0x00, 0x87, 0x87 }, {  15, 0x00, 0x87, 0xaf },
		{  16, 0x00, 0x87, 0xd7 }, {  17, 0x00, 0x87, 0xff },
		{  18, 0x00, 0xaf, 0x00 }, {  19, 0x00, 0xaf, 0x5f },
		{  20, 0x00, 0xaf, 0x87 }, {  21, 0x00, 0xaf, 0xaf },
		{  22, 0x00, 0xaf, 0xd7 }, {  23, 0x00, 0xaf, 0xff },
		{  24, 0x00, 0xd7, 0x00 }, {  25, 0x00, 0xd7, 0x5f },
		{  26, 0x00, 0xd7, 0x87 }, {  27, 0x00, 0xd7, 0xaf },
		{  28, 0x00, 0xd7, 0xd7 }, {  29, 0x00, 0xd7, 0xff },
		{  30, 0x00, 0xff, 0x00 }, {  31, 0x00, 0xff, 0x5f },
		{  32, 0x00, 0xff, 0x87 }, {  33, 0x00, 0xff, 0xaf },
		{  34, 0x00, 0xff, 0xd7 }, {  35, 0x00, 0xff, 0xff },
		{ 216, 0x08, 0x08, 0x08 }, { 217, 0x12, 0x12, 0x12 },
		{ 218, 0x1c, 0x1c, 0x1c }, { 219, 0x26, 0x26, 0x26 },
		{ 220, 0x30, 0x30, 0x30 }, { 221, 0x3a, 0x3a, 0x3a },
		{ 222, 0x44, 0x44, 0x44 }, { 223, 0x4e, 0x4e, 0x4e },
		{ 224, 0x58, 0x58, 0x58 }, {  36, 0x5f, 0x00, 0x00 },
		{  37, 0x5f, 0x00, 0x5f }, {  38, 0x5f, 0x00, 0x87 },
		{  39, 0x5f, 0x00, 0xaf }, {  40, 0x5f, 0x00, 0xd7 },
		{  41, 0x5f, 0x00, 0xff }, {  42, 0x5f, 0x5f, 0x00 },
		{  43, 0x5f, 0x5f, 0x5f }, {  44, 0x5f, 0x5f, 0x87 },
		{  45, 0x5f, 0x5f, 0xaf }, {  46, 0x5f, 0x5f, 0xd7 },
		{  47, 0x5f, 0x5f, 0xff }, {  48, 0x5f, 0x87, 0x00 },
		{  49, 0x5f, 0x87, 0x5f }, {  50, 0x5f, 0x87, 0x87 },
		{  51, 0x5f, 0x87, 0xaf }, {  52, 0x5f, 0x87, 0xd7 },
		{  53, 0x5f, 0x87, 0xff }, {  54, 0x5f, 0xaf, 0x00 },
		{  55, 0x5f, 0xaf, 0x5f }, {  56, 0x5f, 0xaf, 0x87 },
		{  57, 0x5f, 0xaf, 0xaf }, {  58, 0x5f, 0xaf, 0xd7 },
		{  59, 0x5f, 0xaf, 0xff }, {  60, 0x5f, 0xd7, 0x00 },
		{  61, 0x5f, 0xd7, 0x5f }, {  62, 0x5f, 0xd7, 0x87 },
		{  63, 0x5f, 0xd7, 0xaf }, {  64, 0x5f, 0xd7, 0xd7 },
		{  65, 0x5f, 0xd7, 0xff }, {  66, 0x5f, 0xff, 0x00 },
		{  67, 0x5f, 0xff, 0x5f }, {  68, 0x5f, 0xff, 0x87 },
		{  69, 0x5f, 0xff, 0xaf }, {  70, 0x5f, 0xff, 0xd7 },
		{  71, 0x5f, 0xff, 0xff }, { 225, 0x62, 0x62, 0x62 },
		{ 226, 0x6c, 0x6c, 0x6c }, { 227, 0x76, 0x76, 0x76 },
		{ 228, 0x80, 0x80, 0x80 }, {  72, 0x87, 0x00, 0x00 },
		{  73, 0x87, 0x00, 0x5f }, {  74, 0x87, 0x00, 0x87 },
		{  75, 0x87, 0x00, 0xaf }, {  76, 0x87, 0x00, 0xd7 },
		{  77, 0x87, 0x00, 0xff }, {  78, 0x87, 0x5f, 0x00 },
		{  79, 0x87, 0x5f, 0x5f }, {  80, 0x87, 0x5f, 0x87 },
		{  81, 0x87, 0x5f, 0xaf }, {  82, 0x87, 0x5f, 0xd7 },
		{  83, 0x87, 0x5f, 0xff }, {  84, 0x87, 0x87, 0x00 },
		{  85, 0x87, 0x87, 0x5f }, {  86, 0x87, 0x87, 0x87 },
		{  87, 0x87, 0x87, 0xaf }, {  88, 0x87, 0x87, 0xd7 },
		{  89, 0x87, 0x87, 0xff }, {  90, 0x87, 0xaf, 0x00 },
		{  91, 0x87, 0xaf, 0x5f }, {  92, 0x87, 0xaf, 0x87 },
		{  93, 0x87, 0xaf, 0xaf }, {  94, 0x87, 0xaf, 0xd7 },
		{  95, 0x87, 0xaf, 0xff }, {  96, 0x87, 0xd7, 0x00 },
		{  97, 0x87, 0xd7, 0x5f }, {  98, 0x87, 0xd7, 0x87 },
		{  99, 0x87, 0xd7, 0xaf }, { 100, 0x87, 0xd7, 0xd7 },
		{ 101, 0x87, 0xd7, 0xff }, { 102, 0x87, 0xff, 0x00 },
		{ 103, 0x87, 0xff, 0x5f }, { 104, 0x87, 0xff, 0x87 },
		{ 105, 0x87, 0xff, 0xaf }, { 106, 0x87, 0xff, 0xd7 },
		{ 107, 0x87, 0xff, 0xff }, { 229, 0x8a, 0x8a, 0x8a },
		{ 230, 0x94, 0x94, 0x94 }, { 231, 0x9e, 0x9e, 0x9e },
		{ 232, 0xa8, 0xa8, 0xa8 }, { 108, 0xaf, 0x00, 0x00 },
		{ 109, 0xaf, 0x00, 0x5f }, { 110, 0xaf, 0x00, 0x87 },
		{ 111, 0xaf, 0x00, 0xaf }, { 112, 0xaf, 0x00, 0xd7 },
		{ 113, 0xaf, 0x00, 0xff }, { 114, 0xaf, 0x5f, 0x00 },
		{ 115, 0xaf, 0x5f, 0x5f }, { 116, 0xaf, 0x5f, 0x87 },
		{ 117, 0xaf, 0x5f, 0xaf }, { 118, 0xaf, 0x5f, 0xd7 },
		{ 119, 0xaf, 0x5f, 0xff }, { 120, 0xaf, 0x87, 0x00 },
		{ 121, 0xaf, 0x87, 0x5f }, { 122, 0xaf, 0x87, 0x87 },
		{ 123, 0xaf, 0x87, 0xaf }, { 124, 0xaf, 0x87, 0xd7 },
		{ 125, 0xaf, 0x87, 0xff }, { 126, 0xaf, 0xaf, 0x00 },
		{ 127, 0xaf, 0xaf, 0x5f }, { 128, 0xaf, 0xaf, 0x87 },
		{ 129, 0xaf, 0xaf, 0xaf }, { 130, 0xaf, 0xaf, 0xd7 },
		{ 131, 0xaf, 0xaf, 0xff }, { 132, 0xaf, 0xd7, 0x00 },
		{ 133, 0xaf, 0xd7, 0x5f }, { 134, 0xaf, 0xd7, 0x87 },
		{ 135, 0xaf, 0xd7, 0xaf }, { 136, 0xaf, 0xd7, 0xd7 },
		{ 137, 0xaf, 0xd7, 0xff }, { 138, 0xaf, 0xff, 0x00 },
		{ 139, 0xaf, 0xff, 0x5f }, { 140, 0xaf, 0xff, 0x87 },
		{ 141, 0xaf, 0xff, 0xaf }, { 142, 0xaf, 0xff, 0xd7 },
		{ 143, 0xaf, 0xff, 0xff }, { 233, 0xb2, 0xb2, 0xb2 },
		{ 234, 0xbc, 0xbc, 0xbc }, { 235, 0xc6, 0xc6, 0xc6 },
		{ 236, 0xd0, 0xd0, 0xd0 }, { 144, 0xd7, 0x00, 0x00 },
		{ 145, 0xd7, 0x00, 0x5f }, { 146, 0xd7, 0x00, 0x87 },
		{ 147, 0xd7, 0x00, 0xaf }, { 148, 0xd7, 0x00, 0xd7 },
		{ 149, 0xd7, 0x00, 0xff }, { 150, 0xd7, 0x5f, 0x00 },
		{ 151, 0xd7, 0x5f, 0x5f }, { 152, 0xd7, 0x5f, 0x87 },
		{ 153, 0xd7, 0x5f, 0xaf }, { 154, 0xd7, 0x5f, 0xd7 },
		{ 155, 0xd7, 0x5f, 0xff }, { 156, 0xd7, 0x87, 0x00 },
		{ 157, 0xd7, 0x87, 0x5f }, { 158, 0xd7, 0x87, 0x87 },
		{ 159, 0xd7, 0x87, 0xaf }, { 160, 0xd7, 0x87, 0xd7 },
		{ 161, 0xd7, 0x87, 0xff }, { 162, 0xd7, 0xaf, 0x00 },
		{ 163, 0xd7, 0xaf, 0x5f }, { 164, 0xd7, 0xaf, 0x87 },
		{ 165, 0xd7, 0xaf, 0xaf }, { 166, 0xd7, 0xaf, 0xd7 },
		{ 167, 0xd7, 0xaf, 0xff }, { 168, 0xd7, 0xd7, 0x00 },
		{ 169, 0xd7, 0xd7, 0x5f }, { 170, 0xd7, 0xd7, 0x87 },
		{ 171, 0xd7, 0xd7, 0xaf }, { 172, 0xd7, 0xd7, 0xd7 },
		{ 173, 0xd7, 0xd7, 0xff }, { 174, 0xd7, 0xff, 0x00 },
		{ 175, 0xd7, 0xff, 0x5f }, { 176, 0xd7, 0xff, 0x87 },
		{ 177, 0xd7, 0xff, 0xaf }, { 178, 0xd7, 0xff, 0xd7 },
		{ 179, 0xd7, 0xff, 0xff }, { 237, 0xda, 0xda, 0xda },
		{ 238, 0xe4, 0xe4, 0xe4 }, { 239, 0xee, 0xee, 0xee },
		{ 180, 0xff, 0x00, 0x00 }, { 181, 0xff, 0x00, 0x5f },
		{ 182, 0xff, 0x00, 0x87 }, { 183, 0xff, 0x00, 0xaf },
		{ 184, 0xff, 0x00, 0xd7 }, { 185, 0xff, 0x00, 0xff },
		{ 186, 0xff, 0x5f, 0x00 }, { 187, 0xff, 0x5f, 0x5f },
		{ 188, 0xff, 0x5f, 0x87 }, { 189, 0xff, 0x5f, 0xaf },
		{ 190, 0xff, 0x5f, 0xd7 }, { 191, 0xff, 0x5f, 0xff },
		{ 192, 0xff, 0x87, 0x00 }, { 193, 0xff, 0x87, 0x5f },
		{ 194, 0xff, 0x87, 0x87 }, { 195, 0xff, 0x87, 0xaf },
		{ 196, 0xff, 0x87, 0xd7 }, { 197, 0xff, 0x87, 0xff },
		{ 198, 0xff, 0xaf, 0x00 }, { 199, 0xff, 0xaf, 0x5f },
		{ 200, 0xff, 0xaf, 0x87 }, { 201, 0xff, 0xaf, 0xaf },
		{ 202, 0xff, 0xaf, 0xd7 }, { 203, 0xff, 0xaf, 0xff },
		{ 204, 0xff, 0xd7, 0x00 }, { 205, 0xff, 0xd7, 0x5f },
		{ 206, 0xff, 0xd7, 0x87 }, { 207, 0xff, 0xd7, 0xaf },
		{ 208, 0xff, 0xd7, 0xd7 }, { 209, 0xff, 0xd7, 0xff },
		{ 210, 0xff, 0xff, 0x00 }, { 211, 0xff, 0xff, 0x5f },
		{ 212, 0xff, 0xff, 0x87 }, { 213, 0xff, 0xff, 0xaf },
		{ 214, 0xff, 0xff, 0xd7 }, { 215, 0xff, 0xff, 0xff },
	};

	static const unsigned char color_256_to_16[256] = {
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		 0,  4,  4,  4, 12, 12,  2,  6,  4,  4, 12, 12,  2,  2,  6,  4,
		12, 12,  2,  2,  2,  6, 12, 12, 10, 10, 10, 10, 14, 12, 10, 10,
		10, 10, 10, 14,  1,  5,  4,  4, 12, 12,  3,  8,  4,  4, 12, 12,
		 2,  2,  6,  4, 12, 12,  2,  2,  2,  6, 12, 12, 10, 10, 10, 10,
		14, 12, 10, 10, 10, 10, 10, 14,  1,  1,  5,  4, 12, 12,  1,  1,
		 5,  4, 12, 12,  3,  3,  8,  4, 12, 12,  2,  2,  2,  6, 12, 12,
		10, 10, 10, 10, 14, 12, 10, 10, 10, 10, 10, 14,  1,  1,  1,  5,
		12, 12,  1,  1,  1,  5, 12, 12,  1,  1,  1,  5, 12, 12,  3,  3,
		 3,  7, 12, 12, 10, 10, 10, 10, 14, 12, 10, 10, 10, 10, 10, 14,
		 9,  9,  9,  9, 13, 12,  9,  9,  9,  9, 13, 12,  9,  9,  9,  9,
		13, 12,  9,  9,  9,  9, 13, 12, 11, 11, 11, 11,  7, 12, 10, 10,
		10, 10, 10, 14,  9,  9,  9,  9,  9, 13,  9,  9,  9,  9,  9, 13,
		 9,  9,  9,  9,  9, 13,  9,  9,  9,  9,  9, 13,  9,  9,  9,  9,
		 9, 13, 11, 11, 11, 11, 11, 15,  0,  0,  0,  0,  0,  0,  8,  8,
		 8,  8,  8,  8,  7,  7,  7,  7,  7,  7, 15, 15, 15, 15, 15, 15
	};

	Color rgb = { .r = r, .g = g, .b = b };
	const Color *found = bsearch(&rgb, color_to_256, LENGTH(color_to_256),
	    sizeof color_to_256[0], color_compare);

	if (!found) {
		unsigned lowest = UINT_MAX;
		found = color_from_256;
		for (int i = 0; i < 240; i++) {
			int dr = (int)color_from_256[i].r - r;
			int dg = (int)color_from_256[i].g - g;
			int db = (int)color_from_256[i].b - b;

			unsigned int distance = dr * dr + dg * dg + db * db;
			if (distance < lowest) {
				lowest = distance;
				found = &color_from_256[i];
			}
		}
	}

	if (COLORS <= 16)
		return color_256_to_16[found->i + 16];
	return found->i + 16;
}

/* Convert color from string. */
static int color_fromstring(const char *s)
{
	if (!s)
		return -1;
	if (*s == '#' && strlen(s) == 7) {
		const char *cp;
		unsigned char r, g, b;
		for (cp = s + 1; isxdigit((unsigned char)*cp); cp++);
		if (*cp != '\0')
			return -1;
		int n = sscanf(s + 1, "%2hhx%2hhx%2hhx", &r, &g, &b);
		if (n != 3)
			return -1;
		return color_find_rgb(r, g, b);
	} else if ('0' <= *s && *s <= '9') {
		int col = atoi(s);
		return (col <= 0 || col > 255) ? -1 : col;
	}

	if (strcasecmp(s, "black") == 0)
		return 0;
	if (strcasecmp(s, "red") == 0)
		return 1;
	if (strcasecmp(s, "green") == 0)
		return 2;
	if (strcasecmp(s, "yellow") == 0)
		return 3;
	if (strcasecmp(s, "blue") == 0)
		return 4;
	if (strcasecmp(s, "magenta") == 0)
		return 5;
	if (strcasecmp(s, "cyan") == 0)
		return 6;
	if (strcasecmp(s, "white") == 0)
		return 7;
	return -1;
}

static inline unsigned int color_pair_hash(short fg, short bg) {
	if (fg == -1)
		fg = COLORS;
	if (bg == -1)
		bg = COLORS + 1;
	return fg * (COLORS + 2) + bg;
}

static short color_pair_get(short fg, short bg) {
	static bool has_default_colors;
	static short *color2palette, default_fg, default_bg;
	static short color_pairs_max, color_pair_current;

	if (!color2palette) {
		pair_content(0, &default_fg, &default_bg);
		if (default_fg == -1)
			default_fg = COLOR_WHITE;
		if (default_bg == -1)
			default_bg = COLOR_BLACK;
		has_default_colors = (use_default_colors() == OK);
		color_pairs_max = MIN(COLOR_PAIRS, MAX_COLOR_PAIRS);
		if (COLORS)
			color2palette = calloc((COLORS + 2) * (COLORS + 2), sizeof(short));
	}

	if (fg >= COLORS)
		fg = default_fg;
	if (bg >= COLORS)
		bg = default_bg;

	if (!has_default_colors) {
		if (fg == -1)
			fg = default_fg;
		if (bg == -1)
			bg = default_bg;
	}

	if (!color2palette || (fg == -1 && bg == -1))
		return 0;

	unsigned int index = color_pair_hash(fg, bg);
	if (color2palette[index] == 0) {
		short oldfg, oldbg;
		if (++color_pair_current >= color_pairs_max)
			color_pair_current = 1;
		pair_content(color_pair_current, &oldfg, &oldbg);
		unsigned int old_index = color_pair_hash(oldfg, oldbg);
		if (init_pair(color_pair_current, fg, bg) == OK) {
			color2palette[old_index] = 0;
			color2palette[index] = color_pair_current;
		}
	}

	return color2palette[index];
}

static inline attr_t style_to_attr(CellStyle *style) {
	return style->attr | COLOR_PAIR(color_pair_get(style->fg, style->bg));
}

static bool ui_window_syntax_style(UiWin *w, int id, const char *style) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (id >= UI_STYLE_MAX)
		return false;
	if (!style)
		return true;
	CellStyle cell_style = win->styles[UI_STYLE_DEFAULT];
	char *style_copy = strdup(style), *option = style_copy, *next, *p;
	while (option) {
		if ((next = strchr(option, ',')))
			*next++ = '\0';
		if ((p = strchr(option, ':')))
			*p++ = '\0';
		if (!strcasecmp(option, "reverse")) {
			cell_style.attr |= A_REVERSE;
		} else if (!strcasecmp(option, "bold")) {
			cell_style.attr |= A_BOLD;
		} else if (!strcasecmp(option, "notbold")) {
			cell_style.attr &= ~A_BOLD;
#ifdef A_ITALIC
		} else if (!strcasecmp(option, "italics")) {
			cell_style.attr |= A_ITALIC;
		} else if (!strcasecmp(option, "notitalics")) {
			cell_style.attr &= ~A_ITALIC;
#endif
		} else if (!strcasecmp(option, "underlined")) {
			cell_style.attr |= A_UNDERLINE;
		} else if (!strcasecmp(option, "notunderlined")) {
			cell_style.attr &= ~A_UNDERLINE;
		} else if (!strcasecmp(option, "blink")) {
			cell_style.attr |= A_BLINK;
		} else if (!strcasecmp(option, "notblink")) {
			cell_style.attr &= ~A_BLINK;
		} else if (!strcasecmp(option, "fore")) {
			cell_style.fg = color_fromstring(p);
		} else if (!strcasecmp(option, "back")) {
			cell_style.bg = color_fromstring(p);
		}
		option = next;
	}
	win->styles[id] = cell_style;
	free(style_copy);
	return true;
}

static void ui_window_resize(UiCursesWin *win, int width, int height) {
	win->width = width;
	win->height = height;
	if (win->winstatus)
		wresize(win->winstatus, 1, width);
	wresize(win->win, win->winstatus ? height - 1 : height, width - win->sidebar_width);
	if (win->winside)
		wresize(win->winside, height-1, win->sidebar_width);
	view_resize(win->view, width - win->sidebar_width, win->winstatus ? height - 1 : height);
	view_update(win->view);
}

static void ui_window_move(UiCursesWin *win, int x, int y) {
	win->x = x;
	win->y = y;
	mvwin(win->win, y, x + win->sidebar_width);
	if (win->winside)
		mvwin(win->winside, y, x);
	if (win->winstatus)
		mvwin(win->winstatus, y + win->height - 1, x);
}

static bool ui_window_draw_sidebar(UiCursesWin *win) {
	if (!win->winside)
		return true;
	const Line *line = view_lines_get(win->view);
	int sidebar_width = snprintf(NULL, 0, "%zd", line->lineno + win->height - 2) + 1;
	if (win->sidebar_width != sidebar_width) {
		win->sidebar_width = sidebar_width;
		ui_window_resize(win, win->width, win->height);
		ui_window_move(win, win->x, win->y);
		return false;
	} else {
		int i = 0;
		size_t prev_lineno = 0;
		size_t cursor_lineno = view_cursor_getpos(win->view).line;
		werase(win->winside);
		wbkgd(win->winside, style_to_attr(&win->styles[UI_STYLE_DEFAULT]));
		wattrset(win->winside, style_to_attr(&win->styles[UI_STYLE_LINENUMBER]));
		for (const Line *l = line; l; l = l->next, i++) {
			if (l->lineno && l->lineno != prev_lineno) {
				if (win->options & UI_OPTION_LINE_NUMBERS_ABSOLUTE) {
					mvwprintw(win->winside, i, 0, "%*u", sidebar_width-1, l->lineno);
				} else if (win->options & UI_OPTION_LINE_NUMBERS_RELATIVE) {
					size_t rel = (win->options & UI_OPTION_LARGE_FILE) ? 0 : l->lineno;
					if (l->lineno > cursor_lineno)
						rel = l->lineno - cursor_lineno;
					else if (l->lineno < cursor_lineno)
						rel = cursor_lineno - l->lineno;
					mvwprintw(win->winside, i, 0, "%*u", sidebar_width-1, rel);
				}
			}
			prev_lineno = l->lineno;
		}
		mvwvline(win->winside, 0, sidebar_width-1, ACS_VLINE, win->height-1);
		return true;
	}
}

static void ui_window_draw_status(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!win->winstatus)
		return;
	UiCurses *uic = win->ui;
	Vis *vis = uic->vis;
	bool focused = uic->selwin == win;
	const char *filename = vis_file_name(win->file);
	const char *status = vis_mode_status(vis);
	wattrset(win->winstatus, focused ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win->winstatus, 0, 0, ' ', win->width);
	mvwprintw(win->winstatus, 0, 0, "%s %s %s %s",
	          focused && status ? status : "",
	          filename ? filename : "[No Name]",
	          text_modified(vis_file_text(win->file)) ? "[+]" : "",
	          vis_macro_recording(vis) ? "recording": "");

	if (!(win->options & UI_OPTION_LARGE_FILE)) {
		CursorPos pos = view_cursor_getpos(win->view);
		char buf[win->width + 1];
		int len = snprintf(buf, win->width, "%zd, %zd", pos.line, pos.col);
		if (len > 0) {
			buf[len] = '\0';
			mvwaddstr(win->winstatus, 0, win->width - len - 1, buf);
		}
	}
}

static void ui_window_draw(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!ui_window_draw_sidebar(win))
		return;

	wbkgd(win->win, style_to_attr(&win->styles[UI_STYLE_DEFAULT]));
	wmove(win->win, 0, 0);
	int width = view_width_get(win->view);
	CellStyle *prev_style = NULL;
	size_t cursor_lineno = -1;

	if (win->options & UI_OPTION_CURSOR_LINE && win->ui->selwin == win) {
		Cursor *cursor = view_cursors(win->view);
		Filerange selection = view_cursors_selection_get(cursor);
		if (!view_cursors_next(cursor) && !text_range_valid(&selection))
			cursor_lineno = view_cursor_getpos(win->view).line;
	}

	short selection_bg = win->styles[UI_STYLE_SELECTION].bg;
	short cursor_line_bg = win->styles[UI_STYLE_CURSOR_LINE].bg;
	bool multiple_cursors = view_cursors_multiple(win->view);
	attr_t attr = A_NORMAL;

	for (const Line *l = view_lines_get(win->view); l; l = l->next) {
		bool cursor_line = l->lineno == cursor_lineno;
		for (int x = 0; x < width; x++) {
			enum UiStyles style_id = l->cells[x].style;
			if (style_id == 0)
				style_id = UI_STYLE_DEFAULT;
			CellStyle *style = &win->styles[style_id];

			if (l->cells[x].cursor && win->ui->selwin == win) {
				if (multiple_cursors && l->cells[x].cursor_primary)
					attr = style_to_attr(&win->styles[UI_STYLE_CURSOR_PRIMARY]);
				else
					attr = style_to_attr(&win->styles[UI_STYLE_CURSOR]);
				prev_style = NULL;
			} else if (l->cells[x].selected) {
				if (style->fg == selection_bg)
					attr |= A_REVERSE;
				else
					attr = style->attr | COLOR_PAIR(color_pair_get(style->fg, selection_bg));
				prev_style = NULL;
			} else if (cursor_line) {
				attr = style->attr | COLOR_PAIR(color_pair_get(style->fg, cursor_line_bg));
				prev_style = NULL;
			} else if (style != prev_style) {
				attr = style_to_attr(style);
				prev_style = style;
			}
			wattrset(win->win, attr);
			if (multiple_cursors && l->cells[x].cursor_primary && l->cells[x].data[0] == ' ')
				waddstr(win->win, "█");
			else
				waddstr(win->win, l->cells[x].data);
		}
		/* try to fixup display issues, in theory we should always output a full line */
		int x, y;
		getyx(win->win, y, x);
		(void)y;
		wattrset(win->win, A_NORMAL);
		for (; 0 < x && x < width; x++)
			waddstr(win->win, " ");
	}

	wclrtobot(win->win);

	if (win->winstatus)
		ui_window_draw_status(w);
}

static void ui_window_reload(UiWin *w, File *file) {
	UiCursesWin *win = (UiCursesWin*)w;
	win->file = file;
	win->sidebar_width = 0;
	view_reload(win->view, vis_file_text(file));
	ui_window_draw(w);
}

static void ui_window_update(UiCursesWin *win) {
	if (win->winstatus)
		wnoutrefresh(win->winstatus);
	if (win->winside)
		wnoutrefresh(win->winside);
	wnoutrefresh(win->win);
}

static void ui_arrange(Ui *ui, enum UiLayout layout) {
	UiCurses *uic = (UiCurses*)ui;
	uic->layout = layout;
	int n = 0, m = !!uic->info[0], x = 0, y = 0;
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			m++;
		else
			n++;
	}
	int max_height = uic->height - m;
	int width = (uic->width / MAX(1, n)) - 1;
	int height = max_height / MAX(1, n);
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (win->options & UI_OPTION_ONELINE)
			continue;
		n--;
		if (layout == UI_LAYOUT_HORIZONTAL) {
			int h = n ? height : max_height - y;
			ui_window_resize(win, uic->width, h);
			ui_window_move(win, x, y);
			y += h;
		} else {
			int w = n ? width : uic->width - x;
			ui_window_resize(win, w, max_height);
			ui_window_move(win, x, y);
			x += w;
			if (n)
				mvvline(0, x++, ACS_VLINE, max_height);
		}
	}

	if (layout == UI_LAYOUT_VERTICAL)
		y = max_height;

	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (!(win->options & UI_OPTION_ONELINE))
			continue;
		ui_window_resize(win, uic->width, 1);
		ui_window_move(win, 0, y++);
	}
}

static void ui_draw(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	erase();
	ui_arrange(ui, uic->layout);

	for (UiCursesWin *win = uic->windows; win; win = win->next)
		ui_window_draw((UiWin*)win);

	if (uic->info[0]) {
	        attrset(A_BOLD);
        	mvaddstr(uic->height-1, 0, uic->info);
	}

	wnoutrefresh(stdscr);
}

static void ui_redraw(Ui *ui) {
	clear();
	ui_draw(ui);
}

static void ui_resize_to(Ui *ui, int width, int height) {
	UiCurses *uic = (UiCurses*)ui;
	uic->width = width;
	uic->height = height;
	ui_draw(ui);
}

static void ui_resize(Ui *ui) {
	struct winsize ws;
	int width, height;

	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == -1) {
		getmaxyx(stdscr, height, width);
	} else {
		width = ws.ws_col;
		height = ws.ws_row;
	}

	resizeterm(height, width);
	wresize(stdscr, height, width);
	ui_resize_to(ui, width, height);
}

static void ui_update(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (need_resize) {
		ui_resize(ui);
		need_resize = false;
	}
	for (UiCursesWin *win = uic->windows; win; win = win->next) {
		if (win != uic->selwin)
			ui_window_update(win);
	}

	if (uic->selwin)
		ui_window_update(uic->selwin);
	doupdate();
}

static void ui_window_free(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	if (!win)
		return;
	UiCurses *uic = win->ui;
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (uic->windows == win)
		uic->windows = win->next;
	if (uic->selwin == win)
		uic->selwin = NULL;
	win->next = win->prev = NULL;
	if (win->winstatus)
		delwin(win->winstatus);
	if (win->winside)
		delwin(win->winside);
	if (win->win)
		delwin(win->win);
	free(win);
}

static void ui_window_focus(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	UiCursesWin *oldsel = win->ui->selwin;
	win->ui->selwin = win;
	if (oldsel)
		ui_window_draw((UiWin*)oldsel);
	ui_window_draw(w);
}

static void ui_window_options_set(UiWin *w, enum UiOption options) {
	UiCursesWin *win = (UiCursesWin*)w;
	win->options = options;
	if (options & (UI_OPTION_LINE_NUMBERS_ABSOLUTE|UI_OPTION_LINE_NUMBERS_RELATIVE)) {
		if (!win->winside)
			win->winside = newwin(1, 1, 1, 1);
	} else {
		if (win->winside) {
			delwin(win->winside);
			win->winside = NULL;
			win->sidebar_width = 0;
		}
	}
	if (options & UI_OPTION_STATUSBAR) {
		if (!win->winstatus)
			win->winstatus = newwin(1, 0, 0, 0);
	} else {
		if (win->winstatus)
			delwin(win->winstatus);
		win->winstatus = NULL;
	}

	if (options & UI_OPTION_ONELINE) {
		/* move the new window to the end of the list */
		UiCurses *uic = win->ui;
		UiCursesWin *last = uic->windows;
		while (last->next)
			last = last->next;
		if (last != win) {
			if (win->prev)
				win->prev->next = win->next;
			if (win->next)
				win->next->prev = win->prev;
			if (uic->windows == win)
				uic->windows = win->next;
			last->next = win;
			win->prev = last;
			win->next = NULL;
		}
	}

	ui_draw((Ui*)win->ui);
}

static enum UiOption ui_window_options_get(UiWin *w) {
	UiCursesWin *win = (UiCursesWin*)w;
	return win->options;
}

static UiWin *ui_window_new(Ui *ui, View *view, File *file, enum UiOption options) {
	UiCurses *uic = (UiCurses*)ui;
	UiCursesWin *win = calloc(1, sizeof(UiCursesWin));
	if (!win)
		return NULL;

	win->uiwin = (UiWin) {
		.draw = ui_window_draw,
		.draw_status = ui_window_draw_status,
		.options_set = ui_window_options_set,
		.options_get = ui_window_options_get,
		.reload = ui_window_reload,
		.syntax_style = ui_window_syntax_style,
	};

	if (!(win->win = newwin(0, 0, 0, 0))) {
		ui_window_free((UiWin*)win);
		return NULL;
	}


	for (int i = 0; i < UI_STYLE_MAX; i++) {
		win->styles[i] = (CellStyle) {
			.fg = -1, .bg = -1, .attr = A_NORMAL,
		};
	}

	win->styles[UI_STYLE_CURSOR].attr |= A_REVERSE;
	win->styles[UI_STYLE_CURSOR_PRIMARY].attr |= A_REVERSE|A_BLINK;
	win->styles[UI_STYLE_SELECTION].attr |= A_REVERSE;
	win->styles[UI_STYLE_COLOR_COLUMN].attr |= A_REVERSE;

	win->ui = uic;
	win->view = view;
	win->file = file;
	view_ui(view, &win->uiwin);

	if (uic->windows)
		uic->windows->prev = win;
	win->next = uic->windows;
	uic->windows = win;

	ui_window_options_set((UiWin*)win, options);

	return &win->uiwin;
}

__attribute__((noreturn)) static void ui_die(Ui *ui, const char *msg, va_list ap) {
	UiCurses *uic = (UiCurses*)ui;
	endwin();
	if (uic->termkey)
		termkey_stop(uic->termkey);
	vfprintf(stderr, msg, ap);
	exit(EXIT_FAILURE);
}

static void ui_info(Ui *ui, const char *msg, va_list ap) {
	UiCurses *uic = (UiCurses*)ui;
	vsnprintf(uic->info, sizeof(uic->info), msg, ap);
	ui_draw(ui);
}

static void ui_info_hide(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (uic->info[0]) {
		uic->info[0] = '\0';
		ui_draw(ui);
	}
}

static bool ui_init(Ui *ui, Vis *vis) {
	UiCurses *uic = (UiCurses*)ui;
	uic->vis = vis;
	return true;
}

static bool ui_start(Ui *ui) {
	Vis *vis = ((UiCurses*)ui)->vis;
	const char *theme = getenv("VIS_THEME");
	if (theme && theme[0]) {
		if (!vis_theme_load(vis, theme))
			vis_info_show(vis, "Warning: failed to load theme `%s'", theme);
	} else {
		theme = COLORS <= 16 ? "default-16" : "default-256";
		if (!vis_theme_load(vis, theme))
			vis_info_show(vis, "Warning: failed to load theme `%s' set $VIS_PATH", theme);
	}
	return true;
}

static TermKey *ui_termkey_get(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	return uic->termkey;
}

static void ui_suspend(Ui *ui) {
	endwin();
	raise(SIGSTOP);
}

static bool ui_haskey(Ui *ui) {
	nodelay(stdscr, TRUE);
	int c = getch();
	if (c != ERR)
		ungetch(c);
	nodelay(stdscr, FALSE);
	return c != ERR;
}

static const char *ui_getkey(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	TermKeyKey key;
	TermKeyResult ret = termkey_getkey(uic->termkey, &key);

	if (ret == TERMKEY_RES_AGAIN) {
		struct pollfd fd;
		fd.fd = STDIN_FILENO;
		fd.events = POLLIN;
		if (poll(&fd, 1, termkey_get_waittime(uic->termkey)) == 0)
			ret = termkey_getkey_force(uic->termkey, &key);
	}

	if (ret != TERMKEY_RES_KEY)
		return NULL;
	termkey_strfkey(uic->termkey, uic->key, sizeof(uic->key), &key, TERMKEY_FORMAT_VIM);
	return uic->key;
}

static void ui_terminal_save(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	curs_set(1);
	reset_shell_mode();
	termkey_stop(uic->termkey);
}

static void ui_terminal_restore(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	termkey_start(uic->termkey);
	reset_prog_mode();
	wclear(stdscr);
	curs_set(0);
}

Ui *ui_curses_new(void) {

	UiCurses *uic = calloc(1, sizeof(UiCurses));
	Ui *ui = (Ui*)uic;
	if (!uic)
		return NULL;
	if (!(uic->termkey = termkey_new(STDIN_FILENO, TERMKEY_FLAG_UTF8)))
		goto err;
	termkey_set_canonflags(uic->termkey, TERMKEY_CANON_DELBS);
	setlocale(LC_CTYPE, "");
	if (!getenv("ESCDELAY"))
		set_escdelay(50);
	char *term = getenv("TERM");
	if (!term)
		term = "xterm";
	if (!newterm(term, stderr, stdin)) {
		snprintf(uic->info, sizeof(uic->info), "Warning: unknown term `%s'", term);
		if (!newterm(strstr(term, "-256color") ? "xterm-256color" : "xterm", stderr, stdin))
			goto err;
	}
	start_color();
	use_default_colors();
	raw();
	noecho();
	nonl();
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	curs_set(0);
	/* needed because we use getch() which implicitly calls refresh() which
	   would clear the screen (overwrite it with an empty / unused stdscr */
	refresh();

	*ui = (Ui) {
		.init = ui_init,
		.start = ui_start,
		.free = ui_curses_free,
		.termkey_get = ui_termkey_get,
		.suspend = ui_suspend,
		.resize = ui_resize,
		.update = ui_update,
		.window_new = ui_window_new,
		.window_free = ui_window_free,
		.window_focus = ui_window_focus,
		.draw = ui_draw,
		.redraw = ui_redraw,
		.arrange = ui_arrange,
		.die = ui_die,
		.info = ui_info,
		.info_hide = ui_info_hide,
		.haskey = ui_haskey,
		.getkey = ui_getkey,
		.terminal_save = ui_terminal_save,
		.terminal_restore = ui_terminal_restore,
	};

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigwinch_handler;
	sigaction(SIGWINCH, &sa, NULL);
	sigaction(SIGCONT, &sa, NULL);

	ui_resize(ui);

	return ui;
err:
	ui_curses_free(ui);
	return NULL;
}

void ui_curses_free(Ui *ui) {
	UiCurses *uic = (UiCurses*)ui;
	if (!uic)
		return;
	while (uic->windows)
		ui_window_free((UiWin*)uic->windows);
	endwin();
	if (uic->termkey)
		termkey_destroy(uic->termkey);
	free(uic);
}
