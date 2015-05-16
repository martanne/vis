#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include "editor.h"
#include "util.h"

static void file_free(Editor *ed, File *file);
static File *file_new(Editor *ed, const char *filename);
static Win *window_new_file(Editor *ed, File *file);
static void window_free(Win *win);
static void editor_windows_invalidate(Editor *ed, size_t start, size_t end);

static void window_selection_changed(void *win, Filerange *sel) {
	File *file = ((Win*)win)->file;
	if (text_range_valid(sel)) {
		file->marks[MARK_SELECTION_START] = text_mark_set(file->text, sel->start);
		file->marks[MARK_SELECTION_END] = text_mark_set(file->text, sel->end);
	}
}

void editor_windows_arrange(Editor *ed, enum UiLayout layout) {
	ed->ui->arrange(ed->ui, layout);
}

bool editor_window_reload(Win *win) {
	const char *filename = text_filename_get(win->file->text);
	/* can't reload unsaved file */
	if (!filename)
		return false;
	File *file = file_new(win->editor, filename);
	if (!file)
		return false;
	file_free(win->editor, win->file);
	win->file = file;
	view_reload(win->view, file->text);
	return true;
}

bool editor_window_split(Win *original) {
	Win *win = window_new_file(original->editor, original->file);
	if (!win)
		return false;
	win->file = original->file;
	win->file->refcount++;
	view_syntax_set(win->view, view_syntax_get(original->view));
	view_cursor_to(win->view, view_cursor_get(original->view));
	editor_draw(win->editor);
	return true;
}

void editor_window_jumplist_add(Win *win, size_t pos) {
	Mark mark = text_mark_set(win->file->text, pos);
	if (mark && win->jumplist)
		ringbuf_add(win->jumplist, mark);
}

size_t editor_window_jumplist_prev(Win *win) {
	size_t cur = view_cursor_get(win->view);
	while (win->jumplist) {
		Mark mark = ringbuf_prev(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->file->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

size_t editor_window_jumplist_next(Win *win) {
	size_t cur = view_cursor_get(win->view);
	while (win->jumplist) {
		Mark mark = ringbuf_next(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->file->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

void editor_window_jumplist_invalidate(Win *win) {
	if (win->jumplist)
		ringbuf_invalidate(win->jumplist);
}

size_t editor_window_changelist_prev(Win *win) {
	size_t pos = view_cursor_get(win->view);
	if (pos != win->changelist.pos)
		win->changelist.index = 0;
	else
		win->changelist.index++;
	size_t newpos = text_history_get(win->file->text, win->changelist.index);
	if (newpos == EPOS)
		win->changelist.index--;
	else
		win->changelist.pos = newpos;
	return win->changelist.pos;
}

size_t editor_window_changelist_next(Win *win) {
	size_t pos = view_cursor_get(win->view);
	if (pos != win->changelist.pos)
		win->changelist.index = 0;
	else if (win->changelist.index > 0)
		win->changelist.index--;
	size_t newpos = text_history_get(win->file->text, win->changelist.index);
	if (newpos == EPOS)
		win->changelist.index++;
	else
		win->changelist.pos = newpos;
	return win->changelist.pos;
}

void editor_resize(Editor *ed) {
	ed->ui->resize(ed->ui);
}

void editor_window_next(Editor *ed) {
	Win *sel = ed->win;
	if (!sel)
		return;
	ed->win = ed->win->next;
	if (!ed->win)
		ed->win = ed->windows;
	ed->ui->window_focus(ed->win->ui);
}

void editor_window_prev(Editor *ed) {
	Win *sel = ed->win;
	if (!sel)
		return;
	ed->win = ed->win->prev;
	if (!ed->win)
		for (ed->win = ed->windows; ed->win->next; ed->win = ed->win->next);
	ed->ui->window_focus(ed->win->ui);
}

static void editor_windows_invalidate(Editor *ed, size_t start, size_t end) {
	for (Win *win = ed->windows; win; win = win->next) {
		if (ed->win != win && ed->win->file == win->file) {
			Filerange view = view_viewport_get(win->view);
			if ((view.start <= start && start <= view.end) ||
			    (view.start <= end && end <= view.end))
				win->ui->draw(win->ui);
		}
	}
	ed->win->ui->draw(ed->win->ui);
}

int editor_tabwidth_get(Editor *ed) {
	return ed->tabwidth;
}

void editor_tabwidth_set(Editor *ed, int tabwidth) {
	if (tabwidth < 1 || tabwidth > 8)
		return;
	for (Win *win = ed->windows; win; win = win->next)
		view_tabwidth_set(win->view, tabwidth);
	ed->tabwidth = tabwidth;
}

bool editor_syntax_load(Editor *ed, Syntax *syntaxes, Color *colors) {
	bool success = true;
	ed->syntaxes = syntaxes;

	for (Color *color = colors; color && color->fg; color++) {
		if (color->attr == 0)
			color->attr = A_NORMAL;
		color->attr |= COLOR_PAIR(ed->ui->color_get(color->fg, color->bg));
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

void editor_draw(Editor *ed) {
	ed->ui->draw(ed->ui);
}

void editor_update(Editor *ed) {
	ed->ui->update(ed->ui);
}

void editor_suspend(Editor *ed) {
	ed->ui->suspend(ed->ui);
}

static void window_free(Win *win) {
	if (!win)
		return;
	Editor *ed = win->editor;
	if (ed && ed->ui)
		ed->ui->window_free(win->ui);
	view_free(win->view);
	ringbuf_free(win->jumplist);
	free(win);
}

static Win *window_new_file(Editor *ed, File *file) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->editor = ed;
	win->file = file;
	win->events = (ViewEvent) {
		.data = win,
		.selection = window_selection_changed,
	};
	win->jumplist = ringbuf_alloc(31);
	win->view = view_new(file->text, &win->events);
	win->ui = ed->ui->window_new(ed->ui, win->view, file->text);
	if (!win->jumplist || !win->view || !win->ui) {
		window_free(win);
		return NULL;
	}
	view_tabwidth_set(win->view, ed->tabwidth);
	if (ed->windows)
		ed->windows->prev = win;
	win->next = ed->windows;
	ed->windows = win;
	ed->win = win;
	ed->ui->window_focus(win->ui);
	return win;
}

static void file_free(Editor *ed, File *file) {
	if (!file)
		return;
	if (--file->refcount > 0)
		return;
	
	text_free(file->text);
	
	if (file->prev)
		file->prev->next = file->next;
	if (file->next)
		file->next->prev = file->prev;
	if (ed->files == file)
		ed->files = file->next;
	free(file);
}

static File *file_new_text(Editor *ed, Text *text) {
	File *file = calloc(1, sizeof(*file));
	if (!file)
		return NULL;
	file->text = text;
	file->refcount++;
	if (ed->files)
		ed->files->prev = file;
	file->next = ed->files;
	ed->files = file;
	return file;
}

static File *file_new(Editor *ed, const char *filename) {
	if (filename) {
		/* try to detect whether the same file is already open in another window
		 * TODO: do this based on inodes */
		for (File *file = ed->files; file; file = file->next) {
			const char *name = text_filename_get(file->text);
			if (name && strcmp(name, filename) == 0) {
				file->refcount++;
				return file;
			}
		}
	}

	Text *text = text_load(filename);
	if (!text && filename && errno == ENOENT)
		text = text_load(NULL);
	if (!text)
		return NULL;
	if (filename)
		text_filename_set(text, filename);
	
	File *file = file_new_text(ed, text);
	if (!file) {
		text_free(text);
		return NULL;
	}

	return file;
}

bool editor_window_new(Editor *ed, const char *filename) {
	File *file = file_new(ed, filename);
	if (!file)
		return false;
	Win *win = window_new_file(ed, file);
	if (!win) {
		file_free(ed, file);
		return false;
	}

	if (filename) {
		for (Syntax *syn = ed->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				view_syntax_set(win->view, syn);
				break;
			}
		}
	}

	editor_draw(ed);

	return true;
}

bool editor_window_new_fd(Editor *ed, int fd) {
	Text *text = text_load_fd(fd);
	if (!text)
		return false;
	File *file = file_new_text(ed, text);
	if (!file) {
		file_free(ed, file);
		return false;
	}
	Win *win = window_new_file(ed, file);
	if (!win) {
		file_free(ed, file);
		return false;
	}
	return true;
}

void editor_window_close(Win *win) {
	Editor *ed = win->editor;
	file_free(ed, win->file);
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (ed->windows == win)
		ed->windows = win->next;
	if (ed->win == win)
		ed->win = win->next ? win->next : win->prev;
	window_free(win);
	if (ed->win)
		ed->ui->window_focus(ed->win->ui);
	editor_draw(ed);
}

Editor *editor_new(Ui *ui) {
	if (!ui)
		return NULL;
	Editor *ed = calloc(1, sizeof(Editor));
	if (!ed)
		return NULL;
	ed->ui = ui;
	ed->ui->init(ed->ui, ed);
	ed->tabwidth = 8;
	ed->expandtab = false;
	if (!(ed->prompt = calloc(1, sizeof(Win))))
		goto err;
	if (!(ed->prompt->file = calloc(1, sizeof(File))))
		goto err;
	if (!(ed->prompt->file->text = text_load(NULL)))
		goto err;
	if (!(ed->prompt->view = view_new(ed->prompt->file->text, NULL)))
		goto err;
	if (!(ed->prompt->ui = ed->ui->prompt_new(ed->ui, ed->prompt->view, ed->prompt->file->text)))
		goto err;
	if (!(ed->search_pattern = text_regex_new()))
		goto err;
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
	file_free(ed, ed->prompt->file);
	window_free(ed->prompt);
	text_regex_free(ed->search_pattern);
	for (int i = 0; i < REG_LAST; i++)
		register_release(&ed->registers[i]);
	for (int i = 0; i < MACRO_LAST; i++)
		macro_release(&ed->macros[i]);
	editor_syntax_unload(ed);
	ed->ui->free(ed->ui);
	map_free(ed->cmds);
	map_free(ed->options);
	buffer_release(&ed->buffer_repeat);
	free(ed);
}

void editor_insert_key(Editor *ed, const char *c, size_t len) {
	View *view = ed->win->view;
	size_t start = view_cursor_get(view);
	view_insert_key(view, c, len);
	editor_windows_invalidate(ed, start, start + len);
}

void editor_replace_key(Editor *ed, const char *c, size_t len) {
	View *view = ed->win->view;
	size_t start = view_cursor_get(view);
	view_replace_key(view, c, len);
	editor_windows_invalidate(ed, start, start + 6);
}

void editor_backspace_key(Editor *ed) {
	View *view = ed->win->view;
	size_t end = view_cursor_get(view);
	size_t start = view_backspace_key(view);
	editor_windows_invalidate(ed, start, end);
}

void editor_delete_key(Editor *ed) {
	size_t start = view_delete_key(ed->win->view);
	editor_windows_invalidate(ed, start, start + 6);
}

void editor_insert(Editor *ed, size_t pos, const char *c, size_t len) {
	text_insert(ed->win->file->text, pos, c, len);
	editor_windows_invalidate(ed, pos, pos + len);
}

void editor_delete(Editor *ed, size_t pos, size_t len) {
	text_delete(ed->win->file->text, pos, len);
	editor_windows_invalidate(ed, pos, pos + len);
}

void editor_prompt_show(Editor *ed, const char *title, const char *text) {
	if (ed->prompt_window)
		return;
	ed->prompt_window = ed->win;
	ed->win = ed->prompt;
	ed->prompt_type = title[0];
	ed->ui->prompt(ed->ui, title, text);
}

void editor_prompt_hide(Editor *ed) {
	if (!ed->prompt_window)
		return;
	ed->ui->prompt_hide(ed->ui);
	ed->win = ed->prompt_window;
	ed->prompt_window = NULL;
}

char *editor_prompt_get(Editor *ed) {
	return ed->ui->prompt_input(ed->ui);
}

void editor_info_show(Editor *ed, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	ed->ui->info(ed->ui, msg, ap);
	va_end(ap);
}

void editor_info_hide(Editor *ed) {
	ed->ui->info_hide(ed->ui);
}

void editor_window_options(Win *win, enum UiOption options) {
	win->ui->options(win->ui, options);
}

