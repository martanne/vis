#include <stdlib.h>
#include "vis.h"
#include "util.h"

static VisWin *vis_window_new_text(Vis *vis, Text *text); 
static void vis_window_free(VisWin *win);
static void vis_window_split_internal(Vis *vis, const char *filename); 
static void vis_windows_invalidate(Vis *vis, size_t start, size_t end); 
static void vis_window_draw(VisWin *win);
static void vis_search_forward(Vis *vis, Regex *regex);
static void vis_search_backward(Vis *vis, Regex *regex);
static void vis_windows_arrange_horizontal(Vis *vis); 
static void vis_windows_arrange_vertical(Vis *vis);


static void vis_window_resize(VisWin *win, int width, int height) {
	window_resize(win->win, width, height - 1);
	wresize(win->statuswin, 1, width);
	win->width = width;
	win->height = height;
}

static void vis_window_move(VisWin *win, int x, int y) {
	window_move(win->win, x, y);
	mvwin(win->statuswin, y + win->height - 1, x);
}

static void vis_window_statusbar_draw(VisWin *win) {
	size_t line, col;
	if (win->vis->statusbar) {
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

size_t vis_line_goto(Vis *vis, size_t lineno) {
	size_t pos = text_pos_by_lineno(vis->win->text, lineno);
	window_cursor_to(vis->win->win, pos);
	return pos;
}

static void vis_search_forward(Vis *vis, Regex *regex) {
	VisWin *win = vis->win;
	int pos = window_cursor_get(win->win) + 1;
	int end = text_size(win->text);
	RegexMatch match[1];
	bool found = false;
	if (text_search_forward(win->text, pos, end - pos, regex, 1, match, 0)) {
		pos = 0;
		end = window_cursor_get(win->win);
		if (!text_search_forward(win->text, pos, end, regex, 1, match, 0))
			found = true;
	} else {
		found = true;
	}
	if (found)
		window_cursor_to(win->win, match[0].start);
}

static void vis_search_backward(Vis *vis, Regex *regex) {
	VisWin *win = vis->win;
	int pos = 0;
	int end = window_cursor_get(win->win);
	RegexMatch match[1];
	bool found = false;
	if (text_search_backward(win->text, pos, end, regex, 1, match, 0)) {
		pos = window_cursor_get(win->win) + 1;
		end = text_size(win->text);
		if (!text_search_backward(win->text, pos, end - pos, regex, 1, match, 0))
			found = true;
	} else {
		found = true;
	}
	if (found)
		window_cursor_to(win->win, match[0].start);
}

void vis_search(Vis *vis, const char *s, int direction) {
	Regex *regex = text_regex_new();
	if (!regex)
		return;
	if (!text_regex_compile(regex, s, REG_EXTENDED)) {
		if (direction >= 0)
			vis_search_forward(vis, regex);
		else
			vis_search_backward(vis, regex);
	}
	text_regex_free(regex);
}

void vis_snapshot(Vis *vis) {
	text_snapshot(vis->win->text);
}

void vis_undo(Vis *vis) {
	VisWin *win = vis->win;
	// TODO window invalidation
	if (text_undo(win->text))
		window_draw(win->win);
}

void vis_redo(Vis *vis) {
	VisWin *win = vis->win;
	// TODO window invalidation
	if (text_redo(win->text))
		window_draw(win->win);
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

void vis_mark_set(Vis *vis, Mark mark, size_t pos) {
	text_mark_set(vis->win->text, mark, pos);
}

void vis_mark_goto(Vis *vis, Mark mark) {
	VisWin *win = vis->win;
	size_t pos = text_mark_get(win->text, mark);
	if (pos != (size_t)-1)
		window_cursor_to(win->win, pos);
}

void vis_mark_clear(Vis *vis, Mark mark) {
	text_mark_clear(vis->win->text, mark);
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
			wnoutrefresh(win->statuswin);
			window_update(win->win);
		}
	}

	wnoutrefresh(vis->win->statuswin);
	window_update(vis->win->win);
}

static void vis_window_free(VisWin *win) {
	if (!win)
		return;
	window_free(win->win);
	if (win->statuswin)
		delwin(win->statuswin);
	// XXX: open in other windows
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
		vis_window_free(win);
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

Vis *vis_new(int width, int height) {
	Vis *vis = calloc(1, sizeof(Vis));
	if (!vis)
		return NULL;
	vis->width = width;
	vis->height = height;
	vis->windows_arrange = vis_windows_arrange_horizontal;
	return vis;
}

void vis_free(Vis *vis) {
	for (VisWin *next, *win = vis->windows; win; win = next) {
		next = win->next;
		vis_window_free(win);
	}
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
