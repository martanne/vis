/* This file is included from ui-terminal.c */
#include <curses.h>

#define UI_TERMKEY_FLAGS (TERMKEY_FLAG_NOTERMIOS)

#ifndef A_ITALIC
#define A_ITALIC A_NORMAL
#endif

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

VIS_INTERNAL VisCellStyle
vis_ui_backend_style_default(Ui *ui)
{
	VisCellStyle result = {0};
	result.properties |= (VisCellProperty_IndexedFG|VisCellProperty_IndexedBG);
	result.properties |= (VisCellProperty_FGSet|VisCellProperty_BGSet);
	VisCellStyleFGIndexSet(&result, ui->curses.default_fg);
	VisCellStyleBGIndexSet(&result, ui->curses.default_bg);
	return result;
}

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
VIS_INTERNAL VisTerminalStyle
vis_terminal_style_rgb(Vis *vis, u8 r, u8 g, u8 b)
{
	VisTerminalStyle result = {.indexed = true};

	VisCursesUI *cui = &vis->ui.curses;
	static short color_clobber_idx = 0;
	static uint32_t clobbering_colors[MAX_COLOR_CLOBBER];

	if (cui->change_colors == -1)
		cui->change_colors = vis->change_colors && can_change_color() && COLORS >= 256;
	if (cui->change_colors) {
		uint32_t hexrep = (r << 16) | (g << 8) | b;
		for (short i = 0; i < MAX_COLOR_CLOBBER; ++i) {
			if (clobbering_colors[i] == hexrep) {
				result.color.index = i + 16;
				return result;
			} else if (!clobbering_colors[i]) {
				break;
			}
		}

		short i = color_clobber_idx;
		clobbering_colors[i] = hexrep;
		init_color(i + 16, (r * 1000 + 0xfe) / 0xff, (g * 1000 + 0xfe) / 0xff,
		           (b * 1000 + 0xfe) / 0xff);

		/* in the unlikely case a user requests this many colors, reuse old slots */
		if (++color_clobber_idx >= MAX_COLOR_CLOBBER)
			color_clobber_idx = 0;

		result.color.index = i + 16;
		return result;
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

	result.color.index = i;
	if (COLORS <= 16)
		result.color.index = color_256_to_16[i];
	return result;
}

VIS_INTERNAL VisTerminalStyle
vis_terminal_style_indexed(u8 index)
{
	VisTerminalStyle result = {0};
	result.indexed     = true;
	result.color.index = index;
	return result;
}

VIS_INTERNAL u32
vis_ui_curses_color_pair_hash(s16 fg, s16 bg)
{
	if (fg == -1) fg = COLORS;
	if (bg == -1) bg = COLORS + 1;
	u32 result = fg * (COLORS + 2) + bg;
	return result;
}

VIS_INTERNAL u16
vis_ui_curses_color_pair_get(Ui *ui, u16 fg, u16 bg)
{
	VisCursesUI *cui = &ui->curses;
	if (fg >= COLORS)
		fg = cui->default_fg;
	if (bg >= COLORS)
		bg = cui->default_bg;

	if (!cui->palette)
		return 0;

	u32 index = vis_ui_curses_color_pair_hash(fg, bg);
	if (cui->palette[index] == 0) {
		s16 oldfg, oldbg;
		if (++cui->color_pair_current >= cui->color_pairs_max)
			cui->color_pair_current = 1;
		pair_content(cui->color_pair_current, &oldfg, &oldbg);
		u32 old_index = vis_ui_curses_color_pair_hash(oldfg, oldbg);
		if (init_pair(cui->color_pair_current, fg, bg) == OK) {
			cui->palette[old_index] = 0;
			cui->palette[index] = cui->color_pair_current;
		}
	}

	return cui->palette[index];
}

VIS_INTERNAL attr_t
vis_ui_curses_style_to_attr(Ui *ui, VisCellStyle style)
{
	attr_t result = 0;
	result |= (style.attributes & VisCellAttribute_Underline) ? A_UNDERLINE : 0;
	result |= (style.attributes & VisCellAttribute_Reverse)   ? A_REVERSE   : 0;
	result |= (style.attributes & VisCellAttribute_Blink)     ? A_BLINK     : 0;
	result |= (style.attributes & VisCellAttribute_Bold)      ? A_BOLD      : 0;
	result |= (style.attributes & VisCellAttribute_Italic)    ? A_ITALIC    : 0;
	result |= (style.attributes & VisCellAttribute_Dim)       ? A_DIM       : 0;
	result |= COLOR_PAIR(vis_ui_curses_color_pair_get(ui, VisCellStyleFGIndexGet(&style),
	                                                  VisCellStyleBGIndexGet(&style)));
	return result;
}

static void ui_term_backend_blit(Ui *tui) {
	int w = tui->width, h = tui->height;
	VisCell *cell = tui->cell_buffer.cells;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			attrset(vis_ui_curses_style_to_attr(tui, cell->style));
			mvaddnstr(y, x, (char *)cell->data, cell->data_length);
			cell++;
		}
	}
	move(tui->cur_row, tui->cur_col);
	wnoutrefresh(stdscr);
	if (tui->doupdate)
		doupdate();
}

static void ui_term_backend_clear(Ui *tui) {
	clear();
}

static bool ui_term_backend_resize(Ui *tui, int width, int height) {
	return resizeterm(height, width) == OK &&
	       wresize(stdscr, height, width) == OK;
}

static void ui_term_backend_save(Ui *tui, bool fscr) {
	curs_set(1);
	if (fscr) {
		def_prog_mode();
		endwin();
	} else {
		reset_shell_mode();
	}
}

static void ui_term_backend_restore(Ui *tui) {
	reset_prog_mode();
	wclear(stdscr);
	curs_set(0);
}

static void ui_term_backend_cursor(Ui *tui, bool visible) {
	curs_set(visible ? 1 : 0);
}

int ui_terminal_colors(void) {
	return COLORS;
}

static bool
ui_backend_init(Ui *ui, char *term)
{
	bool result = newterm(term, stderr, stdin) != 0;
	if (!result) {
		snprintf(ui->info, sizeof(ui->info), "Warning: unknown term `%s'", term);
		result = newterm(strstr(term, "-256color") ? "xterm-256color" : "xterm", stderr, stdin) != 0;
	}

	if (result) {
		start_color();
		use_default_colors();
		cbreak();
		noecho();
		nonl();
		keypad(stdscr, TRUE);
		meta(stdscr, TRUE);
		curs_set(0);

		pair_content(0, &ui->curses.default_fg, &ui->curses.default_bg);
		ui->curses.change_colors   = -1;
		ui->curses.color_pairs_max = MIN(MAX_COLOR_PAIRS, SHRT_MAX);
		if (COLORS)
			ui->curses.palette = calloc((COLORS + 2) * (COLORS + 2), sizeof(s16));
	}

	return result;
}

VIS_INTERNAL void ui_terminal_resume(Ui *term) {}

VIS_INTERNAL void
ui_term_backend_suspend(Ui *ui)
{
	if (ui->curses.change_colors == 1)
		undo_palette();
}

VIS_INTERNAL void
vis_ui_backend_free(Ui *ui)
{
	ui_term_backend_suspend(ui);
	free(ui->curses.palette);
	endwin();
}
