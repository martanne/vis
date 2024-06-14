#include <string.h>
#include "vis-core.h"
#include "text-motions.h"
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
		register_put0(vis, &vis->registers[VIS_REG_COMMAND], cmd+1);
		return vis_cmd(vis, cmd+1);
	default:
		return false;
	}
}

static void prompt_hide(Win *win) {
	Text *txt = win->file->text;
	size_t size = text_size(txt);
	/* make sure that file is new line terminated */
	char lastchar = '\0';
	if (size >= 1 && text_byte_get(txt, size-1, &lastchar) && lastchar != '\n')
		text_insert(txt, size, "\n", 1);
	/* remove empty entries */
	Filerange line_range = text_object_line(txt, text_size(txt)-1);
	char *line = text_bytes_alloc0(txt, line_range.start, text_range_size(&line_range));
	if (line && (line[0] == '\n' || (strchr(":/?", line[0]) && (line[1] == '\n' || line[1] == '\0'))))
		text_delete_range(txt, &line_range);
	free(line);
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
	View *view = &prompt->view;
	Text *txt = prompt->file->text;
	Win *win = prompt->parent;
	char *cmd = NULL;

	Filerange range = view_selections_get(view->selection);
	if (!vis->mode->visual) {
		const char *pattern = NULL;
		Regex *regex = text_regex_new();
		size_t pos = view_cursor_get(view);
		if (prompt->file == vis->command_file)
			pattern = "^:";
		else if (prompt->file == vis->search_file)
			pattern = "^(/|\\?)";
		int cflags = REG_EXTENDED|REG_NEWLINE|(REG_ICASE*vis->ignorecase);
		if (pattern && regex && text_regex_compile(regex, pattern, 0, cflags) == 0) {
			size_t end = text_line_end(txt, pos);
			size_t prev = text_search_backward(txt, end, regex);
			if (prev > pos)
				prev = EPOS;
			size_t next = text_search_forward(txt, pos, regex);
			if (next < pos)
				next = text_size(txt);
			range = text_range_new(prev, next);
		}
		text_regex_free(regex);
	}
	if (text_range_valid(&range))
		cmd = text_bytes_alloc0(txt, range.start, text_range_size(&range));

	if (!win || !cmd) {
		if (!win)
			vis_info_show(vis, "Prompt window invalid");
		else if (!cmd)
			vis_info_show(vis, "Failed to detect command");
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
	if (prompt->view.selection_count > 1) {
		view_selections_dispose_all(&prompt->view);
	} else {
		prompt_restore(prompt);
		prompt_hide(prompt);
	}
	return keys;
}

static const char *prompt_up(Vis *vis, const char *keys, const Arg *arg) {
	vis_motion(vis, VIS_MOVE_LINE_UP);
	vis_window_mode_unmap(vis->win, VIS_MODE_INSERT, "<Up>");
	win_options_set(vis->win, UI_OPTION_SYMBOL_EOF);
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

static const KeyBinding prompt_tab_binding = {
	.key = "<Tab>",
	.alias = "<C-x><C-o>",
};

void vis_prompt_show(Vis *vis, const char *title) {
	Win *active = vis->win;
	Win *prompt = window_new_file(vis, title[0] == ':' ? vis->command_file : vis->search_file,
		UI_OPTION_ONELINE);
	if (!prompt)
		return;
	Text *txt = prompt->file->text;
	text_appendf(txt, "%s\n", title);
	Selection *sel = view_selections_primary_get(&prompt->view);
	view_cursors_scroll_to(sel, text_size(txt)-1);
	prompt->parent = active;
	prompt->parent_mode = vis->mode;
	vis_window_mode_map(prompt, VIS_MODE_NORMAL, true, "<Enter>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<Enter>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<C-j>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_VISUAL, true, "<Enter>", &prompt_enter_binding);
	vis_window_mode_map(prompt, VIS_MODE_NORMAL, true, "<Escape>", &prompt_esc_binding);
	vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<Up>", &prompt_up_binding);
	if (CONFIG_LUA)
		vis_window_mode_map(prompt, VIS_MODE_INSERT, true, "<Tab>", &prompt_tab_binding);
	vis_mode_switch(vis, VIS_MODE_INSERT);
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
	text_appendf(txt, "%s\n", msg);
	text_save(txt, NULL);
	view_cursors_to(win->view.selection, pos);
	vis_window_focus(win);
}
