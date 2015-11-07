/*
 * Copyright (c) 2014 Marc André Tanner <mat at brain-dump.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
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
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <termkey.h>
#include "vis.h"
#include "text-util.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"
#include "vis-core.h"

static Macro *macro_get(Vis *vis, enum VisMacro m);
static void macro_replay(Vis *vis, const Macro *macro);

const char *expandtab(Vis *vis) {
	static char spaces[9];
	int tabwidth = vis->tabwidth;
	tabwidth = MIN(tabwidth, LENGTH(spaces) - 1);
	for (int i = 0; i < tabwidth; i++)
		spaces[i] = ' ';
	spaces[tabwidth] = '\0';
	return vis->expandtab ? spaces : "\t";
}


/** window / file handling */

static void file_free(Vis *vis, File *file) {
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
	file->refcount++;
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
	
	File *file = file_new_text(vis, text);
	if (!file) {
		text_free(text);
		return NULL;
	}

	if (filename)
		file->name = strdup(filename);
	return file;
}

void vis_window_name(Win *win, const char *filename) {
	File *file = win->file;
	if (filename != file->name) {
		free((char*)file->name);
		file->name = filename ? strdup(filename) : NULL;
	}
	
	if (filename) {
		Vis *vis = win->editor;
		for (Syntax *syn = vis->syntaxes; syn && syn->name; syn++) {
			if (!regexec(&syn->file_regex, filename, 0, NULL, 0)) {
				view_syntax_set(win->view, syn);
				for (const char **opt = syn->settings; opt && *opt; opt++)
					vis_cmd(vis, *opt);
				break;
			}
		}
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

static void window_selection_changed(void *win, Filerange *sel) {
	File *file = ((Win*)win)->file;
	if (text_range_valid(sel)) {
		file->marks[MARK_SELECTION_START] = text_mark_set(file->text, sel->start);
		file->marks[MARK_SELECTION_END] = text_mark_set(file->text, sel->end);
	}
}

static void window_free(Win *win) {
	if (!win)
		return;
	Vis *vis = win->editor;
	if (vis && vis->ui)
		vis->ui->window_free(win->ui);
	view_free(win->view);
	ringbuf_free(win->jumplist);
	free(win);
}

static Win *window_new_file(Vis *vis, File *file) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->editor = vis;
	win->file = file;
	win->events = (ViewEvent) {
		.data = win,
		.selection = window_selection_changed,
	};
	win->jumplist = ringbuf_alloc(31);
	win->view = view_new(file->text, &win->events);
	win->ui = vis->ui->window_new(vis->ui, win->view, file);
	if (!win->jumplist || !win->view || !win->ui) {
		window_free(win);
		return NULL;
	}
	view_tabwidth_set(win->view, vis->tabwidth);
	if (vis->windows)
		vis->windows->prev = win;
	win->next = vis->windows;
	vis->windows = win;
	vis->win = win;
	vis->ui->window_focus(win->ui);
	return win;
}

bool vis_window_reload(Win *win) {
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

bool vis_window_split(Win *original) {
	Win *win = window_new_file(original->editor, original->file);
	if (!win)
		return false;
	win->file = original->file;
	win->file->refcount++;
	view_syntax_set(win->view, view_syntax_get(original->view));
	view_options_set(win->view, view_options_get(original->view));
	view_cursor_to(win->view, view_cursor_get(original->view));
	vis_draw(win->editor);
	return true;
}

void vis_resize(Vis *vis) {
	vis->ui->resize(vis->ui);
}

void vis_window_next(Vis *vis) {
	Win *sel = vis->win;
	if (!sel)
		return;
	vis->win = vis->win->next;
	if (!vis->win)
		vis->win = vis->windows;
	vis->ui->window_focus(vis->win->ui);
}

void vis_window_prev(Vis *vis) {
	Win *sel = vis->win;
	if (!sel)
		return;
	vis->win = vis->win->prev;
	if (!vis->win)
		for (vis->win = vis->windows; vis->win->next; vis->win = vis->win->next);
	vis->ui->window_focus(vis->win->ui);
}

bool vis_syntax_load(Vis *vis, Syntax *syntaxes) {
	bool success = true;
	vis->syntaxes = syntaxes;

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

void vis_draw(Vis *vis) {
	vis->ui->draw(vis->ui);
}

void vis_update(Vis *vis) {
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

void vis_window_close(Win *win) {
	Vis *vis = win->editor;
	file_free(vis, win->file);
	if (win->prev)
		win->prev->next = win->next;
	if (win->next)
		win->next->prev = win->prev;
	if (vis->windows == win)
		vis->windows = win->next;
	if (vis->win == win)
		vis->win = win->next ? win->next : win->prev;
	if (vis->prompt_window == win)
		vis->prompt_window = NULL;
	window_free(win);
	if (vis->win)
		vis->ui->window_focus(vis->win->ui);
	vis_draw(vis);
}

Vis *vis_new(Ui *ui) {
	if (!ui)
		return NULL;
	Vis *vis = calloc(1, sizeof(Vis));
	if (!vis)
		return NULL;
	vis->ui = ui;
	vis->ui->init(vis->ui, vis);
	vis->tabwidth = 8;
	vis->expandtab = false;
	for (int i = 0; i < VIS_MODE_LAST; i++) {
		Mode *mode = &vis_modes[i];
		if (!(mode->bindings = map_new()))
			goto err;
	}
	if (!(vis->prompt = calloc(1, sizeof(Win))))
		goto err;
	if (!(vis->prompt->file = calloc(1, sizeof(File))))
		goto err;
	if (!(vis->prompt->file->text = text_load(NULL)))
		goto err;
	if (!(vis->prompt->view = view_new(vis->prompt->file->text, NULL)))
		goto err;
	if (!(vis->prompt->ui = vis->ui->prompt_new(vis->ui, vis->prompt->view, vis->prompt->file)))
		goto err;
	if (!(vis->search_pattern = text_regex_new()))
		goto err;
	vis->mode_prev = vis->mode = &vis_modes[VIS_MODE_NORMAL];
	return vis;
err:
	vis_free(vis);
	return NULL;
}

void vis_free(Vis *vis) {
	if (!vis)
		return;
	while (vis->windows)
		vis_window_close(vis->windows);
	file_free(vis, vis->prompt->file);
	window_free(vis->prompt);
	text_regex_free(vis->search_pattern);
	for (int i = 0; i < LENGTH(vis->registers); i++)
		register_release(&vis->registers[i]);
	for (int i = 0; i < LENGTH(vis->macros); i++)
		macro_release(&vis->macros[i]);
	vis_syntax_unload(vis);
	vis->ui->free(vis->ui);
	map_free(vis->cmds);
	map_free(vis->options);
	map_free(vis->actions);
	buffer_release(&vis->input_queue);
	for (int i = 0; i < VIS_MODE_LAST; i++) {
		Mode *mode = &vis_modes[i];
		map_free(mode->bindings);
	}
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
	size_t chars = 0;
	for (size_t i = 0; i < len; i++) {
		if (ISUTF8(data[i]))
			chars++;
	}

	Text *txt = vis->win->file->text;
	Iterator it = text_iterator_get(txt, pos);
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

void vis_prompt_show(Vis *vis, const char *title, const char *text) {
	if (vis->prompt_window)
		return;
	vis->prompt_window = vis->win;
	vis->win = vis->prompt;
	vis->prompt_type = title[0];
	vis->ui->prompt(vis->ui, title, text);
}

void vis_prompt_hide(Vis *vis) {
	if (!vis->prompt_window)
		return;
	vis->ui->prompt_hide(vis->ui);
	vis->win = vis->prompt_window;
	vis->prompt_window = NULL;
}

char *vis_prompt_get(Vis *vis) {
	return vis->ui->prompt_input(vis->ui);
}

void vis_info_show(Vis *vis, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vis->ui->info(vis->ui, msg, ap);
	va_end(ap);
}

void vis_info_hide(Vis *vis) {
	vis->ui->info_hide(vis->ui);
}

static TextObject textobjs[] = {
	[TEXT_OBJ_INNER_WORD]           = { text_object_word                  },
	[TEXT_OBJ_OUTER_WORD]           = { text_object_word_outer            },
	[TEXT_OBJ_INNER_LONGWORD]       = { text_object_longword              },
	[TEXT_OBJ_OUTER_LONGWORD]       = { text_object_longword_outer        },
	[TEXT_OBJ_SENTENCE]             = { text_object_sentence              },
	[TEXT_OBJ_PARAGRAPH]            = { text_object_paragraph             },
	[TEXT_OBJ_OUTER_SQUARE_BRACKET] = { text_object_square_bracket, OUTER },
	[TEXT_OBJ_INNER_SQUARE_BRACKET] = { text_object_square_bracket, INNER },
	[TEXT_OBJ_OUTER_CURLY_BRACKET]  = { text_object_curly_bracket,  OUTER },
	[TEXT_OBJ_INNER_CURLY_BRACKET]  = { text_object_curly_bracket,  INNER },
	[TEXT_OBJ_OUTER_ANGLE_BRACKET]  = { text_object_angle_bracket,  OUTER },
	[TEXT_OBJ_INNER_ANGLE_BRACKET]  = { text_object_angle_bracket,  INNER },
	[TEXT_OBJ_OUTER_PARANTHESE]     = { text_object_paranthese,     OUTER },
	[TEXT_OBJ_INNER_PARANTHESE]     = { text_object_paranthese,     INNER },
	[TEXT_OBJ_OUTER_QUOTE]          = { text_object_quote,          OUTER },
	[TEXT_OBJ_INNER_QUOTE]          = { text_object_quote,          INNER },
	[TEXT_OBJ_OUTER_SINGLE_QUOTE]   = { text_object_single_quote,   OUTER },
	[TEXT_OBJ_INNER_SINGLE_QUOTE]   = { text_object_single_quote,   INNER },
	[TEXT_OBJ_OUTER_BACKTICK]       = { text_object_backtick,       OUTER },
	[TEXT_OBJ_INNER_BACKTICK]       = { text_object_backtick,       INNER },
	[TEXT_OBJ_OUTER_ENTIRE]         = { text_object_entire,               },
	[TEXT_OBJ_INNER_ENTIRE]         = { text_object_entire_inner,         },
	[TEXT_OBJ_OUTER_FUNCTION]       = { text_object_function,             },
	[TEXT_OBJ_INNER_FUNCTION]       = { text_object_function_inner,       },
	[TEXT_OBJ_OUTER_LINE]           = { text_object_line,                 },
	[TEXT_OBJ_INNER_LINE]           = { text_object_line_inner,           },
};

bool vis_action_register(Vis *vis, KeyAction *action) {
	if (!vis->actions)
		vis->actions = map_new();
	if (!vis->actions)
		return false;
	return map_put(vis->actions, action->name, action);
}

static const char *getkey(Vis*);
static void action_do(Vis*, Action *a);


/** action processing: execut the operator / movement / text object */

static void window_jumplist_add(Win *win, size_t pos) {
	Mark mark = text_mark_set(win->file->text, pos);
	if (mark && win->jumplist)
		ringbuf_add(win->jumplist, mark);
}

static void window_jumplist_invalidate(Win *win) {
	if (win->jumplist)
		ringbuf_invalidate(win->jumplist);
}

static void action_do(Vis *vis, Action *a) {
	Win *win = vis->win;
	Text *txt = win->file->text;
	View *view = win->view;
	if (a->count < 1)
		a->count = 1;
	bool repeatable = a->op && !vis->macro_operator;
	bool multiple_cursors = view_cursors_count(view) > 1;
	bool linewise = !(a->type & CHARWISE) && (
		a->type & LINEWISE || (a->movement && a->movement->type & LINEWISE) ||
		vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE]);

	for (Cursor *cursor = view_cursors(view), *next; cursor; cursor = next) {

		next = view_cursors_next(cursor);
		size_t pos = view_cursors_pos(cursor);
		Register *reg = a->reg ? a->reg : &vis->registers[REG_DEFAULT];
		if (multiple_cursors)
			reg = view_cursors_register(cursor);

		OperatorContext c = {
			.count = a->count,
			.pos = pos,
			.newpos = EPOS,
			.range = text_range_empty(),
			.reg = reg,
			.linewise = linewise,
			.arg = &a->arg,
		};

		if (a->movement) {
			size_t start = pos;
			for (int i = 0; i < a->count; i++) {
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
			for (int i = 0; i < a->count; i++) {
				Filerange r = a->textobj->range(txt, pos);
				if (!text_range_valid(&r))
					break;
				if (a->textobj->type == OUTER) {
					r.start--;
					r.end++;
				}

				c.range = text_range_union(&c.range, &r);

				if (i < a->count - 1)
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
			if (pos != EPOS) {
				view_cursors_to(cursor, pos);
			} else {
				view_cursors_dispose(cursor);
			}
		}
	}

	if (a->op) {
		/* we do not support visual repeat, still do something resonable */
		if (vis->mode->visual && !a->movement && !a->textobj)
			a->movement = &moves[MOVE_NOP];

		/* operator implementations must not change the mode,
		 * they might get called multiple times (once for every cursor)
		 */
		if (a->op == &ops[OP_INSERT] || a->op == &ops[OP_CHANGE])
			vis_mode_switch(vis, VIS_MODE_INSERT);
		else if (a->op == &ops[OP_REPLACE])
			vis_mode_switch(vis, VIS_MODE_REPLACE);
		else if (vis->mode == &vis_modes[VIS_MODE_OPERATOR])
			mode_set(vis, vis->mode_prev);
		else if (vis->mode->visual)
			vis_mode_switch(vis, VIS_MODE_NORMAL);
		text_snapshot(txt);
		vis_draw(vis);
	}

	if (a != &vis->action_prev) {
		if (repeatable) {
			if (!a->macro)
				a->macro = vis->macro_operator;
			vis->action_prev = *a;
		}
		action_reset(vis, a);
	}
}

void action_reset(Vis *vis, Action *a) {
	memset(a, 0, sizeof(*a));
}

static bool prompt_cmd(Vis *vis, char type, const char *cmd) {
	if (!cmd || !cmd[0])
		return true;
	switch (type) {
	case '/':
		return vis_motion(vis, MOVE_SEARCH_FORWARD, cmd);
	case '?':
		return vis_motion(vis, MOVE_SEARCH_BACKWARD, cmd);
	case '+':
	case ':':
		return vis_cmd(vis, cmd);
	}
	return false;
}

void vis_die(Vis *vis, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vis->ui->die(vis->ui, msg, ap);
	va_end(ap);
}

const char *vis_key_next(Vis *vis, const char *keys) {
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

		if (!(end = (char*)vis_key_next(vis, cur)))
			goto err; // XXX: can't parse key this should never happen

		char tmp = *end;
		*end = '\0';
		prefix = false;
		binding = NULL;

		for (Mode *mode = vis->mode; mode && !binding && !prefix; mode = mode->parent) {
			binding = map_get(mode->bindings, start);
			/* "<" is never treated as a prefix because it is used to denote
			 * special key symbols */
			if (strcmp(cur, "<"))
				prefix = !binding && map_contains(mode->bindings, start);
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

const char *vis_keys(Vis *vis, const char *input) {
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
				vis_die(vis, "vis %s, compiled " __DATE__ " " __TIME__ "\n", VERSION);
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
			prompt_cmd(vis, cmd[0], cmd+1);
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
			prompt_cmd(vis, cmd[0], cmd+1);
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
			vis_keys(vis, key);

		if (vis->mode->idle)
			timeout = &idle;
	}
	return vis->exit_status;
}

bool vis_operator(Vis *vis, enum VisOperator id) {
	switch (id) {
	case OP_CASE_LOWER:
	case OP_CASE_UPPER:
	case OP_CASE_SWAP:
		vis->action.arg.i = id;
		id = OP_CASE_SWAP;
		break;
	case OP_CURSOR_SOL:
	case OP_CURSOR_EOL:
		vis->action.arg.i = id;
		id = OP_CURSOR_SOL;
		break;
	case OP_PUT_AFTER:
	case OP_PUT_AFTER_END:
	case OP_PUT_BEFORE:
	case OP_PUT_BEFORE_END:
		vis->action.arg.i = id;
		id = OP_PUT_AFTER;
		break;
	default:
		break;
	}
	if (id >= LENGTH(ops))
		return false;
	Operator *op = &ops[id];
	if (vis->mode->visual) {
		vis->action.op = op;
		action_do(vis, &vis->action);
		return true;
	}
	/* switch to operator mode inorder to make operator options and
	 * text-object available */
	vis_mode_switch(vis, VIS_MODE_OPERATOR);
	if (vis->action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		vis->action.type = LINEWISE;
		vis_motion(vis, MOVE_LINE_NEXT);
	} else {
		vis->action.op = op;
	}

	/* put is not a real operator, does not need a range to operate on */
	if (id == OP_PUT_AFTER)
		vis_motion(vis, MOVE_NOP);

	return true;
}

void vis_mode_switch(Vis *vis, enum VisMode mode) {
	mode_set(vis, &vis_modes[mode]);
}

bool vis_motion(Vis *vis, enum VisMotion motion, ...) {
	va_list ap;
	va_start(ap, motion);

	switch (motion) {
	case MOVE_WORD_START_NEXT:
		if (vis->action.op == &ops[OP_CHANGE])
			motion = MOVE_WORD_END_NEXT;
		break;
	case MOVE_LONGWORD_START_NEXT:
		if (vis->action.op == &ops[OP_CHANGE])
			motion = MOVE_LONGWORD_END_NEXT;
		break;
	case MOVE_SEARCH_FORWARD:
	case MOVE_SEARCH_BACKWARD:
	{
		const char *pattern = va_arg(ap, char*);
		if (text_regex_compile(vis->search_pattern, pattern, REG_EXTENDED)) {
			action_reset(vis, &vis->action);
			goto err;
		}
		if (motion == MOVE_SEARCH_FORWARD)
			motion = MOVE_SEARCH_NEXT;
		else
			motion = MOVE_SEARCH_PREV;
		break;
	}
	case MOVE_RIGHT_TO:
	case MOVE_LEFT_TO:
	case MOVE_RIGHT_TILL:
	case MOVE_LEFT_TILL:
	{
		const char *key = va_arg(ap, char*);
		if (!key)
			goto err;
		strncpy(vis->search_char, key, sizeof(vis->search_char));
		vis->search_char[sizeof(vis->search_char)-1] = '\0';
		vis->last_totill = motion;
		break;
	}
	case MOVE_TOTILL_REPEAT:
		if (!vis->last_totill)
			goto err;
		motion = vis->last_totill;
		break;
	case MOVE_TOTILL_REVERSE:
		switch (vis->last_totill) {
		case MOVE_RIGHT_TO:
			motion = MOVE_LEFT_TO;
			break;
		case MOVE_LEFT_TO:
			motion = MOVE_RIGHT_TO;
			break;
		case MOVE_RIGHT_TILL:
			motion = MOVE_LEFT_TILL;
			break;
		case MOVE_LEFT_TILL:
			motion = MOVE_RIGHT_TILL;
			break;
		default:
			goto err;
		}
		break;
	case MOVE_MARK:
	case MOVE_MARK_LINE:
	{
		int mark = va_arg(ap, int);
		if (MARK_a <= mark && mark < VIS_MARK_INVALID)
			vis->action.mark = mark;
		else
			goto err;
		break;
	}
	default:
		break;
	}

	vis->action.movement = &moves[motion];
	va_end(ap);
	action_do(vis, &vis->action);
	return true;
err:
	va_end(ap);
	return false;
}

void vis_textobject(Vis *vis, enum VisTextObject textobj) {
	if (textobj < LENGTH(textobjs)) {
		vis->action.textobj = &textobjs[textobj];
		action_do(vis, &vis->action);
	}
}

static Macro *macro_get(Vis *vis, enum VisMacro m) {
	if (m == VIS_MACRO_LAST_RECORDED)
		return vis->last_recording;
	if (m < LENGTH(vis->macros))
		return &vis->macros[m];
	return NULL;
}

void macro_operator_record(Vis *vis) {
	vis->macro_operator = macro_get(vis, VIS_MACRO_OPERATOR);
	macro_reset(vis->macro_operator);
}

void macro_operator_stop(Vis *vis) {
	vis->macro_operator = NULL;
}

bool vis_macro_record(Vis *vis, enum VisMacro id) {
	Macro *macro = macro_get(vis, id);
	if (vis->recording || !macro)
		return false;
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
	vis_keys_raw(vis, &buf, macro->data);
	buffer_release(&buf);
}

bool vis_macro_replay(Vis *vis, enum VisMacro id) {
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
	if (count)
		vis->action_prev.count = count;
	count = vis->action_prev.count;
	/* for some operators count should be applied only to the macro not the motion */
	if (vis->action_prev.op == &ops[OP_INSERT] || vis->action_prev.op == &ops[OP_REPLACE])
		vis->action_prev.count = 1;
	action_do(vis, &vis->action_prev);
	vis->action_prev.count = count;
	if (macro) {
		Mode *mode = vis->mode;
		Action action_prev = vis->action_prev;
		count = action_prev.count;
		if (count < 1 || action_prev.op == &ops[OP_CHANGE])
			count = 1;
		for (int i = 0; i < count; i++) {
			mode_set(vis, mode);
			macro_replay(vis, macro);
		}
		vis->action_prev = action_prev;
	}
	action_reset(vis, &vis->action);
}

void vis_mark_set(Vis *vis, enum VisMark mark, size_t pos) {
	File *file = vis->win->file;
	if (mark < LENGTH(file->marks))
		file->marks[mark] = text_mark_set(file->text, pos);
}

void vis_motion_type(Vis *vis, enum VisMotionType type) {
	vis->action.type = type;
}

int vis_count_get(Vis *vis) {
	return vis->action.count;
}

void vis_count_set(Vis *vis, int count) {
	vis->action.count = count;
}

void vis_register_set(Vis *vis, enum VisRegister reg) {
	if (reg < LENGTH(vis->registers))
		vis->action.reg = &vis->registers[reg];
}

Register *vis_register_get(Vis *vis, enum VisRegister reg) {
	if (reg < LENGTH(vis->registers))
		return &vis->registers[reg];
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
	const char *tab = expandtab(vis);
	vis_insert_key(vis, tab, strlen(tab));
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
	vis_insert_key(win->editor, buf, len);
	free(buf);
}

void vis_insert_nl(Vis *vis) {
	const char *nl;
	switch (text_newline_type(vis->win->file->text)) {
	case TEXT_NEWLINE_CRNL:
		nl = "\r\n";
		break;
	default:
		nl = "\n";
		break;
	}

	vis_insert_key(vis, nl, strlen(nl));

	if (vis->autoindent)
		copy_indent_from_previous_line(vis->win);
}

void vis_prompt_enter(Vis *vis) {
	char *s = vis_prompt_get(vis);
	/* it is important to switch back to the previous mode, which hides
	 * the prompt and more importantly resets vis->win to the currently
	 * focused editor window *before* anything is executed which depends
	 * on vis->win.
	 */
	mode_set(vis, vis->mode_before_prompt);
	if (s && *s && prompt_cmd(vis, vis->prompt_type, s) && vis->running)
		vis_mode_switch(vis, VIS_MODE_NORMAL);
	free(s);
	vis_draw(vis);
}

Text *vis_text(Vis *vis) {
	return vis->win->file->text;
}

View *vis_view(Vis *vis) {
	return vis->win->view;
}

Text *vis_file_text(File *file) {
	return file->text;
}

const char *vis_file_name(File *file) {
	return file->name;
}
