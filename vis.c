#define _BSD_SOURCE
#include <stdlib.h>
#include <string.h>
#include "vis.h"
#include "util.h"

static VisWin *vis_window_new_text(Vis *vis, Text *text); 
static void vis_window_free(Vis *vis, VisWin *win);
static void vis_window_split_internal(Vis *vis, const char *filename); 
static void vis_windows_invalidate(Vis *vis, size_t start, size_t end); 
static void vis_window_draw(VisWin *win);
static void vis_windows_arrange_horizontal(Vis *vis); 
static void vis_windows_arrange_vertical(Vis *vis);

static Prompt *vis_prompt_new();
static void vis_prompt_free(Prompt *prompt);
static void vis_prompt_clear(Prompt *prompt);
static void vis_prompt_resize(Prompt *prompt, int width, int height);
static void vis_prompt_move(Prompt *prompt, int x, int y);
static void vis_prompt_draw(Prompt *prompt);
static void vis_prompt_update(Prompt *prompt);

static void vis_window_resize(VisWin *win, int width, int height) {
	window_resize(win->win, width, win->statuswin ?  height - 1 : height);
	if (win->statuswin)
		wresize(win->statuswin, 1, width);
	win->width = width;
	win->height = height;
}

static void vis_window_move(VisWin *win, int x, int y) {
	window_move(win->win, x, y);
	if (win->statuswin)
		mvwin(win->statuswin, y + win->height - 1, x);
}

static void vis_window_statusbar_draw(VisWin *win) {
	size_t line, col;
	if (win->statuswin && win->vis->statusbar) {
		window_cursor_getxy(win->win, &line, &col);
		win->vis->statusbar(win->statuswin, win->vis->win == win,
		                    text_filename(win->text), line, col);
	}
}

static void vis_window_cursor_moved(Win *win, void *data) {
	vis_window_statusbar_draw(data);
}

void vis_statusbar_set(Vis *vis, vis_statusbar_t statusbar) {
	vis->statusbar = statusbar;
}

static void vis_windows_arrange_horizontal(Vis *vis) {
	int n = 0, x = 0, y = 0;
	for (VisWin *win = vis->windows; win; win = win->next)
		n++;
	int height = vis->height / n; 
	for (VisWin *win = vis->windows; win; win = win->next) {
		vis_window_resize(win, vis->width, win->next ? height : vis->height - y);
		vis_window_move(win, x, y);
		y += height;
	}
}

static void vis_windows_arrange_vertical(Vis *vis) {
	int n = 0, x = 0, y = 0;
	for (VisWin *win = vis->windows; win; win = win->next)
		n++;
	int width = vis->width / n; 
	for (VisWin *win = vis->windows; win; win = win->next) {
		vis_window_resize(win, win->next ? width : vis->width - x, vis->height);
		vis_window_move(win, x, y);
		x += width;
	}
}

static void vis_window_split_internal(Vis *vis, const char *filename) {
	VisWin *sel = vis->win;
	if (filename) {
		// TODO check that this file isn't already open
		vis_window_new(vis, filename);
	} else if (sel) {
		VisWin *win = vis_window_new_text(vis, sel->text);
		win->text = sel->text;
		window_syntax_set(win->win, window_syntax_get(sel->win));
		window_cursor_to(win->win, window_cursor_get(sel->win));
	}
}

void vis_window_split(Vis *vis, const char *filename) {
	vis_window_split_internal(vis, filename);
	vis->windows_arrange = vis_windows_arrange_horizontal;
	vis_draw(vis);
}

void vis_window_vsplit(Vis *vis, const char *filename) {
	vis_window_split_internal(vis, filename);
	vis->windows_arrange = vis_windows_arrange_vertical;
	vis_draw(vis);
}

void vis_resize(Vis *vis, int width, int height) {
	vis->width = width;
	vis->height = height;
	if (vis->prompt->active) {
		vis->height--;
		vis_prompt_resize(vis->prompt, vis->width, 1);
		vis_prompt_move(vis->prompt, 0, vis->height);
		vis_prompt_draw(vis->prompt);
	}
	vis_draw(vis);
}

void vis_window_next(Vis *vis) {
	VisWin *sel = vis->win;
	if (!sel)
		return;
	vis->win = vis->win->next;
	if (!vis->win)
		vis->win = vis->windows;
	vis_window_statusbar_draw(sel);
	vis_window_statusbar_draw(vis->win);
}

void vis_window_prev(Vis *vis) {
	VisWin *sel = vis->win;
	if (!sel)
		return;
	vis->win = vis->win->prev;
	if (!vis->win)
		for (vis->win = vis->windows; vis->win->next; vis->win = vis->win->next);
	vis_window_statusbar_draw(sel);
	vis_window_statusbar_draw(vis->win);
}

static void vis_windows_invalidate(Vis *vis, size_t start, size_t end) {
	for (VisWin *win = vis->windows; win; win = win->next) {
		if (vis->win != win && vis->win->text == win->text) {
			Filerange view = window_viewport_get(win->win);
			if ((view.start <= start && start <= view.end) ||
			    (view.start <= end && end <= view.end))
				vis_window_draw(win);
		}
	}
	vis_window_draw(vis->win);
}


bool vis_syntax_load(Vis *vis, Syntax *syntaxes, Color *colors) {
	bool success = true;
	vis->syntaxes = syntaxes;
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
				rule->color.attr |= COLOR_PAIR(vis_color_reserve(rule->color.fg, rule->color.bg));
			if (regcomp(&rule->regex, rule->rule, REG_EXTENDED|rule->cflags))
				success = false;
		}
	}

	return success;
}

void vis_syntax_unload(Vis *vis) {
	for (Syntax *syn = vis->syntaxes; syn && syn->name; syn++) {
		regfree(&syn->file_regex);
		for (int j = 0; j < LENGTH(syn->rules); j++) {
			SyntaxRule *rule = &syn->rules[j];
			if (!rule->rule)
				break;
			regfree(&rule->regex);
		}
	}

	vis->syntaxes = NULL;
}

static void vis_window_draw(VisWin *win) {
	// TODO window_draw draw should restore cursor position
	window_draw(win->win);
	window_cursor_to(win->win, window_cursor_get(win->win));
}

void vis_draw(Vis *vis) {
	erase();
	wnoutrefresh(stdscr);
	vis->windows_arrange(vis);
	for (VisWin *win = vis->windows; win; win = win->next) {
		if (vis->win != win)
			vis_window_draw(win);
	}
	vis_window_draw(vis->win);
}

void vis_update(Vis *vis) {
	for (VisWin *win = vis->windows; win; win = win->next) {
		if (vis->win != win) {
			if (win->statuswin)
				wnoutrefresh(win->statuswin);
			window_update(win->win);
		}
	}

	if (vis->win->statuswin)
		wnoutrefresh(vis->win->statuswin);
	if (vis->prompt && vis->prompt->active)
		vis_prompt_update(vis->prompt);
	window_update(vis->win->win);
}

static void vis_window_free(Vis *vis, VisWin *win) {
	if (!win)
		return;
	window_free(win->win);
	if (win->statuswin)
		delwin(win->statuswin);
	bool needed = false;
	for (VisWin *w = vis ? vis->windows : NULL; w; w = w->next) {
		if (w->text == win->text) {
			needed = true;
			break;
		}
	}
	if (!needed)
		text_free(win->text);
	free(win);
}

static VisWin *vis_window_new_text(Vis *vis, Text *text) {
	VisWin *win = calloc(1, sizeof(VisWin));
	if (!win)
		return NULL;
	win->vis = vis;
	win->text = text;
	win->win = window_new(win->text);
	win->statuswin = newwin(1, vis->width, 0, 0);
	if (!win->win || !win->statuswin) {
		vis_window_free(vis, win);
		return NULL;
	}
	window_cursor_watch(win->win, vis_window_cursor_moved, win);
	if (vis->windows)
		vis->windows->prev = win;
	win->next = vis->windows;
	win->prev = NULL;
	vis->windows = win;
	vis->win = win;
	return win;
}

bool vis_window_new(Vis *vis, const char *filename) {
	Text *text = text_load(filename);
	if (!text)
		return false;

	VisWin *win = vis_window_new_text(vis, text);
	if (!win) {
		text_free(text);
		return false;
	}

	if (filename) {
		for (Syntax *syn = vis->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				window_syntax_set(win->win, syn);
				break;
			}
		}
	}
	
	vis_draw(vis);

	return true;
}

static void vis_window_detach(Vis *vis, VisWin *win) {
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (vis->windows == win)
		vis->windows = win->next;
	win->next = win->prev = NULL;
}

void vis_window_close(Vis *vis) {
	VisWin *win = vis->win;
	vis->win = win->next ? win->next : win->prev;
	vis_window_detach(vis, win);
	vis_window_free(vis, win);
}

Vis *vis_new(int width, int height) {
	Vis *vis = calloc(1, sizeof(Vis));
	if (!vis)
		return NULL;
	if (!(vis->prompt = vis_prompt_new()))
		goto err;
	if (!(vis->search_pattern = text_regex_new()))
		goto err;
	vis->width = width;
	vis->height = height;
	vis->windows_arrange = vis_windows_arrange_horizontal;
	vis->running = true;
	return vis;
err:
	vis_free(vis);
	return NULL;
}

void vis_free(Vis *vis) {
	while (vis->windows)
		vis_window_close(vis);
	vis_prompt_free(vis->prompt);
	text_regex_free(vis->search_pattern);
	for (int i = 0; i < REG_LAST; i++)
		register_free(&vis->registers[i]);
	vis_syntax_unload(vis);
	free(vis);
}

void vis_insert_key(Vis *vis, const char *c, size_t len) {
	Win *win = vis->win->win;
	size_t start = window_cursor_get(win);
	window_insert_key(win, c, len);
	vis_windows_invalidate(vis, start, start + len);
}

void vis_replace_key(Vis *vis, const char *c, size_t len) {
	Win *win = vis->win->win;
	size_t start = window_cursor_get(win);
	window_replace_key(win, c, len);
	vis_windows_invalidate(vis, start, start + 6);
}

void vis_backspace_key(Vis *vis) {
	Win *win = vis->win->win;
	size_t end = window_cursor_get(win);
	size_t start = window_backspace_key(win);
	vis_windows_invalidate(vis, start, end);
}

void vis_delete_key(Vis *vis) {
	size_t start = window_delete_key(vis->win->win);
	vis_windows_invalidate(vis, start, start + 6);
}

void vis_insert(Vis *vis, size_t pos, const char *c, size_t len) {
	text_insert_raw(vis->win->text, pos, c, len);
	vis_windows_invalidate(vis, pos, pos + len);
}

void vis_delete(Vis *vis, size_t pos, size_t len) {
	text_delete(vis->win->text, pos, len);
	vis_windows_invalidate(vis, pos, pos + len);
}


static void vis_prompt_free(Prompt *prompt) {
	if (!prompt)
		return;
	vis_window_free(NULL, prompt->win);
	if (prompt->titlewin)
		delwin(prompt->titlewin);
	free(prompt->title);
	free(prompt);
}

static Prompt *vis_prompt_new() {
	Text *text = text_load(NULL);
	if (!text)
		return NULL;
	Prompt *prompt = calloc(1, sizeof(Prompt));
	if (!prompt)
		goto err;

	if (!(prompt->win = calloc(1, sizeof(VisWin))))
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
	vis_prompt_free(prompt);
	return NULL;
}

static void vis_prompt_resize(Prompt *prompt, int width, int height) {
	size_t title_width = strlen(prompt->title);
	wresize(prompt->titlewin, height, title_width);
	vis_window_resize(prompt->win, width - title_width, height);
}

static void vis_prompt_move(Prompt *prompt, int x, int y) {
	size_t title_width = strlen(prompt->title);
	mvwin(prompt->titlewin, y, x);
	vis_window_move(prompt->win, x + title_width, y);
}

void vis_prompt_show(Vis *vis, const char *title) {
	Prompt *prompt = vis->prompt;
	if (prompt->active)
		return;
	prompt->active = true;
	prompt->editor = vis->win;
	vis->win = prompt->win;
	free(prompt->title);
	prompt->title = strdup(title);
	vis_resize(vis, vis->width, vis->height);
}

static void vis_prompt_draw(Prompt *prompt) {
	mvwaddstr(prompt->titlewin, 0, 0, prompt->title);
}

static void vis_prompt_update(Prompt *prompt) {
	wnoutrefresh(prompt->titlewin);
}

static void vis_prompt_clear(Prompt *prompt) {
	Text *text = prompt->win->text;
	while (text_undo(text));
}

void vis_prompt_hide(Vis *vis) {
	Prompt *prompt = vis->prompt;
	if (!prompt->active)
		return;
	prompt->active = false;
	vis->win = prompt->editor;
	prompt->editor = NULL;
	vis->height++;
	vis_prompt_clear(prompt);
	vis_draw(vis);
}

void vis_prompt_set(Vis *vis, const char *line) {
	Text *text = vis->prompt->win->text;
	vis_prompt_clear(vis->prompt);
	text_insert_raw(text, 0, line, strlen(line));
	vis_window_draw(vis->prompt->win);
}

char *vis_prompt_get(Vis *vis) {
	Text *text = vis->prompt->win->text;
	char *buf = malloc(text_size(text) + 1);
	if (!buf)
		return NULL;
	size_t len = text_bytes_get(text, 0, text_size(text), buf);
	buf[len] = '\0';
	return buf;
}
