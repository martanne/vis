#define _BSD_SOURCE
#include <stdlib.h>
#include <string.h>
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
static void editor_window_split_internal(Editor *ed, const char *filename);
static void editor_windows_invalidate(Editor *ed, size_t start, size_t end);
static void editor_window_draw(EditorWin *win);
static void editor_windows_arrange_horizontal(Editor *ed);
static void editor_windows_arrange_vertical(Editor *ed);

static Prompt *editor_prompt_new();
static void editor_prompt_free(Prompt *prompt);
static void editor_prompt_clear(Prompt *prompt);
static void editor_prompt_resize(Prompt *prompt, int width, int height);
static void editor_prompt_move(Prompt *prompt, int x, int y);
static void editor_prompt_draw(Prompt *prompt);
static void editor_prompt_update(Prompt *prompt);

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

static void editor_window_cursor_moved(Win *win, void *data) {
	editor_window_statusbar_draw(data);
}

void editor_statusbar_set(Editor *ed, void (*statusbar)(EditorWin*)) {
	ed->statusbar = statusbar;
}

static void editor_windows_arrange_horizontal(Editor *ed) {
	int n = 0, x = 0, y = 0;
	for (EditorWin *win = ed->windows; win; win = win->next)
		n++;
	int height = ed->height / n;
	for (EditorWin *win = ed->windows; win; win = win->next) {
		editor_window_resize(win, ed->width, win->next ? height : ed->height - y);
		editor_window_move(win, x, y);
		y += height;
	}
}

static void editor_windows_arrange_vertical(Editor *ed) {
	int n = 0, x = 0, y = 0;
	for (EditorWin *win = ed->windows; win; win = win->next)
		n++;
	int width = ed->width / n - 1;
	for (EditorWin *win = ed->windows; win; win = win->next) {
		editor_window_resize(win, win->next ? width : ed->width - x, ed->height);
		editor_window_move(win, x, y);
		x += width;
		if (win->next)
			mvvline(0, x++, ACS_VLINE, ed->height);
	}
}

static void editor_window_split_internal(Editor *ed, const char *filename) {
	EditorWin *sel = ed->win;
	if (filename) {
		// TODO? move this to editor_window_new
		sel = NULL;
		for (EditorWin *w = ed->windows; w; w = w->next) {
			const char *f = text_filename(w->text);
			if (f && strcmp(f, filename) == 0) {
				sel = w;
				break;
			}
		}
	}
	if (sel) {
		EditorWin *win = editor_window_new_text(ed, sel->text);
		win->text = sel->text;
		window_syntax_set(win->win, window_syntax_get(sel->win));
		window_cursor_to(win->win, window_cursor_get(sel->win));
	} else {
		editor_window_new(ed, filename);
	}
}

void editor_window_split(Editor *ed, const char *filename) {
	editor_window_split_internal(ed, filename);
	ed->windows_arrange = editor_windows_arrange_horizontal;
	editor_draw(ed);
}

void editor_window_vsplit(Editor *ed, const char *filename) {
	editor_window_split_internal(ed, filename);
	ed->windows_arrange = editor_windows_arrange_vertical;
	editor_draw(ed);
}

void editor_resize(Editor *ed, int width, int height) {
	ed->width = width;
	ed->height = height;
	if (ed->prompt->active) {
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


bool editor_syntax_load(Editor *ed, Syntax *syntaxes, Color *colors) {
	bool success = true;
	ed->syntaxes = syntaxes;
	for (Syntax *syn = syntaxes; syn && syn->name; syn++) {
		if (regcomp(&syn->file_regex, syn->file, REG_EXTENDED|REG_NOSUB|REG_ICASE|REG_NEWLINE))
			success = false;
		Color *color = colors;
		for (int j = 0; j < LENGTH(syn->rules); j++) {
			SyntaxRule *rule = &syn->rules[j];
			if (!rule->rule)
				break;
			if (rule->color.fg == 0 && color && color->fg != 0)
				rule->color = *color++;
			if (rule->color.attr == 0)
				rule->color.attr = A_NORMAL;
			if (rule->color.fg != 0)
				rule->color.attr |= COLOR_PAIR(editor_color_get(rule->color.fg, rule->color.bg));
			if (regcomp(&rule->regex, rule->rule, REG_EXTENDED|rule->cflags))
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
	if (!win->win || !win->statuswin) {
		editor_window_free(ed, win);
		return NULL;
	}
	window_cursor_watch(win->win, editor_window_cursor_moved, win);
	if (ed->windows)
		ed->windows->prev = win;
	win->next = ed->windows;
	win->prev = NULL;
	ed->windows = win;
	ed->win = win;
	return win;
}

bool editor_window_new(Editor *ed, const char *filename) {
	Text *text = text_load(filename);
	if (!text)
		return false;

	EditorWin *win = editor_window_new_text(ed, text);
	if (!win) {
		text_free(text);
		return false;
	}

	if (filename) {
		for (Syntax *syn = ed->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				window_syntax_set(win->win, syn);
				break;
			}
		}
	}

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

void editor_window_close(Editor *ed) {
	EditorWin *win = ed->win;
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
	ed->windows_arrange = editor_windows_arrange_horizontal;
	return ed;
err:
	editor_free(ed);
	return NULL;
}

void editor_free(Editor *ed) {
	while (ed->windows)
		editor_window_close(ed);
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
	text_insert_raw(ed->win->text, pos, c, len);
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

void editor_prompt_show(Editor *ed, const char *title) {
	Prompt *prompt = ed->prompt;
	if (prompt->active)
		return;
	prompt->active = true;
	prompt->editor = ed->win;
	ed->win = prompt->win;
	free(prompt->title);
	prompt->title = strdup(title);
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
	while (text_undo(text));
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
	text_insert_raw(text, 0, line, strlen(line));
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
