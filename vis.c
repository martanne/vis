#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <regex.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pwd.h>
#include <termkey.h>

#include "vis.h"
#include "text-util.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"
#include "vis-core.h"
#include "sam.h"

/* enable large file optimization for files larger than: */
#define LARGE_FILE (1 << 25)

static Macro *macro_get(Vis *vis, enum VisRegister);
static void macro_replay(Vis *vis, const Macro *macro);

/** window / file handling */

static void file_free(Vis *vis, File *file) {
	if (!file)
		return;
	if (file->refcount > 1) {
		--file->refcount;
		return;
	}
	if (vis->event && vis->event->file_close)
		vis->event->file_close(vis, file);
	text_free(file->text);
	free((char*)file->name);

	if (file->prev)
		file->prev->next = file->next;
	if (file->next)
		file->next->prev = file->prev;
	if (vis->files == file)
		vis->files = file->next;
	free(file);
}

static File *file_new_text(Vis *vis, Text *text) {
	File *file = calloc(1, sizeof(*file));
	if (!file)
		return NULL;
	file->text = text;
	file->stat = text_stat(text);
	if (vis->files)
		vis->files->prev = file;
	file->next = vis->files;
	vis->files = file;
	return file;
}

static File *file_new(Vis *vis, const char *filename) {
	if (filename) {
		/* try to detect whether the same file is already open in another window
		 * TODO: do this based on inodes */
		for (File *file = vis->files; file; file = file->next) {
			if (file->name && strcmp(file->name, filename) == 0) {
				return file;
			}
		}
	}

	Text *text = text_load(filename);
	if (!text && filename && errno == ENOENT)
		text = text_load(NULL);
	if (!text)
		return NULL;

	File *file = file_new_text(vis, text);
	if (!file) {
		text_free(text);
		return NULL;
	}

	if (filename)
		file->name = strdup(filename);
	if (vis->event && vis->event->file_open)
		vis->event->file_open(vis, file);
	return file;
}

static File *file_new_internal(Vis *vis, const char *filename) {
	File *file = file_new(vis, filename);
	if (file) {
		file->refcount = 1;
		file->internal = true;
	}
	return file;
}

void vis_window_name(Win *win, const char *filename) {
	File *file = win->file;
	if (filename != file->name) {
		free((char*)file->name);
		file->name = filename ? strdup(filename) : NULL;
	}
}

static void windows_invalidate(Vis *vis, size_t start, size_t end) {
	for (Win *win = vis->windows; win; win = win->next) {
		if (vis->win != win && vis->win->file == win->file) {
			Filerange view = view_viewport_get(win->view);
			if ((view.start <= start && start <= view.end) ||
			    (view.start <= end && end <= view.end))
				view_draw(win->view);
		}
	}
	view_draw(vis->win->view);
}

void window_selection_save(Win *win) {
	File *file = win->file;
	Filerange sel = view_cursors_selection_get(view_cursors(win->view));
	file->marks[VIS_MARK_SELECTION_START] = text_mark_set(file->text, sel.start);
	file->marks[VIS_MARK_SELECTION_END] = text_mark_set(file->text, sel.end);
}

static void window_free(Win *win) {
	if (!win)
		return;
	Vis *vis = win->vis;
	for (Win *other = vis->windows; other; other = other->next) {
		if (other->parent == win)
			other->parent = NULL;
	}
	if (vis->ui)
		vis->ui->window_free(win->ui);
	view_free(win->view);
	for (size_t i = 0; i < LENGTH(win->modes); i++)
		map_free(win->modes[i].bindings);
	ringbuf_free(win->jumplist);
	free(win);
}

Win *window_new_file(Vis *vis, File *file) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->vis = vis;
	win->file = file;
	win->jumplist = ringbuf_alloc(31);
	win->view = view_new(file->text, vis->lua);
	win->ui = vis->ui->window_new(vis->ui, win->view, file, UI_OPTION_STATUSBAR);
	if (!win->jumplist || !win->view || !win->ui) {
		window_free(win);
		return NULL;
	}
	file->refcount++;
	view_tabwidth_set(win->view, vis->tabwidth);

	if (text_size(file->text) > LARGE_FILE) {
		enum UiOption opt = view_options_get(win->view);
		opt |= UI_OPTION_LARGE_FILE;
		opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		view_options_set(win->view, opt);
	}

	if (vis->windows)
		vis->windows->prev = win;
	win->next = vis->windows;
	vis->windows = win;
	vis->win = win;
	vis->ui->window_focus(win->ui);
	for (size_t i = 0; i < LENGTH(win->modes); i++)
		win->modes[i].parent = &vis_modes[i];
	if (vis->event && vis->event->win_open)
		vis->event->win_open(vis, win);
	return win;
}

bool vis_window_reload(Win *win) {
	const char *name = win->file->name;
	if (!name)
		return false; /* can't reload unsaved file */
	/* temporarily unset file name, otherwise file_new returns the same File */
	win->file->name = NULL;
	File *file = file_new(win->vis, name);
	win->file->name = name;
	if (!file)
		return false;
	file_free(win->vis, win->file);
	file->refcount = 1;
	win->file = file;
	win->ui->reload(win->ui, file);
	return true;
}

bool vis_window_split(Win *original) {
	Win *win = window_new_file(original->vis, original->file);
	if (!win)
		return false;
	for (size_t i = 0; i < LENGTH(win->modes); i++) {
		if (original->modes[i].bindings)
			win->modes[i].bindings = map_new();
		if (win->modes[i].bindings)
			map_copy(win->modes[i].bindings, original->modes[i].bindings);
	}
	win->file = original->file;
	win->file->refcount++;
	view_syntax_set(win->view, view_syntax_get(original->view));
	view_options_set(win->view, view_options_get(original->view));
	view_cursor_to(win->view, view_cursor_get(original->view));
	vis_draw(win->vis);
	return true;
}

void vis_window_focus(Win *win) {
	if (!win)
		return;
	Vis *vis = win->vis;
	vis->win = win;
	vis->ui->window_focus(win->ui);
}

void vis_window_next(Vis *vis) {
	Win *sel = vis->win;
	if (!sel)
		return;
	vis_window_focus(sel->next ? sel->next : vis->windows);
}

void vis_window_prev(Vis *vis) {
	Win *sel = vis->win;
	if (!sel)
		return;
	sel = sel->prev;
	if (!sel)
		for (sel = vis->windows; sel->next; sel = sel->next);
	vis_window_focus(sel);
}

void vis_draw(Vis *vis) {
	vis->ui->draw(vis->ui);
}

void vis_redraw(Vis *vis) {
	vis->ui->redraw(vis->ui);
}

void vis_update(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next)
		view_update(win->view);
	view_update(vis->win->view);
	vis->ui->update(vis->ui);
}

void vis_suspend(Vis *vis) {
	vis->ui->suspend(vis->ui);
}

bool vis_window_new(Vis *vis, const char *filename) {
	File *file = file_new(vis, filename);
	if (!file)
		return false;
	Win *win = window_new_file(vis, file);
	if (!win) {
		file_free(vis, file);
		return false;
	}

	vis_window_name(win, filename);
	vis_draw(vis);

	return true;
}

bool vis_window_closable(Win *win) {
	if (!text_modified(win->file->text))
		return true;
	return win->file->refcount > 1;
}

void vis_window_swap(Win *a, Win *b) {
	if (a == b || !a || !b)
		return;
	Vis *vis = a->vis;
	Win *tmp = a->next;
	a->next = b->next;
	b->next = tmp;
	if (a->next)
		a->next->prev = a;
	if (b->next)
		b->next->prev = b;
	tmp = a->prev;
	a->prev = b->prev;
	b->prev = tmp;
	if (a->prev)
		a->prev->next = a;
	if (b->prev)
		b->prev->next = b;
	if (vis->windows == a)
		vis->windows = b;
	else if (vis->windows == b)
		vis->windows = a;
	vis->ui->window_swap(a->ui, b->ui);
	if (vis->win == a)
		vis_window_focus(b);
	else if (vis->win == b)
		vis_window_focus(a);
}

void vis_window_close(Win *win) {
	Vis *vis = win->vis;
	if (vis->event && vis->event->win_close)
		vis->event->win_close(vis, win);
	file_free(vis, win->file);
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (vis->windows == win)
		vis->windows = win->next;
	if (vis->win == win)
		vis->win = win->next ? win->next : win->prev;
	if (win == vis->message_window)
		vis->message_window = NULL;
	window_free(win);
	if (vis->win)
		vis->ui->window_focus(vis->win->ui);
	vis_draw(vis);
}

Vis *vis_new(Ui *ui, VisEvent *event) {
	if (!ui)
		return NULL;
	Vis *vis = calloc(1, sizeof(Vis));
	if (!vis)
		return NULL;
	vis->ui = ui;
	vis->ui->init(vis->ui, vis);
	vis->tabwidth = 8;
	vis->textwidth = 0;
	vis->expandtab = false;
	vis->registers[VIS_REG_BLACKHOLE].type = REGISTER_BLACKHOLE;
	vis->registers[VIS_REG_CLIPBOARD].type = REGISTER_CLIPBOARD;
	array_init(&vis->motions);
	array_init(&vis->textobjects);
	action_reset(&vis->action);
	if (!(vis->command_file = file_new_internal(vis, NULL)))
		goto err;
	if (!(vis->search_file = file_new_internal(vis, NULL)))
		goto err;
	if (!(vis->keymap = map_new()))
		goto err;
	vis->mode_prev = vis->mode = &vis_modes[VIS_MODE_NORMAL];
	vis->event = event;
	if (event && event->vis_start)
		event->vis_start(vis);
	vis->ui->start(vis->ui);
	return vis;
err:
	vis_free(vis);
	return NULL;
}

void vis_free(Vis *vis) {
	if (!vis)
		return;
	if (vis->event && vis->event->vis_quit)
		vis->event->vis_quit(vis);
	vis->event = NULL;
	while (vis->windows)
		vis_window_close(vis->windows);
	file_free(vis, vis->command_file);
	file_free(vis, vis->search_file);
	for (int i = 0; i < LENGTH(vis->registers); i++)
		register_release(&vis->registers[i]);
	vis->ui->free(vis->ui);
	map_free(vis->cmds);
	map_free(vis->options);
	map_free(vis->actions);
	map_free(vis->keymap);
	buffer_release(&vis->input_queue);
	for (int i = 0; i < VIS_MODE_INVALID; i++)
		map_free(vis_modes[i].bindings);
	array_release_full(&vis->motions);
	array_release_full(&vis->textobjects);
	free(vis);
}

void vis_insert(Vis *vis, size_t pos, const char *data, size_t len) {
	text_insert(vis->win->file->text, pos, data, len);
	windows_invalidate(vis, pos, pos + len);
}

void vis_insert_key(Vis *vis, const char *data, size_t len) {
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		vis_insert(vis, pos, data, len);
		view_cursors_scroll_to(c, pos + len);
	}
}

void vis_replace(Vis *vis, size_t pos, const char *data, size_t len) {
	Text *txt = vis->win->file->text;
	Iterator it = text_iterator_get(txt, pos);
	int chars = text_char_count(data, len);
	for (char c; chars-- > 0 && text_iterator_byte_get(&it, &c) && c != '\r' && c != '\n'; )
		text_iterator_char_next(&it, NULL);

	text_delete(txt, pos, it.pos - pos);
	vis_insert(vis, pos, data, len);
}

void vis_replace_key(Vis *vis, const char *data, size_t len) {
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		vis_replace(vis, pos, data, len);
		view_cursors_scroll_to(c, pos + len);
	}
}

void vis_delete(Vis *vis, size_t pos, size_t len) {
	text_delete(vis->win->file->text, pos, len);
	windows_invalidate(vis, pos, pos + len);
}

bool vis_action_register(Vis *vis, const KeyAction *action) {
	if (!vis->actions)
		vis->actions = map_new();
	if (!vis->actions)
		return false;
	return map_put(vis->actions, action->name, action);
}

bool vis_keymap_add(Vis *vis, const char *key, const char *mapping) {
	return map_put(vis->keymap, key, mapping);
}

static void window_jumplist_add(Win *win, size_t pos) {
	Mark mark = text_mark_set(win->file->text, pos);
	if (mark && win->jumplist)
		ringbuf_add(win->jumplist, mark);
}

static void window_jumplist_invalidate(Win *win) {
	if (win->jumplist)
		ringbuf_invalidate(win->jumplist);
}

void action_do(Vis *vis, Action *a) {
	Win *win = vis->win;
	Text *txt = win->file->text;
	View *view = win->view;

	if (a->op == &vis_operators[VIS_OP_FILTER] && !vis->mode->visual)
		vis_mode_switch(vis, VIS_MODE_VISUAL_LINE);

	int count = MAX(a->count, 1);
	bool repeatable = a->op && !vis->macro_operator;
	bool multiple_cursors = view_cursors_multiple(view);
	bool linewise = !(a->type & CHARWISE) && (
		a->type & LINEWISE || (a->movement && a->movement->type & LINEWISE) ||
		vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE]);

	for (Cursor *cursor = view_cursors(view), *next; cursor; cursor = next) {

		next = view_cursors_next(cursor);
		size_t pos = view_cursors_pos(cursor);
		Register *reg = multiple_cursors ? view_cursors_register(cursor) : a->reg;
		if (!reg)
			reg = &vis->registers[win->file->internal ? VIS_REG_PROMPT : VIS_REG_DEFAULT];

		OperatorContext c = {
			.count = count,
			.pos = pos,
			.newpos = EPOS,
			.range = text_range_empty(),
			.reg = reg,
			.linewise = linewise,
			.arg = &a->arg,
		};

		if (a->movement) {
			size_t start = pos;
			for (int i = 0; i < count; i++) {
				if (a->movement->txt)
					pos = a->movement->txt(txt, pos);
				else if (a->movement->cur)
					pos = a->movement->cur(cursor);
				else if (a->movement->file)
					pos = a->movement->file(vis, vis->win->file, pos);
				else if (a->movement->vis)
					pos = a->movement->vis(vis, txt, pos);
				else if (a->movement->view)
					pos = a->movement->view(vis, view);
				else if (a->movement->win)
					pos = a->movement->win(vis, win, pos);
				else if (a->movement->user)
					pos = a->movement->user(vis, win, a->movement->data, pos);
				if (pos == EPOS || a->movement->type & IDEMPOTENT)
					break;
			}

			if (pos == EPOS) {
				c.range.start = start;
				c.range.end = start;
				pos = start;
			} else {
				c.range = text_range_new(start, pos);
				c.newpos = pos;
			}

			if (!a->op) {
				if (a->movement->type & CHARWISE)
					view_cursors_scroll_to(cursor, pos);
				else
					view_cursors_to(cursor, pos);
				if (vis->mode->visual)
					c.range = view_cursors_selection_get(cursor);
				if (a->movement->type & JUMP)
					window_jumplist_add(win, pos);
				else
					window_jumplist_invalidate(win);
			} else if (a->movement->type & INCLUSIVE) {
				c.range.end = text_char_next(txt, c.range.end);
			}
		} else if (a->textobj) {
			if (vis->mode->visual)
				c.range = view_cursors_selection_get(cursor);
			else
				c.range.start = c.range.end = pos;
			for (int i = 0; i < count; i++) {
				Filerange r = text_range_empty();
				if (a->textobj->txt)
					r = a->textobj->txt(txt, pos);
				else if (a->textobj->vis)
					r = a->textobj->vis(vis, txt, pos);
				else if (a->textobj->user)
					r = a->textobj->user(vis, win, a->textobj->data, pos);
				if (!text_range_valid(&r))
					break;
				if (a->textobj->type & OUTER) {
					r.start--;
					r.end++;
				}

				if (a->textobj->type & SPLIT)
					c.range = r;
				else
					c.range = text_range_union(&c.range, &r);

				if (i < count - 1)
					pos = c.range.end + 1;
			}
		} else if (vis->mode->visual) {
			c.range = view_cursors_selection_get(cursor);
			if (!text_range_valid(&c.range))
				c.range.start = c.range.end = pos;
		}

		if (linewise && vis->mode != &vis_modes[VIS_MODE_VISUAL])
			c.range = text_range_linewise(txt, &c.range);
		if (vis->mode->visual) {
			view_cursors_selection_set(cursor, &c.range);
			if (vis->mode == &vis_modes[VIS_MODE_VISUAL] || a->textobj)
				view_cursors_selection_sync(cursor);
		}

		if (a->op) {
			size_t pos = a->op->func(vis, txt, &c);
			if (pos == EPOS) {
				view_cursors_dispose(cursor);
			} else if (pos <= text_size(txt)) {
				/* moving the cursor will affect the selection.
				 * because we want to be able to later restore
				 * the old selection we update it again before
				 * leaving visual mode.
				 */
				Filerange sel = view_cursors_selection_get(cursor);
				view_cursors_to(cursor, pos);
				if (vis->mode->visual) {
					if (sel.start == EPOS && sel.end == EPOS)
						sel = c.range;
					else if (sel.start == EPOS)
						sel = text_range_new(c.range.start, sel.end);
					else if (sel.end == EPOS)
						sel = text_range_new(c.range.start, sel.start);
					if (vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE])
						sel = text_range_linewise(txt, &sel);
					view_cursors_selection_set(cursor, &sel);
				}
			}
		}
	}

	if (a->op) {
		/* we do not support visual repeat, still do something resonable */
		if (vis->mode->visual && !a->movement && !a->textobj)
			a->movement = &vis_motions[VIS_MOVE_NOP];

		/* operator implementations must not change the mode,
		 * they might get called multiple times (once for every cursor)
		 */
		if (a->op == &vis_operators[VIS_OP_INSERT] || a->op == &vis_operators[VIS_OP_CHANGE]) {
			vis_mode_switch(vis, VIS_MODE_INSERT);
		} else if (a->op == &vis_operators[VIS_OP_REPLACE]) {
			vis_mode_switch(vis, VIS_MODE_REPLACE);
		} else if (a->op == &vis_operators[VIS_OP_FILTER]) {
			if (a->arg.s)
				vis_cmd(vis, a->arg.s);
			else
				vis_prompt_show(vis, ":|");
		} else if (vis->mode == &vis_modes[VIS_MODE_OPERATOR_PENDING]) {
			mode_set(vis, vis->mode_prev);
		} else if (vis->mode->visual) {
			vis_mode_switch(vis, VIS_MODE_NORMAL);
		}
		text_snapshot(txt);
		vis_draw(vis);
	}

	if (a != &vis->action_prev) {
		if (repeatable) {
			if (!a->macro)
				a->macro = vis->macro_operator;
			vis->action_prev = *a;
		}
		action_reset(a);
	}
}

void action_reset(Action *a) {
	memset(a, 0, sizeof(*a));
	a->count = VIS_COUNT_UNKNOWN;
}

void vis_cancel(Vis *vis) {
	action_reset(&vis->action);
}

void vis_die(Vis *vis, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vis->ui->die(vis->ui, msg, ap);
	va_end(ap);
}

const char *vis_keys_next(Vis *vis, const char *keys) {
	TermKeyKey key;
	TermKey *termkey = vis->ui->termkey_get(vis->ui);
	const char *next = NULL;
	if (!keys)
		return NULL;
	/* first try to parse a special key of the form <Key> */
	if (*keys == '<' && (next = termkey_strpkey(termkey, keys+1, &key, TERMKEY_FORMAT_VIM)) && *next == '>')
		return next+1;
	if (*keys == '<') {
		const char *start = keys + 1, *end = start;
		while (*end && *end != '>')
			end++;
		if (end > start && end - start - 1 < 64 && *end == '>') {
			char key[64];
			memcpy(key, start, end - start);
			key[end - start] = '\0';
			if (map_get(vis->actions, key))
				return end + 1;
		}
	}
	while (!ISUTF8(*keys))
		keys++;
	return termkey_strpkey(termkey, keys, &key, TERMKEY_FORMAT_VIM);
}

static const char *vis_keys_raw(Vis *vis, Buffer *buf, const char *input) {
	char *keys = buf->data, *start = keys, *cur = keys, *end;
	bool prefix = false;
	KeyBinding *binding = NULL;

	while (cur && *cur) {

		if (!(end = (char*)vis_keys_next(vis, cur)))
			goto err; // XXX: can't parse key this should never happen

		char tmp = *end;
		*end = '\0';
		prefix = false;
		binding = NULL;

		for (Mode *mode = vis->mode; mode && !binding && !prefix; mode = mode->parent) {
			for (int global = 0; global < 2 && !binding && !prefix; global++) {
				Mode *mode_local = global ? mode : &vis->win->modes[mode->id];
				if (!mode_local->bindings)
					continue;
				binding = map_get(mode_local->bindings, start);
				/* "<" is never treated as a prefix because it is used to denote
				 * special key symbols */
				if (strcmp(cur, "<"))
					prefix = !binding && map_contains(mode_local->bindings, start);
			}
		}

		*end = tmp;
		vis->keys = buf;

		if (binding) { /* exact match */
			if (binding->action) {
				end = (char*)binding->action->func(vis, end, &binding->action->arg);
				if (!end)
					break;
				start = cur = end;
			} else if (binding->alias) {
				buffer_put0(buf, end);
				buffer_prepend0(buf, binding->alias);
				start = cur = buf->data;
			}
		} else if (prefix) { /* incomplete key binding? */
			cur = end;
		} else { /* no keybinding */
			KeyAction *action = NULL;
			if (start[0] == '<' && end[-1] == '>') {
				/* test for special editor key command */
				char tmp = end[-1];
				end[-1] = '\0';
				action = map_get(vis->actions, start+1);
				end[-1] = tmp;
				if (action) {
					end = (char*)action->func(vis, end, &action->arg);
					if (!end)
						break;
				}
			}
			if (!action && vis->mode->input)
				vis->mode->input(vis, start, end - start);
			start = cur = end;
		}
	}

	vis->keys = NULL;
	buffer_put0(buf, start);
	return input + (start - keys);
err:
	vis->keys = NULL;
	buffer_truncate(buf);
	return input + strlen(input);
}

bool vis_keys_inject(Vis *vis, const char *pos, const char *input) {
	Buffer *buf = vis->keys;
	if (!buf)
		return false;
	if (pos < buf->data || pos > buf->data + buf->len)
		return false;
	size_t off = pos - buf->data;
	buffer_insert0(buf, off, input);
	if (vis->macro_operator)
		macro_append(vis->macro_operator, input);
	return true;
}

const char *vis_keys_push(Vis *vis, const char *input) {
	if (!input)
		return NULL;

	if (vis->recording)
		macro_append(vis->recording, input);
	if (vis->macro_operator)
		macro_append(vis->macro_operator, input);

	if (!buffer_append0(&vis->input_queue, input)) {
		buffer_truncate(&vis->input_queue);
		return NULL;
	}

	return vis_keys_raw(vis, &vis->input_queue, input);
}

static const char *getkey(Vis *vis) {
	const char *key = vis->ui->getkey(vis->ui);
	if (!key)
		return NULL;
	vis_info_hide(vis);
	if (!vis->mode->input) {
		const char *mapped = map_get(vis->keymap, key);
		if (mapped)
			return mapped;
	}
	return key;
}

bool vis_signal_handler(Vis *vis, int signum, const siginfo_t *siginfo, const void *context) {
	switch (signum) {
	case SIGBUS:
		for (File *file = vis->files; file; file = file->next) {
			if (text_sigbus(file->text, siginfo->si_addr))
				file->truncated = true;
		}
		vis->sigbus = true;
		if (vis->running)
			siglongjmp(vis->sigbus_jmpbuf, 1);
		return true;
	case SIGINT:
		vis->cancel_filter = true;
		return true;
	}
	return false;
}

static void vis_args(Vis *vis, int argc, char *argv[]) {
	char *cmd = NULL;
	bool end_of_options = false;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && !end_of_options) {
			switch (argv[i][1]) {
			case '-':
				end_of_options = true;
				break;
			case 'v':
				vis_die(vis, "vis %s\n", VERSION);
				break;
			case '\0':
				break;
			default:
				vis_die(vis, "Unknown command option: %s\n", argv[i]);
				break;
			}
		} else if (argv[i][0] == '+') {
			cmd = argv[i] + (argv[i][1] == '/' || argv[i][1] == '?');
		} else if (!vis_window_new(vis, argv[i])) {
			vis_die(vis, "Can not load `%s': %s\n", argv[i], strerror(errno));
		} else if (cmd) {
			vis_prompt_cmd(vis, cmd);
			cmd = NULL;
		}
	}

	if (!vis->windows) {
		if (!strcmp(argv[argc-1], "-")) {
			if (!vis_window_new(vis, NULL))
				vis_die(vis, "Can not create empty buffer\n");
			ssize_t len = 0;
			char buf[PIPE_BUF];
			File *file = vis->win->file;
			Text *txt = file->text;
			file->is_stdin = true;
			while ((len = read(STDIN_FILENO, buf, sizeof buf)) > 0)
				text_insert(txt, text_size(txt), buf, len);
			if (len == -1)
				vis_die(vis, "Can not read from stdin\n");
			text_snapshot(txt);
			int fd = open("/dev/tty", O_RDONLY);
			if (fd == -1)
				vis_die(vis, "Can not reopen stdin\n");
			dup2(fd, STDIN_FILENO);
			close(fd);
		} else if (!vis_window_new(vis, NULL)) {
			vis_die(vis, "Can not create empty buffer\n");
		}
		if (cmd)
			vis_prompt_cmd(vis, cmd);
	}
}

int vis_run(Vis *vis, int argc, char *argv[]) {
	vis_args(vis, argc, argv);

	struct timespec idle = { .tv_nsec = 0 }, *timeout = NULL;

	sigset_t emptyset;
	sigemptyset(&emptyset);
	vis_draw(vis);
	vis->running = true;
	vis->exit_status = EXIT_SUCCESS;

	sigsetjmp(vis->sigbus_jmpbuf, 1);

	while (vis->running) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);

		if (vis->sigbus) {
			char *name = NULL;
			for (Win *next, *win = vis->windows; win; win = next) {
				next = win->next;
				if (win->file->truncated) {
					free(name);
					name = strdup(win->file->name);
					vis_window_close(win);
				}
			}
			if (!vis->windows)
				vis_die(vis, "WARNING: file `%s' truncated!\n", name ? name : "-");
			else
				vis_info_show(vis, "WARNING: file `%s' truncated!\n", name ? name : "-");
			vis->sigbus = false;
			free(name);
		}

		vis_update(vis);
		idle.tv_sec = vis->mode->idle_timeout;
		int r = pselect(1, &fds, NULL, NULL, timeout, &emptyset);
		if (r == -1 && errno == EINTR)
			continue;

		if (r < 0) {
			/* TODO save all pending changes to a ~suffixed file */
			vis_die(vis, "Error in mainloop: %s\n", strerror(errno));
		}

		if (!FD_ISSET(STDIN_FILENO, &fds)) {
			if (vis->mode->idle)
				vis->mode->idle(vis);
			timeout = NULL;
			continue;
		}

		TermKey *termkey = vis->ui->termkey_get(vis->ui);
		termkey_advisereadable(termkey);
		const char *key;

		while ((key = getkey(vis)))
			vis_keys_push(vis, key);

		if (vis->mode->idle)
			timeout = &idle;
	}
	return vis->exit_status;
}

static Macro *macro_get(Vis *vis, enum VisRegister id) {
	if (id == VIS_MACRO_LAST_RECORDED)
		return vis->last_recording;
	if (VIS_REG_A <= id && id <= VIS_REG_Z)
		id -= VIS_REG_A;
	if (id < LENGTH(vis->registers))
		return &vis->registers[id].buf;
	return NULL;
}

void macro_operator_record(Vis *vis) {
	vis->macro_operator = macro_get(vis, VIS_MACRO_OPERATOR);
	macro_reset(vis->macro_operator);
}

void macro_operator_stop(Vis *vis) {
	vis->macro_operator = NULL;
}

bool vis_macro_record(Vis *vis, enum VisRegister id) {
	Macro *macro = macro_get(vis, id);
	if (vis->recording || !macro)
		return false;
	if (!(VIS_REG_A <= id && id <= VIS_REG_Z))
		macro_reset(macro);
	vis->recording = macro;
	return true;
}

bool vis_macro_record_stop(Vis *vis) {
	if (!vis->recording)
		return false;
	/* XXX: hack to remove last recorded key, otherwise upon replay
	 * we would start another recording */
	if (vis->recording->len > 1) {
		vis->recording->len--;
		vis->recording->data[vis->recording->len-1] = '\0';
	}
	vis->last_recording = vis->recording;
	vis->recording = NULL;
	return true;
}

bool vis_macro_recording(Vis *vis) {
	return vis->recording;
}

static void macro_replay(Vis *vis, const Macro *macro) {
	Buffer buf;
	buffer_init(&buf);
	buffer_put(&buf, macro->data, macro->len);
	buffer_append(&buf, "\0", 1);
	vis_keys_raw(vis, &buf, macro->data);
	buffer_release(&buf);
}

bool vis_macro_replay(Vis *vis, enum VisRegister id) {
	if (id == VIS_REG_SEARCH)
		return vis_motion(vis, VIS_MOVE_SEARCH_NEXT);
	if (id == VIS_REG_COMMAND) {
		const char *cmd = register_get(vis, &vis->registers[id], NULL);
		return vis_cmd(vis, cmd);
	}

	Macro *macro = macro_get(vis, id);
	if (!macro || macro == vis->recording)
		return false;
	macro_replay(vis, macro);
	return true;
}

void vis_repeat(Vis *vis) {
	int count = vis->action.count;
	Macro *macro_operator = macro_get(vis, VIS_MACRO_OPERATOR);
	Macro *macro_repeat = macro_get(vis, VIS_MACRO_REPEAT);
	const Macro *macro = vis->action_prev.macro;
	if (macro == macro_operator) {
		buffer_put(macro_repeat, macro_operator->data, macro_operator->len);
		macro = macro_repeat;
		vis->action_prev.macro = macro;
	}
	if (count != VIS_COUNT_UNKNOWN)
		vis->action_prev.count = count;
	count = vis->action_prev.count;
	/* for some operators count should be applied only to the macro not the motion */
	if (vis->action_prev.op == &vis_operators[VIS_OP_INSERT] || vis->action_prev.op == &vis_operators[VIS_OP_REPLACE])
		vis->action_prev.count = 1;
	action_do(vis, &vis->action_prev);
	vis->action_prev.count = count;
	if (macro) {
		Mode *mode = vis->mode;
		Action action_prev = vis->action_prev;
		count = action_prev.count;
		if (count < 1 || action_prev.op == &vis_operators[VIS_OP_CHANGE] || action_prev.op == &vis_operators[VIS_OP_FILTER])
			count = 1;
		for (int i = 0; i < count; i++) {
			mode_set(vis, mode);
			macro_replay(vis, macro);
		}
		vis->action_prev = action_prev;
	}
	vis_cancel(vis);
}

void vis_mark_set(Vis *vis, enum VisMark mark, size_t pos) {
	File *file = vis->win->file;
	if (mark < LENGTH(file->marks))
		file->marks[mark] = text_mark_set(file->text, pos);
}

int vis_count_get(Vis *vis) {
	return vis->action.count;
}

int vis_count_get_default(Vis *vis, int def) {
	if (vis->action.count == VIS_COUNT_UNKNOWN)
		return def;
	return vis->action.count;
}

void vis_count_set(Vis *vis, int count) {
	vis->action.count = (count >= 0 ? count : VIS_COUNT_UNKNOWN);
}

void vis_register_set(Vis *vis, enum VisRegister reg) {
	if (reg >= VIS_REG_A && reg <= VIS_REG_Z) {
		vis->action.reg = &vis->registers[reg - VIS_REG_A];
		vis->action.reg->append = true;
	} else if (reg < LENGTH(vis->registers)) {
		vis->action.reg = &vis->registers[reg];
		vis->action.reg->append = false;
	}
}

const char *vis_register_get(Vis *vis, enum VisRegister reg, size_t *len) {
	if (VIS_REG_A <= reg && reg <= VIS_REG_Z)
		reg -= VIS_REG_A;
	if (reg < LENGTH(vis->registers))
		return register_get(vis, &vis->registers[reg], len);
	*len = 0;
	return NULL;
}

void vis_exit(Vis *vis, int status) {
	vis->running = false;
	vis->exit_status = status;
}

const char *vis_mode_status(Vis *vis) {
	return vis->mode->status;
}

void vis_insert_tab(Vis *vis) {
	if (!vis->expandtab) {
		vis_insert_key(vis, "\t", 1);
		return;
	}
	char spaces[9];
	int tabwidth = MIN(vis->tabwidth, LENGTH(spaces) - 1);
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		size_t pos = view_cursors_pos(c);
		int width = text_line_width_get(vis->win->file->text, pos);
		int count = tabwidth - (width % tabwidth);
		for (int i = 0; i < count; i++)
			spaces[i] = ' ';
		spaces[count] = '\0';
		vis_insert(vis, pos, spaces, count);
		view_cursors_scroll_to(c, pos + count);
	}
}

static void copy_indent_from_previous_line(Win *win) {
	View *view = win->view;
	Text *text = win->file->text;
	size_t pos = view_cursor_get(view);
	size_t prev_line = text_line_prev(text, pos);
	if (pos == prev_line)
		return;
	size_t begin = text_line_begin(text, prev_line);
	size_t start = text_line_start(text, begin);
	size_t len = start-begin;
	char *buf = malloc(len);
	if (!buf)
		return;
	len = text_bytes_get(text, begin, len, buf);
	vis_insert_key(win->vis, buf, len);
	free(buf);
}

void vis_insert_nl(Vis *vis) {
	const char *nl = text_newline_char(vis->win->file->text);
	vis_insert_key(vis, nl, strlen(nl));

	if (vis->autoindent)
		copy_indent_from_previous_line(vis->win);
}

Regex *vis_regex(Vis *vis, const char *pattern) {
	if (!pattern && !(pattern = register_get(vis, &vis->registers[VIS_REG_SEARCH], NULL)))
		return NULL;
	Regex *regex = text_regex_new();
	if (!regex)
		return NULL;
	if (text_regex_compile(regex, pattern, REG_EXTENDED|REG_NEWLINE) != 0) {
		text_regex_free(regex);
		return NULL;
	}
	register_put0(vis, &vis->registers[VIS_REG_SEARCH], pattern);
	return regex;
}

int vis_pipe(Vis *vis, Filerange *range, const char *argv[],
	void *stdout_context, ssize_t (*read_stdout)(void *stdout_context, char *data, size_t len),
	void *stderr_context, ssize_t (*read_stderr)(void *stderr_context, char *data, size_t len)) {

	/* if an invalid range was given, stdin (i.e. key board input) is passed
	 * through the external command. */
	Text *text = vis->win->file->text;
	View *view = vis->win->view;
	int pin[2], pout[2], perr[2], status = -1;
	bool interactive = !text_range_valid(range);
	size_t pos = view_cursor_get(view);
	Filerange rout = *range;
	if (interactive)
		rout = (Filerange){ .start = pos, .end = pos };

	if (pipe(pin) == -1)
		return -1;
	if (pipe(pout) == -1) {
		close(pin[0]);
		close(pin[1]);
		return -1;
	}

	if (pipe(perr) == -1) {
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		return -1;
	}

	vis->ui->terminal_save(vis->ui);
	pid_t pid = fork();

	if (pid == -1) {
		close(pin[0]);
		close(pin[1]);
		close(pout[0]);
		close(pout[1]);
		close(perr[0]);
		close(perr[1]);
		vis_info_show(vis, "fork failure: %s", strerror(errno));
		return -1;
	} else if (pid == 0) { /* child i.e filter */
		if (!interactive)
			dup2(pin[0], STDIN_FILENO);
		close(pin[0]);
		close(pin[1]);
		dup2(pout[1], STDOUT_FILENO);
		close(pout[1]);
		close(pout[0]);
		if (!interactive)
			dup2(perr[1], STDERR_FILENO);
		close(perr[0]);
		close(perr[1]);
		if (!argv[1])
			execl("/bin/sh", "sh", "-c", argv[0], NULL);
		else
			execvp(argv[0], (char* const*)argv);
		vis_info_show(vis, "exec failure: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	vis->cancel_filter = false;

	close(pin[0]);
	close(pout[1]);
	close(perr[1]);

	fcntl(pout[0], F_SETFL, O_NONBLOCK);
	fcntl(perr[0], F_SETFL, O_NONBLOCK);


	fd_set rfds, wfds;

	do {
		if (vis->cancel_filter) {
			kill(-pid, SIGTERM);
			break;
		}

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if (pin[1] != -1)
			FD_SET(pin[1], &wfds);
		if (pout[0] != -1)
			FD_SET(pout[0], &rfds);
		if (perr[0] != -1)
			FD_SET(perr[0], &rfds);

		if (select(FD_SETSIZE, &rfds, &wfds, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			vis_info_show(vis, "Select failure");
			break;
		}

		if (pin[1] != -1 && FD_ISSET(pin[1], &wfds)) {
			Filerange junk = rout;
			if (junk.end > junk.start + PIPE_BUF)
				junk.end = junk.start + PIPE_BUF;
			ssize_t len = text_write_range(text, &junk, pin[1]);
			if (len > 0) {
				rout.start += len;
				if (text_range_size(&rout) == 0) {
					close(pout[1]);
					pout[1] = -1;
				}
			} else {
				close(pin[1]);
				pin[1] = -1;
				if (len == -1)
					vis_info_show(vis, "Error writing to external command");
			}
		}

		if (pout[0] != -1 && FD_ISSET(pout[0], &rfds)) {
			char buf[BUFSIZ];
			ssize_t len = read(pout[0], buf, sizeof buf);
			if (len > 0) {
				if (read_stdout)
					(*read_stdout)(stdout_context, buf, len);
			} else if (len == 0) {
				close(pout[0]);
				pout[0] = -1;
			} else if (errno != EINTR && errno != EWOULDBLOCK) {
				vis_info_show(vis, "Error reading from filter stdout");
				close(pout[0]);
				pout[0] = -1;
			}
		}

		if (perr[0] != -1 && FD_ISSET(perr[0], &rfds)) {
			char buf[BUFSIZ];
			ssize_t len = read(perr[0], buf, sizeof buf);
			if (len > 0) {
				if (read_stderr)
					(*read_stderr)(stderr_context, buf, len);
			} else if (len == 0) {
				close(perr[0]);
				perr[0] = -1;
			} else if (errno != EINTR && errno != EWOULDBLOCK) {
				vis_info_show(vis, "Error reading from filter stderr");
				close(perr[0]);
				perr[0] = -1;
			}
		}

	} while (pin[1] != -1 || pout[0] != -1 || perr[0] != -1);

	if (pin[1] != -1)
		close(pin[1]);
	if (pout[0] != -1)
		close(pout[0]);
	if (perr[0] != -1)
		close(perr[0]);

	for (pid_t died; (died = waitpid(pid, &status, 0)) != -1 && pid != died;);

	vis->ui->terminal_restore(vis->ui);

	return status;
}

bool vis_cmd(Vis *vis, const char *cmdline) {
	if (!cmdline)
		return true;
	while (*cmdline == ':')
		cmdline++;
	size_t len = strlen(cmdline);
	char *line = malloc(len+2);
	if (!line)
		return false;
	strncpy(line, cmdline, len+1);

	for (char *end = line + len - 1; end >= line && isspace((unsigned char)*end); end--)
		*end = '\0';

	enum SamError err = sam_cmd(vis, line);
	if (err != SAM_ERR_OK)
		vis_info_show(vis, "%s", sam_error(err));
	free(line);
	return err == SAM_ERR_OK;
}

Text *vis_text(Vis *vis) {
	return vis->win->file->text;
}

View *vis_view(Vis *vis) {
	return vis->win->view;
}

Win *vis_window(Vis *vis) {
	return vis->win;
}

Text *vis_file_text(File *file) {
	return file->text;
}

const char *vis_file_name(File *file) {
	return file->name;
}

