/* :set command options */
typedef struct {
	const char *names[3];            /* name and optional alias */
	VisOptionFlags flags;            /* option type, etc. */
	VIS_HELP_DECL(const char *help;) /* short, one line help text */
	VisOptionSetFunction *set;       /* option handler, NULL for builtins */
	VisOptionGetFunction *get;       /* option handler, NULL for builtins */
	void *set_context;               /* context passed to option handler function */
	void *get_context;               /* context passed to option handler function */
} VisOption;

enum {
	OPTION_SHELL,
	OPTION_ESCDELAY,
	OPTION_AUTOINDENT,
	OPTION_EXPANDTAB,
	OPTION_TABWIDTH,
	OPTION_SHOW_SPACES,
	OPTION_SHOW_TABS,
	OPTION_SHOW_NEWLINES,
	OPTION_SHOW_EOF,
	OPTION_STATUSBAR,
	OPTION_NUMBER,
	OPTION_NUMBER_RELATIVE,
	OPTION_NUMBER_WIDTH,
	OPTION_CURSOR_LINE,
	OPTION_COLOR_COLUMN,
	OPTION_SAVE_METHOD,
	OPTION_LOAD_METHOD,
	OPTION_CHANGE_256COLORS,
	OPTION_LAYOUT,
	OPTION_IGNORECASE,
	OPTION_BREAKAT,
	OPTION_WRAP_COLUMN,
};

static const VisOption vis_options_table[] = {
	[OPTION_SHELL] = {
		{ "shell" },
		VIS_OPTION_TYPE_STRING,
		VIS_HELP("Shell to use for external commands (default: $SHELL, /etc/passwd, /bin/sh)")
	},
	[OPTION_ESCDELAY] = {
		{ "escdelay" },
		VIS_OPTION_TYPE_NUMBER,
		VIS_HELP("Milliseconds to wait to distinguish <Escape> from terminal escape sequences")
	},
	[OPTION_AUTOINDENT] = {
		{ "autoindent", "ai" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Copy leading white space from previous line")
	},
	[OPTION_EXPANDTAB] = {
		{ "expandtab", "et" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Replace entered <Tab> with `tabwidth` spaces")
	},
	[OPTION_TABWIDTH] = {
		{ "tabwidth", "tw" },
		VIS_OPTION_TYPE_NUMBER|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Number of spaces to display (and insert if `expandtab` is enabled) for a tab")
	},
	[OPTION_SHOW_SPACES] = {
		{ "showspaces" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol instead of a space")
	},
	[OPTION_SHOW_TABS] = {
		{ "showtabs" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol for tabs")
	},
	[OPTION_SHOW_NEWLINES] = {
		{ "shownewlines" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol for newlines")
	},
	[OPTION_SHOW_EOF] = {
		{ "showeof" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol for lines after the end of the file")
	},
	[OPTION_STATUSBAR] = {
		{ "statusbar", "sb" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display status bar")
	},
	[OPTION_NUMBER] = {
		{ "numbers", "nu" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display absolute line numbers")
	},
	[OPTION_NUMBER_RELATIVE] = {
		{ "relativenumbers", "rnu" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display relative line numbers")
	},
	[OPTION_NUMBER_WIDTH] = {
		{ "numberwidth", "nuw" },
		VIS_OPTION_TYPE_NUMBER|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Minimum sidebar width")
	},
	[OPTION_CURSOR_LINE] = {
		{ "cursorline", "cul" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Highlight current cursor line")
	},
	[OPTION_COLOR_COLUMN] = {
		{ "colorcolumn", "cc" },
		VIS_OPTION_TYPE_NUMBER|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Highlight a fixed column")
	},
	[OPTION_SAVE_METHOD] = {
		{ "savemethod" },
		VIS_OPTION_TYPE_STRING|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Save method to use for current file 'auto', 'atomic' or 'inplace'")
	},
	[OPTION_LOAD_METHOD] = {
		{ "loadmethod" },
		VIS_OPTION_TYPE_STRING,
		VIS_HELP("How to load existing files 'auto', 'read' or 'mmap'")
	},
	[OPTION_CHANGE_256COLORS] = {
		{ "change256colors" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Change 256 color palette to support 24bit colors")
	},
	[OPTION_LAYOUT] = {
		{ "layout" },
		// NOTE(rnp): layout technically wants an int but also has semantic names.
		// lua can pass the int directly but :set will use the string name
		VIS_OPTION_TYPE_STRING|VIS_OPTION_TYPE_NUMBER,
		VIS_HELP("Vertical or horizontal window layout")
	},
	[OPTION_IGNORECASE] = {
		{ "ignorecase", "ic" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Ignore case when searching")
	},
	[OPTION_BREAKAT] = {
		{ "breakat", "brk" },
		VIS_OPTION_TYPE_STRING|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Characters which might cause a word wrap")
	},
	[OPTION_WRAP_COLUMN] = {
		{ "wrapcolumn", "wc" },
		VIS_OPTION_TYPE_NUMBER|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Wrap lines at minimum of window width and wrapcolumn")
	},
};

VIS_INTERNAL void
vis_option_free(VisOption *opt)
{
	if (!opt || Between(opt, vis_options_table, (vis_options_table + countof(vis_options_table) - 1)))
		return;

	for (const char **name = opt->names; *name; name++)
		free((char*)*name);
	free(VIS_HELP_USE((char*)opt->help));
	free(opt);
}

VIS_EXPORT bool
vis_option_register(Vis *vis, const char *names[], VisOptionFlags flags,
                    VisOptionSetFunction *set, VisOptionGetFunction *get,
                    void *set_context, void *get_context, const char *help)
{
	if (!names || !names[0])
		return false;

	for (const char **name = names; *name; name++) {
		if (map_get(vis->options, *name))
			return false;
	}
	VisOption *opt = calloc(1, sizeof *opt);
	if (!opt)
		return false;
	for (size_t i = 0; i < countof(opt->names)-1 && names[i]; i++) {
		if (!(opt->names[i] = strdup(names[i])))
			goto err;
	}
	opt->flags       = flags;
	opt->set         = set;
	opt->get         = get;
	opt->set_context = set_context;
	opt->get_context = get_context;
#if CONFIG_HELP
	if (help && !(opt->help = strdup(help)))
		goto err;
#endif
	for (const char **name = names; *name; name++)
		map_put(vis->options, *name, opt);
	return true;
err:
	vis_option_free(opt);
	return false;
}

VIS_EXPORT bool
vis_option_unregister(Vis *vis, const char *name)
{
	VisOption *opt = map_get(vis->options, name);
	if (!opt)
		return false;
	for (const char **alias = opt->names; *alias; alias++) {
		if (!map_delete(vis->options, *alias))
			return false;
	}
	vis_option_free(opt);
	return true;
}

VIS_INTERNAL VisOption *
vis_option_from_string(Vis *vis, str8 string)
{
	// TODO(rnp): this is pure c brain damage. make map system use strings with length
	char name[256];

	ptrdiff_t length = MIN(countof(name) - 1, string.length);
	memcpy(name, string.data, length);
	name[length] = 0;

	VisOption *result = map_closest(vis->options, name);
	return result;
}

VIS_EXPORT void
vis_shell_set(Vis *vis, const char *new_shell)
{
	char *shell =  strdup(new_shell);
	if (!shell) {
		vis_info_show(vis, "Failed to change shell");
	} else {
		free(vis->shell);
		vis->shell = shell;
	}
}

VIS_INTERNAL bool
vis_option_set(Vis *vis, Win *win, VisOption *option, VisValue value, bool toggle)
{
	// NOTE(rnp): the switch statement below forces the compiler to clamp this result
	ptrdiff_t option_index = option - vis_options_table;

	bool result = true;
	switch (option_index) {
	case OPTION_AUTOINDENT:{       vis->autoindent = toggle ? !vis->autoindent : value.u.boolean;       }break;
	case OPTION_CHANGE_256COLORS:{ vis->change_colors = toggle ? !vis->change_colors : value.u.boolean; }break;
	case OPTION_COLOR_COLUMN:{     win->view.colorcolumn = MAX(value.u.integer, 0);                     }break;
	case OPTION_ESCDELAY:{         termkey_set_waittime(vis->ui.termkey, value.u.integer);              }break;
	case OPTION_EXPANDTAB:{        win->expandtab = toggle ? !win->expandtab : value.u.boolean;         }break;
	case OPTION_IGNORECASE:{       vis->ignorecase = toggle ? !vis->ignorecase : value.u.boolean;       }break;
	case OPTION_NUMBER_WIDTH:{     win->min_sidebar_width = MAX(0, value.u.integer);                    }break;
	case OPTION_SHELL:{            vis_shell_set(vis, value.u.string);                                  }break;
	case OPTION_TABWIDTH:{         view_tabwidth_set(&win->view, value.u.integer);                      }break;
	case OPTION_WRAP_COLUMN:{      win->view.wrapcolumn = MAX(0, value.u.integer);                      }break;

	case OPTION_CURSOR_LINE:
	case OPTION_SHOW_EOF:
	case OPTION_SHOW_NEWLINES:
	case OPTION_SHOW_SPACES:
	case OPTION_SHOW_TABS:
	case OPTION_STATUSBAR:
	{
		const int values[] = {
			[OPTION_CURSOR_LINE]   = UI_OPTION_CURSOR_LINE,
			[OPTION_SHOW_EOF]      = UI_OPTION_SYMBOL_EOF,
			[OPTION_SHOW_NEWLINES] = UI_OPTION_SYMBOL_EOL,
			[OPTION_SHOW_SPACES]   = UI_OPTION_SYMBOL_SPACE,
			[OPTION_SHOW_TABS]     = UI_OPTION_SYMBOL_TAB|UI_OPTION_SYMBOL_TAB_FILL,
			[OPTION_STATUSBAR]     = UI_OPTION_STATUSBAR,
		};
		int flags = win->options;
		if (toggle) {
			flags ^=  values[option_index];
		} else if (value.u.boolean) {
			flags |=  values[option_index];
		} else {
			flags &= ~values[option_index];
		}
		win_options_set(win, flags);
	}break;

	case OPTION_NUMBER:{
		enum UiOption opt = win->options;
		if (value.u.boolean || (toggle && !(opt & UI_OPTION_LINE_NUMBERS_ABSOLUTE))) {
			opt &= ~UI_OPTION_LINE_NUMBERS_RELATIVE;
			opt |=  UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		} else {
			opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
		}
		win_options_set(win, opt);
	}break;

	case OPTION_NUMBER_RELATIVE:{
		enum UiOption opt = win->options;
		if (value.u.boolean || (toggle && !(opt & UI_OPTION_LINE_NUMBERS_RELATIVE))) {
			opt &= ~UI_OPTION_LINE_NUMBERS_ABSOLUTE;
			opt |=  UI_OPTION_LINE_NUMBERS_RELATIVE;
		} else {
			opt &= ~UI_OPTION_LINE_NUMBERS_RELATIVE;
		}
		win_options_set(win, opt);
	}break;

	case OPTION_SAVE_METHOD:{
		if (strcmp("auto", value.u.string) == 0) {
			win->file->save_method = TEXT_SAVE_AUTO;
		} else if (strcmp("atomic", value.u.string) == 0) {
			win->file->save_method = TEXT_SAVE_ATOMIC;
		} else if (strcmp("inplace", value.u.string) == 0) {
			win->file->save_method = TEXT_SAVE_INPLACE;
		} else {
			vis_info_show(vis, "Invalid save method `%s', expected "
			              "'auto', 'atomic' or 'inplace'", value.u.string);
			return false;
		}
	}break;

	case OPTION_LOAD_METHOD:{
		if (strcmp("auto", value.u.string) == 0) {
			vis->load_method = TEXT_LOAD_AUTO;
		} else if (strcmp("read", value.u.string) == 0) {
			vis->load_method = TEXT_LOAD_READ;
		} else if (strcmp("mmap", value.u.string) == 0) {
			vis->load_method = TEXT_LOAD_MMAP;
		} else {
			vis_info_show(vis, "Invalid load method `%s', expected "
			              "'auto', 'read' or 'mmap'", value.u.string);
			result = false;
		}
	}break;

	case OPTION_LAYOUT:{
		enum UiLayout layout;
		if (value.kind == VisValueKind_String) {
			if (strcmp("h", value.u.string) == 0) {
				layout = UI_LAYOUT_HORIZONTAL;
			} else if (strcmp("v", value.u.string) == 0) {
				layout = UI_LAYOUT_VERTICAL;
			} else {
				vis_info_show(vis, "Invalid layout `%s', expected 'h' or 'v'", value.u.string);
				result = false;
			}
		} else {
			assert(value.kind == VisValueKind_Integer);
			layout = Clamp(value.u.integer, 0, UI_LAYOUT_COUNT - 1);
		}
		if (result) ui_arrange(&vis->ui, layout);
	}break;

	case OPTION_BREAKAT:{
		if (!view_breakat_set(&win->view, value.u.string)) {
			vis_info_show(vis, "Failed to set breakat");
			result = false;
		}
	}break;

	default:{
		result = option->set ? option->set(vis, win, option->names[0], option->set_context, option->flags, value, toggle)
		                     : false;
	}break;
	}

	return result;
}


VIS_INTERNAL VisValue
vis_option_get(Vis *vis, Win *win, VisOption *option)
{
	VisValue result = {0};
	if (option && (!(option->flags & VIS_OPTION_NEED_WINDOW) || win)) {
		// NOTE(rnp): the switch statement below forces the compiler to clamp this result
		ptrdiff_t option_index = option - vis_options_table;

		if (option->flags & VIS_OPTION_TYPE_NUMBER) {
			result.kind = VisValueKind_Integer;
		} else if (option->flags & VIS_OPTION_TYPE_STRING) {
			result.kind = VisValueKind_String;
		} else if (option->flags & VIS_OPTION_TYPE_BOOL) {
			result.kind = VisValueKind_Boolean;
		}

		switch (option_index) {
		case OPTION_AUTOINDENT:{       result.u.boolean = vis->autoindent;                       }break;
		case OPTION_BREAKAT:{          result.u.string  = win->view.breakat;                     }break;
		case OPTION_CHANGE_256COLORS:{ result.u.boolean = vis->change_colors;                    }break;
		case OPTION_COLOR_COLUMN:{     result.u.integer = win->view.colorcolumn;                 }break;
		case OPTION_ESCDELAY:{         result.u.integer = termkey_get_waittime(vis->ui.termkey); }break;
		case OPTION_EXPANDTAB:{        result.u.boolean = win->expandtab;                        }break;
		case OPTION_IGNORECASE:{       result.u.boolean = vis->ignorecase;                       }break;
		case OPTION_LAYOUT:{           result.u.integer = vis->ui.layout;                        }break;
		case OPTION_NUMBER_WIDTH:{     result.u.integer = win->min_sidebar_width;                }break;
		case OPTION_SHELL:{            result.u.string  = vis->shell;                            }break;
		case OPTION_TABWIDTH:{         result.u.integer = win->view.tabwidth;                    }break;
		case OPTION_WRAP_COLUMN:{      result.u.integer = win->view.wrapcolumn;                  }break;

		case OPTION_CURSOR_LINE:
		case OPTION_NUMBER:
		case OPTION_NUMBER_RELATIVE:
		case OPTION_SHOW_EOF:
		case OPTION_SHOW_NEWLINES:
		case OPTION_SHOW_SPACES:
		case OPTION_SHOW_TABS:
		case OPTION_STATUSBAR:
		{
			const int values[] = {
				[OPTION_NUMBER]          = UI_OPTION_LINE_NUMBERS_ABSOLUTE,
				[OPTION_NUMBER_RELATIVE] = UI_OPTION_LINE_NUMBERS_RELATIVE,
				[OPTION_CURSOR_LINE]     = UI_OPTION_CURSOR_LINE,
				[OPTION_SHOW_EOF]        = UI_OPTION_SYMBOL_EOF,
				[OPTION_SHOW_NEWLINES]   = UI_OPTION_SYMBOL_EOL,
				[OPTION_SHOW_SPACES]     = UI_OPTION_SYMBOL_SPACE,
				[OPTION_SHOW_TABS]       = UI_OPTION_SYMBOL_TAB|UI_OPTION_SYMBOL_TAB_FILL,
				[OPTION_STATUSBAR]       = UI_OPTION_STATUSBAR,
			};
			result.u.boolean = (win->options & values[option_index]) != 0;
		}break;

		case OPTION_SAVE_METHOD:{
			switch (win->file->save_method) {
			case TEXT_SAVE_AUTO:{   result.u.string = "auto";   }break;
			case TEXT_SAVE_ATOMIC:{ result.u.string = "atomic"; }break;
			case TEXT_SAVE_INPLACE:{result.u.string = "inplace";}break;
			}
		}break;

		case OPTION_LOAD_METHOD:{
			switch (vis->load_method) {
			case TEXT_LOAD_AUTO:{result.u.string = "auto";}break;
			case TEXT_LOAD_READ:{result.u.string = "read";}break;
			case TEXT_LOAD_MMAP:{result.u.string = "mmap";}break;
			}
		}break;

		default:{
			result = option->get ? option->get(vis, win, option->names[0], option->get_context, option->flags)
			                     : (VisValue){0};
		}break;
		}
	}
	return result;
}
