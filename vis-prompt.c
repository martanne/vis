#include "vis-core.h"

bool vis_prompt_cmd(Vis *vis, const char *cmd) {
	if (!cmd || !cmd[0] || !cmd[1])
		return true;
	switch (cmd[0]) {
	case '/':
		return vis_motion(vis, VIS_MOVE_SEARCH_FORWARD, cmd+1);
	case '?':
		return vis_motion(vis, VIS_MOVE_SEARCH_BACKWARD, cmd+1);
	case '+':
	case ':':
		register_put0(vis, &vis->registers[VIS_REG_COMMAND], cmd+1);
		return vis_cmd(vis, cmd+1);
	default:
		return false;
	}
}

static void
vis_prompt_remove_empty_line(Text *text)
{
	Filerange line_range = text_object_line(text, text->size - 1);
	char *line = text_bytes_alloc0(text, line_range.start, text_range_size(line_range));
	if (line && (line[0] == '\n' ||
	    ((line[0] == ':' || line[0] == '/' || line[0] == '?') && (line[1] == '\n' || line[1] == '\0'))))
	{
		text_delete_range(text, line_range);
	}
	free(line);
}

static void
vis_prompt_hide(Win *win)
{
	Text *txt = win->file->text;
	size_t size = txt->size;
	/* make sure that file is new line terminated */
	char lastchar = 0;
	if (size >= 1 && text_byte_get(txt, size - 1, &lastchar) && lastchar != '\n')
		text_insert(win->vis, txt, size, "\n", 1);
	vis_prompt_remove_empty_line(txt);
	win->vis->prompt_state = PROMPTSTATE_NONE;
	vis_window_close(win);
}

static void prompt_restore(Win *win) {
	Vis *vis = win->vis;
	/* restore window and mode which was active before the prompt window
	 * we deliberately don't use vis_mode_switch because we do not want
	 * to invoke the modes enter/leave functions */
	if (win->parent)
		vis->win = win->parent;
	vis->mode = win->parent_mode;
}

static const char *
vis_prompt_enter(Vis *vis, const char *keys, const Arg *arg)
{
	Win  *prompt = vis->win;
	Text *txt    = prompt->file->text;

	Filerange range = view_selections_get(prompt->view.selection);
	if (!vis->mode->visual)
		range = text_object_line(txt, view_cursor_get(&prompt->view));

	char *cmd = 0;
	if (text_range_valid(range))
		cmd = text_bytes_alloc0(txt, range.start, text_range_size(range));

	if (!cmd) {
		vis_info_show(vis, "Failed to detect command");
		prompt_restore(prompt);
		vis_prompt_hide(prompt);
		free(cmd);
		return keys;
	}

	size_t len = strlen(cmd);
	if (len > 0 && cmd[len - 1] == '\n')
		cmd[len - 1] = 0;

	// TODO(rnp): cleanup: this is dumb and requires the range to contain the newline
	bool lastline = range.end == txt->size;

	prompt_restore(prompt);
	vis->prompt_state = PROMPTSTATE_COMMAND;
	vis_redraw(vis);
	if (vis_prompt_cmd(vis, cmd)) {
		vis_prompt_hide(prompt);
		/* hide cursor in case it was made visible */
		// TODO(rnp): cleanup: this looks like a hack
		fprintf(stderr, "\x1b[?25l");
		if (!lastline) {
			text_delete(txt, range.start, text_range_size(range));
			text_appendf(vis, txt, "%s\n", cmd);
		}
	} else {
		vis->win  = prompt;
		vis->mode = vis_modes + VIS_MODE_INSERT;
		vis->prompt_state = PROMPTSTATE_ONELINE;
	}
	free(cmd);
	return keys;
}

static const char *prompt_esc(Vis *vis, const char *keys, const Arg *arg) {
	Win *prompt = vis->win;
	if (prompt->view.selection_count > 1) {
		view_selections_dispose_all(&prompt->view);
	} else {
		prompt_restore(prompt);
		vis_prompt_hide(prompt);
	}
	return keys;
}

static const char *
vis_prompt_up(Vis *vis, const char *keys, const Arg *arg)
{
	vis_motion(vis, VIS_MOVE_LINE_UP);
	vis_window_mode_unmap(vis->win, VIS_MODE_INSERT, "<Up>");
	win_options_set(vis->win, UI_OPTION_SYMBOL_EOF);
	view_slide_down(&vis->win->view, vis->win->view.height - 1);
	return keys;
}

static const KeyBinding prompt_enter_binding = {
	.key = "<Enter>",
	.action = &(KeyAction){
		.func = vis_prompt_enter,
	},
};

static const KeyBinding prompt_esc_binding = {
	.key = "<Escape>",
	.action = &(KeyAction){
		.func = prompt_esc,
	},
};

static const KeyBinding prompt_up_binding = {
	.key = "<Up>",
	.action = &(KeyAction){
		.func = vis_prompt_up,
	},
};

static const KeyBinding prompt_tab_binding = {
	.key = "<Tab>",
	.alias = "<C-x><C-o>",
};

VIS_EXPORT void
vis_prompt_show(Vis *vis, const char *title)
{
	Win *active_window = vis->win;
	Win *prompt        = active_window;

	if (vis->prompt_state != PROMPTSTATE_ONELINE)
		prompt = window_new_file(vis, vis->prompt_file, UI_OPTION_ONELINE);

	if (prompt && prompt != active_window) {
		prompt->parent      = active_window;
		prompt->parent_mode = vis->mode;
		vis_window_mode_map(prompt, VIS_MODE_NORMAL, true, "<Enter>",  &prompt_enter_binding);
		vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<Enter>",  &prompt_enter_binding);
		vis_window_mode_map(prompt, VIS_MODE_VISUAL, true, "<Enter>",  &prompt_enter_binding);
		vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<C-j>",    &prompt_enter_binding);
		vis_window_mode_map(prompt, VIS_MODE_NORMAL, true, "<Escape>", &prompt_esc_binding);
		vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<Up>",     &prompt_up_binding);
		if (CONFIG_LUA)
			vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<Tab>", &prompt_tab_binding);
	}

	if (prompt) {
		Text *txt = prompt->file->text;
		vis_prompt_remove_empty_line(txt);
		text_appendf(vis, txt, "%s\n", title);
		view_cursors_scroll_to(view_selections_primary_get(&prompt->view), txt->size - 1);
		vis_mode_switch(vis, VIS_MODE_INSERT);
		vis->prompt_state = PROMPTSTATE_ONELINE;
		vis_window_focus(vis, prompt);
	}
}

void vis_info_show(Vis *vis, const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	ui_info_show(&vis->ui, msg, ap);
	va_end(ap);
}

void vis_message_show(Vis *vis, const char *msg) {
	if (!msg)
		return;
	if (!vis->message_window)
		vis->message_window = window_new_file(vis, vis->error_file, UI_OPTION_STATUSBAR);
	Win *win = vis->message_window;
	if (!win)
		return;
	Text *txt = win->file->text;
	size_t pos = text_size(txt);
	text_appendf(vis, txt, "%s\n", msg);
	text_mark_current_revision(txt);
	view_cursors_to(win->view.selection, pos);
	vis_window_focus(vis, win);
}
