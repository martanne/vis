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
#include "ui.h"


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
	case VIS_EVENT_WIN_STATUS:
	{
		Win *win = va_arg(ap, Win*);
		if (win->file->internal && id != VIS_EVENT_WIN_STATUS)
			break;
		if (vis->event->win_open && id == VIS_EVENT_WIN_OPEN) {
			vis->event->win_open(vis, win);
		} else if (vis->event->win_close && id == VIS_EVENT_WIN_CLOSE) {
			vis->event->win_close(vis, win);
		} else if (vis->event->win_highlight && id == VIS_EVENT_WIN_HIGHLIGHT) {
			vis->event->win_highlight(vis, win);
		} else if (vis->event->win_status && id == VIS_EVENT_WIN_STATUS) {
			vis->event->win_status(vis, win);
		}
		break;
	}
	case VIS_EVENT_QUIT:
		if (vis->event->quit)
			vis->event->quit(vis);
		break;
	case VIS_EVENT_TERM_CSI:
		if (vis->event->term_csi)
			vis->event->term_csi(vis, va_arg(ap, const long *));
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
	for (size_t i = 0; i < LENGTH(file->marks); i++)
		mark_release(&file->marks[i]);
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
	for (size_t i = 0; i < LENGTH(file->marks); i++)
		mark_init(&file->marks[i]);
	if (vis->files)
		vis->files->prev = file;
	file->next = vis->files;
	vis->files = file;
	return file;
}

char *absolute_path(const char *name) {
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
	if (strcmp(path_absolute, "/") == 0)
		path_absolute[0] = '\0';

	snprintf(path_normalized, sizeof(path_normalized), "%s/%s",
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
	Text *text = text_load_method(name, vis->load_method);
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

void window_selection_save(Win *win) {
	Vis *vis = win->vis;
	View *view = win->view;
	Array sel = view_selections_get_all(view);
	vis_mark_set(win, VIS_MARK_SELECTION, &sel);
	array_release(&sel);
	vis_jumplist_save(vis);
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
	marklist_release(&win->jumplist);
	mark_release(&win->saved_selections);
	free(win);
}

static void window_draw_colorcolumn(Win *win) {
	View *view = win->view;
	int cc = view_colorcolumn_get(view);
	if (cc <= 0)
		return;
	CellStyle style = win->ui->style_get(win->ui, UI_STYLE_COLOR_COLUMN);
	size_t lineno = 0;
	int line_cols = 0; /* Track the number of columns we've passed on each line */
	bool line_cc_set = false; /* Has the colorcolumn attribute been set for this line yet */
	int width = view_width_get(view);

	for (Line *l = view_lines_first(view); l; l = l->next) {
		if (l->lineno != lineno) {
			line_cols = 0;
			line_cc_set = false;
			if (!(lineno = l->lineno))
				break;
		}
		if (line_cc_set)
			continue;

		/* This screen line contains the cell we want to highlight */
		if (cc <= line_cols + width) {
			CellStyle *orig = &l->cells[cc - 1 - line_cols].style;
			orig->attr = style.attr;
			orig->fg = is_default_color(style.fg) ? orig->fg : style.fg;
			orig->bg = is_default_color(style.bg) ? orig->bg : style.bg;
			line_cc_set = true;
		} else {
			line_cols += width;
		}
	}
}

static void window_draw_cursorline(Win *win) {
	Vis *vis = win->vis;
	View *view = win->view;
	enum UiOption options = view_options_get(view);
	if (!(options & UI_OPTION_CURSOR_LINE))
		return;
	if (vis->mode->visual || vis->win != win)
		return;
	if (view_selections_count(view) > 1)
		return;

	int width = view_width_get(view);
	CellStyle style = win->ui->style_get(win->ui, UI_STYLE_CURSOR_LINE);
	Selection *sel = view_selections_primary_get(view);
	size_t lineno = view_cursors_line_get(sel)->lineno;
	for (Line *l = view_lines_first(view); l; l = l->next) {
		if (l->lineno == lineno) {
			for (int x = 0; x < width; x++) {
				l->cells[x].style.attr |= style.attr;
				l->cells[x].style.bg = style.bg;
			}
		} else if (l->lineno > lineno) {
			break;
		}
	}
}

static void window_draw_selection(View *view, Selection *cur, CellStyle *style) {
	Filerange sel = view_selections_get(cur);
	if (!text_range_valid(&sel))
		return;
	Line *start_line; int start_col;
	Line *end_line; int end_col;
	view_coord_get(view, sel.start, &start_line, NULL, &start_col);
	view_coord_get(view, sel.end, &end_line, NULL, &end_col);
	if (!start_line && !end_line)
		return;
	if (!start_line) {
		start_line = view_lines_first(view);
		start_col = 0;
	}
	if (!end_line) {
		end_line = view_lines_last(view);
		end_col = end_line->width;
	}
	for (Line *l = start_line; l != end_line->next; l = l->next) {
		int col = (l == start_line) ? start_col : 0;
		int end = (l == end_line) ? end_col : l->width;
		while (col < end) {
			if (cell_color_equal(l->cells[col].style.fg, style->bg)) {
				CellStyle old = l->cells[col].style;
				if (!cell_color_equal(old.fg, old.bg)) {
					l->cells[col].style.fg = old.bg;
					l->cells[col].style.bg = old.fg;
				} else {
					l->cells[col].style.attr = style->attr;
				}
			} else {
				l->cells[col].style.bg = style->bg;
			}
			col++;
		}
	}
}

static void window_draw_cursor_matching(Win *win, Selection *cur, CellStyle *style) {
	if (win->vis->mode->visual)
		return;
	Line *line_match; int col_match;
	size_t pos = view_cursors_pos(cur);
	Filerange limits = view_viewport_get(win->view);
	size_t pos_match = text_bracket_match_symbol(win->file->text, pos, "(){}[]\"'`", &limits);
	if (pos == pos_match)
		return;
	if (!view_coord_get(win->view, pos_match, &line_match, NULL, &col_match))
		return;
	if (cell_color_equal(line_match->cells[col_match].style.fg, style->fg)) {
		CellStyle old = line_match->cells[col_match].style;
		line_match->cells[col_match].style.fg = old.bg;
		line_match->cells[col_match].style.bg = old.fg;
	} else {
		line_match->cells[col_match].style.bg = style->bg;
	}
}

static void window_draw_cursor(Win *win, Selection *cur, CellStyle *style, CellStyle *sel_style) {
	if (win->vis->win != win)
		return;
	Line *line = view_cursors_line_get(cur);
	int col = view_cursors_cell_get(cur);
	if (!line || col == -1)
		return;
	line->cells[col].style = *style;
	window_draw_cursor_matching(win, cur, sel_style);
	return;
}

static void window_draw_selections(Win *win) {
	View *view = win->view;
	Filerange viewport = view_viewport_get(view);
	Selection *sel = view_selections_primary_get(view);
	CellStyle style_cursor = win->ui->style_get(win->ui, UI_STYLE_CURSOR);
	CellStyle style_cursor_primary = win->ui->style_get(win->ui, UI_STYLE_CURSOR_PRIMARY);
	CellStyle style_selection = win->ui->style_get(win->ui, UI_STYLE_SELECTION);
	for (Selection *s = view_selections_prev(sel); s; s = view_selections_prev(s)) {
		window_draw_selection(win->view, s, &style_selection);
		size_t pos = view_cursors_pos(s);
		if (pos < viewport.start)
			break;
		window_draw_cursor(win, s, &style_cursor, &style_selection);
	}
	window_draw_selection(win->view, sel, &style_selection);
	window_draw_cursor(win, sel, &style_cursor_primary, &style_selection);
	for (Selection *s = view_selections_next(sel); s; s = view_selections_next(s)) {
		window_draw_selection(win->view, s, &style_selection);
		size_t pos = view_cursors_pos(s);
		if (pos > viewport.end)
			break;
		window_draw_cursor(win, s, &style_cursor, &style_selection);
	}
}

static void window_draw_eof(Win *win) {
	View *view = win->view;
	if (view_width_get(view) == 0)
		return;
	CellStyle style = win->ui->style_get(win->ui, UI_STYLE_EOF);
	for (Line *l = view_lines_last(view)->next; l; l = l->next) {
		strncpy(l->cells[0].data, view_symbol_eof_get(view), sizeof(l->cells[0].data)-1);
		l->cells[0].style = style;
	}
}

void vis_window_draw(Win *win) {
	if (!win->ui || !view_update(win->view))
		return;
	Vis *vis = win->vis;
	vis_event_emit(vis, VIS_EVENT_WIN_HIGHLIGHT, win);

	window_draw_colorcolumn(win);
	window_draw_cursorline(win);
	if (!vis->win || vis->win == win || vis->win->parent == win)
		window_draw_selections(win);
	window_draw_eof(win);

	vis_event_emit(vis, VIS_EVENT_WIN_STATUS, win);
}


void vis_window_invalidate(Win *win) {
	for (Win *w = win->vis->windows; w; w = w->next) {
		if (w->file == win->file)
			view_draw(w->view);
	}
}

Win *window_new_file(Vis *vis, File *file, enum UiOption options) {
	Win *win = calloc(1, sizeof(Win));
	if (!win)
		return NULL;
	win->vis = vis;
	win->file = file;
	win->view = view_new(file->text);
	win->ui = vis->ui->window_new(vis->ui, win, options);
	if (!win->view || !win->ui) {
		window_free(win);
		return NULL;
	}
	marklist_init(&win->jumplist, 32);
	mark_init(&win->saved_selections);
	file->refcount++;
	view_options_set(win->view, view_options_get(win->view));
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
	view_reload(win->view, file->text);
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

int vis_window_width_get(const Win *win) {
	return win->ui->window_width(win->ui);
}

int vis_window_height_get(const Win *win) {
	return win->ui->window_height(win->ui);
}

void vis_draw(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next)
		view_draw(win->view);
}

void vis_redraw(Vis *vis) {
	vis->ui->redraw(vis->ui);
	vis_update(vis);
}

void vis_update(Vis *vis) {
	vis->ui->draw(vis->ui);
}

void vis_suspend(Vis *vis) {
	vis->ui->suspend(vis->ui);
}

void vis_resume(Vis *vis) {
	vis->ui->resume(vis->ui);
}

bool vis_window_new(Vis *vis, const char *filename) {
	File *file = file_new(vis, filename);
	if (!file)
		return false;
	Win *win = window_new_file(vis, file, UI_OPTION_STATUSBAR|UI_OPTION_SYMBOL_EOF);
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
	vis->exit_status = -1;
	vis->ui = ui;
	vis->tabwidth = 8;
	vis->expandtab = false;
	vis->change_colors = true;
	for (size_t i = 0; i < LENGTH(vis->registers); i++)
		register_init(&vis->registers[i]);
	vis->registers[VIS_REG_BLACKHOLE].type = REGISTER_BLACKHOLE;
	vis->registers[VIS_REG_CLIPBOARD].type = REGISTER_CLIPBOARD;
	vis->registers[VIS_REG_PRIMARY].type = REGISTER_CLIPBOARD;
	vis->registers[VIS_REG_NUMBER].type = REGISTER_NUMBER;
	array_init(&vis->operators);
	array_init(&vis->motions);
	array_init(&vis->textobjects);
	array_init(&vis->bindings);
	array_init(&vis->actions_user);
	action_reset(&vis->action);
	buffer_init(&vis->input_queue);
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
	if (vis->options) {
		const char *name;
		while (map_first(vis->options, &name) && vis_option_unregister(vis, name));
	}
	map_free(vis->options);
	map_free(vis->actions);
	map_free(vis->keymap);
	buffer_release(&vis->input_queue);
	for (int i = 0; i < VIS_MODE_INVALID; i++)
		map_free(vis_modes[i].bindings);
	array_release_full(&vis->operators);
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
	Win *win = vis->win;
	if (!win)
		return;
	text_insert(win->file->text, pos, data, len);
	vis_window_invalidate(win);
}

void vis_insert_key(Vis *vis, const char *data, size_t len) {
	Win *win = vis->win;
	if (!win)
		return;
	for (Selection *s = view_selections(win->view); s; s = view_selections_next(s)) {
		size_t pos = view_cursors_pos(s);
		vis_insert(vis, pos, data, len);
		view_cursors_scroll_to(s, pos + len);
	}
}

void vis_replace(Vis *vis, size_t pos, const char *data, size_t len) {
	Win *win = vis->win;
	if (!win)
		return;
	Text *txt = win->file->text;
	Iterator it = text_iterator_get(txt, pos);
	int chars = text_char_count(data, len);
	for (char c; chars-- > 0 && text_iterator_byte_get(&it, &c) && c != '\n'; )
		text_iterator_char_next(&it, NULL);

	text_delete(txt, pos, it.pos - pos);
	vis_insert(vis, pos, data, len);
}

void vis_replace_key(Vis *vis, const char *data, size_t len) {
	Win *win = vis->win;
	if (!win)
		return;
	for (Selection *s = view_selections(win->view); s; s = view_selections_next(s)) {
		size_t pos = view_cursors_pos(s);
		vis_replace(vis, pos, data, len);
		view_cursors_scroll_to(s, pos + len);
	}
}

void vis_delete(Vis *vis, size_t pos, size_t len) {
	Win *win = vis->win;
	if (!win)
		return;
	text_delete(win->file->text, pos, len);
	vis_window_invalidate(win);
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

void vis_interrupt(Vis *vis) {
	vis->interrupted = true;
}

bool vis_interrupt_requested(Vis *vis) {
	return vis->interrupted;
}

void vis_do(Vis *vis) {
	Win *win = vis->win;
	if (!win)
		return;
	File *file = win->file;
	Text *txt = file->text;
	View *view = win->view;
	Action *a = &vis->action;

	int count = MAX(a->count, 1);
	if (a->op == &vis_operators[VIS_OP_MODESWITCH])
		count = 1; /* count should apply to inserted text not motion */
	bool repeatable = a->op && !vis->macro_operator && !vis->win->parent;
	bool multiple_cursors = view_selections_count(view) > 1;

	bool linewise = !(a->type & CHARWISE) && (
		a->type & LINEWISE || (a->movement && a->movement->type & LINEWISE) ||
		vis->mode == &vis_modes[VIS_MODE_VISUAL_LINE]);

	Register *reg = a->reg;
	size_t reg_slot = multiple_cursors ? EPOS : 0;
	size_t last_reg_slot = reg_slot;
	if (!reg)
		reg = &vis->registers[file->internal ? VIS_REG_PROMPT : VIS_REG_DEFAULT];
	if (a->op == &vis_operators[VIS_OP_PUT_AFTER] && multiple_cursors && vis_register_count(vis, reg) == 1)
		reg_slot = 0;

	if (vis->mode->visual && a->op)
		window_selection_save(win);

	for (Selection *sel = view_selections(view), *next; sel; sel = next) {
		if (vis->interrupted)
			break;

		next = view_selections_next(sel);

		size_t pos = view_cursors_pos(sel);
		if (pos == EPOS) {
			if (!view_selections_dispose(sel))
				view_cursors_to(sel, 0);
			continue;
		}

		OperatorContext c = {
			.count = count,
			.pos = pos,
			.newpos = EPOS,
			.range = text_range_empty(),
			.reg = reg,
			.reg_slot = reg_slot == EPOS ? (size_t)view_selections_number(sel) : reg_slot,
			.linewise = linewise,
			.arg = &a->arg,
			.context = a->op ? a->op->context : NULL,
		};

		last_reg_slot = c.reg_slot;

		bool err = false;
		if (a->movement) {
			size_t start = pos;
			for (int i = 0; i < count; i++) {
				size_t pos_prev = pos;
				if (a->movement->txt)
					pos = a->movement->txt(txt, pos);
				else if (a->movement->cur)
					pos = a->movement->cur(sel);
				else if (a->movement->file)
					pos = a->movement->file(vis, file, sel);
				else if (a->movement->vis)
					pos = a->movement->vis(vis, txt, pos);
				else if (a->movement->view)
					pos = a->movement->view(vis, view);
				else if (a->movement->win)
					pos = a->movement->win(vis, win, pos);
				else if (a->movement->user)
					pos = a->movement->user(vis, win, a->movement->data, pos);
				if (pos == EPOS || a->movement->type & IDEMPOTENT || pos == pos_prev) {
					err = a->movement->type & COUNT_EXACT;
					break;
				}
			}

			if (err) {
				repeatable = false;
				continue; // break?
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
					view_cursors_scroll_to(sel, pos);
				else
					view_cursors_to(sel, pos);
				if (vis->mode->visual)
					c.range = view_selections_get(sel);
			} else if (a->movement->type & INCLUSIVE && c.range.end > start) {
				c.range.end = text_char_next(txt, c.range.end);
			} else if (linewise && (a->movement->type & LINEWISE_INCLUSIVE)) {
				c.range.end = text_char_next(txt, c.range.end);
			}
		} else if (a->textobj) {
			if (vis->mode->visual)
				c.range = view_selections_get(sel);
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
				if (a->textobj->type & TEXTOBJECT_DELIMITED_OUTER) {
					r.start--;
					r.end++;
				}

				if (vis->mode->visual || (i > 0 && !(a->textobj->type & TEXTOBJECT_NON_CONTIGUOUS)))
					c.range = text_range_union(&c.range, &r);
				else
					c.range = r;

				if (i < count - 1) {
					if (a->textobj->type & TEXTOBJECT_EXTEND_BACKWARD) {
						pos = c.range.start;
						if ((a->textobj->type & TEXTOBJECT_DELIMITED_INNER) && pos > 0)
							pos--;
					} else {
						pos = c.range.end;
						if (a->textobj->type & TEXTOBJECT_DELIMITED_INNER)
							pos++;
					}
				}
			}
		} else if (vis->mode->visual) {
			c.range = view_selections_get(sel);
			if (!text_range_valid(&c.range))
				c.range.start = c.range.end = pos;
		}

		if (linewise && vis->mode != &vis_modes[VIS_MODE_VISUAL])
			c.range = text_range_linewise(txt, &c.range);
		if (vis->mode->visual) {
			view_selections_set(sel, &c.range);
			view_selections_anchor(sel, true);
		}

		if (a->op) {
			size_t pos = a->op->func(vis, txt, &c);
			if (pos == EPOS) {
				view_selections_dispose(sel);
			} else if (pos <= text_size(txt)) {
				view_selection_clear(sel);
				view_cursors_to(sel, pos);
			}
		}
	}

	view_selections_normalize(view);
	if (a->movement && (a->movement->type & JUMP))
		vis_jumplist_save(vis);

	if (a->op) {

		if (a->op == &vis_operators[VIS_OP_YANK] ||
		    a->op == &vis_operators[VIS_OP_DELETE] ||
		    a->op == &vis_operators[VIS_OP_CHANGE] ||
		    a->op == &vis_operators[VIS_OP_REPLACE]) {
			register_resize(reg, last_reg_slot+1);
		}

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
	if (strncmp(keys, "<vis-", 5) == 0) {
		const char *start = keys + 1, *end = start;
		while (*end && *end != '>')
			end++;
		if (end > start && end - start - 1 < VIS_KEY_LENGTH_MAX && *end == '>') {
			char key[VIS_KEY_LENGTH_MAX];
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

bool vis_keys_utf8(Vis *vis, const char *keys, char utf8[static UTFmax+1]) {
	Rune rune = vis_keys_codepoint(vis, keys);
	if (rune == (Rune)-1)
		return false;
	size_t len = runetochar(utf8, &rune);
	utf8[len] = '\0';
	return true;
}

typedef struct {
	Vis *vis;
	size_t len;         // length of the prefix
	int count;          // how many bindings can complete this prefix
	bool angle_bracket; // does the prefix end with '<'
} PrefixCompletion;

static bool isprefix(const char *key, void *value, void *data) {
	PrefixCompletion *completion = data;
	if (!completion->angle_bracket) {
		completion->count++;
	} else {
		const char *start = key + completion->len;
		const char *end = vis_keys_next(completion->vis, start);
		if (end && start + 1 == end)
			completion->count++;
	}
	return completion->count == 1;
}

static void vis_keys_process(Vis *vis, size_t pos) {
	Buffer *buf = &vis->input_queue;
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

				const Map *pmap = map_prefix(mode->bindings, start);
				PrefixCompletion completions = {
					.vis = vis,
					.len = cur - start,
					.count = 0,
					.angle_bracket = !strcmp(cur, "<"),
				};
				map_iterate(pmap, isprefix, &completions);

				prefix = (!match && completions.count > 0) ||
				         ( match && completions.count > 1);
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
	if (buffer_append0(&vis->input_queue, input))
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
	if (key.type == TERMKEY_TYPE_UNKNOWN_CSI) {
		long args[18];
		size_t nargs;
		unsigned long cmd;
		if (termkey_interpret_csi(termkey, &key, &args[2], &nargs, &cmd) == TERMKEY_RES_KEY) {
			args[0] = (long)cmd;
			args[1] = nargs;
			vis_event_emit(vis, VIS_EVENT_TERM_CSI, args);
		}
		return getkey(vis);
	}
	termkey_strfkey(termkey, vis->key, sizeof(vis->key), &key, TERMKEY_FORMAT_VIM);
	return vis->key;
}

bool vis_signal_handler(Vis *vis, int signum, const siginfo_t *siginfo, const void *context) {
	switch (signum) {
	case SIGBUS:
		for (File *file = vis->files; file; file = file->next) {
			if (text_mmaped(file->text, siginfo->si_addr))
				file->truncated = true;
		}
		vis->sigbus = true;
		if (vis->running)
			siglongjmp(vis->sigbus_jmpbuf, 1);
		return true;
	case SIGINT:
		vis->interrupted = true;
		return true;
	case SIGCONT:
		vis->resume = true;
		/* fall through */
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

int vis_run(Vis *vis) {
	if (!vis->windows)
		return EXIT_SUCCESS;
	if (vis->exit_status != -1)
		return vis->exit_status;
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
		if (vis->interrupted) {
			vis->interrupted = false;
			vis_keys_push(vis, "<C-c>", 0, true);
			continue;
		}

		if (vis->resume) {
			vis_resume(vis);
			vis->resume = false;
		}

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

Macro *macro_get(Vis *vis, enum VisRegister id) {
	if (id == VIS_MACRO_LAST_RECORDED)
		return vis->last_recording;
	if (VIS_REG_A <= id && id <= VIS_REG_Z)
		id -= VIS_REG_A;
	if (id < LENGTH(vis->registers))
		return array_get(&vis->registers[id].values, 0);
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
	size_t pos = buffer_length0(&vis->input_queue);
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
		return vis_motion(vis, VIS_MOVE_SEARCH_REPEAT_FORWARD);
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
	Win *win = vis->win;
	if (win)
		vis_file_snapshot(vis, win->file);
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
	vis_mode_switch(vis, VIS_MODE_OPERATOR_PENDING);
	vis_do(vis);
	if (macro) {
		Mode *mode = vis->mode;
		Action action_prev = vis->action_prev;
		if (count < 1 || action_prev.op == &vis_operators[VIS_OP_CHANGE])
			count = 1;
		if (vis->action_prev.op == &vis_operators[VIS_OP_MODESWITCH])
			vis->action_prev.count = 1;
		for (int i = 0; i < count; i++) {
			if (vis->interrupted)
				break;
			mode_set(vis, mode);
			macro_replay(vis, macro);
		}
		vis->action_prev = action_prev;
	}
	vis_cancel(vis);
	Win *win = vis->win;
	if (win)
		vis_file_snapshot(vis, win->file);
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

VisCountIterator vis_count_iterator_get(Vis *vis, int def) {
	return (VisCountIterator) {
		.vis = vis,
		.iteration = 0,
		.count = vis_count_get_default(vis, def),
	};
}

VisCountIterator vis_count_iterator_init(Vis *vis, int count) {
	return (VisCountIterator) {
		.vis = vis,
		.iteration = 0,
		.count = count,
	};
}

bool vis_count_iterator_next(VisCountIterator *it) {
	if (it->vis->interrupted)
		return false;
	return it->iteration++ < it->count;
}

void vis_exit(Vis *vis, int status) {
	vis->running = false;
	vis->exit_status = status;
}

void vis_insert_tab(Vis *vis) {
	Win *win = vis->win;
	if (!win)
		return;
	if (!vis->expandtab) {
		vis_insert_key(vis, "\t", 1);
		return;
	}
	char spaces[9];
	int tabwidth = MIN(vis->tabwidth, LENGTH(spaces) - 1);
	for (Selection *s = view_selections(win->view); s; s = view_selections_next(s)) {
		size_t pos = view_cursors_pos(s);
		int width = text_line_width_get(win->file->text, pos);
		int count = tabwidth - (width % tabwidth);
		for (int i = 0; i < count; i++)
			spaces[i] = ' ';
		spaces[count] = '\0';
		vis_insert(vis, pos, spaces, count);
		view_cursors_scroll_to(s, pos + count);
	}
}

size_t vis_text_insert_nl(Vis *vis, Text *txt, size_t pos) {
	size_t indent_len = 0;
	char byte, *indent = NULL;
	/* insert second newline at end of file, except if there is already one */
	bool eof = pos == text_size(txt);
	bool nl2 = eof && !(pos > 0 && text_byte_get(txt, pos-1, &byte) && byte == '\n');

	if (vis->autoindent) {
		/* copy leading white space of current line */
		size_t begin = text_line_begin(txt, pos);
		size_t start = text_line_start(txt, begin);
		size_t end = text_line_end(txt, start);
		if (start > pos)
			start = pos;
		indent_len = start >= begin ? start-begin : 0;
		if (start == end) {
			pos = begin;
		} else {
			indent = malloc(indent_len+1);
			if (indent)
				indent_len = text_bytes_get(txt, begin, indent_len, indent);
		}
	}

	text_insert(txt, pos, "\n", 1);
	if (eof) {
		if (nl2)
			text_insert(txt, text_size(txt), "\n", 1);
		else
			pos--; /* place cursor before, not after nl */
	}
	pos++;

	if (indent)
		text_insert(txt, pos, indent, indent_len);
	free(indent);
	return pos + indent_len;
}

void vis_insert_nl(Vis *vis) {
	Win *win = vis->win;
	if (!win)
		return;
	View *view = win->view;
	Text *txt = win->file->text;
	for (Selection *s = view_selections(view); s; s = view_selections_next(s)) {
		size_t pos = view_cursors_pos(s);
		size_t newpos = vis_text_insert_nl(vis, txt, pos);
		/* This is a bit of a hack to fix cursor positioning when
		 * inserting a new line at the start of the view port.
		 * It has the effect of reseting the mark used by the view
		 * code to keep track of the start of the visible region.
		 */
		view_cursors_to(s, pos);
		view_cursors_to(s, newpos);
	}
	vis_window_invalidate(win);
}

Regex *vis_regex(Vis *vis, const char *pattern) {
	if (!pattern && !(pattern = register_get(vis, &vis->registers[VIS_REG_SEARCH], NULL)))
		return NULL;
	Regex *regex = text_regex_new();
	if (!regex)
		return NULL;
	int cflags = REG_EXTENDED|REG_NEWLINE|(REG_ICASE*vis->ignorecase);
	if (text_regex_compile(regex, pattern, cflags) != 0) {
		text_regex_free(regex);
		return NULL;
	}
	register_put0(vis, &vis->registers[VIS_REG_SEARCH], pattern);
	return regex;
}

int vis_pipe(Vis *vis, File *file, Filerange *range, const char *argv[],
	void *stdout_context, ssize_t (*read_stdout)(void *stdout_context, char *data, size_t len),
	void *stderr_context, ssize_t (*read_stderr)(void *stderr_context, char *data, size_t len)) {

	/* if an invalid range was given, stdin (i.e. key board input) is passed
	 * through the external command. */
	Text *text = file->text;
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
		sigset_t sigterm_mask;
		sigemptyset(&sigterm_mask);
		sigaddset(&sigterm_mask, SIGTERM);
		if (sigprocmask(SIG_UNBLOCK, &sigterm_mask, NULL) == -1) {
			fprintf(stderr, "failed to reset signal mask");
			exit(EXIT_FAILURE);
		}

		int null = open("/dev/null", O_RDWR);
		if (null == -1) {
			fprintf(stderr, "failed to open /dev/null");
			exit(EXIT_FAILURE);
		}

		if (!interactive) {
			/* If we have nothing to write, let stdin point to
			 * /dev/null instead of a pipe which is immediately
			 * closed. Some programs behave differently when used
			 * in a pipeline.
			 */
			if (text_range_size(range) == 0)
				dup2(null, STDIN_FILENO);
			else
				dup2(pin[0], STDIN_FILENO);
		}

		close(pin[0]);
		close(pin[1]);
		if (interactive) {
			dup2(STDERR_FILENO, STDOUT_FILENO);
			/* For some reason the first byte written by the
			 * interactive application is not being displayed.
			 * It probably has something to do with the terminal
			 * state change. By writing a dummy byte ourself we
			 * ensure that the complete output is visible.
			 */
			while(write(STDOUT_FILENO, " ", 1) == -1 && errno == EINTR);
		} else if (read_stdout) {
			dup2(pout[1], STDOUT_FILENO);
		} else {
			dup2(null, STDOUT_FILENO);
		}
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

		if (file->name) {
			char *name = strrchr(file->name, '/');
			setenv("vis_filepath", file->name, 1);
			setenv("vis_filename", name ? name+1 : file->name, 1);
		}

		if (!argv[1])
			execlp(vis->shell, vis->shell, "-c", argv[0], (char*)NULL);
		else
			execvp(argv[0], (char* const*)argv);
		fprintf(stderr, "exec failure: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	vis->interrupted = false;

	close(pin[0]);
	close(pout[1]);
	close(perr[1]);

	if (fcntl(pout[0], F_SETFL, O_NONBLOCK) == -1 ||
	    fcntl(perr[0], F_SETFL, O_NONBLOCK) == -1)
		goto err;

	fd_set rfds, wfds;

	do {
		if (vis->interrupted) {
			kill(0, SIGTERM);
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

	for (;;) {
		if (vis->interrupted)
			kill(0, SIGTERM);
		pid_t died = waitpid(pid, &status, 0);
		if ((died == -1 && errno == ECHILD) || pid == died)
			break;
	}

	/* clear any pending SIGTERM */
	struct sigaction sigterm_ignore, sigterm_old;
	sigterm_ignore.sa_handler = SIG_IGN;
	sigterm_ignore.sa_flags = 0;
	sigemptyset(&sigterm_ignore.sa_mask);

	sigaction(SIGTERM, &sigterm_ignore, &sigterm_old);
	sigaction(SIGTERM, &sigterm_old, NULL);

	vis->interrupted = false;
	vis->ui->terminal_restore(vis->ui);

	return status;
}

static ssize_t read_buffer(void *context, char *data, size_t len) {
	buffer_append(context, data, len);
	return len;
}

int vis_pipe_collect(Vis *vis, File *file, Filerange *range, const char *argv[], char **out, char **err) {
	Buffer bufout, buferr;
	buffer_init(&bufout);
	buffer_init(&buferr);
	int status = vis_pipe(vis, file, range, argv,
	                      &bufout, out ? read_buffer : NULL,
	                      &buferr, err ? read_buffer : NULL);
	buffer_terminate(&bufout);
	buffer_terminate(&buferr);
	if (out)
		*out = buffer_move(&bufout);
	if (err)
		*err = buffer_move(&buferr);
	buffer_release(&bufout);
	buffer_release(&buferr);
	return status;
}

bool vis_cmd(Vis *vis, const char *cmdline) {
	if (!cmdline)
		return true;
	while (*cmdline == ':')
		cmdline++;
	char *line = strdup(cmdline);
	if (!line)
		return false;

	size_t len = strlen(line);
	while (len > 0 && isspace((unsigned char)line[len-1]))
		len--;
	line[len] = '\0';

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
	Win *win = vis->win;
	return win ? win->file->text : NULL;
}

View *vis_view(Vis *vis) {
	Win *win = vis->win;
	return win ? win->view : NULL;
}

Win *vis_window(Vis *vis) {
	return vis->win;
}

bool vis_get_autoindent(const Vis *vis) {
	return vis->autoindent;
}
