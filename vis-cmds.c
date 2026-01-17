/* this file is included from sam.c */

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

static bool cmd_user(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	CmdUser *user = map_get(vis->usercmds, argv[0]);
	return user && user->func(vis, win, user->data, cmd->flags == '!', argv, sel, range);
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

	str8 name = str8_from_c_str(argv[1]);
	if (name.length == 0 || argv[3]) {
		vis_info_show(vis, "Expecting: set option [value]");
		return false;
	}

	bool toggle  = name.data[name.length - 1] == '!';
	name.length -= (int)toggle;

	VisOption *opt = vis_option_from_string(vis, name);
	if (!opt) {
		vis_info_show(vis, "Unknown option: `%.*s'", (int)name.length, name.data);
		return false;
	}

	if (opt->flags & VIS_OPTION_DEPRECATED && str8_equal(name, str8_from_c_str(opt->set_context)))
		vis_info_show(vis, "%.*s is deprecated and will be removed in the next release", (int)name.length, name.data);

	if (!win && (opt->flags & VIS_OPTION_NEED_WINDOW)) {
		vis_info_show(vis, "Need active window for `:set %.*s'", (int)name.length, name.data);
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

	VisValue value = {0};
	if (opt->flags & VIS_OPTION_TYPE_STRING) {
		if (!argv[2]) {
			vis_info_show(vis, "Expecting string option value");
			return false;
		}
		value.kind     = VisValueKind_String;
		value.u.string = argv[2];
	} else if (opt->flags & VIS_OPTION_TYPE_BOOL) {
		bool boolean = !toggle;
		if (argv[2] && !parse_bool(argv[2], &boolean)) {
			vis_info_show(vis, "Expecting boolean option value not: `%s'", argv[2]);
			return false;
		}
		value.kind      = VisValueKind_Boolean;
		value.u.boolean = boolean;
	} else if (opt->flags & VIS_OPTION_TYPE_NUMBER) {
		if (!argv[2]) {
			vis_info_show(vis, "Expecting number");
			return false;
		}

		IntegerConversion integer = integer_conversion(str8_from_c_str((char *)argv[2]), 0);
		long lval = integer.as.S64;
		if (integer.result == IntegerConversionResult_OutOfRange || lval > INT_MAX || lval < INT_MIN) {
			vis_info_show(vis, "Number overflow");
			return false;
		}

		if (integer.result != IntegerConversionResult_Success || integer.unparsed.length > 0) {
			vis_info_show(vis, "Invalid number: %s", argv[2]);
			return false;
		}

		value.kind      = VisValueKind_Integer;
		value.u.integer = lval;
	} else {
		return false;
	}

	return vis_option_set(vis, win, opt, value, toggle);
}

static bool is_file_pattern(const char *pattern) {
	if (!pattern)
		return false;
	struct stat meta;
	if (stat(pattern, &meta) == 0 && S_ISDIR(meta.st_mode))
		return true;
	/* tilde expansion is defined only for the tilde at the
	   beginning of the pattern. */
	if (pattern[0] == '~')
		return true;
	for (char special[] = "*?[{$", *s = special; *s; s++) {
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

	Buffer bufcmd = {0}, bufout = {0}, buferr = {0};

	if (!buffer_put0(&bufcmd, VIS_OPEN " ") || !buffer_append0(&bufcmd, pattern ? pattern : ""))
		return NULL;

	int status = vis_pipe(vis, vis->win->file, text_range_new(0, 0),
	                      (const char*[]){buffer_content0(&bufcmd), 0},
	                      &bufout, read_into_buffer, &buferr, read_into_buffer, false);

	if (status == 0)
		strncpy(name, buffer_content0(&bufout), sizeof(name)-1);
	else if (status != 1)
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
		vis_window_focus(vis, newwin);
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

VIS_INTERNAL bool
vis_cmd_try_exit(Vis *vis, str8 exit_code_string)
{
	bool result = true;
	for (Win *win = vis->windows; win; win = win->next) {
		if (!win->file->internal) {
			result = false;
			break;
		}
	}

	if (result) {
		int exit_code = EXIT_SUCCESS;
		IntegerConversion integer = integer_conversion(exit_code_string, 0);
		if (integer.result == IntegerConversionResult_Success)
			exit_code = (int)integer.as.S64;
		vis_exit(vis, exit_code);
	}

	return result;
}

VIS_INTERNAL bool
vis_cmd_quit(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range)
{
	bool result = cmd->flags == '!' || vis_window_closable(win);
	if (result) {
		vis_window_close(win);
		vis_cmd_try_exit(vis, str8_from_c_str(argv[1]));
	} else {
		info_unsaved_changes(vis);
	}
	return result;
}

VIS_INTERNAL bool
vis_cmd_qall(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range)
{
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (!win->file->internal && (!text_modified(win->file->text) || cmd->flags == '!'))
			vis_window_close(win);
	}

	bool result = vis_cmd_try_exit(vis, str8_from_c_str(argv[1]));
	if (!result) info_unsaved_changes(vis);
	return result;
}

static bool cmd_split(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	enum UiOption options = win->options;
	ui_arrange(vis, UI_LAYOUT_HORIZONTAL);
	if (!argv[1])
		return vis_window_split(win);
	bool ret = openfiles(vis, &argv[1]);
	if (ret)
		win_options_set(vis->win, options);
	return ret;
}

static bool cmd_vsplit(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	enum UiOption options = win->options;
	ui_arrange(vis, UI_LAYOUT_VERTICAL);
	if (!argv[1])
		return vis_window_split(win);
	bool ret = openfiles(vis, &argv[1]);
	if (ret)
		win_options_set(vis->win, options);
	return ret;
}

static bool cmd_new(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	ui_arrange(vis, UI_LAYOUT_HORIZONTAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_vnew(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	ui_arrange(vis, UI_LAYOUT_VERTICAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_wq(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	File *file = win->file;
	bool unmodified = file->fd == -1 && !file->name && !text_modified(file->text);
	if (unmodified || cmd_write(vis, win, cmd, argv, sel, range))
		return vis_cmd_quit(vis, win, cmd, (const char*[]){argv[0], NULL}, sel, range);
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
		str8 arg = str8_from_c_str((char *)argv[1]);
		IntegerConversion integer = integer_conversion(arg, 0);
		long count = integer.as.S64;
		if (integer.result != IntegerConversionResult_Success || arg.data == integer.unparsed.data || count < 0) {
			vis_info_show(vis, "Invalid number: %s", argv[1]);
			return false;
		}

		if (integer.unparsed.length > 0) {
			str8 sunit = str8_skip_space(integer.unparsed);

			if (sunit.length > 0) {
				switch (sunit.data[0]) {
				case 'd': count *= 24; /* fall through */
				case 'h': count *= 60; /* fall through */
				case 'm': count *= 60; /* fall through */
				case 's': break;
				default:
					vis_info_show(vis, "Unknown time specifier (use: s,m,h or d)");
					return false;
				}
			}
			unit = (char *)sunit.data;

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

	struct tm tm;
	time_t state = text_state(txt);
	char buf[32];
	strftime(buf, sizeof buf, "State from %H:%M", localtime_r(&state, &tm));
	vis_info_show(vis, "%s", buf);

	return pos != EPOS;
}

static int space_replace(char *dest, const char *src, size_t dlen) {
	int invisiblebytes = 0;
	size_t i, size = LENGTH("␣") - 1;
	for (i = 0; *src && i < dlen; src++) {
		if (*src == ' ' && i < dlen - size - 1) {
			memcpy(&dest[i], "␣", size);
			i += size;
			invisiblebytes += size - 1;
		} else {
			dest[i] = *src;
			i++;
		}
	}
	dest[i] = '\0';
	return invisiblebytes;
}

static bool print_keylayout(const char *key, void *value, void *data)
{
	Vis  *vis = ((void **)data)[0];
	Text *txt = ((void **)data)[1];
	char buf[64];
	int invisiblebytes = space_replace(buf, key, sizeof(buf));
	return text_appendf(vis, txt, "  %-*s\t%s\n", 18+invisiblebytes, buf, (char*)value);
}

static bool print_keybinding(const char *key, void *value, void *data)
{
	KeyBinding *binding = value;
	const char *desc = binding->alias;
	if (!desc && binding->action)
		desc = VIS_HELP_USE(binding->action->help);

	Vis  *vis = ((void **)data)[0];
	Text *txt = ((void **)data)[1];
	char buf[64];
	int invisiblebytes = space_replace(buf, key, sizeof(buf));
	return text_appendf(vis, txt, "  %-*s\t%s\n", 18+invisiblebytes, buf, desc ? desc : "");
}

static void print_mode(Vis *vis, Text *txt, Mode *mode)
{
	if (!map_empty(mode->bindings))
		text_appendf(vis, txt, "\n %s\n\n", mode->name);
	void *data[2] = {vis, txt};
	map_iterate(mode->bindings, print_keybinding, data);
}

static bool print_action(const char *key, void *value, void *data)
{
	const char *help = VIS_HELP_USE(((KeyAction*)value)->help);
	Vis  *vis = ((void **)data)[0];
	Text *txt = ((void **)data)[1];
	return text_appendf(vis, txt, "  %-30s\t%s\n", key, help ? help : "");
}

static bool print_cmd(const char *key, void *value, void *data)
{
	CommandDef *cmd = value;
	const char *help = VIS_HELP_USE(cmd->help);
	char usage[256];
	Vis  *vis = ((void **)data)[0];
	Text *txt = ((void **)data)[1];
	snprintf(usage, sizeof usage, "%s%s%s%s%s%s%s",
	         cmd->name,
	         (cmd->flags & CMD_FORCE) ? "[!]" : "",
	         (cmd->flags & CMD_TEXT) ? "/text/" : "",
	         (cmd->flags & CMD_REGEX) ? "/regexp/" : "",
	         (cmd->flags & CMD_CMD) ? " command" : "",
	         (cmd->flags & CMD_SHELL) ? (!strcmp(cmd->name, "s") ? "/regexp/text/" : " shell-command") : "",
	         (cmd->flags & CMD_ARGV) ? " [args...]" : "");
	return text_appendf(vis, txt, "  %-30s %s\n", usage, help ? help : "");
}

static bool print_cmd_name(const char *key, void *value, void *data) {
	CommandDef *cmd = value;
	bool result = buffer_append(data, cmd->name, strlen(cmd->name));
	return result && buffer_append(data, "\n", 1);
}

void vis_print_cmds(Vis *vis, Buffer *buf, const char *prefix) {
	map_iterate(map_prefix(vis->cmds, prefix), print_cmd_name, buf);
}

static bool print_option(const char *key, void *value, void *data)
{
	char desc[256];
	const VisOption *opt = value;
	const char *help = VIS_HELP_USE(opt->help);
	if (strcmp(key, opt->names[0]))
		return true;
	snprintf(desc, sizeof desc, "%s%s%s%s%s",
	         opt->names[0],
	         opt->names[1] ? "|" : "",
	         opt->names[1] ? opt->names[1] : "",
	         opt->flags & VIS_OPTION_TYPE_BOOL ? " on|off" : "",
	         opt->flags & VIS_OPTION_TYPE_NUMBER ? " nn" : "");
	Vis  *vis = ((void **)data)[0];
	Text *txt = ((void **)data)[1];
	return text_appendf(vis, txt, "  %-30s %s\n", desc, help ? help : "");
}

static void print_symbolic_keys(Vis *vis, Text *txt)
{
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

	TermKey *termkey = vis->ui.termkey;
	text_appendf(vis, txt, "  ␣ (a literal \" \" space symbol must be used to refer to <Space>)\n");
	for (size_t i = 0; i < LENGTH(keys); i++) {
		text_appendf(vis, txt, "  <%s>\n", termkey_get_keyname(termkey, keys[i]));
	}
}

static bool cmd_help(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!vis_window_new(vis, NULL))
		return false;

	Text *txt = vis->win->file->text;
	void *map_data[2] = {vis, txt};

	text_appendf(vis, txt, "vis %s (PID: %ld)\n\n", VERSION, (long)getpid());

	text_appendf(vis, txt, " Modes\n\n");
	for (int i = 0; i < LENGTH(vis_modes); i++) {
		Mode *mode = &vis_modes[i];
		if (mode->help)
			text_appendf(vis, txt, "  %-18s\t%s\n", mode->name, mode->help);
	}

	if (!map_empty(vis->keymap)) {
		text_appendf(vis, txt, "\n Layout specific mappings (affects all modes except INSERT/REPLACE)\n\n");
		map_iterate(vis->keymap, print_keylayout, map_data);
	}

	print_mode(vis, txt, &vis_modes[VIS_MODE_NORMAL]);
	print_mode(vis, txt, &vis_modes[VIS_MODE_OPERATOR_PENDING]);
	print_mode(vis, txt, &vis_modes[VIS_MODE_VISUAL]);
	print_mode(vis, txt, &vis_modes[VIS_MODE_INSERT]);

	text_appendf(vis, txt, "\n :-Commands\n\n");
	map_iterate(vis->cmds, print_cmd, map_data);

	text_appendf(vis, txt, "\n Marks\n\n");
	text_appendf(vis, txt, "  a-z General purpose marks\n");
	for (size_t i = 0; i < LENGTH(vis_marks); i++) {
		const char *help = VIS_HELP_USE(vis_marks[i].help);
		text_appendf(vis, txt, "  %c   %s\n", vis_marks[i].name, help ? help : "");
	}

	text_appendf(vis, txt, "\n Registers\n\n");
	text_appendf(vis, txt, "  a-z General purpose registers\n");
	text_appendf(vis, txt, "  A-Z Append to corresponding general purpose register\n");
	for (size_t i = 0; i < LENGTH(vis_registers); i++) {
		const char *help = VIS_HELP_USE(vis_registers[i].help);
		text_appendf(vis, txt, "  %c   %s\n", vis_registers[i].name, help ? help : "");
	}

	text_appendf(vis, txt, "\n :set command options\n\n");
	map_iterate(vis->options, print_option, map_data);

	text_appendf(vis, txt, "\n Key binding actions\n\n");
	map_iterate(vis->actions, print_action, map_data);

	text_appendf(vis, txt, "\n Symbolic keys usable for key bindings "
		"(prefix with C-, S-, and M- for Ctrl, Shift and Alt respectively)\n\n");
	print_symbolic_keys(vis, txt);

	char *paths[] = { NULL, NULL };
	char *paths_description[] = {
		"Lua paths used to load runtime files (? will be replaced by filename):",
		"Lua paths used to load C libraries (? will be replaced by filename):",
	};

	if (vis_lua_paths_get(vis, &paths[0], &paths[1])) {
		for (size_t i = 0; i < LENGTH(paths); i++) {
			text_appendf(vis, txt, "\n %s\n\n", paths_description[i]);
			for (char *elem = paths[i], *next; elem; elem = next) {
				if ((next = strstr(elem, ";")))
					*next++ = '\0';
				if (*elem)
					text_appendf(vis, txt, "  %s\n", elem);
			}
			free(paths[i]);
		}
	}

	text_appendf(vis, txt, "\n Compile time configuration\n\n");

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
		text_appendf(vis, txt, "  %-32s\t%s\n", configs[i].name, configs[i].enabled ? "yes" : "no");

	text_mark_current_revision(txt);
	view_cursors_to(vis->win->view.selection, 0);

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
