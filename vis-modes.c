#include <string.h>
#include <strings.h>
#include "vis-core.h"
#include "text-motions.h"
#include "util.h"

static void keyaction_free(KeyAction *action) {
	if (!action)
		return;
	free((char*)action->name);
	free(VIS_HELP_USE((char*)action->help));
	free(action);
}

KeyAction *vis_action_new(Vis *vis, const char *name, const char *help, KeyActionFunction *func, Arg arg) {
	KeyAction *action = calloc(1, sizeof *action);
	if (!action)
		return NULL;
	if (name && !(action->name = strdup(name)))
		goto err;
#if CONFIG_HELP
	if (help && !(action->help = strdup(help)))
		goto err;
#endif
	action->func = func;
	action->arg = arg;
	if (!array_add_ptr(&vis->actions_user, action))
		goto err;
	return action;
err:
	keyaction_free(action);
	return NULL;
}

void vis_action_free(Vis *vis, KeyAction *action) {
	if (!action)
		return;
	size_t len = array_length(&vis->actions_user);
	for (size_t i = 0; i < len; i++) {
		if (action == array_get_ptr(&vis->actions_user, i)) {
			keyaction_free(action);
			array_remove(&vis->actions_user, i);
			return;
		}
	}
}

KeyBinding *vis_binding_new(Vis *vis) {
	KeyBinding *binding = calloc(1, sizeof *binding);
	if (binding && array_add_ptr(&vis->bindings, binding))
		return binding;
	free(binding);
	return NULL;
}

void vis_binding_free(Vis *vis, KeyBinding *binding) {
	if (!binding)
		return;
	size_t len = array_length(&vis->bindings);
	for (size_t i = 0; i < len; i++) {
		if (binding == array_get_ptr(&vis->bindings, i)) {
			if (binding->alias)
				free((char*)binding->alias);
			if (binding->action && !binding->action->name)
				vis_action_free(vis, (KeyAction*)binding->action);
			free(binding);
			array_remove(&vis->bindings, i);
			return;
		}
	}
}

Mode *mode_get(Vis *vis, enum VisMode mode) {
	if (mode < LENGTH(vis_modes))
		return &vis_modes[mode];
	return NULL;
}

void mode_set(Vis *vis, Mode *new_mode) {
	if (vis->mode == new_mode)
		return;
	if (vis->mode->leave)
		vis->mode->leave(vis, new_mode);
	if (vis->mode != &vis_modes[VIS_MODE_OPERATOR_PENDING])
		vis->mode_prev = vis->mode;
	vis->mode = new_mode;
	if (new_mode->enter)
		new_mode->enter(vis, vis->mode_prev);
}

void vis_mode_switch(Vis *vis, enum VisMode mode) {
	if (mode < LENGTH(vis_modes))
		mode_set(vis, &vis_modes[mode]);
}

enum VisMode vis_mode_from(Vis *vis, const char *name) {
	for (size_t i = 0; name && i < LENGTH(vis_modes); i++) {
		Mode *mode = &vis_modes[i];
		if (!strcasecmp(mode->name, name))
			return mode->id;
	}
	return VIS_MODE_INVALID;
}

enum VisMode vis_mode_get(Vis *vis) {
	return vis->mode->id;
}

static bool mode_unmap(Mode *mode, const char *key) {
	return mode && mode->bindings && map_delete(mode->bindings, key);
}

bool vis_mode_unmap(Vis *vis, enum VisMode id, const char *key) {
	return id < LENGTH(vis_modes) && mode_unmap(&vis_modes[id], key);
}

bool vis_window_mode_unmap(Win *win, enum VisMode id, const char *key) {
	return id < LENGTH(win->modes) && mode_unmap(&win->modes[id], key);
}

static bool mode_map(Vis *vis, Mode *mode, bool force, const char *key, const KeyBinding *binding) {
	if (!mode)
		return false;
	if (binding->alias && key[0] != '<' && strncmp(key, binding->alias, strlen(key)) == 0)
		return false;
	if (!mode->bindings && !(mode->bindings = map_new()))
		return false;
	if (force)
		map_delete(mode->bindings, key);
	return map_put(mode->bindings, key, binding);
}

bool vis_mode_map(Vis *vis, enum VisMode id, bool force, const char *key, const KeyBinding *binding) {
	return id < LENGTH(vis_modes) && mode_map(vis, &vis_modes[id], force, key, binding);
}

bool vis_window_mode_map(Win *win, enum VisMode id, bool force, const char *key, const KeyBinding *binding) {
	return id < LENGTH(win->modes) && mode_map(win->vis, &win->modes[id], force, key, binding);
}

/** mode switching event handlers */

static void vis_mode_normal_enter(Vis *vis, Mode *old) {
	Win *win = vis->win;
	if (!win)
		return;
	if (old != mode_get(vis, VIS_MODE_INSERT) && old != mode_get(vis, VIS_MODE_REPLACE))
		return;
	if (vis->autoindent && strcmp(vis->key_prev, "<Enter>") == 0) {
		Text *txt = win->file->text;
		for (Selection *s = view_selections(&win->view); s; s = view_selections_next(s)) {
			size_t pos = view_cursors_pos(s);
			size_t start = text_line_start(txt, pos);
			size_t end = text_line_end(txt, pos);
			if (start == pos && start == end) {
				size_t begin = text_line_begin(txt, pos);
				size_t len = start - begin;
				if (len) {
					text_delete(txt, begin, len);
					view_cursors_to(s, pos-len);
				}
			}
		}
	}
	macro_operator_stop(vis);
	if (!win->parent && vis->action_prev.op == &vis_operators[VIS_OP_MODESWITCH] &&
	    vis->action_prev.count > 1) {
		/* temporarily disable motion, in something like `5atext`
		 * we should only move the cursor once then insert the text */
		const Movement *motion = vis->action_prev.movement;
		if (motion)
			vis->action_prev.movement = &vis_motions[VIS_MOVE_NOP];
		/* we already inserted the text once, so temporarily decrease count */
		vis->action_prev.count--;
		vis_repeat(vis);
		vis->action_prev.count++;
		vis->action_prev.movement = motion;
	}
	/* make sure we can recover the current state after an editing operation */
	vis_file_snapshot(vis, win->file);
}

static void vis_mode_operator_input(Vis *vis, const char *str, size_t len) {
	/* invalid operator */
	vis_cancel(vis);
	mode_set(vis, vis->mode_prev);
}

static void vis_mode_visual_enter(Vis *vis, Mode *old) {
	Win *win = vis->win;
	if (!old->visual && win) {
		for (Selection *s = view_selections(&win->view); s; s = view_selections_next(s))
			s->anchored = true;
	}
}

static void vis_mode_visual_line_enter(Vis *vis, Mode *old) {
	Win *win = vis->win;
	if (!old->visual && win) {
		for (Selection *s = view_selections(&win->view); s; s = view_selections_next(s))
			s->anchored = true;
	}
	if (!vis->action.op)
		vis_motion(vis, VIS_MOVE_NOP);
}

static void vis_mode_visual_line_leave(Vis *vis, Mode *new) {
	Win *win = vis->win;
	if (!win)
		return;
	if (!new->visual) {
		if (!vis->action.op)
			window_selection_save(win);
		view_selections_clear_all(&win->view);
	} else {
		view_cursors_to(win->view.selection, view_cursor_get(&win->view));
	}
}

static void vis_mode_visual_leave(Vis *vis, Mode *new) {
	Win *win = vis->win;
	if (!new->visual && win) {
		if (!vis->action.op)
			window_selection_save(win);
		view_selections_clear_all(&win->view);
	}
}

static void vis_mode_insert_replace_enter(Vis *vis, Mode *old) {
	if (!vis->win || vis->win->parent)
		return;
	if (!vis->action.op) {
		action_reset(&vis->action_prev);
		vis->action_prev.op = &vis_operators[VIS_OP_MODESWITCH];
		vis->action_prev.mode = vis->mode->id;
	}
	macro_operator_record(vis);
}

static void vis_mode_insert_idle(Vis *vis) {
	Win *win = vis->win;
	if (win)
		vis_file_snapshot(vis, win->file);
}

static void vis_mode_insert_input(Vis *vis, const char *str, size_t len) {
	vis_insert_key(vis, str, len);
}

static void vis_mode_replace_input(Vis *vis, const char *str, size_t len) {
	vis_replace_key(vis, str, len);
}

Mode vis_modes[] = {
	[VIS_MODE_OPERATOR_PENDING] = {
		.id = VIS_MODE_OPERATOR_PENDING,
		.name = "OPERATOR-PENDING",
		.input = vis_mode_operator_input,
		.help = "",
	},
	[VIS_MODE_NORMAL] = {
		.id = VIS_MODE_NORMAL,
		.name = "NORMAL",
		.help = "",
		.enter = vis_mode_normal_enter,
	},
	[VIS_MODE_VISUAL] = {
		.id = VIS_MODE_VISUAL,
		.name = "VISUAL",
		.status = "VISUAL",
		.help = "",
		.enter = vis_mode_visual_enter,
		.leave = vis_mode_visual_leave,
		.visual = true,
	},
	[VIS_MODE_VISUAL_LINE] = {
		.id = VIS_MODE_VISUAL_LINE,
		.name = "VISUAL-LINE",
		.parent = &vis_modes[VIS_MODE_VISUAL],
		.status = "VISUAL-LINE",
		.help = "",
		.enter = vis_mode_visual_line_enter,
		.leave = vis_mode_visual_line_leave,
		.visual = true,
	},
	[VIS_MODE_INSERT] = {
		.id = VIS_MODE_INSERT,
		.name = "INSERT",
		.status = "INSERT",
		.help = "",
		.enter = vis_mode_insert_replace_enter,
		.input = vis_mode_insert_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
	[VIS_MODE_REPLACE] = {
		.id = VIS_MODE_REPLACE,
		.name = "REPLACE",
		.parent = &vis_modes[VIS_MODE_INSERT],
		.status = "REPLACE",
		.help = "",
		.enter = vis_mode_insert_replace_enter,
		.input = vis_mode_replace_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
};

