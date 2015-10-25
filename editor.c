#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include "editor.h"
#include "util.h"
#include "text-motions.h"
#include "text-util.h"

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

void editor_window_name(Win *win, const char *filename) {
	File *file = win->file;
	free((char*)file->name);
	file->name = filename ? strdup(filename) : NULL;
	
	if (filename) {
		for (Syntax *syn = win->editor->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				view_syntax_set(win->view, syn);
				break;
			}
		}
	}
}

void editor_windows_arrange(Editor *ed, enum UiLayout layout) {
	ed->ui->arrange(ed->ui, layout);
}

bool editor_window_reload(Win *win) {
	const char *name = win->file->name;
	if (!name)
		return false; /* can't reload unsaved file */
	/* temporarily unset file name, otherwise file_new returns the same File */
	win->file->name = NULL;
	File *file = file_new(win->editor, name);
	win->file->name = name;
	if (!file)
		return false;
	file_free(win->editor, win->file);
	win->file = file;
	win->ui->reload(win->ui, file);
	return true;
}

bool editor_window_split(Win *original) {
	Win *win = window_new_file(original->editor, original->file);
	if (!win)
		return false;
	win->file = original->file;
	win->file->refcount++;
	view_syntax_set(win->view, view_syntax_get(original->view));
	view_options_set(win->view, view_options_get(original->view));
	view_cursor_to(win->view, view_cursor_get(original->view));
	editor_draw(win->editor);
	return true;
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
				view_draw(win->view);
		}
	}
	view_draw(ed->win->view);
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

bool editor_syntax_load(Editor *ed, Syntax *syntaxes) {
	bool success = true;
	ed->syntaxes = syntaxes;

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

bool editor_mode_map(Mode *mode, const char *name, KeyBinding *binding) {
	return map_put(mode->bindings, name, binding);
}

bool editor_mode_bindings(Mode *mode, KeyBinding **bindings) {
	if (!mode->bindings)
		mode->bindings = map_new();
	if (!mode->bindings)
		return false;
	bool success = true;
	for (KeyBinding *kb = *bindings; kb->key; kb++) {
		if (!editor_mode_map(mode, kb->key, kb))
			success = false;
	}
	return success;
}

bool editor_mode_unmap(Mode *mode, const char *name) {
	return map_delete(mode->bindings, name);
}

bool editor_action_register(Editor *ed, KeyAction *action) {
	if (!ed->actions)
		ed->actions = map_new();
	if (!ed->actions)
		return false;
	return map_put(ed->actions, action->name, action);
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
	win->ui = ed->ui->window_new(ed->ui, win->view, file);
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
	free((char*)file->name);
	
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
	file->stat = text_stat(text);
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
			if (file->name && strcmp(file->name, filename) == 0) {
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
	
	File *file = file_new_text(ed, text);
	if (!file) {
		text_free(text);
		return NULL;
	}

	if (filename)
		file->name = strdup(filename);
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

	editor_window_name(win, filename);
	editor_draw(ed);

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
	if (ed->prompt_window == win)
		ed->prompt_window = NULL;
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
	if (!(ed->prompt->ui = ed->ui->prompt_new(ed->ui, ed->prompt->view, ed->prompt->file)))
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
	map_free(ed->actions);
	buffer_release(&ed->buffer_repeat);
	free(ed);
}

void editor_insert(Editor *ed, size_t pos, const char *data, size_t len) {
	text_insert(ed->win->file->text, pos, data, len);
	editor_windows_invalidate(ed, pos, pos + len);
}

void editor_insert_key(Editor *ed, const char *data, size_t len) {
	for (Cursor *c = view_cursors(ed->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		editor_insert(ed, pos, data, len);
		view_cursors_scroll_to(c, pos + len);
	}
}

void editor_replace(Editor *ed, size_t pos, const char *data, size_t len) {
	size_t chars = 0;
	for (size_t i = 0; i < len; i++) {
		if (ISUTF8(data[i]))
			chars++;
	}

	Text *txt = ed->win->file->text;
	Iterator it = text_iterator_get(txt, pos);
	for (char c; chars-- > 0 && text_iterator_byte_get(&it, &c) && c != '\r' && c != '\n'; )
		text_iterator_char_next(&it, NULL);

	text_delete(txt, pos, it.pos - pos);
	editor_insert(ed, pos, data, len);
}

void editor_replace_key(Editor *ed, const char *data, size_t len) {
	for (Cursor *c = view_cursors(ed->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		editor_replace(ed, pos, data, len);
		view_cursors_scroll_to(c, pos + len);
	}
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
