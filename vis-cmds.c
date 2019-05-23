/* this file is included from sam.c */

#include <termkey.h>
#include "vis-lua.h"

// FIXME: avoid this redirection?
typedef struct {
	CommandDef def;
	VisCommandFunction *func;
	void *data;
} CmdUser;

static void cmdfree(CmdUser *cmd) {
	if (!cmd)
		return;
	free((char*)cmd->def.name);
	free(VIS_HELP_USE((char*)cmd->def.help));
	free(cmd);
}

bool vis_cmd_register(Vis *vis, const char *name, const char *help, void *data, VisCommandFunction *func) {
	if (!name)
		return false;
	if (!vis->usercmds && !(vis->usercmds = map_new()))
		return false;
	CmdUser *cmd = calloc(1, sizeof *cmd);
	if (!cmd)
		return false;
	if (!(cmd->def.name = strdup(name)))
		goto err;
#if CONFIG_HELP
	if (help && !(cmd->def.help = strdup(help)))
		goto err;
#endif
	cmd->def.flags = CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_ALL;
	cmd->def.func = cmd_user;
	cmd->func = func;
	cmd->data = data;
	if (!map_put(vis->cmds, name, &cmd->def))
		goto err;
	if (!map_put(vis->usercmds, name, cmd)) {
		map_delete(vis->cmds, name);
		goto err;
	}
	return true;
err:
	cmdfree(cmd);
	return false;
}

bool vis_cmd_unregister(Vis *vis, const char *name) {
	if (!name)
		return true;
	CmdUser *cmd = map_get(vis->usercmds, name);
	if (!cmd)
		return false;
	if (!map_delete(vis->cmds, name))
		return false;
	if (!map_delete(vis->usercmds, name))
		return false;
	cmdfree(cmd);
	return true;
}

static void option_free(OptionDef *opt) {
	if (!opt)
		return;
	for (size_t i = 0; i < LENGTH(options); i++) {
		if (opt == &options[i])
			return;
	}

	for (const char **name = opt->names; *name; name++)
		free((char*)*name);
	free(VIS_HELP_USE((char*)opt->help));
	free(opt);
}

bool vis_option_register(Vis *vis, const char *names[], enum VisOption flags,
                         VisOptionFunction *func, void *context, const char *help) {

	if (!names || !names[0])
		return false;

	for (const char **name = names; *name; name++) {
		if (map_get(vis->options, *name))
			return false;
	}
	OptionDef *opt = calloc(1, sizeof *opt);
	if (!opt)
		return false;
	for (size_t i = 0; i < LENGTH(opt->names)-1 && names[i]; i++) {
		if (!(opt->names[i] = strdup(names[i])))
			goto err;
	}
	opt->flags = flags;
	opt->func = func;
	opt->context = context;
#if CONFIG_HELP
	if (help && !(opt->help = strdup(help)))
		goto err;
#endif
	for (const char **name = names; *name; name++)
		map_put(vis->options, *name, opt);
	return true;
err:
	option_free(opt);
	return false;
}

bool vis_option_unregister(Vis *vis, const char *name) {
	OptionDef *opt = map_get(vis->options, name);
	if (!opt)
		return false;
	for (const char **alias = opt->names; *alias; alias++) {
		if (!map_delete(vis->options, *alias))
			return false;
	}
	option_free(opt);
	return true;
}

static bool cmd_user(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	CmdUser *user = map_get(vis->usercmds, argv[0]);
	return user && user->func(vis, win, user->data, cmd->flags == '!', argv, sel, range);
}

static void windows_arrange(Vis *vis, enum UiLayout layout) {
	vis->ui->arrange(vis->ui, layout);
}

static void tabwidth_set(Vis *vis, int tabwidth) {
	if (tabwidth < 1 || tabwidth > 8)
		return;
	for (Win *win = vis->windows; win; win = win->next)
		view_tabwidth_set(win->view, tabwidth);
	vis->tabwidth = tabwidth;
}

/* parse human-readable boolean value in s. If successful, store the result in
 * outval and return true. Else return false and leave outval alone. */
static bool parse_bool(const char *s, bool *outval) {
	for (const char **t = (const char*[]){"1", "true", "yes", "on", NULL}; *t; t++) {
		if (!strcasecmp(s, *t)) {
			*outval = true;
			return true;
		}
	}
	for (const char **f = (const char*[]){"0", "false", "no", "off", NULL}; *f; f++) {
		if (!strcasecmp(s, *f)) {
			*outval = false;
			return true;
		}
	}
	return false;
}

static bool cmd_set(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {

	if (!argv[1] || !argv[1][0] || argv[3]) {
		vis_info_show(vis, "Expecting: set option [value]");
		return false;
	}

	char name[256];
	strncpy(name, argv[1], sizeof(name)-1);
	char *lastchar = &name[strlen(name)-1];
	bool toggle = (*lastchar == '!');
	if (toggle)
		*lastchar = '\0';

	OptionDef *opt = map_closest(vis->options, name);
	if (!opt) {
		vis_info_show(vis, "Unknown option: `%s'", name);
		return false;
	}

	if (!win && (opt->flags & VIS_OPTION_NEED_WINDOW)) {
		vis_info_show(vis, "Need active window for `:set %s'", name);
		return false;
	}

	if (toggle) {
		if (!(opt->flags & VIS_OPTION_TYPE_BOOL)) {
			vis_info_show(vis, "Only boolean options can be toggled");
			return false;
		}
		if (argv[2]) {
			vis_info_show(vis, "Can not specify option value when toggling");
			return false;
		}
	}

	Arg arg;
	if (opt->flags & VIS_OPTION_TYPE_STRING) {
		if (!(opt->flags & VIS_OPTION_VALUE_OPTIONAL) && !argv[2]) {
			vis_info_show(vis, "Expecting string option value");
			return false;
		}
		arg.s = argv[2];
	} else if (opt->flags & VIS_OPTION_TYPE_BOOL) {
		if (!argv[2]) {
			arg.b = !toggle;
		} else if (!parse_bool(argv[2], &arg.b)) {
			vis_info_show(vis, "Expecting boolean option value not: `%s'", argv[2]);
			return false;
		}
	} else if (opt->flags & VIS_OPTION_TYPE_NUMBER) {
		if (!argv[2]) {
			vis_info_show(vis, "Expecting number");
			return false;
		}
		char *ep;
		errno = 0;
		long lval = strtol(argv[2], &ep, 10);
		if (argv[2][0] == '\0' || *ep != '\0') {
			vis_info_show(vis, "Invalid number");
			return false;
		}

		if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
		    (lval > INT_MAX || lval < INT_MIN)) {
			vis_info_show(vis, "Number overflow");
			return false;
		}

		if (lval < 0) {
			vis_info_show(vis, "Expecting positive number");
			return false;
		}
		arg.i = lval;
	} else {
		return false;
	}

	size_t opt_index = 0;
	for (; opt_index < LENGTH(options); opt_index++) {
		if (opt == &options[opt_index])
			break;
	}

	switch (opt_index) {
	case OPTION_SHELL:
	{
		char *shell = strdup(arg.s);
		if (!shell) {
			vis_info_show(vis, "Failed to change shell");
			return false;
		}
		free(vis->shell);
		vis->shell = shell;
		break;
	}
	case OPTION_ESCDELAY:
	{
		TermKey *termkey = vis->ui->termkey_get(vis->ui);
		termkey_set_waittime(termkey, arg.i);
		break;
	}
	case OPTION_EXPANDTAB:
		vis->expandtab = toggle ? !vis->expandtab : arg.b;
		break;
	case OPTION_AUTOINDENT:
		vis->autoindent = toggle ? !vis->autoindent : arg.b;
		break;
	case OPTION_TABWIDTH:
		tabwidth_set(vis, arg.i);
		break;
	case OPTION_SHOW_SPACES:
	case OPTION_SHOW_TABS:
	case OPTION_SHOW_NEWLINES:
	case OPTION_SHOW_EOF:
	{
		const int values[] = {
			[OPTION_SHOW_SPACES] = UI_OPTION_SYMBOL_SPACE,
			[OPTION_SHOW_TABS] = UI_OPTION_SYMBOL_TAB|UI_OPTION_SYMBOL_TAB_FILL,
			[OPTION_SHOW_NEWLINES] = UI_OPTION_SYMBOL_EOL,
			[OPTION_SHOW_EOF] = UI_OPTION_SYMBOL_EOF,
		};
		int flags = view_options_get(win->view);
		if (arg.b || (toggle && !(flags & values[opt_index])))
			flags |= values[opt_index];
		else
			flags &= ~values[opt_index];
		view_options_set(win->view, flags);
		break;
	}
	case OPTION_NUMBER: {
		enum UiOption opt = view_options_get(win->view);
		if (arg.b || (toggle && !(opt & UI_OPTION_LINE_NUMBERS_ABSOLUTE))) {
			opt &= ~UI_OPTION_LINE_NUMBERS_RELATIVE;
			opt |=  UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		} else {
			opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		}
		view_options_set(win->view, opt);
		break;
	}
	case OPTION_NUMBER_RELATIVE: {
		enum UiOption opt = view_options_get(win->view);
		if (arg.b || (toggle && !(opt & UI_OPTION_LINE_NUMBERS_RELATIVE))) {
			opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
			opt |=  UI_OPTION_LINE_NUMBERS_RELATIVE;
		} else {
			opt &= ~UI_OPTION_LINE_NUMBERS_RELATIVE;
		}
		view_options_set(win->view, opt);
		break;
	}
	case OPTION_CURSOR_LINE: {
		enum UiOption opt = view_options_get(win->view);
		if (arg.b || (toggle && !(opt & UI_OPTION_CURSOR_LINE)))
			opt |= UI_OPTION_CURSOR_LINE;
		else
			opt &= ~UI_OPTION_CURSOR_LINE;
		view_options_set(win->view, opt);
		break;
	}
	case OPTION_COLOR_COLUMN:
		view_colorcolumn_set(win->view, arg.i);
		break;
	case OPTION_SAVE_METHOD:
		if (strcmp("auto", arg.s) == 0) {
			win->file->save_method = TEXT_SAVE_AUTO;
		} else if (strcmp("atomic", arg.s) == 0) {
			win->file->save_method = TEXT_SAVE_ATOMIC;
		} else if (strcmp("inplace", arg.s) == 0) {
			win->file->save_method = TEXT_SAVE_INPLACE;
		} else {
			vis_info_show(vis, "Invalid save method `%s', expected "
			              "'auto', 'atomic' or 'inplace'", arg.s);
			return false;
		}
		break;
	case OPTION_LOAD_METHOD:
		if (strcmp("auto", arg.s) == 0) {
			vis->load_method = TEXT_LOAD_AUTO;
		} else if (strcmp("read", arg.s) == 0) {
			vis->load_method = TEXT_LOAD_READ;
		} else if (strcmp("mmap", arg.s) == 0) {
			vis->load_method = TEXT_LOAD_MMAP;
		} else {
			vis_info_show(vis, "Invalid load method `%s', expected "
			              "'auto', 'read' or 'mmap'", arg.s);
			return false;
		}
		break;
	case OPTION_CHANGE_256COLORS:
		vis->change_colors = toggle ? !vis->change_colors : arg.b;
		break;
	case OPTION_LAYOUT: {
		enum UiLayout layout;
		if (strcmp("h", arg.s) == 0) {
			layout = UI_LAYOUT_HORIZONTAL;
		} else if (strcmp("v", arg.s) == 0) {
			layout = UI_LAYOUT_VERTICAL;
		} else {
			vis_info_show(vis, "Invalid layout `%s', expected 'h' or 'v'", arg.s);
			return false;
		}
		windows_arrange(vis, layout);
		break;
	}
	default:
		if (!opt->func)
			return false;
		return opt->func(vis, win, opt->context, toggle, opt->flags, name, &arg);
	}

	return true;
}

static bool is_file_pattern(const char *pattern) {
	if (!pattern)
		return false;
	struct stat meta;
	if (stat(pattern, &meta) == 0 && S_ISDIR(meta.st_mode))
		return true;
	for (char special[] = "*?[{$~", *s = special; *s; s++) {
		if (strchr(pattern, *s))
			return true;
	}
	return false;
}

static const char *file_open_dialog(Vis *vis, const char *pattern) {
	static char name[PATH_MAX];
	name[0] = '\0';

	if (!is_file_pattern(pattern))
		return pattern;

	Buffer bufcmd, bufout, buferr;
	buffer_init(&bufcmd);
	buffer_init(&bufout);
	buffer_init(&buferr);

	if (!buffer_put0(&bufcmd, VIS_OPEN " ") || !buffer_append0(&bufcmd, pattern ? pattern : ""))
		return NULL;

	Filerange empty = text_range_new(0,0);
	int status = vis_pipe(vis, vis->win->file, &empty,
		(const char*[]){ buffer_content0(&bufcmd), NULL },
		&bufout, read_buffer, &buferr, read_buffer);

	if (status == 0)
		strncpy(name, buffer_content0(&bufout), sizeof(name)-1);
	else
		vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));

	buffer_release(&bufcmd);
	buffer_release(&bufout);
	buffer_release(&buferr);

	for (char *end = name+strlen(name)-1; end >= name && isspace((unsigned char)*end); end--)
		*end = '\0';

	return name[0] ? name : NULL;
}

static bool openfiles(Vis *vis, const char **files) {
	for (; *files; files++) {
		const char *file = file_open_dialog(vis, *files);
		if (!file)
			return false;
		errno = 0;
		if (!vis_window_new(vis, file)) {
			vis_info_show(vis, "Could not open `%s' %s", file,
			                 errno ? strerror(errno) : "");
			return false;
		}
	}
	return true;
}

static bool cmd_open(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!argv[1])
		return vis_window_new(vis, NULL);
	return openfiles(vis, &argv[1]);
}

static void info_unsaved_changes(Vis *vis) {
	vis_info_show(vis, "No write since last change (add ! to override)");
}

static bool cmd_edit(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (argv[2]) {
		vis_info_show(vis, "Only 1 filename allowed");
		return false;
	}
	Win *oldwin = win;
	if (!oldwin)
		return false;
	if (cmd->flags != '!' && !vis_window_closable(oldwin)) {
		info_unsaved_changes(vis);
		return false;
	}
	if (!argv[1]) {
		if (oldwin->file->refcount > 1) {
			vis_info_show(vis, "Can not reload file being opened multiple times");
			return false;
		}
		return vis_window_reload(oldwin);
	}
	if (!openfiles(vis, &argv[1]))
		return false;
	if (vis->win != oldwin) {
		Win *newwin = vis->win;
		vis_window_swap(oldwin, newwin);
		vis_window_close(oldwin);
		vis_window_focus(newwin);
	}
	return vis->win != oldwin;
}

static bool cmd_read(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	bool ret = false;
	const size_t first_file = 3;
	const char *args[MAX_ARGV] = { argv[0], "cat", "--" };
	const char **name = argv[1] ? &argv[1] : (const char*[]){ ".", NULL };
	for (size_t i = first_file; *name && i < LENGTH(args)-1; name++, i++) {
		const char *file = file_open_dialog(vis, *name);
		if (!file || !(args[i] = strdup(file)))
			goto err;
	}
	args[LENGTH(args)-1] = NULL;
	ret = cmd_pipein(vis, win, cmd, args, sel, range);
err:
	for (size_t i = first_file; i < LENGTH(args); i++)
		free((char*)args[i]);
	return ret;
}

static bool has_windows(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next) {
		if (!win->file->internal)
			return true;
	}
	return false;
}

static bool cmd_quit(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (cmd->flags != '!' && !vis_window_closable(win)) {
		info_unsaved_changes(vis);
		return false;
	}
	vis_window_close(win);
	if (!has_windows(vis))
		vis_exit(vis, EXIT_SUCCESS);
	return true;
}

static bool cmd_qall(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (!win->file->internal && (!text_modified(win->file->text) || cmd->flags == '!'))
			vis_window_close(win);
	}
	if (!has_windows(vis)) {
		vis_exit(vis, EXIT_SUCCESS);
		return true;
	} else {
		info_unsaved_changes(vis);
		return false;
	}
}

static bool cmd_split(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	enum UiOption options = view_options_get(win->view);
	windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	if (!argv[1])
		return vis_window_split(win);
	bool ret = openfiles(vis, &argv[1]);
	if (ret)
		view_options_set(vis->win->view, options);
	return ret;
}

static bool cmd_vsplit(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	enum UiOption options = view_options_get(win->view);
	windows_arrange(vis, UI_LAYOUT_VERTICAL);
	if (!argv[1])
		return vis_window_split(win);
	bool ret = openfiles(vis, &argv[1]);
	if (ret)
		view_options_set(vis->win->view, options);
	return ret;
}

static bool cmd_new(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_vnew(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	windows_arrange(vis, UI_LAYOUT_VERTICAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_wq(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	File *file = win->file;
	bool unmodified = file->fd == -1 && !file->name && !text_modified(file->text);
	if (unmodified || cmd_write(vis, win, cmd, argv, sel, range))
		return cmd_quit(vis, win, cmd, argv, sel, range);
	return false;
}

static bool cmd_earlier_later(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	Text *txt = win->file->text;
	char *unit = "";
	long count = 1;
	size_t pos = EPOS;
	if (argv[1]) {
		errno = 0;
		count = strtol(argv[1], &unit, 10);
		if (errno || unit == argv[1] || count < 0) {
			vis_info_show(vis, "Invalid number");
			return false;
		}

		if (*unit) {
			while (*unit && isspace((unsigned char)*unit))
				unit++;
			switch (*unit) {
			case 'd': count *= 24; /* fall through */
			case 'h': count *= 60; /* fall through */
			case 'm': count *= 60; /* fall through */
			case 's': break;
			default:
				vis_info_show(vis, "Unknown time specifier (use: s,m,h or d)");
				return false;
			}

			if (argv[0][0] == 'e')
				count = -count; /* earlier, move back in time */

			pos = text_restore(txt, text_state(txt) + count);
		}
	}

	if (!*unit) {
		VisCountIterator it = vis_count_iterator_init(vis, count);
		while (vis_count_iterator_next(&it)) {
			if (argv[0][0] == 'e')
				pos = text_earlier(txt);
			else
				pos = text_later(txt);
		}
	}

	time_t state = text_state(txt);
	char buf[32];
	strftime(buf, sizeof buf, "State from %H:%M", localtime(&state));
	vis_info_show(vis, "%s", buf);

	return pos != EPOS;
}

static bool print_keylayout(const char *key, void *value, void *data) {
	return text_appendf(data, "  %-18s\t%s\n", key[0] == ' ' ? "␣" : key, (char*)value);
}

static bool print_keybinding(const char *key, void *value, void *data) {
	KeyBinding *binding = value;
	const char *desc = binding->alias;
	if (!desc && binding->action)
		desc = VIS_HELP_USE(binding->action->help);
	return text_appendf(data, "  %-18s\t%s\n", key[0] == ' ' ? "␣" : key, desc ? desc : "");
}

static void print_mode(Mode *mode, Text *txt) {
	if (!map_empty(mode->bindings))
		text_appendf(txt, "\n %s\n\n", mode->name);
	map_iterate(mode->bindings, print_keybinding, txt);
}

static bool print_action(const char *key, void *value, void *data) {
	const char *help = VIS_HELP_USE(((KeyAction*)value)->help);
	return text_appendf(data, "  %-30s\t%s\n", key, help ? help : "");
}

static bool print_cmd(const char *key, void *value, void *data) {
	CommandDef *cmd = value;
	const char *help = VIS_HELP_USE(cmd->help);
	char usage[256];
	snprintf(usage, sizeof usage, "%s%s%s%s%s%s%s",
	         cmd->name,
	         (cmd->flags & CMD_FORCE) ? "[!]" : "",
	         (cmd->flags & CMD_TEXT) ? "/text/" : "",
	         (cmd->flags & CMD_REGEX) ? "/regexp/" : "",
	         (cmd->flags & CMD_CMD) ? " command" : "",
	         (cmd->flags & CMD_SHELL) ? (!strcmp(cmd->name, "s") ? "/regexp/text/" : " shell-command") : "",
	         (cmd->flags & CMD_ARGV) ? " [args...]" : "");
	return text_appendf(data, "  %-30s %s\n", usage, help ? help : "");
}

static bool print_option(const char *key, void *value, void *txt) {
	char desc[256];
	const OptionDef *opt = value;
	const char *help = VIS_HELP_USE(opt->help);
	if (strcmp(key, opt->names[0]))
		return true;
	snprintf(desc, sizeof desc, "%s%s%s%s%s",
	         opt->names[0],
	         opt->names[1] ? "|" : "",
	         opt->names[1] ? opt->names[1] : "",
	         opt->flags & VIS_OPTION_TYPE_BOOL ? " on|off" : "",
	         opt->flags & VIS_OPTION_TYPE_NUMBER ? " nn" : "");
	return text_appendf(txt, "  %-30s %s\n", desc, help ? help : "");
}

static void print_symbolic_keys(Vis *vis, Text *txt) {
	static const int keys[] = {
		TERMKEY_SYM_BACKSPACE,
		TERMKEY_SYM_TAB,
		TERMKEY_SYM_ENTER,
		TERMKEY_SYM_ESCAPE,
		//TERMKEY_SYM_SPACE,
		TERMKEY_SYM_DEL,
		TERMKEY_SYM_UP,
		TERMKEY_SYM_DOWN,
		TERMKEY_SYM_LEFT,
		TERMKEY_SYM_RIGHT,
		TERMKEY_SYM_BEGIN,
		TERMKEY_SYM_FIND,
		TERMKEY_SYM_INSERT,
		TERMKEY_SYM_DELETE,
		TERMKEY_SYM_SELECT,
		TERMKEY_SYM_PAGEUP,
		TERMKEY_SYM_PAGEDOWN,
		TERMKEY_SYM_HOME,
		TERMKEY_SYM_END,
		TERMKEY_SYM_CANCEL,
		TERMKEY_SYM_CLEAR,
		TERMKEY_SYM_CLOSE,
		TERMKEY_SYM_COMMAND,
		TERMKEY_SYM_COPY,
		TERMKEY_SYM_EXIT,
		TERMKEY_SYM_HELP,
		TERMKEY_SYM_MARK,
		TERMKEY_SYM_MESSAGE,
		TERMKEY_SYM_MOVE,
		TERMKEY_SYM_OPEN,
		TERMKEY_SYM_OPTIONS,
		TERMKEY_SYM_PRINT,
		TERMKEY_SYM_REDO,
		TERMKEY_SYM_REFERENCE,
		TERMKEY_SYM_REFRESH,
		TERMKEY_SYM_REPLACE,
		TERMKEY_SYM_RESTART,
		TERMKEY_SYM_RESUME,
		TERMKEY_SYM_SAVE,
		TERMKEY_SYM_SUSPEND,
		TERMKEY_SYM_UNDO,
		TERMKEY_SYM_KP0,
		TERMKEY_SYM_KP1,
		TERMKEY_SYM_KP2,
		TERMKEY_SYM_KP3,
		TERMKEY_SYM_KP4,
		TERMKEY_SYM_KP5,
		TERMKEY_SYM_KP6,
		TERMKEY_SYM_KP7,
		TERMKEY_SYM_KP8,
		TERMKEY_SYM_KP9,
		TERMKEY_SYM_KPENTER,
		TERMKEY_SYM_KPPLUS,
		TERMKEY_SYM_KPMINUS,
		TERMKEY_SYM_KPMULT,
		TERMKEY_SYM_KPDIV,
		TERMKEY_SYM_KPCOMMA,
		TERMKEY_SYM_KPPERIOD,
		TERMKEY_SYM_KPEQUALS,
	};

	TermKey *termkey = vis->ui->termkey_get(vis->ui);
	text_appendf(txt, "  ␣ (a literal \" \" space symbol must be used to refer to <Space>)\n");
	for (size_t i = 0; i < LENGTH(keys); i++) {
		text_appendf(txt, "  <%s>\n", termkey_get_keyname(termkey, keys[i]));
	}
}

static bool cmd_help(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!vis_window_new(vis, NULL))
		return false;

	Text *txt = vis->win->file->text;

	text_appendf(txt, "vis %s (PID: %ld)\n\n", VERSION, (long)getpid());

	text_appendf(txt, " Modes\n\n");
	for (int i = 0; i < LENGTH(vis_modes); i++) {
		Mode *mode = &vis_modes[i];
		if (mode->help)
			text_appendf(txt, "  %-18s\t%s\n", mode->name, mode->help);
	}

	if (!map_empty(vis->keymap)) {
		text_appendf(txt, "\n Layout specific mappings (affects all modes except INSERT/REPLACE)\n\n");
		map_iterate(vis->keymap, print_keylayout, txt);
	}

	print_mode(&vis_modes[VIS_MODE_NORMAL], txt);
	print_mode(&vis_modes[VIS_MODE_OPERATOR_PENDING], txt);
	print_mode(&vis_modes[VIS_MODE_VISUAL], txt);
	print_mode(&vis_modes[VIS_MODE_INSERT], txt);

	text_appendf(txt, "\n :-Commands\n\n");
	map_iterate(vis->cmds, print_cmd, txt);

	text_appendf(txt, "\n Marks\n\n");
	text_appendf(txt, "  a-z General purpose marks\n");
	for (size_t i = 0; i < LENGTH(vis_marks); i++) {
		const char *help = VIS_HELP_USE(vis_marks[i].help);
		text_appendf(txt, "  %c   %s\n", vis_marks[i].name, help ? help : "");
	}

	text_appendf(txt, "\n Registers\n\n");
	text_appendf(txt, "  a-z General purpose registers\n");
	text_appendf(txt, "  A-Z Append to corresponding general purpose register\n");
	for (size_t i = 0; i < LENGTH(vis_registers); i++) {
		const char *help = VIS_HELP_USE(vis_registers[i].help);
		text_appendf(txt, "  %c   %s\n", vis_registers[i].name, help ? help : "");
	}

	text_appendf(txt, "\n :set command options\n\n");
	map_iterate(vis->options, print_option, txt);

	text_appendf(txt, "\n Key binding actions\n\n");
	map_iterate(vis->actions, print_action, txt);

	text_appendf(txt, "\n Symbolic keys usable for key bindings "
		"(prefix with C-, S-, and M- for Ctrl, Shift and Alt respectively)\n\n");
	print_symbolic_keys(vis, txt);

	char *paths[] = { NULL, NULL };
	char *paths_description[] = {
		"Lua paths used to load runtime files (? will be replaced by filename):",
		"Lua paths used to load C libraries (? will be replaced by filename):",
	};

	if (vis_lua_paths_get(vis, &paths[0], &paths[1])) {
		for (size_t i = 0; i < LENGTH(paths); i++) {
			text_appendf(txt, "\n %s\n\n", paths_description[i]);
			for (char *elem = paths[i], *next; elem; elem = next) {
				if ((next = strstr(elem, ";")))
					*next++ = '\0';
				if (*elem)
					text_appendf(txt, "  %s\n", elem);
			}
			free(paths[i]);
		}
	}

	text_appendf(txt, "\n Compile time configuration\n\n");

	const struct {
		const char *name;
		bool enabled;
	} configs[] = {
		{ "Curses support: ", CONFIG_CURSES },
		{ "Lua support: ", CONFIG_LUA },
		{ "Lua LPeg statically built-in: ", CONFIG_LPEG },
		{ "TRE based regex support: ", CONFIG_TRE },
		{ "POSIX ACL support: ", CONFIG_ACL },
		{ "SELinux support: ", CONFIG_SELINUX },
	};

	for (size_t i = 0; i < LENGTH(configs); i++)
		text_appendf(txt, "  %-32s\t%s\n", configs[i].name, configs[i].enabled ? "yes" : "no");

	text_save(txt, NULL);
	view_cursor_to(vis->win->view, 0);

	if (argv[1])
		vis_motion(vis, VIS_MOVE_SEARCH_FORWARD, argv[1]);
	return true;
}

static bool cmd_langmap(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	const char *nonlatin = argv[1];
	const char *latin = argv[2];
	bool mapped = true;

	if (!latin || !nonlatin) {
		vis_info_show(vis, "usage: langmap <non-latin keys> <latin keys>");
		return false;
	}

	while (*latin && *nonlatin) {
		size_t i = 0, j = 0;
		char latin_key[8], nonlatin_key[8];
		do {
			if (i < sizeof(latin_key)-1)
				latin_key[i++] = *latin;
			latin++;
		} while (!ISUTF8(*latin));
		do {
			if (j < sizeof(nonlatin_key)-1)
				nonlatin_key[j++] = *nonlatin;
			nonlatin++;
		} while (!ISUTF8(*nonlatin));
		latin_key[i] = '\0';
		nonlatin_key[j] = '\0';
		mapped &= vis_keymap_add(vis, nonlatin_key, strdup(latin_key));
	}

	return mapped;
}

static bool cmd_map(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	bool mapped = false;
	bool local = strstr(argv[0], "-") != NULL;
	enum VisMode mode = vis_mode_from(vis, argv[1]);

	if (local && !win) {
		vis_info_show(vis, "Invalid window for :%s", argv[0]);
		return false;
	}

	if (mode == VIS_MODE_INVALID || !argv[2] || !argv[3]) {
		vis_info_show(vis, "usage: %s mode lhs rhs", argv[0]);
		return false;
	}

	const char *lhs = argv[2];
	KeyBinding *binding = vis_binding_new(vis);
	if (!binding || !(binding->alias = strdup(argv[3])))
		goto err;

	if (local)
		mapped = vis_window_mode_map(win, mode, cmd->flags == '!', lhs, binding);
	else
		mapped = vis_mode_map(vis, mode, cmd->flags == '!', lhs, binding);

err:
	if (!mapped) {
		vis_info_show(vis, "Failed to map `%s' in %s mode%s", lhs, argv[1],
		              cmd->flags != '!' ? ", mapping already exists, "
		              "override with `!'" : "");
		vis_binding_free(vis, binding);
	}
	return mapped;
}

static bool cmd_unmap(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	bool unmapped = false;
	bool local = strstr(argv[0], "-") != NULL;
	enum VisMode mode = vis_mode_from(vis, argv[1]);
	const char *lhs = argv[2];

	if (local && !win) {
		vis_info_show(vis, "Invalid window for :%s", argv[0]);
		return false;
	}

	if (mode == VIS_MODE_INVALID || !lhs) {
		vis_info_show(vis, "usage: %s mode lhs", argv[0]);
		return false;
	}

	if (local)
		unmapped = vis_window_mode_unmap(win, mode, lhs);
	else
		unmapped = vis_mode_unmap(vis, mode, lhs);
	if (!unmapped)
		vis_info_show(vis, "Failed to unmap `%s' in %s mode", lhs, argv[1]);
	return unmapped;
}
