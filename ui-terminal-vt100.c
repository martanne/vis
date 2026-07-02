/* This file is included from ui-terminal.c
 *
 * The goal is *not* to reimplement curses. Instead we aim to provide the
 * simplest possible drawing backend for VT-100 compatible terminals.
 * This is useful for debugging and fuzzing purposes as well as for environments
 * with no curses support.
 *
 * The following terminal escape sequences are used:
 *
 *  - CSI ? 1049 h             Save cursor and use Alternate Screen Buffer (DECSET)
 *  - CSI ? 1049 l             Use Normal Screen Buffer and restore cursor (DECRST)
 *  - CSI ? 25 l               Hide Cursor (DECTCEM)
 *  - CSI ? 25 h               Show Cursor (DECTCEM)
 *  - CSI 2 J                  Erase in Display (ED)
 *  - CSI row ; column H       Cursor Position (CUP)
 *  - CSI ... m                Character Attributes (SGR)
 *    - CSI 0 m                     Normal
 *    - CSI 1 m                     Bold
 *    - CSI 3 m                     Italicized
 *    - CSI 4 m                     Underlined
 *    - CSI 5 m                     Blink
 *    - CSI 7 m                     Inverse
 *    - CSI 22 m                    Normal (not bold)
 *    - CSI 23 m                    Not italicized
 *    - CSI 24 m                    Not underlined
 *    - CSI 25 m                    Not blinking
 *    - CSI 27 m                    Not inverse
 *    - CSI 38 ; 2 ; R ; G ; B m    Set RGB foreground color
 *    - CSI 38 ; 5 ; I m            Set indexed foreground color
 *    - CSI 48 ; 2 ; R ; G ; B m    Set RGB background color
 *    - CSI 48 ; 5 ; I m            Set indexed background color
 *
 * See http://invisible-island.net/xterm/ctlseqs/ctlseqs.txt
 * for further information.
 */
#define UI_TERMKEY_FLAGS 0

VIS_INTERNAL VisCellStyle
vis_ui_backend_style_default(Ui *ui)
{
	VisCellStyle result = {0};
	result.properties |= (VisCellProperty_IndexedFG|VisCellProperty_IndexedBG);
	result.properties |= (VisCellProperty_FGSet|VisCellProperty_BGSet);
	VisCellStyleBGIndexSet(&result, 0);
	VisCellStyleFGIndexSet(&result, 7);
	return result;
}

VIS_INTERNAL VisTerminalStyle
vis_terminal_style_fg(VisCellStyle a)
{
	VisTerminalStyle result = {0};
	result.indexed = (a.properties & VisCellProperty_IndexedFG) != 0;
	if (result.indexed) {
		result.color.index = VisCellStyleFGIndexGet(&a);
	} else {
		result.color.rgb.r = a.fg_r;
		result.color.rgb.g = a.fg_g;
		result.color.rgb.b = a.fg_b;
	}
	return result;
}

VIS_INTERNAL VisTerminalStyle
vis_terminal_style_bg(VisCellStyle a)
{
	VisTerminalStyle result = {0};
	result.indexed = (a.properties & VisCellProperty_IndexedBG) != 0;
	if (result.indexed) {
		result.color.index = VisCellStyleBGIndexGet(&a);
	} else {
		result.color.rgb.r = a.bg_r;
		result.color.rgb.g = a.bg_g;
		result.color.rgb.b = a.bg_b;
	}
	return result;
}

VIS_INTERNAL bool
vis_terminal_style_equal(VisTerminalStyle a, VisTerminalStyle b)
{
	bool result = a.indexed == b.indexed &&
	              a.color.rgb.r == b.color.rgb.r &&
	              a.color.rgb.g == b.color.rgb.g &&
	              a.color.rgb.b == b.color.rgb.b;
	return result;
}

VIS_INTERNAL VisTerminalStyle
vis_terminal_style_rgb(Vis *vis, u8 r, u8 g, u8 b)
{
	VisTerminalStyle result = {0};
	result.color.rgb.r = r;
	result.color.rgb.g = g;
	result.color.rgb.b = b;
	return result;
}

VIS_INTERNAL VisTerminalStyle
vis_terminal_style_indexed(u16 index)
{
	VisTerminalStyle result = {0};
	result.indexed     = true;
	result.color.index = index;
	return result;
}

VIS_INTERNAL void
vis_ui_vt100_output(str8 s)
{
	// TODO(rnp): eintr
	(void)write(STDERR_FILENO, s.data, s.length);
}

VIS_INTERNAL void
vis_ui_vt100_altscreen(bool enable)
{
	vis_ui_vt100_output(enable ? str8("\x1b[?1049h") : str8("\x1b[?1049l" "\x1b[0m"));
}

VIS_INTERNAL void
vis_ui_vt100_cursor_visible(bool visible)
{
	vis_ui_vt100_output(visible ? str8("\x1b[?25h") : str8("\x1b[?25l"));
}

VIS_INTERNAL bool
vis_cell_equal(VisCell *a, VisCell *b)
{
	return memcmp(a, b, sizeof(*a)) == 0;
}

VIS_INTERNAL void
ui_term_backend_blit(Ui *ui)
{
	VisVT100UI *vt  = &ui->vt100;
	Buffer     *buf = &vt->output_buffer;
	buf->length = 0;

	VisCell *bb = ui->cell_buffer.cells;
	VisCell *fb = vt->cell_buffer.cells;

	if unlikely(vt->flush_terminal) {
		memset(fb, 0, vt->cell_buffer.size);
		vt->flush_terminal = false;
	}

	VisTerminalStyle fg = {0};
	VisTerminalStyle bg = {0};
	s32 cursor_x = 0, cursor_y = 0;
	u8  attributes = 0;
	s32 width  = ui->width;
	s32 height = ui->height;

	/* reposition cursor, reset attributes */
	str8 command = str8("\x1b[H" "\x1b[0m");
	buffer_append(buf, command.data, command.length);
	for (s32 y = 0; y < height; y++) {
		for (s32 x = 0; x < width; x++) {
			if (!vis_cell_equal(fb, bb)) {
				if (cursor_x != x || cursor_y != y) {
					vis_buffer_appendf(buf, "\x1b[%d;%dH", y + 1, x + 1);
					cursor_x = x;
					cursor_y = y;
				}

				VisCellStyle style = bb->style;
				if (style.attributes != attributes) {
					static const struct {
						u8 flag;
						char on[2], off[4];
					} cell_attrs[] = {
						{VisCellAttribute_Bold,      "1", "22"},
						{VisCellAttribute_Dim,       "2", "22"},
						{VisCellAttribute_Italic,    "3", "23"},
						{VisCellAttribute_Underline, "4", "24"},
						{VisCellAttribute_Blink,     "5", "25"},
						{VisCellAttribute_Reverse,   "7", "27"},
					};

					for (u64 i = 0; i < countof(cell_attrs); i++) {
						u8 flag = cell_attrs[i].flag;
						if ((attributes & flag) != (style.attributes & flag)) {
							vis_buffer_appendf(buf, "\x1b[%sm", (style.attributes & flag) ?
							                   cell_attrs[i].on :
							                   cell_attrs[i].off);
						}
					}
					attributes = style.attributes;
				}

				VisTerminalStyle style_fg = vis_terminal_style_fg(style);
				if (!vis_terminal_style_equal(fg, style_fg)) {
					if (style_fg.indexed) {
						vis_buffer_appendf(buf, "\x1b[38;5;%um", (u32)style_fg.color.index);
					} else {
						vis_buffer_appendf(buf, "\x1b[38;2;%u;%u;%um", (u32)style_fg.color.rgb.r,
						                   (u32)style_fg.color.rgb.g, (u32)style_fg.color.rgb.b);
					}
					fg = style_fg;
				}

				VisTerminalStyle style_bg = vis_terminal_style_bg(style);
				if (!vis_terminal_style_equal(bg, style_bg)) {
					if (style_bg.indexed) {
						vis_buffer_appendf(buf, "\x1b[48;5;%um", (u32)style_bg.color.index);
					} else {
						vis_buffer_appendf(buf, "\x1b[48;2;%u;%u;%um", (u32)style_bg.color.rgb.r,
						                   (u32)style_bg.color.rgb.g, (u32)style_bg.color.rgb.b);
					}
					bg = style_bg;
				}

				buffer_append(buf, bb->data, bb->data_length);
				memory_copy(fb, bb, sizeof(*fb));

				cursor_x += bb->width;
				if (cursor_x >= width) {
					cursor_x = 0;
					cursor_y++;
				}
			}

			fb++;
			bb++;
		}
	}

	// NOTE(rnp): maintain cursor position in case it gets queried through escape codes
	vis_buffer_appendf(buf, "\x1b[%d;%dH", ui->cur_row + 1, ui->cur_col + 1);
	vis_ui_vt100_output((str8){.data = (u8 *)buf->data, .length = buf->length});
}

VIS_INTERNAL void ui_term_backend_clear(Ui *ui) {}

VIS_INTERNAL void
ui_term_backend_save(Ui *ui, bool fscr)
{
	vis_ui_vt100_cursor_visible(true);
}

VIS_INTERNAL void
ui_term_backend_restore(Ui *ui)
{
	vis_ui_vt100_cursor_visible(false);
	ui->vt100.flush_terminal = true;
}

VIS_INTERNAL void
ui_term_backend_cursor(Ui *ui, bool visible)
{
	vis_ui_vt100_cursor_visible(visible);
}

VIS_INTERNAL bool
ui_term_backend_resize(Ui *ui, int width, int height)
{
	bool result = vis_cell_buffer_resize(&ui->vt100.cell_buffer, width, height);
	// NOTE(rnp): clear screen on successful resize
	if (result) vis_ui_vt100_output(str8("\x1b[J"));
	return result;
}

int ui_terminal_colors(void) {
	char *term = getenv("TERM");
	return (term && strstr(term, "-256color")) ? 256 : 16;
}

VIS_INTERNAL void
ui_term_backend_suspend(Ui *tui)
{
	termkey_stop(&tui->termkey);
	vis_ui_vt100_altscreen(false);
	vis_ui_vt100_cursor_visible(true);
}

VIS_INTERNAL void
ui_terminal_resume(Ui *tui)
{
	vis_ui_vt100_altscreen(true);
	vis_ui_vt100_cursor_visible(false);
	termkey_start(&tui->termkey, UI_TERMKEY_FLAGS);
}

static bool
ui_backend_init(Ui *ui, char *term)
{
	ui_terminal_resume(ui);
	return true;
}

VIS_INTERNAL void
vis_ui_backend_free(Ui *ui)
{
	ui_term_backend_suspend(ui);
	if (ui->vt100.cell_buffer.size) munmap(ui->vt100.cell_buffer.cells, ui->vt100.cell_buffer.size);
	buffer_release(&ui->vt100.output_buffer);
}
