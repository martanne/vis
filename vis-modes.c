#include "vis-core.h"
#include "util.h"

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
	if (vis->mode->isuser)
		vis->mode_prev = vis->mode;
	vis->mode = new_mode;
	if (new_mode->enter)
		new_mode->enter(vis, vis->mode_prev);
	vis->win->ui->draw_status(vis->win->ui);
}

static bool mode_map(Mode *mode, const char *name, KeyBinding *binding) {
	return map_put(mode->bindings, name, binding);
}

bool vis_mode_map(Vis *vis, enum VisMode modeid, const char *name, KeyBinding *binding) {
	Mode *mode = mode_get(vis, modeid);
	return mode && map_put(mode->bindings, name, binding);
}

bool vis_mode_bindings(Vis *vis, enum VisMode modeid, KeyBinding **bindings) {
	Mode *mode = mode_get(vis, modeid);
	if (!mode)
		return false;
	bool success = true;
	for (KeyBinding *kb = *bindings; kb->key; kb++) {
		if (!mode_map(mode, kb->key, kb))
			success = false;
	}
	return success;
}

bool vis_mode_unmap(Vis *vis, enum VisMode modeid, const char *name) {
	Mode *mode = mode_get(vis, modeid);
	return mode && map_delete(mode->bindings, name);
}

/** mode switching event handlers */

static void vis_mode_operator_enter(Vis *vis, Mode *old) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_OPERATOR_OPTION];
}

static void vis_mode_operator_leave(Vis *vis, Mode *new) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
}

static void vis_mode_operator_input(Vis *vis, const char *str, size_t len) {
	/* invalid operator */
	action_reset(vis, &vis->action);
	mode_set(vis, vis->mode_prev);
}

static void vis_mode_visual_enter(Vis *vis, Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
}

static void vis_mode_visual_line_enter(Vis *vis, Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
	vis_motion(vis, MOVE_LINE_END);
}

static void vis_mode_visual_line_leave(Vis *vis, Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	} else {
		view_cursor_to(vis->win->view, view_cursor_get(vis->win->view));
	}
}

static void vis_mode_visual_leave(Vis *vis, Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	}
}

static void vis_mode_prompt_input(Vis *vis, const char *str, size_t len) {
	vis_insert_key(vis, str, len);
}

static void vis_mode_prompt_enter(Vis *vis, Mode *old) {
	if (old->isuser && old != &vis_modes[VIS_MODE_PROMPT])
		vis->mode_before_prompt = old;
}

static void vis_mode_prompt_leave(Vis *vis, Mode *new) {
	if (new->isuser)
		vis_prompt_hide(vis);
}

static void vis_mode_insert_enter(Vis *vis, Mode *old) {
	if (!vis->macro_operator) {
		macro_operator_record(vis);
		action_reset(vis, &vis->action_prev);
		vis->action_prev.macro = vis->macro_operator;
		vis->action_prev.op = &ops[OP_INSERT];
	}
}

static void vis_mode_insert_leave(Vis *vis, Mode *new) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
	if (new == mode_get(vis, VIS_MODE_NORMAL))
		macro_operator_stop(vis);
}

static void vis_mode_insert_idle(Vis *vis) {
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_input(Vis *vis, const char *str, size_t len) {
	vis_insert_key(vis, str, len);
}

static void vis_mode_replace_enter(Vis *vis, Mode *old) {
	if (!vis->macro_operator) {
		macro_operator_record(vis);
		action_reset(vis, &vis->action_prev);
		vis->action_prev.macro = vis->macro_operator;
		vis->action_prev.op = &ops[OP_REPLACE];
	}
}

static void vis_mode_replace_leave(Vis *vis, Mode *new) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
	if (new == mode_get(vis, VIS_MODE_NORMAL))
		macro_operator_stop(vis);
}

static void vis_mode_replace_input(Vis *vis, const char *str, size_t len) {
	vis_replace_key(vis, str, len);
}



/*
 * the tree of modes currently looks like this. the double line between OPERATOR-OPTION
 * and OPERATOR is only in effect once an operator is detected. that is when entering the
 * OPERATOR mode its parent is set to OPERATOR-OPTION which makes {INNER-,}TEXTOBJ
 * reachable. once the operator is processed (i.e. the OPERATOR mode is left) its parent
 * mode is reset back to MOVE.
 *
 * Similarly the +-ed line between OPERATOR and TEXTOBJ is only active within the visual
 * modes.
 *
 *
 *                                         BASIC
 *                                    (arrow keys etc.)
 *                                    /      |
 *               /-------------------/       |
 *            READLINE                      MOVE
 *            /       \                 (h,j,k,l ...)
 *           /         \                     |       \-----------------\
 *          /           \                    |                         |
 *       INSERT       PROMPT             OPERATOR ++++           INNER-TEXTOBJ
 *          |      (history etc)       (d,c,y,p ..)   +      (i [wsp[]()b<>{}B"'`] )
 *          |                                |     \\  +               |
 *          |                                |      \\  +              |
 *       REPLACE                           NORMAL    \\  +          TEXTOBJ
 *                                           |        \\  +     (a [wsp[]()b<>{}B"'`] )
 *                                           |         \\  +   +       |
 *                                           |          \\  + +        |
 *                                         VISUAL        \\     OPERATOR-OPTION
 *                                           |            \\        (v,V)
 *                                           |             \\        //
 *                                           |              \\======//
 *                                      VISUAL-LINE
 */

Mode vis_modes[] = {
	[VIS_MODE_BASIC] = {
		.name = "BASIC",
		.parent = NULL,
	},
	[VIS_MODE_MOVE] = {
		.name = "MOVE",
		.parent = &vis_modes[VIS_MODE_BASIC],
	},
	[VIS_MODE_TEXTOBJ] = {
		.name = "TEXT-OBJECTS",
		.parent = &vis_modes[VIS_MODE_MOVE],
	},
	[VIS_MODE_OPERATOR_OPTION] = {
		.name = "OPERATOR-OPTION",
		.parent = &vis_modes[VIS_MODE_TEXTOBJ],
	},
	[VIS_MODE_OPERATOR] = {
		.name = "OPERATOR",
		.parent = &vis_modes[VIS_MODE_MOVE],
		.enter = vis_mode_operator_enter,
		.leave = vis_mode_operator_leave,
		.input = vis_mode_operator_input,
	},
	[VIS_MODE_NORMAL] = {
		.name = "NORMAL",
		.status = "",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
	},
	[VIS_MODE_VISUAL] = {
		.name = "VISUAL",
		.status = "--VISUAL--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.enter = vis_mode_visual_enter,
		.leave = vis_mode_visual_leave,
		.visual = true,
	},
	[VIS_MODE_VISUAL_LINE] = {
		.name = "VISUAL LINE",
		.status = "--VISUAL LINE--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_VISUAL],
		.enter = vis_mode_visual_line_enter,
		.leave = vis_mode_visual_line_leave,
		.visual = true,
	},
	[VIS_MODE_READLINE] = {
		.name = "READLINE",
		.parent = &vis_modes[VIS_MODE_BASIC],
	},
	[VIS_MODE_PROMPT] = {
		.name = "PROMPT",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.input = vis_mode_prompt_input,
		.enter = vis_mode_prompt_enter,
		.leave = vis_mode_prompt_leave,
	},
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.status = "--INSERT--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.enter = vis_mode_insert_enter,
		.leave = vis_mode_insert_leave,
		.input = vis_mode_insert_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.status = "--REPLACE--",
		.help = "",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_INSERT],
		.enter = vis_mode_replace_enter,
		.leave = vis_mode_replace_leave,
		.input = vis_mode_replace_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
};

