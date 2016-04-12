/* this file is included from sam.c */

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

static void textwidth_set(Vis *vis, int textwidth) {
	if (textwidth < 0)
		return;
	vis->textwidth = textwidth;
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

static bool cmd_set(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {

	typedef struct {
		const char *names[3];
		enum {
			OPTION_TYPE_STRING,
			OPTION_TYPE_BOOL,
			OPTION_TYPE_NUMBER,
		} type;
		bool optional;
		int index;
	} OptionDef;

	enum {
		OPTION_AUTOINDENT,
		OPTION_EXPANDTAB,
		OPTION_TABWIDTH,
		OPTION_TEXTWIDTH,
		OPTION_SYNTAX,
		OPTION_SHOW,
		OPTION_NUMBER,
		OPTION_NUMBER_RELATIVE,
		OPTION_CURSOR_LINE,
		OPTION_THEME,
		OPTION_COLOR_COLUMN,
	};

	/* definitions have to be in the same order as the enum above */
	static OptionDef options[] = {
		[OPTION_AUTOINDENT]      = { { "autoindent", "ai"       }, OPTION_TYPE_BOOL   },
		[OPTION_EXPANDTAB]       = { { "expandtab", "et"        }, OPTION_TYPE_BOOL   },
		[OPTION_TABWIDTH]        = { { "tabwidth", "tw"         }, OPTION_TYPE_NUMBER },
		[OPTION_TEXTWIDTH]       = { { "textwidth"              }, OPTION_TYPE_NUMBER },
		[OPTION_SYNTAX]          = { { "syntax"                 }, OPTION_TYPE_STRING, true },
		[OPTION_SHOW]            = { { "show"                   }, OPTION_TYPE_STRING },
		[OPTION_NUMBER]          = { { "numbers", "nu"          }, OPTION_TYPE_BOOL   },
		[OPTION_NUMBER_RELATIVE] = { { "relativenumbers", "rnu" }, OPTION_TYPE_BOOL   },
		[OPTION_CURSOR_LINE]     = { { "cursorline", "cul"      }, OPTION_TYPE_BOOL   },
		[OPTION_THEME]           = { { "theme"                  }, OPTION_TYPE_STRING },
		[OPTION_COLOR_COLUMN]    = { { "colorcolumn", "cc"      }, OPTION_TYPE_NUMBER },
	};

	if (!vis->options) {
		if (!(vis->options = map_new()))
			return false;
		for (int i = 0; i < LENGTH(options); i++) {
			options[i].index = i;
			for (const char **name = options[i].names; *name; name++) {
				if (!map_put(vis->options, *name, &options[i]))
					return false;
			}
		}
	}

	if (!argv[1]) {
		vis_info_show(vis, "Expecting: set option [value]");
		return false;
	}

	if (!win) {
		vis_info_show(vis, "Need active window for :set command");
		return false;
	}

	Arg arg;
	bool invert = false;
	OptionDef *opt = NULL;

	if (!strncasecmp(argv[1], "no", 2)) {
		opt = map_closest(vis->options, argv[1]+2);
		if (opt && opt->type == OPTION_TYPE_BOOL)
			invert = true;
		else
			opt = NULL;
	}

	if (!opt)
		opt = map_closest(vis->options, argv[1]);
	if (!opt) {
		vis_info_show(vis, "Unknown option: `%s'", argv[1]);
		return false;
	}

	switch (opt->type) {
	case OPTION_TYPE_STRING:
		if (!opt->optional && !argv[2]) {
			vis_info_show(vis, "Expecting string option value");
			return false;
		}
		arg.s = argv[2];
		break;
	case OPTION_TYPE_BOOL:
		if (!argv[2]) {
			arg.b = true;
		} else if (!parse_bool(argv[2], &arg.b)) {
			vis_info_show(vis, "Expecting boolean option value not: `%s'", argv[2]);
			return false;
		}
		if (invert)
			arg.b = !arg.b;
		break;
	case OPTION_TYPE_NUMBER:
		if (!argv[2]) {
			vis_info_show(vis, "Expecting number");
			return false;
		}
		/* TODO: error checking? long type */
		arg.i = strtoul(argv[2], NULL, 10);
		break;
	}

	switch (opt->index) {
	case OPTION_EXPANDTAB:
		vis->expandtab = arg.b;
		break;
	case OPTION_AUTOINDENT:
		vis->autoindent = arg.b;
		break;
	case OPTION_TABWIDTH:
		tabwidth_set(vis, arg.i);
		break;
	case OPTION_TEXTWIDTH:
		textwidth_set(vis, arg.i);
		break;
	case OPTION_SYNTAX:
		if (!argv[2]) {
			const char *syntax = view_syntax_get(win->view);
			if (syntax)
				vis_info_show(vis, "Syntax definition in use: `%s'", syntax);
			else
				vis_info_show(vis, "No syntax definition in use");
			return true;
		}

		if (parse_bool(argv[2], &arg.b) && !arg.b)
			return view_syntax_set(win->view, NULL);
		if (!view_syntax_set(win->view, argv[2])) {
			vis_info_show(vis, "Unknown syntax definition: `%s'", argv[2]);
			return false;
		}
		break;
	case OPTION_SHOW:
		if (!argv[2]) {
			vis_info_show(vis, "Expecting: spaces, tabs, newlines");
			return false;
		}
		const char *keys[] = { "spaces", "tabs", "newlines" };
		const int values[] = {
			UI_OPTION_SYMBOL_SPACE,
			UI_OPTION_SYMBOL_TAB|UI_OPTION_SYMBOL_TAB_FILL,
			UI_OPTION_SYMBOL_EOL,
		};
		int flags = view_options_get(win->view);
		for (const char **args = &argv[2]; *args; args++) {
			for (int i = 0; i < LENGTH(keys); i++) {
				if (strcmp(*args, keys[i]) == 0) {
					flags |= values[i];
				} else if (strstr(*args, keys[i]) == *args) {
					bool show;
					const char *v = *args + strlen(keys[i]);
					if (*v == '=' && parse_bool(v+1, &show)) {
						if (show)
							flags |= values[i];
						else
							flags &= ~values[i];
					}
				}
			}
		}
		view_options_set(win->view, flags);
		break;
	case OPTION_NUMBER: {
		enum UiOption opt = view_options_get(win->view);
		if (arg.b) {
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
		if (arg.b) {
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
		if (arg.b)
			opt |= UI_OPTION_CURSOR_LINE;
		else
			opt &= ~UI_OPTION_CURSOR_LINE;
		view_options_set(win->view, opt);
		break;
	}
	case OPTION_THEME:
		if (!vis_theme_load(vis, arg.s)) {
			vis_info_show(vis, "Failed to load theme: `%s'", arg.s);
			return false;
		}
		break;
	case OPTION_COLOR_COLUMN:
		view_colorcolumn_set(win->view, arg.i);
		break;
	}

	return true;
}

static bool is_file_pattern(const char *pattern) {
	if (!pattern)
		return false;
	struct stat meta;
	if (stat(pattern, &meta) == 0 && S_ISDIR(meta.st_mode))
		return true;
	return strchr(pattern, '*') || strchr(pattern, '[') || strchr(pattern, '{');
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

	if (!buffer_put0(&bufcmd, "vis-open ") || !buffer_append0(&bufcmd, pattern ? pattern : ""))
		return NULL;

	Filerange empty = text_range_empty();
	int status = vis_pipe(vis, &empty, (const char*[]){ buffer_content0(&bufcmd), NULL },
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

static bool cmd_open(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!argv[1])
		return vis_window_new(vis, NULL);
	return openfiles(vis, &argv[1]);
}

static void info_unsaved_changes(Vis *vis) {
	vis_info_show(vis, "No write since last change (add ! to override)");
}

static bool cmd_edit(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Win *oldwin = win;
	if (cmd->flags != '!' && !vis_window_closable(oldwin)) {
		info_unsaved_changes(vis);
		return false;
	}
	if (!argv[1])
		return vis_window_reload(oldwin);
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

static bool has_windows(Vis *vis) {
	for (Win *win = vis->windows; win; win = win->next) {
		if (!win->file->internal)
			return true;
	}
	return false;
}

static bool cmd_quit(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (cmd->flags != '!' && !vis_window_closable(win)) {
		info_unsaved_changes(vis);
		return false;
	}
	vis_window_close(win);
	if (!has_windows(vis))
		vis_exit(vis, EXIT_SUCCESS);
	return true;
}

static bool cmd_bdelete(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Text *txt = win->file->text;
	if (text_modified(txt) && cmd->flags != '!') {
		info_unsaved_changes(vis);
		return false;
	}
	for (Win *next, *win = vis->windows; win; win = next) {
		next = win->next;
		if (win->file->text == txt)
			vis_window_close(win);
	}
	if (!has_windows(vis))
		vis_exit(vis, EXIT_SUCCESS);
	return true;
}

static bool cmd_qall(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
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

static bool cmd_split(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	enum UiOption options = view_options_get(win->view);
	windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	if (!argv[1])
		return vis_window_split(win);
	bool ret = openfiles(vis, &argv[1]);
	if (ret)
		view_options_set(vis->win->view, options);
	return ret;
}

static bool cmd_vsplit(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	enum UiOption options = view_options_get(win->view);
	windows_arrange(vis, UI_LAYOUT_VERTICAL);
	if (!argv[1])
		return vis_window_split(win);
	bool ret = openfiles(vis, &argv[1]);
	if (ret)
		view_options_set(vis->win->view, options);
	return ret;
}

static bool cmd_new(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	windows_arrange(vis, UI_LAYOUT_HORIZONTAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_vnew(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	windows_arrange(vis, UI_LAYOUT_VERTICAL);
	return vis_window_new(vis, NULL);
}

static bool cmd_wq(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	File *file = win->file;
	bool unmodified = !file->is_stdin && !file->name && !text_modified(file->text);
	if (unmodified || cmd_write(vis, win, cmd, argv, cur, range))
		return cmd_quit(vis, win, cmd, argv, cur, range);
	return false;
}

static bool cmd_earlier_later(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
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
		if (argv[0][0] == 'e')
			pos = text_earlier(txt, count);
		else
			pos = text_later(txt, count);
	}

	time_t state = text_state(txt);
	char buf[32];
	strftime(buf, sizeof buf, "State from %H:%M", localtime(&state));
	vis_info_show(vis, "%s", buf);

	return pos != EPOS;
}

static bool print_keylayout(const char *key, void *value, void *data) {
	return text_appendf(data, "  %-15s\t%s\n", key, (char*)value);
}

static bool print_keybinding(const char *key, void *value, void *data) {
	Text *txt = data;
	KeyBinding *binding = value;
	const char *desc = binding->alias;
	if (!desc && binding->action)
		desc = binding->action->help;
	return text_appendf(txt, "  %-15s\t%s\n", key[0] == ' ' ? "<Space>" : key, desc ? desc : "");
}

static void print_mode(Mode *mode, Text *txt) {
	if (!map_empty(mode->bindings))
		text_appendf(txt, "\n %s\n\n", mode->name);
	map_iterate(mode->bindings, print_keybinding, txt);
}

static bool print_action(const char *key, void *value, void *data) {
	Text *txt = data;
	KeyAction *action = value;
	return text_appendf(txt, "  %-30s\t%s\n", key, action->help);
}

static bool cmd_help(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!vis_window_new(vis, NULL))
		return false;

	Text *txt = vis->win->file->text;

	text_appendf(txt, "vis %s\n\n", VERSION);

	text_appendf(txt, " Modes\n\n");
	for (int i = 0; i < LENGTH(vis_modes); i++) {
		Mode *mode = &vis_modes[i];
		if (mode->help)
			text_appendf(txt, "  %-15s\t%s\n", mode->name, mode->help);
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
	for (const CommandDef *cmd = cmds; cmd && cmd->name[0]; cmd++)
		text_appendf(txt, "  %s\n", cmd->name[0]);

	text_appendf(txt, "\n Key binding actions\n\n");
	map_iterate(vis->actions, print_action, txt);

	text_save(txt, NULL);
	return true;
}

static enum VisMode str2vismode(const char *mode) {
	const char *modes[] = {
		[VIS_MODE_NORMAL] = "normal",
		[VIS_MODE_OPERATOR_PENDING] = "operator-pending",
		[VIS_MODE_VISUAL] = "visual",
		[VIS_MODE_VISUAL_LINE] = "visual-line",
		[VIS_MODE_INSERT] = "insert",
		[VIS_MODE_REPLACE] = "replace",
	};

	for (size_t i = 0; i < LENGTH(modes); i++) {
		if (mode && modes[i] && strcmp(mode, modes[i]) == 0)
			return i;
	}
	return VIS_MODE_INVALID;
}

static bool cmd_langmap(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
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

static bool cmd_map(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool local = strstr(argv[0], "-") != NULL;
	enum VisMode mode = str2vismode(argv[1]);
	const char *lhs = argv[2];
	const char *rhs = argv[3];

	if (mode == VIS_MODE_INVALID || !lhs || !rhs) {
		vis_info_show(vis, "usage: map mode lhs rhs\n");
		return false;
	}

	KeyBinding *binding = calloc(1, sizeof *binding);
	if (!binding)
		return false;
	if (rhs[0] == '<') {
		const char *next = vis_keys_next(vis, rhs);
		if (next && next[-1] == '>') {
			const char *start = rhs + 1;
			const char *end = next - 1;
			char key[64];
			if (end > start && end - start - 1 < (ptrdiff_t)sizeof key) {
				memcpy(key, start, end - start);
				key[end - start] = '\0';
				binding->action = map_get(vis->actions, key);
			}
		}
	}

	if (!binding->action) {
		binding->alias = strdup(rhs);
		if (!binding->alias) {
			free(binding);
			return false;
		}
	}

	bool mapped;
	if (local)
		mapped = vis_window_mode_map(win, mode, lhs, binding);
	else
		mapped = vis_mode_map(vis, mode, lhs, binding);

	if (!mapped && cmd->flags == '!') {
		if (local) {
			mapped = vis_window_mode_unmap(win, mode, lhs) &&
			         vis_window_mode_map(win, mode, lhs, binding);
		} else {
			mapped = vis_mode_unmap(vis, mode, lhs) &&
			         vis_mode_map(vis, mode, lhs, binding);
		}
	}

	if (!mapped)
		free(binding);
	return mapped;
}

static bool cmd_unmap(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool local = strstr(argv[0], "-") != NULL;
	enum VisMode mode = str2vismode(argv[1]);
	const char *lhs = argv[2];

	if (mode == VIS_MODE_INVALID || !lhs) {
		vis_info_show(vis, "usage: unmap mode lhs rhs\n");
		return false;
	}

	if (local)
		return vis_window_mode_unmap(win, mode, lhs);
	else
		return vis_mode_unmap(vis, mode, lhs);
}
