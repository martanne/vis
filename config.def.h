#define ESC        0x1B
#define NONE(k)    { .str = { k }, .code = 0 }
#define KEY(k)     { .str = { '\0' }, .code = KEY_##k }
#define CONTROL(k) NONE((k)&0x1F)
#define META(k)    { .str = { ESC, (k) }, .code = 0 }
#define BACKSPACE(func, arg) \
	{ { KEY(BACKSPACE) }, (func), { .m = (arg) } }, \
	{ { CONTROL('H') },   (func), { .m = (arg) } }, \
	{ { NONE(127) },      (func), { .m = (arg) } }, \
	{ { CONTROL('B') },   (func), { .m = (arg) } }

/* draw a statubar, do whatever you want with the given curses window */
static void statusbar(WINDOW *win, bool active, const char *filename, int line, int col) {
	int width, height;
	getmaxyx(win, height, width);
	(void)height;
	wattrset(win, active ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win, 0, 0, ' ', width);
	mvwprintw(win, 0, 0, "%s", filename);
	char buf[width + 1];
	int len = snprintf(buf, width, "%d, %d", line, col);
	if (len > 0) {
		buf[len] = '\0';
		mvwaddstr(win, 0, width - len - 1, buf);
	}
}

static void switchmode(const Arg *arg);
enum {
	VIS_MODE_BASIC,
	VIS_MODE_MOVE,
	VIS_MODE_NORMAL,
	VIS_MODE_VISUAL,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE, 
};

void quit(const Arg *arg) {
	endwin();
	exit(0);
}

static void split(const Arg *arg) {
	editor_window_split(editor, arg->s);
}

static void mark_set(const Arg *arg) {
	editor_mark_set(editor, arg->i, editor_cursor_get(editor));
}

static void mark_goto(const Arg *arg) {
	editor_mark_goto(editor, arg->i);
}
/* use vim's  
   :help motion
   :h operator
   :h text-objects
 as reference 
*/

static KeyBinding movement[] = {
	{ { KEY(LEFT)               }, cursor,   { .m = editor_char_prev       } },
	{ { KEY(RIGHT)              }, cursor,   { .m = editor_char_next       } },
	{ { KEY(UP)                 }, cursor,   { .m = editor_line_up         } },
	{ { KEY(DOWN)               }, cursor,   { .m = editor_line_down       } },
	{ { KEY(PPAGE)              }, cursor,   { .m = editor_page_up         } },
	{ { KEY(NPAGE)              }, cursor,   { .m = editor_page_down       } },
	{ { KEY(HOME)               }, cursor,   { .m = editor_line_start      } },
	{ { KEY(END)                }, cursor,   { .m = editor_line_end        } },
	// temporary until we have a way to enter user commands
	{ { CONTROL('c')            }, quit,                                   },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_movement[] = {
	BACKSPACE(                     cursor,          editor_char_prev         ),
	{ { NONE(' ')               }, cursor,   { .m = editor_char_next       } },
	{ { CONTROL('w'), NONE('c') }, split,    { .s = NULL                   } },
	{ { CONTROL('w'), NONE('j') }, call,     { .f = editor_window_next     } },
	{ { CONTROL('w'), NONE('k') }, call,     { .f = editor_window_prev     } },
	{ { NONE('h')               }, cursor,   { .m = editor_char_prev       } },
	{ { NONE('l')               }, cursor,   { .m = editor_char_next       } },
	{ { NONE('k')               }, cursor,   { .m = editor_line_up         } },
	{ { CONTROL('P')            }, cursor,   { .m = editor_line_up         } },
	{ { NONE('j')               }, cursor,   { .m = editor_line_down       } },
	{ { CONTROL('J')            }, cursor,   { .m = editor_line_down       } },
	{ { CONTROL('N')            }, cursor,   { .m = editor_line_down       } },
	{ { KEY(ENTER)              }, cursor,   { .m = editor_line_down       } },
	{ { NONE('0')               }, cursor,   { .m = editor_line_begin      } },
	{ { NONE('^')               }, cursor,   { .m = editor_line_start      } },
	{ { NONE('g'), NONE('_')    }, cursor,   { .m = editor_line_finish     } },
	{ { NONE('$')               }, cursor,   { .m = editor_line_end        } },
	{ { CONTROL('F')            }, cursor,   { .m = editor_page_up         } },
	{ { CONTROL('B')            }, cursor,   { .m = editor_page_down       } },
	{ { NONE('%')               }, cursor,   { .m = editor_bracket_match   } },
	{ { NONE('b')               }, cursor,   { .m = editor_word_start_prev } },
	{ { KEY(SLEFT)              }, cursor,   { .m = editor_word_start_prev } },
	{ { NONE('w')               }, cursor,   { .m = editor_word_start_next } },
	{ { KEY(SRIGHT)             }, cursor,   { .m = editor_word_start_next } },
	{ { NONE('g'), NONE('e')    }, cursor,   { .m = editor_word_end_prev   } },
	{ { NONE('e')               }, cursor,   { .m = editor_word_end_next   } },
	{ { NONE('}')               }, cursor,   { .m = editor_paragraph_next  } },
	{ { NONE('{')               }, cursor,   { .m = editor_paragraph_prev  } },
	{ { NONE(')')               }, cursor,   { .m = editor_sentence_next   } },
	{ { NONE('(')               }, cursor,   { .m = editor_sentence_prev   } },
	{ { NONE('g'), NONE('g')    }, cursor,   { .m = editor_file_begin      } },
	{ { NONE('G')               }, cursor,   { .m = editor_file_end        } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_normal[] = {
	{ { NONE('x')               }, cursor,        { .m = editor_delete   } },
	{ { NONE('i')               }, switchmode,    { .i = VIS_MODE_INSERT } },
	{ { NONE('v')               }, switchmode,    { .i = VIS_MODE_VISUAL } },
	{ { NONE('R')               }, switchmode,    { .i = VIS_MODE_REPLACE} },
	{ { NONE('u')               }, call,          { .f = editor_undo     } },
	{ { CONTROL('R')            }, call,          { .f = editor_redo     } },
	{ { CONTROL('L')            }, call,          { .f = editor_draw     } },
	// DEMO STUFF
	{ { NONE('n')               }, find_forward,  { .s = "if"            } },
	{ { NONE('p')               }, find_backward, { .s = "if"            } },
	{ { NONE('5')               }, line,          { .i = 50              } },
	{ { NONE('s')               }, mark_set,      { .i = 0               } },
	{ { NONE('9')               }, mark_goto,     { .i = 0               } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_visual[] = {
	{ { NONE(ESC)               }, switchmode,    { .i = VIS_MODE_NORMAL } },
	{ /* empty last element, array terminator */                           },
};

static void vis_visual_enter(void) {
	editor_selection_start(editor);
}

static void vis_visual_leave(void) {
	editor_selection_clear(editor);
}

static KeyBinding vis_insert[] = {
	{ { NONE(ESC)               }, switchmode,    { .i = VIS_MODE_NORMAL } },
	{ { CONTROL('D')            }, cursor,          { .m = editor_delete   } },
	BACKSPACE(                     cursor,          editor_backspace         ),
	{ /* empty last element, array terminator */                           },
};

static bool vis_insert_input(const char *str, size_t len) {
	editor_insert(editor, str, len);
	return true;
}

static KeyBinding vis_replace[] = {
	{ { NONE(ESC)               }, switchmode,   { .i = VIS_MODE_NORMAL  } },
	{ { CONTROL('D')            }, cursor,         { .m = editor_delete    } },
	BACKSPACE(                     cursor,          editor_backspace         ),
	{ /* empty last element, array terminator */                           },
};

static bool vis_replace_input(const char *str, size_t len) {
	editor_replace(editor, str, len);
	return true;
}

static Mode vis[] = {
	[VIS_MODE_BASIC] = {
		.parent = NULL,
		.bindings = movement,
	},
	[VIS_MODE_MOVE] = { 
		.parent = &vis[VIS_MODE_BASIC],
		.bindings = vis_movement,
	},
	[VIS_MODE_NORMAL] = {
		.parent = &vis[VIS_MODE_MOVE],
		.bindings = vis_normal,
	},
	[VIS_MODE_VISUAL] = {
		.name = "VISUAL",
		.parent = &vis[VIS_MODE_MOVE],
		.bindings = vis_visual,
		.enter = vis_visual_enter,
		.leave = vis_visual_leave,
	},
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.parent = &vis[VIS_MODE_BASIC],
		.bindings = vis_insert,
		.input = vis_insert_input,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.parent = &vis[VIS_MODE_BASIC],
		.bindings = vis_replace,
		.input = vis_replace_input,
	},
};

static void switchmode(const Arg *arg) {
	if (mode->leave)
		mode->leave();
	mode = &vis[arg->i];
	if (mode->enter)
		mode->enter();
	// TODO display mode name somewhere?
}

/* incomplete list of usefule but currently missing functionality from nanos help ^G:

^X      (F2)            Close the current file buffer / Exit from nano
^O      (F3)            Write the current file to disk
^J      (F4)            Justify the current paragraph

^R      (F5)            Insert another file into the current one
^W      (F6)            Search for a string or a regular expression

^K      (F9)            Cut the current line and store it in the cutbuffer
^U      (F10)           Uncut from the cutbuffer into the current line
^T      (F12)           Invoke the spell checker, if available


^_      (F13)   (M-G)   Go to line and column number
^\      (F14)   (M-R)   Replace a string or a regular expression
^^      (F15)   (M-A)   Mark text at the cursor position
M-W     (F16)           Repeat last search

M-^     (M-6)           Copy the current line and store it in the cutbuffer
M-V                     Insert the next keystroke verbatim

XXX: CONTROL(' ') = 0, ^Space                  Go forward one word
*/

/* key binding configuration */
#if 1
static KeyBinding nano_keys[] = {
	{ { CONTROL('D') },   cursor,   { .m = editor_delete          } },
	BACKSPACE(            cursor,          editor_backspace         ),
	{ { KEY(LEFT) },      cursor,   { .m = editor_char_prev       } },
	{ { KEY(RIGHT) },     cursor,   { .m = editor_char_next       } },
	{ { CONTROL('F') },   cursor,   { .m = editor_char_next       } },
	{ { KEY(UP) },        cursor,   { .m = editor_line_up         } },
	{ { CONTROL('P') },   cursor,   { .m = editor_line_up         } },
	{ { KEY(DOWN) },      cursor,   { .m = editor_line_down       } },
	{ { CONTROL('N') },   cursor,   { .m = editor_line_down       } },
	{ { KEY(PPAGE) },     cursor,   { .m = editor_page_up         } },
	{ { CONTROL('Y') },   cursor,   { .m = editor_page_up         } },
	{ { KEY(F(7)) },      cursor,   { .m = editor_page_up         } },
	{ { KEY(NPAGE) },     cursor,   { .m = editor_page_down       } },
	{ { CONTROL('V') },   cursor,   { .m = editor_page_down       } },
	{ { KEY(F(8)) },      cursor,   { .m = editor_page_down       } },
//	{ { CONTROL(' ') },   cursor,   { .m = editor_word_start_next } },
	{ { META(' ') },      cursor,   { .m = editor_word_start_prev } },
	{ { CONTROL('A') },   cursor,   { .m = editor_line_start      } },
	{ { CONTROL('E') },   cursor,   { .m = editor_line_end        } },
	{ { META(']') },      cursor,   { .m = editor_bracket_match   } },
	{ { META(')') },      cursor,   { .m = editor_paragraph_next  } },
	{ { META('(') },      cursor,   { .m = editor_paragraph_prev  } },
	{ { META('\\') },     cursor,   { .m = editor_file_begin      } },
	{ { META('|') },      cursor,   { .m = editor_file_begin      } },
	{ { META('/') },      cursor,   { .m = editor_file_end        } },
	{ { META('?') },      cursor,   { .m = editor_file_end        } },
	{ { META('U') },      call,     { .f = editor_undo            } },
	{ { META('E') },      call,     { .f = editor_redo            } },
	{ { CONTROL('I') },   insert,   { .s = "\t"                   } },
	/* TODO: handle this in editor to insert \n\r when appriopriate */
	{ { CONTROL('M') },   insert,   { .s = "\n"                   } },
	{ { CONTROL('L') },   call,     { .f = editor_draw            } },
	{ /* empty last element, array terminator */              },
};
#endif

static Mode nano[] = {
	{ .parent = NULL,     .bindings = movement,  },
	{ .parent = &nano[0], .bindings = nano_keys, .input = vis_insert_input, },
};

/* list of editor configurations, first entry is default. name is matched with
 * argv[0] i.e. program name upon execution
 */
static Config editors[] = {
	{ .name = "vis",  .mode = &vis[VIS_MODE_NORMAL] },
	{ .name = "nano", .mode = &nano[1] },
};

/* Color definitions, by default the i-th color is used for the i-th syntax
 * rule below. A color value of -1 specifies the default terminal color.
 */
static Color colors[] = {
	{ .fg = COLOR_RED,     .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_GREEN,   .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_GREEN,   .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_MAGENTA, .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_MAGENTA, .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_BLUE,    .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_RED,     .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_BLUE,    .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_BLUE,    .bg = -1, .attr = A_NORMAL },
	{ /* empty last element, array terminator */ }
};

/* Syntax color definition, you can define up to SYNTAX_REGEX_RULES
 * number of regex rules per file type. Each rule is requires a regular
 * expression and corresponding compilation flags. Optionally a color in
 * the form
 *
 *   { .fg = COLOR_YELLOW, .bg = -1, .attr = A_BOLD }
 *
 * can be specified. If such a color definition is missing the i-th element
 * of the colors array above is used instead.
 *
 * The array of syntax definition must be terminated with an empty element.
 */
#define B "\\b"
/* Use this if \b is not in your libc's regex implementation */
// #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$

// changes wrt sandy #precoressor: #   idfdef, #include <file.h> between brackets
static Syntax syntaxes[] = {{
	.name = "c",
	.file = "\\.(c(pp|xx)?|h(pp|xx)?|cc)$",
	.rules = {{
		"$^",
		REG_NEWLINE,
	},{
		B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		REG_NEWLINE,
	},{
		B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|"
		"typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|"
		"namespace|template|public|protected|private|typename|this|friend|virtual|using|"
		"mutable|volatile|register|explicit)"B,
		REG_NEWLINE,
	},{
		B"(goto|continue|break|return)"B,
		REG_NEWLINE,
	},{
		"(^#[\\t ]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma))|"
		B"[A-Z_][0-9A-Z_]+"B"",
		REG_NEWLINE,
	},{
		"(\\(|\\)|\\{|\\}|\\[|\\])",
		REG_NEWLINE,
	},{
		"(\"(\\\\.|[^\"])*\")",
		//"([\"<](\\\\.|[^ \">])*[\">])",
		REG_NEWLINE,
	},{
		"(//.*)",
		REG_NEWLINE,
	},{
		"(/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	}},
},{
	/* empty last element, array terminator */
}};
