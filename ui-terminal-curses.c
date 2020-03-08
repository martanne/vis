/* This file is included from ui-terminal.c */
#include <stdio.h>
#include <curses.h>

#define UI_TERMKEY_FLAGS (TERMKEY_FLAG_UTF8|TERMKEY_FLAG_NOTERMIOS)

#define ui_term_backend_init ui_curses_init
#define ui_term_backend_blit ui_curses_blit
#define ui_term_backend_clear ui_curses_clear
#define ui_term_backend_colors ui_curses_colors
#define ui_term_backend_resize ui_curses_resize
#define ui_term_backend_restore ui_curses_restore
#define ui_term_backend_save ui_curses_save
#define ui_term_backend_new ui_curses_new
#define ui_term_backend_resume ui_curses_resume
#define ui_term_backend_suspend ui_curses_suspend
#define ui_term_backend_free ui_curses_free

#define CELL_COLOR_BLACK   COLOR_BLACK
#define CELL_COLOR_RED     COLOR_RED
#define CELL_COLOR_GREEN   COLOR_GREEN
#define CELL_COLOR_YELLOW  COLOR_YELLOW
#define CELL_COLOR_BLUE    COLOR_BLUE
#define CELL_COLOR_MAGENTA COLOR_MAGENTA
#define CELL_COLOR_CYAN    COLOR_CYAN
#define CELL_COLOR_WHITE   COLOR_WHITE
#define CELL_COLOR_DEFAULT (-1)

#ifndef A_ITALIC
#define A_ITALIC A_NORMAL
#endif
#define CELL_ATTR_NORMAL    A_NORMAL
#define CELL_ATTR_UNDERLINE A_UNDERLINE
#define CELL_ATTR_REVERSE   A_REVERSE
#define CELL_ATTR_BLINK     A_BLINK
#define CELL_ATTR_BOLD      A_BOLD
#define CELL_ATTR_ITALIC    A_ITALIC

#ifdef NCURSES_VERSION
# ifndef NCURSES_EXT_COLORS
#  define NCURSES_EXT_COLORS 0
# endif
# if !NCURSES_EXT_COLORS
#  define MAX_COLOR_PAIRS MIN(COLOR_PAIRS, 256)
# endif
#endif
#ifndef MAX_COLOR_PAIRS
# define MAX_COLOR_PAIRS COLOR_PAIRS
#endif

#define MAX_COLOR_CLOBBER 240

static short color_clobber_idx = 0;
static uint32_t clobbering_colors[MAX_COLOR_CLOBBER];
static int change_colors = -1;

/* Calculate r,g,b components of one of the standard upper 240 colors */
static void get_6cube_rgb(unsigned int n, int *r, int *g, int *b)
{
	if (n < 16) {
		return;
	} else if (n < 232) {
		n -= 16;
		*r = (n / 36) ? (n / 36) * 40 + 55 : 0;
		*g = ((n / 6) % 6) ? ((n / 6) % 6) * 40 + 55 : 0;
		*b = (n % 6) ? (n % 6) * 40 + 55 : 0;
	} else if (n < 256) {
		n -= 232;
		*r = n * 10 + 8;
		*g = n * 10 + 8;
		*b = n * 10 + 8;
	}
}

/* Reset color palette to default values using OSC 104 */
static void undo_palette(void)
{
	fputs("\033]104;\a", stderr);
	fflush(stderr);
}

/* Work out the nearest color from the 256 color set, or perhaps exactly. */
static CellColor color_rgb(UiTerm *ui, uint8_t r, uint8_t g, uint8_t b)
{
	if (change_colors == -1)
		change_colors = ui->vis->change_colors && can_change_color() && COLORS >= 256;
	if (change_colors) {
		uint32_t hexrep = ((r << 16) | (g << 8) | b) + 1;
		for (short i = 0; i < MAX_COLOR_CLOBBER; ++i) {
			if (clobbering_colors[i] == hexrep)
				return i + 16;
			else if (!clobbering_colors[i])
				break;
		}

		short i = color_clobber_idx;
		clobbering_colors[i] = hexrep;
		init_color(i + 16, (r * 1000) / 0xff, (g * 1000) / 0xff,
		           (b * 1000) / 0xff);

		/* in the unlikely case a user requests this many colors, reuse old slots */
		if (++color_clobber_idx >= MAX_COLOR_CLOBBER)
			color_clobber_idx = 0;

		return i + 16;
	}

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

	int i = 0;
	if ((!r || (r - 55) % 40 == 0) &&
	    (!g || (g - 55) % 40 == 0) &&
	    (!b || (b - 55) % 40 == 0)) {
		i = 16;
		i += r ? ((r - 55) / 40) * 36 : 0;
		i += g ? ((g - 55) / 40) * 6 : 0;
		i += g ? ((b - 55) / 40) : 0;
	} else if (r == g && g == b && (r - 8) % 10 == 0 && r < 239) {
		i = 232 + ((r - 8) / 10);
	} else {
		unsigned lowest = UINT_MAX;
		for (int j = 16; j < 256; ++j) {
			int jr = 0, jg = 0, jb = 0;
			get_6cube_rgb(j, &jr, &jg, &jb);
			int dr = jr - r;
			int dg = jg - g;
			int db = jb - b;
			unsigned int distance = dr * dr + dg * dg + db * db;
			if (distance < lowest) {
				lowest = distance;
				i = j;
			}
		}
	}

	if (COLORS <= 16)
		return color_256_to_16[i];
	return i;
}

static CellColor color_terminal(UiTerm *ui, uint8_t index) {
	return index;
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
			default_fg = CELL_COLOR_WHITE;
		if (default_bg == -1)
			default_bg = CELL_COLOR_BLACK;
		has_default_colors = (use_default_colors() == OK);
		color_pairs_max = MIN(MAX_COLOR_PAIRS, SHRT_MAX);
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

static void ui_curses_blit(UiTerm *tui) {
	int w = tui->width, h = tui->height;
	Cell *cell = tui->cells;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			attrset(style_to_attr(&cell->style));
			mvaddstr(y, x, cell->data);
			cell++;
		}
	}
	wnoutrefresh(stdscr);
	doupdate();
}

static void ui_curses_clear(UiTerm *tui) {
	clear();
}

static bool ui_curses_resize(UiTerm *tui, int width, int height) {
	return resizeterm(height, width) == OK &&
	       wresize(stdscr, height, width) == OK;
}

static void ui_curses_save(UiTerm *tui) {
	curs_set(1);
	reset_shell_mode();
}

static void ui_curses_restore(UiTerm *tui) {
	reset_prog_mode();
	wclear(stdscr);
	curs_set(0);
}

static int ui_curses_colors(Ui *ui) {
	return COLORS;
}

static bool ui_curses_init(UiTerm *tui, char *term) {
	if (!newterm(term, stderr, stdin)) {
		snprintf(tui->info, sizeof(tui->info), "Warning: unknown term `%s'", term);
		if (!newterm(strstr(term, "-256color") ? "xterm-256color" : "xterm", stderr, stdin))
			return false;
	}
	start_color();
	use_default_colors();
	cbreak();
	noecho();
	nonl();
	keypad(stdscr, TRUE);
	meta(stdscr, TRUE);
	curs_set(0);
	return true;
}

static UiTerm *ui_curses_new(void) {
	return calloc(1, sizeof(UiTerm));
}

static void ui_curses_resume(UiTerm *term) { }

static void ui_curses_suspend(UiTerm *term) {
	if (change_colors == 1)
		undo_palette();
}

static void ui_curses_free(UiTerm *term) {
	ui_curses_suspend(term);
	endwin();
}

bool is_default_color(CellColor c) {
	return c == CELL_COLOR_DEFAULT;
}
