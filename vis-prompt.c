#include <string.h>
#include "vis-core.h"
#include "text-objects.h"
#include "text-util.h"

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
	{
		register_put0(vis, &vis->registers[VIS_REG_COMMAND], cmd+1);
		bool ret = vis_cmd(vis, cmd+1);
		if (ret && vis->mode->visual)
			vis_mode_switch(vis, VIS_MODE_NORMAL);
		return ret;
	}
	default:
		return false;
	}
}

static void prompt_hide(Win *win) {
	Text *txt = win->file->text;
	size_t size = text_size(txt);
	/* make sure that file is new line terminated */
	char lastchar;
	if (size > 1 && text_byte_get(txt, size-1, &lastchar) && lastchar != '\n')
		text_insert(txt, size, "\n", 1);
	/* remove empty entries */
	Filerange line = text_object_line(txt, size);
	size_t line_size = text_range_size(&line);
	if (line_size <= 2)
		text_delete(txt, line.start, line_size);
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

static const char *prompt_enter(Vis *vis, const char *keys, const Arg *arg) {
	Win *prompt = vis->win;
	View *view = prompt->view;
	Text *txt = prompt->file->text;
	Win *win = prompt->parent;
	char *cmd = NULL;

	Filerange range = view_selection_get(view);
	if (!text_range_valid(&range))
		range = text_object_line(txt, view_cursor_get(view));
	if (text_range_valid(&range))
		cmd = text_bytes_alloc0(txt, range.start, text_range_size(&range));

	if (!win || !cmd) {
		vis_info_show(vis, "Prompt window invalid\n");
		prompt_restore(prompt);
		prompt_hide(prompt);
		free(cmd);
		return keys;
	}

	size_t len = strlen(cmd);
	if (len > 0 && cmd[len-1] == '\n')
		cmd[len-1] = '\0';

	bool lastline = (range.end == text_size(txt));

	prompt_restore(prompt);
	if (vis_prompt_cmd(vis, cmd)) {
		prompt_hide(prompt);
		if (!lastline) {
			text_delete(txt, range.start, text_range_size(&range));
			text_appendf(txt, "%s\n", cmd);
		}
	} else {
		vis->win = prompt;
		vis->mode = &vis_modes[VIS_MODE_INSERT];
	}
	free(cmd);
	vis_draw(vis);
	return keys;
}

static const char *prompt_esc(Vis *vis, const char *keys, const Arg *arg) {
	Win *prompt = vis->win;
	if (view_cursors_count(prompt->view) > 1) {
		view_cursors_clear(prompt->view);
	} else {
		prompt_restore(prompt);
		prompt_hide(prompt);
	}
	return keys;
}

static const char *prompt_up(Vis *vis, const char *keys, const Arg *arg) {
	vis_motion(vis, VIS_MOVE_LINE_UP);
	vis_window_mode_unmap(vis->win, VIS_MODE_INSERT, "<Up>");
	view_options_set(vis->win->view, UI_OPTION_NONE);
	return keys;
}

static const char *prompt_backspace(Vis *vis, const char *keys, const Arg *arg) {
	Win *prompt = vis->win;
	Text *txt = prompt->file->text;
	size_t size = text_size(txt);
	size_t pos = view_cursor_get(prompt->view);
	char c;
	if (pos == size && (pos == 1 || (size >= 2 && text_byte_get(txt, size-2, &c) && c == '\n'))) {
		prompt_restore(prompt);
		prompt_hide(prompt);
	} else {
		vis_operator(vis, VIS_OP_DELETE);
		vis_motion(vis, VIS_MOVE_CHAR_PREV);
	}
	return keys;
}

static const KeyBinding prompt_enter_binding = {
	.key = "<Enter>",
	.action = &(KeyAction){
		.func = prompt_enter,
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
		.func = prompt_up,
	},
};

static const KeyBinding prompt_backspace_binding = {
	.key = "<Enter>",
	.action = &(KeyAction){
		.func = prompt_backspace,
	},
};

void vis_prompt_show(Vis *vis, const char *title) {
	Win *active = vis->win;
	Win *prompt = window_new_file(vis, title[0] == ':' ? vis->command_file : vis->search_file);
	if (!prompt)
		return;
	if (vis->mode->visual)
		window_selection_save(active);
	view_options_set(prompt->view, UI_OPTION_ONELINE);
	Text *txt = prompt->file->text;
	text_insert(txt, text_size(txt), title, strlen(title));
	Cursor *cursor = view_cursors_primary_get(prompt->view);
	view_cursors_scroll_to(cursor, text_size(txt));
	view_draw(prompt->view);
	prompt->parent = active;
	prompt->parent_mode = vis->mode;
	vis_window_mode_map(prompt, VIS_MODE_NORMAL, "<Enter>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_INSERT, "<Enter>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_VISUAL, "<Enter>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_NORMAL, "<Escape>", &prompt_esc_binding);
	vis_window_mode_map(prompt, VIS_MODE_INSERT, "<Up>", &prompt_up_binding);
	vis_window_mode_map(prompt, VIS_MODE_INSERT, "<Backspace>", &prompt_backspace_binding);
	vis_mode_switch(vis, VIS_MODE_INSERT);
	vis_draw(vis);
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

void vis_message_show(Vis *vis, const char *msg) {
	if (!msg)
		return;
	if (!vis->message_window) {
		if (!vis_window_new(vis, NULL))
			return;
		vis->message_window = vis->win;
	}

	Win *win = vis->message_window;
	Text *txt = win->file->text;
	size_t pos = text_size(txt);
	text_appendf(txt, "%s\n", msg);
	text_save(txt, NULL);
	view_cursor_to(win->view, pos);
}

void vis_message_hide(Vis *vis) {
	if (!vis->message_window)
		return;
	vis_window_close(vis->message_window);
	vis->message_window = NULL;
}
