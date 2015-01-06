#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "editor.h"
#include "util.h"

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

static EditorWin *editor_window_new_text(Editor *ed, Text *text);
static void editor_window_free(Editor *ed, EditorWin *win);
static void editor_windows_invalidate(Editor *ed, size_t start, size_t end);
static void editor_window_draw(EditorWin *win);
static void windows_arrange_horizontal(Editor *ed);
static void windows_arrange_vertical(Editor *ed);

static Prompt *editor_prompt_new();
static void editor_prompt_free(Prompt *prompt);
static void editor_prompt_clear(Prompt *prompt);
static void editor_prompt_resize(Prompt *prompt, int width, int height);
static void editor_prompt_move(Prompt *prompt, int x, int y);
static void editor_prompt_draw(Prompt *prompt);
static void editor_prompt_update(Prompt *prompt);
static void editor_info_draw(Editor *ed);

static void editor_window_resize(EditorWin *win, int width, int height) {
	window_resize(win->win, width, win->statuswin ?  height - 1 : height);
	if (win->statuswin)
		wresize(win->statuswin, 1, width);
	win->width = width;
	win->height = height;
}

static void editor_window_move(EditorWin *win, int x, int y) {
	window_move(win->win, x, y);
	if (win->statuswin)
		mvwin(win->statuswin, y + win->height - 1, x);
}

static void editor_window_statusbar_draw(EditorWin *win) {
	if (win->statuswin && win->editor->statusbar)
		win->editor->statusbar(win);
}

static void editor_window_cursor_moved(Win *winwin, void *data) {
	EditorWin *win = data;
	Filerange sel = window_selection_get(winwin);
	if (text_range_valid(&sel) && sel.start != sel.end) {
		text_mark_intern_set(win->text, MARK_SELECTION_START, sel.start);
		text_mark_intern_set(win->text, MARK_SELECTION_END, sel.end);
	}
	editor_window_statusbar_draw(win);
}

void editor_statusbar_set(Editor *ed, void (*statusbar)(EditorWin*)) {
	ed->statusbar = statusbar;
}

static void windows_arrange_horizontal(Editor *ed) {
	int n = 0, x = 0, y = 0;
	for (EditorWin *win = ed->windows; win; win = win->next)
		n++;
	int height = ed->height / MAX(1, n);
	for (EditorWin *win = ed->windows; win; win = win->next) {
		editor_window_resize(win, ed->width, win->next ? height : ed->height - y);
		editor_window_move(win, x, y);
		y += height;
	}
}

static void windows_arrange_vertical(Editor *ed) {
	int n = 0, x = 0, y = 0;
	for (EditorWin *win = ed->windows; win; win = win->next)
		n++;
	int width = (ed->width / MAX(1, n)) - 1;
	for (EditorWin *win = ed->windows; win; win = win->next) {
		editor_window_resize(win, win->next ? width : ed->width - x, ed->height);
		editor_window_move(win, x, y);
		x += width;
		if (win->next)
			mvvline(0, x++, ACS_VLINE, ed->height);
	}
}

bool editor_window_reload(EditorWin *win) {
	const char *filename = text_filename_get(win->text);
	/* can't reload unsaved file */
	if (!filename)
		return false;
	Text *text = text_load(filename);
	if (!text)
		return false;
	/* check wether the text is displayed in another window */
	bool needed = false;
	for (EditorWin *w = win->editor->windows; w; w = w->next) {
		if (w != win && w->text == win->text) {
			needed = true;
			break;
		}
	}
	if (!needed)
		text_free(win->text);
	win->text = text;
	window_reload(win->win, text);
	return true;
}

void editor_windows_arrange_vertical(Editor *ed) {
	ed->windows_arrange = windows_arrange_vertical;
	editor_draw(ed);
}

void editor_windows_arrange_horizontal(Editor *ed) {
	ed->windows_arrange = windows_arrange_horizontal;
	editor_draw(ed);
}

bool editor_window_split(EditorWin *original) {
	EditorWin *win = editor_window_new_text(original->editor, original->text);
	if (!win)
		return false;
	win->text = original->text;
	window_syntax_set(win->win, window_syntax_get(original->win));
	window_cursor_to(win->win, window_cursor_get(original->win));
	editor_draw(win->editor);
	return true;
}

void editor_window_jumplist_add(EditorWin *win, size_t pos) {
	Mark mark = text_mark_set(win->text, pos);
	if (mark)
		ringbuf_add(win->jumplist, mark);
}

size_t editor_window_jumplist_prev(EditorWin *win) {
	size_t cur = window_cursor_get(win->win);
	for (;;) {
		Mark mark = ringbuf_prev(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

size_t editor_window_jumplist_next(EditorWin *win) {
	size_t cur = window_cursor_get(win->win);
	for (;;) {
		Mark mark = ringbuf_next(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

void editor_window_jumplist_invalidate(EditorWin *win) {
	ringbuf_invalidate(win->jumplist);
}

size_t editor_window_changelist_prev(EditorWin *win) {
	size_t pos = window_cursor_get(win->win);
	if (pos != win->changelist.pos)
		win->changelist.index = 0;
	else
		win->changelist.index++;
	size_t newpos = text_history_get(win->text, win->changelist.index);
	if (newpos == EPOS)
		win->changelist.index--;
	else
		win->changelist.pos = newpos;
	return win->changelist.pos;
}

size_t editor_window_changelist_next(EditorWin *win) {
	size_t pos = window_cursor_get(win->win);
	if (pos != win->changelist.pos)
		win->changelist.index = 0;
	else if (win->changelist.index > 0)
		win->changelist.index--;
	size_t newpos = text_history_get(win->text, win->changelist.index);
	if (newpos == EPOS)
		win->changelist.index++;
	else
		win->changelist.pos = newpos;
	return win->changelist.pos;
}

void editor_resize(Editor *ed, int width, int height) {
	ed->width = width;
	ed->height = height;
	if (ed->info[0]) {
		ed->height--;
	} else if (ed->prompt->active) {
		ed->height--;
		editor_prompt_resize(ed->prompt, ed->width, 1);
		editor_prompt_move(ed->prompt, 0, ed->height);
		editor_prompt_draw(ed->prompt);
	}
	editor_draw(ed);
}

void editor_window_next(Editor *ed) {
	EditorWin *sel = ed->win;
	if (!sel)
		return;
	ed->win = ed->win->next;
	if (!ed->win)
		ed->win = ed->windows;
	editor_window_statusbar_draw(sel);
	editor_window_statusbar_draw(ed->win);
}

void editor_window_prev(Editor *ed) {
	EditorWin *sel = ed->win;
	if (!sel)
		return;
	ed->win = ed->win->prev;
	if (!ed->win)
		for (ed->win = ed->windows; ed->win->next; ed->win = ed->win->next);
	editor_window_statusbar_draw(sel);
	editor_window_statusbar_draw(ed->win);
}

static void editor_windows_invalidate(Editor *ed, size_t start, size_t end) {
	for (EditorWin *win = ed->windows; win; win = win->next) {
		if (ed->win != win && ed->win->text == win->text) {
			Filerange view = window_viewport_get(win->win);
			if ((view.start <= start && start <= view.end) ||
			    (view.start <= end && end <= view.end))
				editor_window_draw(win);
		}
	}
	editor_window_draw(ed->win);
}

int editor_tabwidth_get(Editor *ed) {
	return ed->tabwidth;
}

void editor_tabwidth_set(Editor *ed, int tabwidth) {
	if (tabwidth < 1 || tabwidth > 8)
		return;
	for (EditorWin *win = ed->windows; win; win = win->next)
		window_tabwidth_set(win->win, tabwidth);
	ed->tabwidth = tabwidth;
}

bool editor_syntax_load(Editor *ed, Syntax *syntaxes, Color *colors) {
	bool success = true;
	ed->syntaxes = syntaxes;

	for (Color *color = colors; color && color->fg; color++) {
		if (color->attr == 0)
			color->attr = A_NORMAL;
		color->attr |= COLOR_PAIR(editor_color_get(color->fg, color->bg));
	}

	for (Syntax *syn = syntaxes; syn && syn->name; syn++) {
		if (regcomp(&syn->file_regex, syn->file, REG_EXTENDED|REG_NOSUB|REG_ICASE|REG_NEWLINE))
			success = false;
		for (int j = 0; j < LENGTH(syn->rules); j++) {
			SyntaxRule *rule = &syn->rules[j];
			if (!rule->rule)
				break;
			int cflags = REG_EXTENDED;
			if (!rule->multiline)
				cflags |= REG_NEWLINE;
			if (regcomp(&rule->regex, rule->rule, cflags))
				success = false;
		}
	}

	return success;
}

void editor_syntax_unload(Editor *ed) {
	for (Syntax *syn = ed->syntaxes; syn && syn->name; syn++) {
		regfree(&syn->file_regex);
		for (int j = 0; j < LENGTH(syn->rules); j++) {
			SyntaxRule *rule = &syn->rules[j];
			if (!rule->rule)
				break;
			regfree(&rule->regex);
		}
	}

	ed->syntaxes = NULL;
}

static void editor_window_draw(EditorWin *win) {
	// TODO window_draw draw should restore cursor position
	window_draw(win->win);
	window_cursor_to(win->win, window_cursor_get(win->win));
}

void editor_draw(Editor *ed) {
	erase();
	if (ed->windows) {
		ed->windows_arrange(ed);
		for (EditorWin *win = ed->windows; win; win = win->next) {
			if (ed->win != win)
				editor_window_draw(win);
		}
		editor_window_draw(ed->win);
	}
	if (ed->info[0])
		editor_info_draw(ed);
	wnoutrefresh(stdscr);
}

void editor_update(Editor *ed) {
	for (EditorWin *win = ed->windows; win; win = win->next) {
		if (ed->win != win) {
			if (win->statuswin)
				wnoutrefresh(win->statuswin);
			window_update(win->win);
		}
	}

	if (ed->win->statuswin)
		wnoutrefresh(ed->win->statuswin);
	if (ed->prompt && ed->prompt->active)
		editor_prompt_update(ed->prompt);
	window_update(ed->win->win);
}

static void editor_window_free(Editor *ed, EditorWin *win) {
	if (!win)
		return;
	window_free(win->win);
	if (win->statuswin)
		delwin(win->statuswin);
	ringbuf_free(win->jumplist);
	bool needed = false;
	for (EditorWin *w = ed ? ed->windows : NULL; w; w = w->next) {
		if (w->text == win->text) {
			needed = true;
			break;
		}
	}
	if (!needed)
		text_free(win->text);
	free(win);
}

static EditorWin *editor_window_new_text(Editor *ed, Text *text) {
	EditorWin *win = calloc(1, sizeof(EditorWin));
	if (!win)
		return NULL;
	win->editor = ed;
	win->text = text;
	win->win = window_new(win->text);
	win->statuswin = newwin(1, ed->width, 0, 0);
	win->jumplist = ringbuf_alloc(31);
	if (!win->win || !win->statuswin || !win->jumplist) {
		editor_window_free(ed, win);
		return NULL;
	}
	window_cursor_watch(win->win, editor_window_cursor_moved, win);
	window_tabwidth_set(win->win, ed->tabwidth);
	if (ed->windows)
		ed->windows->prev = win;
	win->next = ed->windows;
	win->prev = NULL;
	ed->windows = win;
	ed->win = win;
	return win;
}

bool editor_window_new(Editor *ed, const char *filename) {
	Text *text = NULL;
	/* try to detect whether the same file is already open in another window
	 * TODO: do this based on inodes */
	EditorWin *original = NULL;
	if (filename) {
		for (EditorWin *win = ed->windows; win; win = win->next) {
			const char *f = text_filename_get(win->text);
			if (f && strcmp(f, filename) == 0) {
				original = win;
				break;
			}
		}
	}

	if (original)
		text = original->text;
	else
		text = text_load(filename && access(filename, F_OK) == 0 ? filename : NULL);
	if (!text)
		return false;

	EditorWin *win = editor_window_new_text(ed, text);
	if (!win) {
		if (!original)
			text_free(text);
		return false;
	}

	if (filename) {
		text_filename_set(text, filename);
		for (Syntax *syn = ed->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				window_syntax_set(win->win, syn);
				break;
			}
		}
	} else if (original) {
		window_syntax_set(win->win, window_syntax_get(original->win));
		window_cursor_to(win->win, window_cursor_get(original->win));
	}

	editor_draw(ed);

	return true;
}

bool editor_window_new_fd(Editor *ed, int fd) {
	Text *txt = text_load_fd(fd);
	if (!txt)
		return false;
	EditorWin *win = editor_window_new_text(ed, txt);
	if (!win)
		return false;
	editor_draw(ed);
	return true;
}

static void editor_window_detach(Editor *ed, EditorWin *win) {
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (ed->windows == win)
		ed->windows = win->next;
	win->next = win->prev = NULL;
}

void editor_window_close(EditorWin *win) {
	Editor *ed = win->editor;
	if (ed->win == win)
		ed->win = win->next ? win->next : win->prev;
	editor_window_detach(ed, win);
	editor_window_free(ed, win);
	editor_draw(ed);
}

Editor *editor_new(int width, int height) {
	Editor *ed = calloc(1, sizeof(Editor));
	if (!ed)
		return NULL;
	if (!(ed->prompt = editor_prompt_new()))
		goto err;
	if (!(ed->search_pattern = text_regex_new()))
		goto err;
	ed->width = width;
	ed->height = height;
	ed->tabwidth = 8;
	ed->expandtab = false;
	ed->windows_arrange = windows_arrange_horizontal;
	return ed;
err:
	editor_free(ed);
	return NULL;
}

void editor_free(Editor *ed) {
	if (!ed)
		return;
	while (ed->windows)
		editor_window_close(ed->windows);
	editor_prompt_free(ed->prompt);
	text_regex_free(ed->search_pattern);
	for (int i = 0; i < REG_LAST; i++)
		register_free(&ed->registers[i]);
	editor_syntax_unload(ed);
	free(ed);
}

void editor_insert_key(Editor *ed, const char *c, size_t len) {
	Win *win = ed->win->win;
	size_t start = window_cursor_get(win);
	window_insert_key(win, c, len);
	editor_windows_invalidate(ed, start, start + len);
}

void editor_replace_key(Editor *ed, const char *c, size_t len) {
	Win *win = ed->win->win;
	size_t start = window_cursor_get(win);
	window_replace_key(win, c, len);
	editor_windows_invalidate(ed, start, start + 6);
}

void editor_backspace_key(Editor *ed) {
	Win *win = ed->win->win;
	size_t end = window_cursor_get(win);
	size_t start = window_backspace_key(win);
	editor_windows_invalidate(ed, start, end);
}

void editor_delete_key(Editor *ed) {
	size_t start = window_delete_key(ed->win->win);
	editor_windows_invalidate(ed, start, start + 6);
}

void editor_insert(Editor *ed, size_t pos, const char *c, size_t len) {
	text_insert(ed->win->text, pos, c, len);
	editor_windows_invalidate(ed, pos, pos + len);
}

void editor_delete(Editor *ed, size_t pos, size_t len) {
	text_delete(ed->win->text, pos, len);
	editor_windows_invalidate(ed, pos, pos + len);
}

static void editor_prompt_free(Prompt *prompt) {
	if (!prompt)
		return;
	editor_window_free(NULL, prompt->win);
	if (prompt->titlewin)
		delwin(prompt->titlewin);
	free(prompt->title);
	free(prompt);
}

static Prompt *editor_prompt_new() {
	Text *text = text_load(NULL);
	if (!text)
		return NULL;
	Prompt *prompt = calloc(1, sizeof(Prompt));
	if (!prompt)
		goto err;

	if (!(prompt->win = calloc(1, sizeof(EditorWin))))
		goto err;

	if (!(prompt->win->win = window_new(text)))
		goto err;

	prompt->win->text = text;

	if (!(prompt->titlewin = newwin(0, 0, 0, 0)))
		goto err;

	return prompt;
err:
	if (!prompt || !prompt->win)
		text_free(text);
	editor_prompt_free(prompt);
	return NULL;
}

static void editor_prompt_resize(Prompt *prompt, int width, int height) {
	size_t title_width = strlen(prompt->title);
	wresize(prompt->titlewin, height, title_width);
	editor_window_resize(prompt->win, width - title_width, height);
}

static void editor_prompt_move(Prompt *prompt, int x, int y) {
	size_t title_width = strlen(prompt->title);
	mvwin(prompt->titlewin, y, x);
	editor_window_move(prompt->win, x + title_width, y);
}

void editor_prompt_show(Editor *ed, const char *title, const char *text) {
	Prompt *prompt = ed->prompt;
	if (prompt->active)
		return;
	prompt->active = true;
	prompt->editor = ed->win;
	free(prompt->title);
	prompt->title = strdup(title);
	text_insert(prompt->win->text, 0, text, strlen(text));
	window_cursor_to(prompt->win->win, text_size(prompt->win->text));
	ed->win = prompt->win;
	editor_resize(ed, ed->width, ed->height);
}

static void editor_prompt_draw(Prompt *prompt) {
	mvwaddstr(prompt->titlewin, 0, 0, prompt->title);
}

static void editor_prompt_update(Prompt *prompt) {
	wnoutrefresh(prompt->titlewin);
}

static void editor_prompt_clear(Prompt *prompt) {
	Text *text = prompt->win->text;
	while (text_undo(text) != EPOS);
	window_cursor_to(prompt->win->win, 0);
}

void editor_prompt_hide(Editor *ed) {
	Prompt *prompt = ed->prompt;
	if (!prompt->active)
		return;
	prompt->active = false;
	ed->win = prompt->editor;
	prompt->editor = NULL;
	ed->height++;
	editor_prompt_clear(prompt);
	editor_draw(ed);
}

void editor_prompt_set(Editor *ed, const char *line) {
	Text *text = ed->prompt->win->text;
	editor_prompt_clear(ed->prompt);
	text_insert(text, 0, line, strlen(line));
	editor_window_draw(ed->prompt->win);
}

char *editor_prompt_get(Editor *ed) {
	Text *text = ed->prompt->win->text;
	char *buf = malloc(text_size(text) + 1);
	if (!buf)
		return NULL;
	size_t len = text_bytes_get(text, 0, text_size(text), buf);
	buf[len] = '\0';
	return buf;
}

void editor_info_show(Editor *ed, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vsnprintf(ed->info, sizeof(ed->info), msg, ap);
	va_end(ap);
	editor_resize(ed, ed->width, ed->height);
}

void editor_info_hide(Editor *ed) {
	if (!ed->info[0])
		return;
	ed->info[0] = '\0';
	ed->height++;
	editor_draw(ed);
}

static void editor_info_draw(Editor *ed) {
	attrset(A_BOLD);
	mvaddstr(ed->height, 0, ed->info);
}

static unsigned int color_hash(short fg, short bg)
{
	if (fg == -1)
		fg = COLORS;
	if (bg == -1)
		bg = COLORS + 1;
	return fg * (COLORS + 2) + bg;
}

short editor_color_get(short fg, short bg)
{
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

	unsigned int index = color_hash(fg, bg);
	if (color2palette[index] == 0) {
		short oldfg, oldbg;
		if (++color_pair_current >= color_pairs_max)
			color_pair_current = 1;
		pair_content(color_pair_current, &oldfg, &oldbg);
		unsigned int old_index = color_hash(oldfg, oldbg);
		if (init_pair(color_pair_current, fg, bg) == OK) {
			color2palette[old_index] = 0;
			color2palette[index] = color_pair_current;
		}
	}

	return color2palette[index];
}
