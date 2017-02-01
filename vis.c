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
#include <pwd.h>
#include <libgen.h>
#include <termkey.h>

#include "vis.h"
#include "text-util.h"
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"
#include "vis-core.h"
#include "sam.h"

const MarkDef vis_marks[] = {
	[VIS_MARK_SELECTION_START] = { '<', "Last selection start" },
	[VIS_MARK_SELECTION_END]   = { '>', "Last selection end" },
};

const RegisterDef vis_registers[] = {
	[VIS_REG_DEFAULT]    = { '"', "Unnamed register"                                 },
	[VIS_REG_ZERO]       = { '0', "Yank register"                                    },
	[VIS_REG_1]          = { '1', "1st sub-expression match"                         },
	[VIS_REG_2]          = { '2', "2nd sub-expression match"                         },
	[VIS_REG_3]          = { '3', "3rd sub-expression match"                         },
	[VIS_REG_4]          = { '4', "4th sub-expression match"                         },
	[VIS_REG_5]          = { '5', "5th sub-expression match"                         },
	[VIS_REG_6]          = { '6', "6th sub-expression match"                         },
	[VIS_REG_7]          = { '7', "7th sub-expression match"                         },
	[VIS_REG_8]          = { '8', "8th sub-expression match"                         },
	[VIS_REG_9]          = { '9', "9th sub-expression match"                         },
	[VIS_REG_AMPERSAND]  = { '&', "Last regex match"                                 },
	[VIS_REG_BLACKHOLE]  = { '_', "/dev/null register"                               },
	[VIS_REG_CLIPBOARD]  = { '*', "System clipboard register, see vis-clipboard(1)"  },
	[VIS_REG_DOT]        = { '.', "Last inserted text"                               },
	[VIS_REG_SEARCH]     = { '/', "Last search pattern"                              },
	[VIS_REG_COMMAND]    = { ':', "Last :-command"                                   },
	[VIS_REG_SHELL]      = { '!', "Last shell command given to either <, >, |, or !" },
};

static Macro *macro_get(Vis *vis, enum VisRegister);
static void macro_replay(Vis *vis, const Macro *macro);
static void macro_replay_internal(Vis *vis, const Macro *macro);
static void vis_keys_push(Vis *vis, const char *input, size_t pos, bool record);

bool vis_event_emit(Vis *vis, enum VisEvents id, ...) {
	if (!vis->event)
		return true;

	if (!vis->initialized) {
		vis->initialized = true;
		vis->ui->init(vis->ui, vis);
		if (vis->event->init)
			vis->event->init(vis);
	}

	va_list ap;
	va_start(ap, id);
	bool ret = true;

	switch (id) {
	case VIS_EVENT_INIT:
		break;
	case VIS_EVENT_START:
		if (vis->event->start)
			vis->event->start(vis);
		break;
	case VIS_EVENT_FILE_OPEN:
	case VIS_EVENT_FILE_SAVE_PRE:
	case VIS_EVENT_FILE_SAVE_POST:
	case VIS_EVENT_FILE_CLOSE:
	{
		File *file = va_arg(ap, File*);
		if (file->internal)
			break;
		if (id == VIS_EVENT_FILE_OPEN && vis->event->file_open) {
			vis->event->file_open(vis, file);
		} else if (id == VIS_EVENT_FILE_SAVE_PRE && vis->event->file_save_pre) {
			const char *path = va_arg(ap, const char*);
			ret = vis->event->file_save_pre(vis, file, path);
		} else if (id == VIS_EVENT_FILE_SAVE_POST && vis->event->file_save_post) {
			const char *path = va_arg(ap, const char*);
			vis->event->file_save_post(vis, file, path);
		} else if (id == VIS_EVENT_FILE_CLOSE && vis->event->file_close) {
			vis->event->file_close(vis, file);
		}
		break;
	}
	case VIS_EVENT_WIN_OPEN:
	case VIS_EVENT_WIN_CLOSE:
	case VIS_EVENT_WIN_HIGHLIGHT:
	case VIS_EVENT_WIN_SYNTAX:
	case VIS_EVENT_WIN_STATUS:
	{
		Win *win = va_arg(ap, Win*);
		if (win->file->internal)
			break;
		if (vis->event->win_open && id == VIS_EVENT_WIN_OPEN) {
			vis->event->win_open(vis, win);
		} else if (vis->event->win_close && id == VIS_EVENT_WIN_CLOSE) {
			vis->event->win_close(vis, win);
		} else if (vis->event->win_highlight && id == VIS_EVENT_WIN_HIGHLIGHT) {
			vis->event->win_highlight(vis, win, win->horizon);
		} else if (vis->event->win_syntax && id == VIS_EVENT_WIN_SYNTAX) {
			const char *syntax = va_arg(ap, const char*);
			ret = vis->event->win_syntax(vis, win, syntax);
		} else if (vis->event->win_status && id == VIS_EVENT_WIN_STATUS) {
			vis->event->win_status(vis, win);
		}
		break;
	}
	case VIS_EVENT_QUIT:
		if (vis->event->quit)
			vis->event->quit(vis);
		break;
	}

	va_end(ap);
	return ret;
}

/** window / file handling */

static void file_free(Vis *vis, File *file) {
	if (!file)
		return;
	if (file->refcount > 1) {
		--file->refcount;
		return;
	}
	vis_event_emit(vis, VIS_EVENT_FILE_CLOSE, file);
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
	file->fd = -1;
	file->text = text;
	file->stat = text_stat(text);
	if (vis->files)
		vis->files->prev = file;
	file->next = vis->files;
	vis->files = file;
	return file;
}

static char *absolute_path(const char *name) {
	if (!name)
		return NULL;
	char *copy1 = strdup(name);
	char *copy2 = strdup(name);
	char *path_absolute = NULL;
	char path_normalized[PATH_MAX] = "";

	if (!copy1 || !copy2)
		goto err;

	char *dir = dirname(copy1);
	char *base = basename(copy2);
	if (!(path_absolute = realpath(dir, NULL)))
		goto err;

	snprintf(path_normalized, sizeof(path_normalized)-1, "%s/%s",
	         path_absolute, base);
err:
	free(copy1);
	free(copy2);
	free(path_absolute);
	return path_normalized[0] ? strdup(path_normalized) : NULL;
}

static File *file_new(Vis *vis, const char *name) {
	char *name_absolute = NULL;
	if (name) {
		if (!(name_absolute = absolute_path(name)))
			return NULL;
		File *existing = NULL;
		/* try to detect whether the same file is already open in another window
		 * TODO: do this based on inodes */
		for (File *file = vis->files; file; file = file->next) {
			if (file->name && strcmp(file->name, name_absolute) == 0) {
				existing = file;
				break;
			}
		}
		if (existing) {
			free(name_absolute);
			return existing;
		}
	}

	File *file = NULL;
	Text *text = text_load(name);
	if (!text && name && errno == ENOENT)
		text = text_load(NULL);
	if (!text)
		goto err;
	if (!(file = file_new_text(vis, text)))
		goto err;
	file->name = name_absolute;
	vis_event_emit(vis, VIS_EVENT_FILE_OPEN, file);
	return file;
err:
	free(name_absolute);
	text_free(text);
	file_free(vis, file);
	return NULL;
}

static File *file_new_internal(Vis *vis, const char *filename) {
	File *file = file_new(vis, filename);
	if (file) {
		file->refcount = 1;
		file->internal = true;
	}
	return file;
}

void file_name_set(File *file, const char *name) {
	if (name == file->name)
		return;
	free((char*)file->name);
	file->name = absolute_path(name);
}

const char *file_name_get(File *file) {
	/* TODO: calculate path relative to working directory, cache result */
	if (!file->name)
		return NULL;
	char cwd[PATH_MAX];
	if (!getcwd(cwd, sizeof cwd))
		return file->name;
	const char *path = strstr(file->name, cwd);
	if (path != file->name)
		return file->name;
	size_t cwdlen = strlen(cwd);
	return file->name[cwdlen] == '/' ? file->name+cwdlen+1 : file->name;
}

void vis_window_status(Win *win, const char *status) {
	win->ui->status(win->ui, status);
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
	free(win->lexer_name);
	free(win);
}

static void window_draw(void *ctx) {
	Win *win = ctx;
	if (!win->ui)
		return;
	Vis *vis = win->vis;
	vis_event_emit(vis, VIS_EVENT_WIN_HIGHLIGHT, win);
	vis_event_emit(vis, VIS_EVENT_WIN_STATUS, win);
}

Win *window_new_file(Vis *vis, File *file, enum UiOption options) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->vis = vis;
	win->file = file;
	win->jumplist = ringbuf_alloc(31);
	win->event.data = win;
	win->event.draw = window_draw;
	win->horizon = 1 << 15;
	win->view = view_new(file->text, &win->event);
	win->ui = vis->ui->window_new(vis->ui, win->view, file, options);
	if (!win->jumplist || !win->view || !win->ui) {
		window_free(win);
		return NULL;
	}
	file->refcount++;
	view_tabwidth_set(win->view, vis->tabwidth);

	if (vis->windows)
		vis->windows->prev = win;
	win->next = vis->windows;
	vis->windows = win;
	vis->win = win;
	vis->ui->window_focus(win->ui);
	for (size_t i = 0; i < LENGTH(win->modes); i++)
		win->modes[i].parent = &vis_modes[i];
	vis_event_emit(vis, VIS_EVENT_WIN_OPEN, win);
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
	Win *win = window_new_file(original->vis, original->file, UI_OPTION_STATUSBAR);
	if (!win)
		return false;
	for (size_t i = 0; i < LENGTH(win->modes); i++) {
		if (original->modes[i].bindings)
			win->modes[i].bindings = map_new();
		if (win->modes[i].bindings)
			map_copy(win->modes[i].bindings, original->modes[i].bindings);
	}
	win->file = original->file;
	vis_window_syntax_set(win, vis_window_syntax_get(original));
	view_options_set(win->view, view_options_get(original->view));
	view_cursor_to(win->view, view_cursor_get(original->view));
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

const char *vis_window_syntax_get(Win *win) {
	return win->lexer_name;
}

bool vis_window_syntax_set(Win *win, const char *syntax) {
	if (!vis_event_emit(win->vis, VIS_EVENT_WIN_SYNTAX, win, syntax))
		return false;
	free(win->lexer_name);
	win->lexer_name = syntax ? strdup(syntax) : NULL;
	return !syntax || win->lexer_name;
}

int vis_window_width_get(const Win *win) {
	return win->ui->window_width(win->ui);
}

int vis_window_height_get(const Win *win) {
	return win->ui->window_height(win->ui);
}

void vis_draw(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next)
		view_draw(win->view);
	vis->ui->draw(vis->ui);
}

void vis_redraw(Vis *vis) {
	vis->ui->redraw(vis->ui);
}

void vis_update(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next)
		view_update(win->view);
	vis->ui->update(vis->ui);
}

void vis_suspend(Vis *vis) {
	vis->ui->suspend(vis->ui);
}

bool vis_window_new(Vis *vis, const char *filename) {
	File *file = file_new(vis, filename);
	if (!file)
		return false;
	Win *win = window_new_file(vis, file, UI_OPTION_STATUSBAR);
	if (!win) {
		file_free(vis, file);
		return false;
	}

	return true;
}

bool vis_window_new_fd(Vis *vis, int fd) {
	if (fd == -1)
		return false;
	if (!vis_window_new(vis, NULL))
		return false;
	vis->win->file->fd = fd;
	return true;
}

bool vis_window_closable(Win *win) {
	if (!win || !text_modified(win->file->text))
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
	if (!win)
		return;
	Vis *vis = win->vis;
	vis_event_emit(vis, VIS_EVENT_WIN_CLOSE, win);
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
	vis->tabwidth = 8;
	vis->expandtab = false;
	vis->change_colors = true;
	vis->registers[VIS_REG_BLACKHOLE].type = REGISTER_BLACKHOLE;
	vis->registers[VIS_REG_CLIPBOARD].type = REGISTER_CLIPBOARD;
	array_init(&vis->motions);
	array_init(&vis->textobjects);
	array_init(&vis->bindings);
	array_init(&vis->actions_user);
	action_reset(&vis->action);
	buffer_init(&vis->input_queue);
	vis->keys = &vis->input_queue;
	if (!(vis->command_file = file_new_internal(vis, NULL)))
		goto err;
	if (!(vis->search_file = file_new_internal(vis, NULL)))
		goto err;
	if (!(vis->error_file = file_new_internal(vis, NULL)))
		goto err;
	if (!(vis->actions = map_new()))
		goto err;
	if (!(vis->keymap = map_new()))
		goto err;
	if (!sam_init(vis))
		goto err;
	struct passwd *pw;
	char *shell = getenv("SHELL");
	if ((!shell || !*shell) && (pw = getpwuid(getuid())))
		shell = pw->pw_shell;
	if (!shell || !*shell)
		shell = "/bin/sh";
	if (!(vis->shell = strdup(shell)))
		goto err;
	vis->mode_prev = vis->mode = &vis_modes[VIS_MODE_NORMAL];
	vis->event = event;
	if (event) {
		if (event->mode_insert_input)
			vis_modes[VIS_MODE_INSERT].input = event->mode_insert_input;
		if (event->mode_replace_input)
			vis_modes[VIS_MODE_REPLACE].input = event->mode_replace_input;
	}
	return vis;
err:
	vis_free(vis);
	return NULL;
}

void vis_free(Vis *vis) {
	if (!vis)
		return;
	vis_event_emit(vis, VIS_EVENT_QUIT);
	vis->event = NULL;
	while (vis->windows)
		vis_window_close(vis->windows);
	file_free(vis, vis->command_file);
	file_free(vis, vis->search_file);
	file_free(vis, vis->error_file);
	for (int i = 0; i < LENGTH(vis->registers); i++)
		register_release(&vis->registers[i]);
	vis->ui->free(vis->ui);
	if (vis->usercmds) {
		const char *name;
		while (map_first(vis->usercmds, &name) && vis_cmd_unregister(vis, name));
	}
	map_free(vis->usercmds);
	map_free(vis->cmds);
	map_free(vis->options);
	map_free(vis->actions);
	map_free(vis->keymap);
	buffer_release(&vis->input_queue);
	for (int i = 0; i < VIS_MODE_INVALID; i++)
		map_free(vis_modes[i].bindings);
	array_release_full(&vis->motions);
	array_release_full(&vis->textobjects);
	while (array_length(&vis->bindings))
		vis_binding_free(vis, array_get_ptr(&vis->bindings, 0));
	array_release(&vis->bindings);
	while (array_length(&vis->actions_user))
		vis_action_free(vis, array_get_ptr(&vis->actions_user, 0));
	array_release(&vis->actions_user);
	free(vis->shell);
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
	return map_put(vis->actions, action->name, action);
}

bool vis_keymap_add(Vis *vis, const char *key, const char *mapping) {
	return map_put(vis->keymap, key, mapping);
}

void vis_keymap_disable(Vis *vis) {
	vis->keymap_disabled = true;
}

static void window_jumplist_add(Win *win, size_t pos) {
	Mark mark = text_mark_set(win->file->text, pos);
	if (mark && win->jumplist)
		ringbuf_add(win->jumplist, (void*)mark);
}

static void window_jumplist_invalidate(Win *win) {
	if (win->jumplist)
		ringbuf_invalidate(win->jumplist);
}

void vis_do(Vis *vis) {
	Win *win = vis->win;
	File *file = win->file;
	Text *txt = file->text;
	View *view = win->view;
	Action *a = &vis->action;

	if (a->op == &vis_operators[VIS_OP_FILTER] && !vis->mode->visual)
		vis_mode_switch(vis, VIS_MODE_VISUAL_LINE);

	int count = MAX(a->count, 1);
	if (a->op == &vis_operators[VIS_OP_MODESWITCH])
		count = 1; /* count should apply to inserted text not motion */
	bool repeatable = a->op && !vis->macro_operator && !vis->win->parent;
	bool multiple_cursors = view_cursors_multiple(view);
	bool linewise = !(a->type & CHARWISE) && (
		a->type & LINEWISE || (a->movement && a->movement->type & LINEWISE) ||
		vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE]);

	for (Cursor *cursor = view_cursors(view), *next; cursor; cursor = next) {

		next = view_cursors_next(cursor);
		size_t pos = view_cursors_pos(cursor);
		Register *reg = multiple_cursors ? view_cursors_register(cursor) : a->reg;
		if (!reg)
			reg = &vis->registers[file->internal ? VIS_REG_PROMPT : VIS_REG_DEFAULT];

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
				size_t pos_prev = pos;
				if (a->movement->txt)
					pos = a->movement->txt(txt, pos);
				else if (a->movement->cur)
					pos = a->movement->cur(cursor);
				else if (a->movement->file)
					pos = a->movement->file(vis, file, pos);
				else if (a->movement->vis)
					pos = a->movement->vis(vis, txt, pos);
				else if (a->movement->view)
					pos = a->movement->view(vis, view);
				else if (a->movement->win)
					pos = a->movement->win(vis, win, pos);
				else if (a->movement->user)
					pos = a->movement->user(vis, win, a->movement->data, pos);
				if (pos == EPOS || a->movement->type & IDEMPOTENT || pos == pos_prev)
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
			} else if (a->movement->type & INCLUSIVE ||
			          (linewise && a->movement->type & LINEWISE_INCLUSIVE)) {
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

				if (vis->mode->visual)
					c.range = text_range_union(&c.range, &r);
				else
					c.range = r;

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
					if (!text_range_contains(&sel, pos)) {
						Filerange cur = text_range_new(pos, pos);
						sel = text_range_union(&sel, &cur);
					}
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
		if (a->op == &vis_operators[VIS_OP_CHANGE]) {
			vis_mode_switch(vis, VIS_MODE_INSERT);
		} else if (a->op == &vis_operators[VIS_OP_MODESWITCH]) {
			vis_mode_switch(vis, a->mode);
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

		if (vis->mode == &vis_modes[VIS_MODE_NORMAL])
			vis_file_snapshot(vis, file);
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
	if (!keys || !*keys)
		return NULL;
	TermKeyKey key;
	TermKey *termkey = vis->ui->termkey_get(vis->ui);
	const char *next = NULL;
	/* first try to parse a special key of the form <Key> */
	if (*keys == '<' && keys[1] && (next = termkey_strpkey(termkey, keys+1, &key, TERMKEY_FORMAT_VIM)) && *next == '>')
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
	if (ISUTF8(*keys))
		keys++;
	while (!ISUTF8(*keys))
		keys++;
	return keys;
}

long vis_keys_codepoint(Vis *vis, const char *keys) {
	long codepoint = -1;
	const char *next;
	TermKeyKey key;
	TermKey *termkey = vis->ui->termkey_get(vis->ui);

	if (!keys[0])
		return -1;
	if (keys[0] == '<' && !keys[1])
		return '<';

	if (keys[0] == '<' && (next = termkey_strpkey(termkey, keys+1, &key, TERMKEY_FORMAT_VIM)) && *next == '>')
		codepoint = (key.type == TERMKEY_TYPE_UNICODE) ? key.code.codepoint : -1;
	else if ((next = termkey_strpkey(termkey, keys, &key, TERMKEY_FORMAT_VIM)))
		codepoint = (key.type == TERMKEY_TYPE_UNICODE) ? key.code.codepoint : -1;

	if (codepoint != -1) {
		if (key.modifiers == TERMKEY_KEYMOD_CTRL)
			codepoint &= 0x1f;
		return codepoint;
	}

	if (!next || key.type != TERMKEY_TYPE_KEYSYM)
		return -1;

	const int keysym[] = {
		TERMKEY_SYM_ENTER, '\n',
		TERMKEY_SYM_TAB, '\t',
		TERMKEY_SYM_BACKSPACE, '\b',
		TERMKEY_SYM_ESCAPE, 0x1b,
		TERMKEY_SYM_DELETE, 0x7f,
		0,
	};

	for (const int *k = keysym; k[0]; k += 2) {
		if (key.code.sym == k[0])
			return k[1];
	}

	return -1;
}

static void vis_keys_process(Vis *vis, size_t pos) {
	Buffer *buf = vis->keys;
	char *keys = buf->data + pos, *start = keys, *cur = keys, *end = keys, *binding_end = keys;;
	bool prefix = false;
	KeyBinding *binding = NULL;

	while (cur && *cur) {

		if (!(end = (char*)vis_keys_next(vis, cur))) {
			buffer_remove(buf, keys - buf->data, strlen(keys));
			return;
		}

		char tmp = *end;
		*end = '\0';
		prefix = false;

		for (Mode *global_mode = vis->mode; global_mode && !prefix; global_mode = global_mode->parent) {
			for (int global = 0; global < 2 && !prefix; global++) {
				Mode *mode = (global || !vis->win) ?
					     global_mode :
				             &vis->win->modes[global_mode->id];
				if (!mode->bindings)
					continue;
				/* keep track of longest matching binding */
				KeyBinding *match = map_get(mode->bindings, start);
				if (match && end > binding_end) {
					binding = match;
					binding_end = end;
				}
				/* "<" is never treated as a prefix because it
				 * is used to denote special key symbols */
				if (strcmp(start, "<")) {
					prefix = (!match && map_contains(mode->bindings, start)) ||
					         (match && !map_leaf(mode->bindings, start));
				}
			}
		}

		*end = tmp;

		if (prefix) {
			/* input sofar is ambigious, wait for more */
			cur = end;
			end = start;
		} else if (binding) { /* exact match */
			if (binding->action) {
				size_t len = binding_end - start;
				strcpy(vis->key_prev, vis->key_current);
				strncpy(vis->key_current, start, len);
				vis->key_current[len] = '\0';
				end = (char*)binding->action->func(vis, binding_end, &binding->action->arg);
				if (!end) {
					end = start;
					break;
				}
				start = cur = end;
			} else if (binding->alias) {
				buffer_remove(buf, start - buf->data, binding_end - start);
				buffer_insert0(buf, start - buf->data, binding->alias);
				cur = end = start;
			}
			binding = NULL;
			binding_end = start;
		} else { /* no keybinding */
			KeyAction *action = NULL;
			if (start[0] == '<' && end[-1] == '>') {
				/* test for special editor key command */
				char tmp = end[-1];
				end[-1] = '\0';
				action = map_get(vis->actions, start+1);
				end[-1] = tmp;
				if (action) {
					size_t len = end - start;
					strcpy(vis->key_prev, vis->key_current);
					strncpy(vis->key_current, start, len);
					vis->key_current[len] = '\0';
					end = (char*)action->func(vis, end, &action->arg);
					if (!end) {
						end = start;
						break;
					}
				}
			}
			if (!action && vis->mode->input) {
				end = (char*)vis_keys_next(vis, start);
				vis->mode->input(vis, start, end - start);
			}
			start = cur = end;
		}
	}

	buffer_remove(buf, keys - buf->data, end - keys);
}

void vis_keys_feed(Vis *vis, const char *input) {
	if (!input)
		return;
	Macro macro;
	macro_init(&macro);
	if (!macro_append(&macro, input))
		return;
	/* use internal function, to keep Lua based tests which use undo points working */
	macro_replay_internal(vis, &macro);
	macro_release(&macro);
}

static void vis_keys_push(Vis *vis, const char *input, size_t pos, bool record) {
	if (!input)
		return;
	if (record && vis->recording)
		macro_append(vis->recording, input);
	if (vis->macro_operator)
		macro_append(vis->macro_operator, input);
	if (buffer_append0(vis->keys, input))
		vis_keys_process(vis, pos);
}

static const char *getkey(Vis *vis) {
	TermKeyKey key = { 0 };
	if (!vis->ui->getkey(vis->ui, &key))
		return NULL;
	vis_info_hide(vis);
	bool use_keymap = vis->mode->id != VIS_MODE_INSERT &&
	                  vis->mode->id != VIS_MODE_REPLACE &&
	                  !vis->keymap_disabled;
	vis->keymap_disabled = false;
	if (key.type == TERMKEY_TYPE_UNICODE && use_keymap) {
		const char *mapped = map_get(vis->keymap, key.utf8);
		if (mapped) {
			size_t len = strlen(mapped)+1;
			if (len <= sizeof(key.utf8))
				memcpy(key.utf8, mapped, len);
		}
	}

	TermKey *termkey = vis->ui->termkey_get(vis->ui);
	termkey_strfkey(termkey, vis->key, sizeof(vis->key), &key, TERMKEY_FORMAT_VIM);
	return vis->key;
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
	case SIGCONT:
	case SIGWINCH:
		vis->need_resize = true;
		return true;
	case SIGTERM:
	case SIGHUP:
		vis->terminate = true;
		return true;
	}
	return false;
}

int vis_run(Vis *vis, int argc, char *argv[]) {
	if (!vis->windows)
		return EXIT_SUCCESS;
	vis->running = true;

	vis_event_emit(vis, VIS_EVENT_START);

	struct timespec idle = { .tv_nsec = 0 }, *timeout = NULL;

	sigset_t emptyset;
	sigemptyset(&emptyset);
	vis_draw(vis);
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

		if (vis->terminate)
			vis_die(vis, "Killed by SIGTERM\n");

		if (vis->need_resize) {
			vis->ui->resize(vis->ui);
			vis->need_resize = false;
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
			vis_keys_push(vis, key, 0, true);

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
	if (vis->macro_operator)
		return;
	vis->macro_operator = macro_get(vis, VIS_MACRO_OPERATOR);
	macro_reset(vis->macro_operator);
}

void macro_operator_stop(Vis *vis) {
	if (!vis->macro_operator)
		return;
	Macro *dot = macro_get(vis, VIS_REG_DOT);
	buffer_put(dot, vis->macro_operator->data, vis->macro_operator->len);
	vis->action_prev.macro = dot;
	vis->macro_operator = NULL;
}

bool vis_macro_record(Vis *vis, enum VisRegister id) {
	Macro *macro = macro_get(vis, id);
	if (vis->recording || !macro)
		return false;
	if (!(VIS_REG_A <= id && id <= VIS_REG_Z))
		macro_reset(macro);
	vis->recording = macro;
	vis_event_emit(vis, VIS_EVENT_WIN_STATUS, vis->win);
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
	vis_event_emit(vis, VIS_EVENT_WIN_STATUS, vis->win);
	return true;
}

bool vis_macro_recording(Vis *vis) {
	return vis->recording;
}

static void macro_replay(Vis *vis, const Macro *macro) {
	const Macro *replaying = vis->replaying;
	vis->replaying = macro;
	macro_replay_internal(vis, macro);
	vis->replaying = replaying;
}

static void macro_replay_internal(Vis *vis, const Macro *macro) {
	size_t pos = buffer_length0(vis->keys);
	for (char *key = macro->data, *next; key; key = next) {
		char tmp;
		next = (char*)vis_keys_next(vis, key);
		if (next) {
			tmp = *next;
			*next = '\0';
		}

		vis_keys_push(vis, key, pos, false);

		if (next)
			*next = tmp;
	}
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
	int count = vis_count_get_default(vis, 1);
	vis_cancel(vis);
	for (int i = 0; i < count; i++)
		macro_replay(vis, macro);
	vis_file_snapshot(vis, vis->win->file);
	return true;
}

void vis_repeat(Vis *vis) {
	const Macro *macro = vis->action_prev.macro;
	int count = vis->action.count;
	if (count != VIS_COUNT_UNKNOWN)
		vis->action_prev.count = count;
	else
		count = vis->action_prev.count;
	vis->action = vis->action_prev;
	vis_do(vis);
	if (macro) {
		Mode *mode = vis->mode;
		Action action_prev = vis->action_prev;
		if (count < 1 ||
		    action_prev.op == &vis_operators[VIS_OP_CHANGE] ||
		    action_prev.op == &vis_operators[VIS_OP_FILTER])
			count = 1;
		if (vis->action_prev.op == &vis_operators[VIS_OP_MODESWITCH])
			vis->action_prev.count = 1;
		for (int i = 0; i < count; i++) {
			mode_set(vis, mode);
			macro_replay(vis, macro);
		}
		vis->action_prev = action_prev;
	}
	vis_cancel(vis);
	vis_file_snapshot(vis, vis->win->file);
}

enum VisMark vis_mark_from(Vis *vis, char mark) {
	if (mark >= 'a' && mark <= 'z')
		return VIS_MARK_a + mark - 'a';
	for (size_t i = 0; i < LENGTH(vis_marks); i++) {
		if (vis_marks[i].name == mark)
			return i;
	}
	return VIS_MARK_INVALID;
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

enum VisRegister vis_register_from(Vis *vis, char reg) {
	switch (reg) {
	case '+': return VIS_REG_CLIPBOARD;
	case '@': return VIS_MACRO_LAST_RECORDED;
	}

	if ('a' <= reg && reg <= 'z')
		return VIS_REG_a + reg - 'a';
	if ('A' <= reg && reg <= 'Z')
		return VIS_REG_A + reg - 'A';
	for (size_t i = 0; i < LENGTH(vis_registers); i++) {
		if (vis_registers[i].name == reg)
			return i;
	}
	return VIS_REG_INVALID;
}

void vis_register_set(Vis *vis, enum VisRegister reg) {
	if (VIS_REG_A <= reg && reg <= VIS_REG_Z) {
		vis->action.reg = &vis->registers[VIS_REG_a + reg - VIS_REG_A];
		vis->action.reg->append = true;
	} else if (reg < LENGTH(vis->registers)) {
		vis->action.reg = &vis->registers[reg];
		vis->action.reg->append = false;
	}
}

const char *vis_register_get(Vis *vis, enum VisRegister reg, size_t *len) {
	if (VIS_REG_A <= reg && reg <= VIS_REG_Z)
		reg = VIS_REG_a + reg - VIS_REG_A;
	if (reg < LENGTH(vis->registers))
		return register_get(vis, &vis->registers[reg], len);
	*len = 0;
	return NULL;
}

void vis_exit(Vis *vis, int status) {
	vis->running = false;
	vis->exit_status = status;
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

static void copy_indent_from_previous_line(Win *win, Cursor *cur) {
	Text *text = win->file->text;
	size_t pos = view_cursors_pos(cur);
	size_t prev_line = text_line_prev(text, pos);
	if (pos == prev_line)
		return;
	size_t begin = text_line_begin(text, prev_line);
	size_t start = text_line_start(text, begin);
	size_t end = text_line_end(text, begin);
	size_t len = start-begin;
	char *buf = malloc(len);
	if (!buf)
		return;
	len = text_bytes_get(text, begin, len, buf);
	text_insert(text, pos, buf, len);
	if (start == end) {
		text_delete(text, begin, len);
		pos -= len;
	}
	view_cursors_to(cur, pos + len);
	free(buf);
}

void vis_insert_nl(Vis *vis) {
	Text *txt = vis->win->file->text;
	const char *nl = text_newline_char(txt);
	size_t len = strlen(nl);
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		char byte;
		size_t pos = view_cursors_pos(c);
		/* insert second newline at end of file, except if there is already one */
		bool eof = pos == text_size(txt);
		bool nl2 = eof && !(pos > 0 && text_byte_get(txt, pos-1, &byte) && byte == '\n');
		vis_insert(vis, pos, nl, len);
		if (eof) {
			if (nl2)
				vis_insert(vis, pos, nl, len);
			else
				pos -= len; /* place cursor before, not after nl */
		}
		view_cursors_scroll_to(c, pos + len);
	}

	if (!vis->autoindent)
		return;

	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
		copy_indent_from_previous_line(vis->win, c);
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
	int pin[2], pout[2], perr[2], status = -1;
	bool interactive = !text_range_valid(range);
	Filerange rout = interactive ? text_range_new(0, 0) : *range;

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
		int null = open("/dev/null", O_WRONLY);
		if (null == -1) {
			fprintf(stderr, "failed to open /dev/null");
			exit(EXIT_FAILURE);
		}
		if (!interactive)
			dup2(pin[0], STDIN_FILENO);
		close(pin[0]);
		close(pin[1]);
		if (interactive)
			dup2(STDERR_FILENO, STDOUT_FILENO);
		else if (read_stdout)
			dup2(pout[1], STDOUT_FILENO);
		else
			dup2(null, STDOUT_FILENO);
		close(pout[1]);
		close(pout[0]);
		if (!interactive) {
			if (read_stderr)
				dup2(perr[1], STDERR_FILENO);
			else
				dup2(null, STDERR_FILENO);
		}
		close(perr[0]);
		close(perr[1]);
		close(null);
		if (!argv[1])
			execlp(vis->shell, vis->shell, "-c", argv[0], (char*)NULL);
		else
			execvp(argv[0], (char* const*)argv);
		fprintf(stderr, "exec failure: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	vis->cancel_filter = false;

	close(pin[0]);
	close(pout[1]);
	close(perr[1]);

	if (fcntl(pout[0], F_SETFL, O_NONBLOCK) == -1 ||
	    fcntl(perr[0], F_SETFL, O_NONBLOCK) == -1)
		goto err;

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

err:
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

static ssize_t read_buffer(void *context, char *data, size_t len) {
	buffer_append(context, data, len);
	return len;
}

int vis_pipe_collect(Vis *vis, Filerange *range, const char *argv[], char **out, char **err) {
	Buffer bufout, buferr;
	buffer_init(&bufout);
	buffer_init(&buferr);
	int status = vis_pipe(vis, range, argv, &bufout, read_buffer, &buferr, read_buffer);
	buffer_terminate(&bufout);
	buffer_terminate(&buferr);
	if (out)
		*out = bufout.data;
	if (err)
		*err = buferr.data;
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

void vis_file_snapshot(Vis *vis, File *file) {
	if (!vis->replaying)
		text_snapshot(file->text);
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

bool vis_get_autoindent(const Vis *vis) {
	return vis->autoindent;
}
