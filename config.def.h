/** start by reading from the top of vis.c up until config.h is included */

/* macros used to specify keys for key bindings */
#define ESC        0x1B
#define NONE(k)    { .str = { k }, .code = 0 }
#define KEY(k)     { .str = { '\0' }, .code = KEY_##k }
#define CONTROL(k) NONE((k)&0x1F)
#define META(k)    { .str = { ESC, (k) }, .code = 0 }
#define BACKSPACE(func, name, arg) \
	{ { KEY(BACKSPACE) }, (func), { .name = (arg) } }, \
	{ { CONTROL('H') },   (func), { .name = (arg) } }, \
	{ { NONE(127) },      (func), { .name = (arg) } }, \
	{ { CONTROL('B') },   (func), { .name = (arg) } }

/* a mode contains a set of key bindings which are currently valid.
 *
 * each mode can specify one parent mode which is consultated if a given key
 * is not found in the current mode. hence the modes form a tree which is
 * searched from the current mode up towards the root mode until a valid binding
 * is found.
 *
 * if no binding is found, mode->input(...) is called and the user entered
 * keys are passed as argument. this is used to change the document content.
 */
static Mode vis_modes[];

enum {
	VIS_MODE_BASIC,
	VIS_MODE_MARK,
	VIS_MODE_MARK_LINE,
	VIS_MODE_MARK_SET,
	VIS_MODE_MOVE,
	VIS_MODE_TEXTOBJ,
	VIS_MODE_INNER_TEXTOBJ,
	VIS_MODE_OPERATOR,
	VIS_MODE_OPERATOR_OPTION,
	VIS_MODE_REGISTER,
	VIS_MODE_NORMAL,
	VIS_MODE_VISUAL,
	VIS_MODE_READLINE,
	VIS_MODE_PROMPT,
	VIS_MODE_INSERT_REGISTER,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
};

/* command recognized at the ':'-prompt, matched using a greedy top to bottom,
 * regex search. make sure the longer commands are listed before the shorter ones
 * e.g. 'sp' before 's' and 'wq' before 'w'.
 */
static Command cmds[] = {
	{ "^[0-9]+",        cmd_gotoline   },
	{ "^o(pen)?",       cmd_open       },
	{ "^q(quit)?",      cmd_quit       },
	{ "^r(ead)?",       cmd_read       },
	{ "^sp(lit)?",      cmd_split      },
	{ "^s(ubstitute)?", cmd_substitute },
	{ "^v(split)?",     cmd_vsplit     },
	{ "^wq",            cmd_wq         },
	{ "^w(rite)?",      cmd_write      },
	{ /* array terminator */           },
};

/* draw a statubar, do whatever you want with win->statuswin curses window */
static void statusbar(EditorWin *win) {
	size_t line, col;
	bool focused = vis->win == win || vis->prompt->editor == win;
	window_cursor_getxy(win->win, &line, &col);
	wattrset(win->statuswin, focused ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win->statuswin, 0, 0, ' ', win->width);
	mvwprintw(win->statuswin, 0, 0, "%s %s", text_filename(win->text),
	          text_modified(win->text) ? "[+]" : "");
	char buf[win->width + 1];
	int len = snprintf(buf, win->width, "%d, %d", line, col);
	if (len > 0) {
		buf[len] = '\0';
		mvwaddstr(win->statuswin, 0, win->width - len - 1, buf);
	}
}

static KeyBinding basic_movement[] = {
	{ { KEY(LEFT)               }, movement, { .i = MOVE_CHAR_PREV         } },
	{ { KEY(SLEFT)              }, movement, { .i = MOVE_WORD_START_PREV   } },
	{ { KEY(RIGHT)              }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { KEY(SRIGHT)             }, movement, { .i = MOVE_WORD_START_NEXT   } },
	{ { KEY(UP)                 }, movement, { .i = MOVE_LINE_UP           } },
	{ { KEY(DOWN)               }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { KEY(PPAGE)              }, cursor,   { .m = window_page_up         } },
	{ { KEY(NPAGE)              }, cursor,   { .m = window_page_down       } },
	{ { KEY(HOME)               }, movement, { .i = MOVE_LINE_START        } },
	{ { KEY(END)                }, movement, { .i = MOVE_LINE_FINISH       } },
	// temporary until we have a way to enter user commands
	{ { CONTROL('c')            }, quit,                                   },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_movements[] = {
	BACKSPACE(                     movement,    i,  MOVE_CHAR_PREV           ),
	{ { NONE('h')               }, movement, { .i = MOVE_CHAR_PREV         } },
	{ { NONE(' ')               }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { NONE('l')               }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { NONE('k')               }, movement, { .i = MOVE_LINE_UP           } },
	{ { CONTROL('P')            }, movement, { .i = MOVE_LINE_UP           } },
	{ { NONE('j')               }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { CONTROL('J')            }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { CONTROL('N')            }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { KEY(ENTER)              }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { NONE('^')               }, movement, { .i = MOVE_LINE_START        } },
	{ { NONE('g'), NONE('_')    }, movement, { .i = MOVE_LINE_FINISH       } },
	{ { NONE('$')               }, movement, { .i = MOVE_LINE_END          } },
	{ { NONE('%')               }, movement, { .i = MOVE_BRACKET_MATCH     } },
	{ { NONE('b')               }, movement, { .i = MOVE_WORD_START_PREV   } },
	{ { NONE('w')               }, movement, { .i = MOVE_WORD_START_NEXT   } },
	{ { NONE('g'), NONE('e')    }, movement, { .i = MOVE_WORD_END_PREV     } },
	{ { NONE('e')               }, movement, { .i = MOVE_WORD_END_NEXT     } },
	{ { NONE('{')               }, movement, { .i = MOVE_PARAGRAPH_PREV    } },
	{ { NONE('}')               }, movement, { .i = MOVE_PARAGRAPH_NEXT    } },
	{ { NONE('(')               }, movement, { .i = MOVE_SENTENCE_PREV     } },
	{ { NONE(')')               }, movement, { .i = MOVE_SENTENCE_NEXT     } },
	{ { NONE('g'), NONE('g')    }, movement, { .i = MOVE_FILE_BEGIN        } },
	{ { NONE('G')               }, movement, { .i = MOVE_LINE              } },
	{ { NONE('|')               }, movement, { .i = MOVE_COLUMN            } },
	{ { NONE('f')               }, movement_key, { .i = MOVE_RIGHT_TO      } },
	{ { NONE('F')               }, movement_key, { .i = MOVE_LEFT_TO       } },
	{ { NONE('t')               }, movement_key, { .i = MOVE_RIGHT_TILL    } },
	{ { NONE('T')               }, movement_key, { .i = MOVE_LEFT_TILL     } },
	{ { NONE('/')               }, prompt,        { .s = "/"               } },
	{ { NONE('?')               }, prompt,        { .s = "?"               } },
	{ /* empty last element, array terminator */                             },
};

static KeyBinding vis_textobjs[] = {
	{ { NONE('a'), NONE('w')  }, textobj, { .i = TEXT_OBJ_WORD                 } },
	{ { NONE('a'), NONE('s')  }, textobj, { .i = TEXT_OBJ_SENTENCE             } },
	{ { NONE('a'), NONE('p')  }, textobj, { .i = TEXT_OBJ_PARAGRAPH            } },
	{ { NONE('a'), NONE('[')  }, textobj, { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ { NONE('a'), NONE(']')  }, textobj, { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ { NONE('a'), NONE('(')  }, textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE(')')  }, textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE('b')  }, textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE('<')  }, textobj, { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ { NONE('a'), NONE('>')  }, textobj, { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ { NONE('a'), NONE('{')  }, textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('}')  }, textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('B')  }, textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('"')  }, textobj, { .i = TEXT_OBJ_OUTER_QUOTE          } },
	{ { NONE('a'), NONE('\'') }, textobj, { .i = TEXT_OBJ_OUTER_SINGLE_QUOTE   } },
	{ { NONE('a'), NONE('`')  }, textobj, { .i = TEXT_OBJ_OUTER_BACKTICK       } },
	{ /* empty last element, array terminator */                                 },
};

static KeyBinding vis_inner_textobjs[] = {
	{ { NONE('i'), NONE('w')  }, textobj, { .i = TEXT_OBJ_WORD                 } },
	{ { NONE('i'), NONE('s')  }, textobj, { .i = TEXT_OBJ_SENTENCE             } },
	{ { NONE('i'), NONE('p')  }, textobj, { .i = TEXT_OBJ_PARAGRAPH            } },
	{ { NONE('i'), NONE('[')  }, textobj, { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ { NONE('i'), NONE(']')  }, textobj, { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ { NONE('i'), NONE('(')  }, textobj, { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE(')')  }, textobj, { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE('b')  }, textobj, { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE('<')  }, textobj, { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ { NONE('i'), NONE('>')  }, textobj, { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ { NONE('i'), NONE('{')  }, textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('}')  }, textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('B')  }, textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('"')  }, textobj, { .i = TEXT_OBJ_INNER_QUOTE          } },
	{ { NONE('i'), NONE('\'') }, textobj, { .i = TEXT_OBJ_INNER_SINGLE_QUOTE   } },
	{ { NONE('i'), NONE('`')  }, textobj, { .i = TEXT_OBJ_INNER_BACKTICK       } },
	{ /* empty last element, array terminator */                                 },
};

static KeyBinding vis_operators[] = {
	{ { NONE('0')               }, zero,          { NULL                 } },
	{ { NONE('1')               }, count,         { .i = 1               } },
	{ { NONE('2')               }, count,         { .i = 2               } },
	{ { NONE('3')               }, count,         { .i = 3               } },
	{ { NONE('4')               }, count,         { .i = 4               } },
	{ { NONE('5')               }, count,         { .i = 5               } },
	{ { NONE('6')               }, count,         { .i = 6               } },
	{ { NONE('7')               }, count,         { .i = 7               } },
	{ { NONE('8')               }, count,         { .i = 8               } },
	{ { NONE('9')               }, count,         { .i = 9               } },
	{ { NONE('d')               }, operator,      { .i = OP_DELETE       } },
	{ { NONE('c')               }, operator,      { .i = OP_CHANGE       } },
	{ { NONE('y')               }, operator,      { .i = OP_YANK         } },
	{ { NONE('p')               }, operator,      { .i = OP_PASTE        } },
	{ /* empty last element, array terminator */                           },
};

static void vis_operators_enter(Mode *old) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_OPERATOR_OPTION];
}

static void vis_operators_leave(Mode *new) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
}

static void operator_invalid(const char *str, size_t len) {
	action_reset(&action);
	switchmode_to(mode_prev);
}

static KeyBinding vis_operator_options[] = {
	{ { NONE('v')               }, linewise,      { .b = false           } },
	{ { NONE('V')               }, linewise,      { .b = true            } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_registers[] = { /* {a-zA-Z0-9.%#:-"} */
	{ { NONE('"'), NONE('a')    }, reg,           { .i = REG_a           } },
	{ { NONE('"'), NONE('b')    }, reg,           { .i = REG_b           } },
	{ { NONE('"'), NONE('c')    }, reg,           { .i = REG_c           } },
	{ { NONE('"'), NONE('d')    }, reg,           { .i = REG_d           } },
	{ { NONE('"'), NONE('e')    }, reg,           { .i = REG_e           } },
	{ { NONE('"'), NONE('f')    }, reg,           { .i = REG_f           } },
	{ { NONE('"'), NONE('g')    }, reg,           { .i = REG_g           } },
	{ { NONE('"'), NONE('h')    }, reg,           { .i = REG_h           } },
	{ { NONE('"'), NONE('i')    }, reg,           { .i = REG_i           } },
	{ { NONE('"'), NONE('j')    }, reg,           { .i = REG_j           } },
	{ { NONE('"'), NONE('k')    }, reg,           { .i = REG_k           } },
	{ { NONE('"'), NONE('l')    }, reg,           { .i = REG_l           } },
	{ { NONE('"'), NONE('m')    }, reg,           { .i = REG_m           } },
	{ { NONE('"'), NONE('n')    }, reg,           { .i = REG_n           } },
	{ { NONE('"'), NONE('o')    }, reg,           { .i = REG_o           } },
	{ { NONE('"'), NONE('p')    }, reg,           { .i = REG_p           } },
	{ { NONE('"'), NONE('q')    }, reg,           { .i = REG_q           } },
	{ { NONE('"'), NONE('r')    }, reg,           { .i = REG_r           } },
	{ { NONE('"'), NONE('s')    }, reg,           { .i = REG_s           } },
	{ { NONE('"'), NONE('t')    }, reg,           { .i = REG_t           } },
	{ { NONE('"'), NONE('u')    }, reg,           { .i = REG_u           } },
	{ { NONE('"'), NONE('v')    }, reg,           { .i = REG_v           } },
	{ { NONE('"'), NONE('w')    }, reg,           { .i = REG_w           } },
	{ { NONE('"'), NONE('x')    }, reg,           { .i = REG_x           } },
	{ { NONE('"'), NONE('y')    }, reg,           { .i = REG_y           } },
	{ { NONE('"'), NONE('z')    }, reg,           { .i = REG_z           } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks[] = {
	{ { NONE('`'), NONE('a')    }, mark,          { .i = MARK_a          } },
	{ { NONE('`'), NONE('b')    }, mark,          { .i = MARK_b          } },
	{ { NONE('`'), NONE('c')    }, mark,          { .i = MARK_c          } },
	{ { NONE('`'), NONE('d')    }, mark,          { .i = MARK_d          } },
	{ { NONE('`'), NONE('e')    }, mark,          { .i = MARK_e          } },
	{ { NONE('`'), NONE('f')    }, mark,          { .i = MARK_f          } },
	{ { NONE('`'), NONE('g')    }, mark,          { .i = MARK_g          } },
	{ { NONE('`'), NONE('h')    }, mark,          { .i = MARK_h          } },
	{ { NONE('`'), NONE('i')    }, mark,          { .i = MARK_i          } },
	{ { NONE('`'), NONE('j')    }, mark,          { .i = MARK_j          } },
	{ { NONE('`'), NONE('k')    }, mark,          { .i = MARK_k          } },
	{ { NONE('`'), NONE('l')    }, mark,          { .i = MARK_l          } },
	{ { NONE('`'), NONE('m')    }, mark,          { .i = MARK_m          } },
	{ { NONE('`'), NONE('n')    }, mark,          { .i = MARK_n          } },
	{ { NONE('`'), NONE('o')    }, mark,          { .i = MARK_o          } },
	{ { NONE('`'), NONE('p')    }, mark,          { .i = MARK_p          } },
	{ { NONE('`'), NONE('q')    }, mark,          { .i = MARK_q          } },
	{ { NONE('`'), NONE('r')    }, mark,          { .i = MARK_r          } },
	{ { NONE('`'), NONE('s')    }, mark,          { .i = MARK_s          } },
	{ { NONE('`'), NONE('t')    }, mark,          { .i = MARK_t          } },
	{ { NONE('`'), NONE('u')    }, mark,          { .i = MARK_u          } },
	{ { NONE('`'), NONE('v')    }, mark,          { .i = MARK_v          } },
	{ { NONE('`'), NONE('w')    }, mark,          { .i = MARK_w          } },
	{ { NONE('`'), NONE('x')    }, mark,          { .i = MARK_x          } },
	{ { NONE('`'), NONE('y')    }, mark,          { .i = MARK_y          } },
	{ { NONE('`'), NONE('z')    }, mark,          { .i = MARK_z          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks_line[] = {
	{ { NONE('\''), NONE('a')   }, mark_line,     { .i = MARK_a          } },
	{ { NONE('\''), NONE('b')   }, mark_line,     { .i = MARK_b          } },
	{ { NONE('\''), NONE('c')   }, mark_line,     { .i = MARK_c          } },
	{ { NONE('\''), NONE('d')   }, mark_line,     { .i = MARK_d          } },
	{ { NONE('\''), NONE('e')   }, mark_line,     { .i = MARK_e          } },
	{ { NONE('\''), NONE('f')   }, mark_line,     { .i = MARK_f          } },
	{ { NONE('\''), NONE('g')   }, mark_line,     { .i = MARK_g          } },
	{ { NONE('\''), NONE('h')   }, mark_line,     { .i = MARK_h          } },
	{ { NONE('\''), NONE('i')   }, mark_line,     { .i = MARK_i          } },
	{ { NONE('\''), NONE('j')   }, mark_line,     { .i = MARK_j          } },
	{ { NONE('\''), NONE('k')   }, mark_line,     { .i = MARK_k          } },
	{ { NONE('\''), NONE('l')   }, mark_line,     { .i = MARK_l          } },
	{ { NONE('\''), NONE('m')   }, mark_line,     { .i = MARK_m          } },
	{ { NONE('\''), NONE('n')   }, mark_line,     { .i = MARK_n          } },
	{ { NONE('\''), NONE('o')   }, mark_line,     { .i = MARK_o          } },
	{ { NONE('\''), NONE('p')   }, mark_line,     { .i = MARK_p          } },
	{ { NONE('\''), NONE('q')   }, mark_line,     { .i = MARK_q          } },
	{ { NONE('\''), NONE('r')   }, mark_line,     { .i = MARK_r          } },
	{ { NONE('\''), NONE('s')   }, mark_line,     { .i = MARK_s          } },
	{ { NONE('\''), NONE('t')   }, mark_line,     { .i = MARK_t          } },
	{ { NONE('\''), NONE('u')   }, mark_line,     { .i = MARK_u          } },
	{ { NONE('\''), NONE('v')   }, mark_line,     { .i = MARK_v          } },
	{ { NONE('\''), NONE('w')   }, mark_line,     { .i = MARK_w          } },
	{ { NONE('\''), NONE('x')   }, mark_line,     { .i = MARK_x          } },
	{ { NONE('\''), NONE('y')   }, mark_line,     { .i = MARK_y          } },
	{ { NONE('\''), NONE('z')   }, mark_line,     { .i = MARK_z          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks_set[] = {
	{ { NONE('m'), NONE('a')    }, mark_set,      { .i = MARK_a          } },
	{ { NONE('m'), NONE('b')    }, mark_set,      { .i = MARK_b          } },
	{ { NONE('m'), NONE('c')    }, mark_set,      { .i = MARK_c          } },
	{ { NONE('m'), NONE('d')    }, mark_set,      { .i = MARK_d          } },
	{ { NONE('m'), NONE('e')    }, mark_set,      { .i = MARK_e          } },
	{ { NONE('m'), NONE('f')    }, mark_set,      { .i = MARK_f          } },
	{ { NONE('m'), NONE('g')    }, mark_set,      { .i = MARK_g          } },
	{ { NONE('m'), NONE('h')    }, mark_set,      { .i = MARK_h          } },
	{ { NONE('m'), NONE('i')    }, mark_set,      { .i = MARK_i          } },
	{ { NONE('m'), NONE('j')    }, mark_set,      { .i = MARK_j          } },
	{ { NONE('m'), NONE('k')    }, mark_set,      { .i = MARK_k          } },
	{ { NONE('m'), NONE('l')    }, mark_set,      { .i = MARK_l          } },
	{ { NONE('m'), NONE('m')    }, mark_set,      { .i = MARK_m          } },
	{ { NONE('m'), NONE('n')    }, mark_set,      { .i = MARK_n          } },
	{ { NONE('m'), NONE('o')    }, mark_set,      { .i = MARK_o          } },
	{ { NONE('m'), NONE('p')    }, mark_set,      { .i = MARK_p          } },
	{ { NONE('m'), NONE('q')    }, mark_set,      { .i = MARK_q          } },
	{ { NONE('m'), NONE('r')    }, mark_set,      { .i = MARK_r          } },
	{ { NONE('m'), NONE('s')    }, mark_set,      { .i = MARK_s          } },
	{ { NONE('m'), NONE('t')    }, mark_set,      { .i = MARK_t          } },
	{ { NONE('m'), NONE('u')    }, mark_set,      { .i = MARK_u          } },
	{ { NONE('m'), NONE('v')    }, mark_set,      { .i = MARK_v          } },
	{ { NONE('m'), NONE('w')    }, mark_set,      { .i = MARK_w          } },
	{ { NONE('m'), NONE('x')    }, mark_set,      { .i = MARK_x          } },
	{ { NONE('m'), NONE('y')    }, mark_set,      { .i = MARK_y          } },
	{ { NONE('m'), NONE('z')    }, mark_set,      { .i = MARK_z          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_normal[] = {
	{ { CONTROL('w'), NONE('s') }, split,    { NULL                        } },
	{ { CONTROL('w'), NONE('j') }, call,     { .f = editor_window_next     } },
	{ { CONTROL('w'), NONE('k') }, call,     { .f = editor_window_prev     } },
	{ { CONTROL('F')            }, cursor,   { .m = window_page_up         } },
	{ { CONTROL('B')            }, cursor,   { .m = window_page_down       } },
	{ { NONE('.')               }, repeat,   {                             } },
	{ { NONE('n')               }, movement, { .i = MOVE_SEARCH_FORWARD    } },
	{ { NONE('N')               }, movement, { .i = MOVE_SEARCH_BACKWARD   } },
	{ { NONE('x')               }, call,          { .f = editor_delete_key   } },
	{ { NONE('i')               }, switchmode,    { .i = VIS_MODE_INSERT } },
	{ { NONE('v')               }, switchmode,    { .i = VIS_MODE_VISUAL } },
	{ { NONE('R')               }, switchmode,    { .i = VIS_MODE_REPLACE} },
	{ { NONE('u')               }, undo,          { NULL                   } },
	{ { CONTROL('R')            }, redo,          { NULL                   } },
	{ { CONTROL('L')            }, call,          { .f = editor_draw     } },
	{ { NONE(':')               }, prompt,        { .s = ":"             } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_visual[] = {
	{ { NONE(ESC)               }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('c')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	BACKSPACE(                     operator,           i,  OP_DELETE           ),
	{ { CONTROL('H')            }, operator,        { .i = OP_DELETE         } },
	{ { NONE('d')               }, operator,        { .i = OP_DELETE         } },
	{ { NONE('x')               }, operator,        { .i = OP_DELETE         } },
	{ { NONE('y')               }, operator,        { .i = OP_YANK           } },
	{ { NONE('c')               }, operator,        { .i = OP_CHANGE         } },
	{ { NONE('r')               }, operator,        { .i = OP_CHANGE         } },
	{ { NONE('s')               }, operator,        { .i = OP_CHANGE         } },
	{ /* empty last element, array terminator */                               },
};

static void vis_visual_enter(Mode *old) {
	window_selection_start(vis->win->win);
}

static void vis_visual_leave(Mode *new) {
	window_selection_clear(vis->win->win);
}

static KeyBinding vis_readline_mode[] = {
	{ { NONE(ESC)               }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('c')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	BACKSPACE(                     call,               f,  editor_backspace_key   ),
	{ { CONTROL('D')            }, call,            { .f = editor_delete_key    } },
	{ { CONTROL('W')            }, delete_word,     { NULL                   } },
	{ /* empty last element, array terminator */                               },
};

static KeyBinding vis_prompt_mode[] = {
	{ { KEY(ENTER)              }, prompt_enter,    { NULL                   } },
	{ { CONTROL('J')            }, prompt_enter,    { NULL                   } },
	{ { KEY(UP)                 }, prompt_up,       { NULL                   } },
	{ { KEY(DOWN)               }, prompt_down,     { NULL                   } },
	{ { KEY(HOME)               }, movement,        { .i = MOVE_FILE_BEGIN   } },
	{ { KEY(END)                }, movement,        { .i = MOVE_FILE_END     } },
	{ /* empty last element, array terminator */                               },
};

static void vis_prompt_leave(Mode *new) {
	/* prompt mode may be left for operator mode when editing the command prompt.
	 * for example during Ctrl+w / delete_word. don't hide the prompt in this case */
	if (new != &vis_modes[VIS_MODE_OPERATOR])
		editor_prompt_hide(vis);
}

static KeyBinding vis_insert_register_mode[] = {
	{ { CONTROL('R'), NONE('a') }, insert_register, { .i = REG_a             } },
	{ { CONTROL('R'), NONE('b') }, insert_register, { .i = REG_b             } },
	{ { CONTROL('R'), NONE('c') }, insert_register, { .i = REG_c             } },
	{ { CONTROL('R'), NONE('d') }, insert_register, { .i = REG_d             } },
	{ { CONTROL('R'), NONE('e') }, insert_register, { .i = REG_e             } },
	{ { CONTROL('R'), NONE('f') }, insert_register, { .i = REG_f             } },
	{ { CONTROL('R'), NONE('g') }, insert_register, { .i = REG_g             } },
	{ { CONTROL('R'), NONE('h') }, insert_register, { .i = REG_h             } },
	{ { CONTROL('R'), NONE('i') }, insert_register, { .i = REG_i             } },
	{ { CONTROL('R'), NONE('j') }, insert_register, { .i = REG_j             } },
	{ { CONTROL('R'), NONE('k') }, insert_register, { .i = REG_k             } },
	{ { CONTROL('R'), NONE('l') }, insert_register, { .i = REG_l             } },
	{ { CONTROL('R'), NONE('m') }, insert_register, { .i = REG_m             } },
	{ { CONTROL('R'), NONE('n') }, insert_register, { .i = REG_n             } },
	{ { CONTROL('R'), NONE('o') }, insert_register, { .i = REG_o             } },
	{ { CONTROL('R'), NONE('p') }, insert_register, { .i = REG_p             } },
	{ { CONTROL('R'), NONE('q') }, insert_register, { .i = REG_q             } },
	{ { CONTROL('R'), NONE('r') }, insert_register, { .i = REG_r             } },
	{ { CONTROL('R'), NONE('s') }, insert_register, { .i = REG_s             } },
	{ { CONTROL('R'), NONE('t') }, insert_register, { .i = REG_t             } },
	{ { CONTROL('R'), NONE('u') }, insert_register, { .i = REG_u             } },
	{ { CONTROL('R'), NONE('v') }, insert_register, { .i = REG_v             } },
	{ { CONTROL('R'), NONE('w') }, insert_register, { .i = REG_w             } },
	{ { CONTROL('R'), NONE('x') }, insert_register, { .i = REG_x             } },
	{ { CONTROL('R'), NONE('y') }, insert_register, { .i = REG_y             } },
	{ { CONTROL('R'), NONE('z') }, insert_register, { .i = REG_z             } },
	{ /* empty last element, array terminator */                               },
};

static KeyBinding vis_insert_mode[] = {
	{ { CONTROL('L')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('[')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('I')            }, insert_tab,      { NULL                   } },
	{ { CONTROL('J')            }, insert_newline,  { NULL                   } },
	{ { CONTROL('M')            }, insert_newline,  { NULL                   } },
	{ { CONTROL('O')            }, switchmode,      { .i = VIS_MODE_OPERATOR } },
	{ { CONTROL('V')            }, insert_verbatim, { NULL                   } },
	{ /* empty last element, array terminator */                               },
};

static void vis_insert_idle(void) {
	text_snapshot(vis->win->text);
}

static void vis_insert_input(const char *str, size_t len) {
	editor_insert_key(vis, str, len);
}

static KeyBinding vis_replace[] = {
	{ { NONE(ESC)               }, switchmode,   { .i = VIS_MODE_NORMAL  } },
	{ /* empty last element, array terminator */                           },
};

static void vis_replace_input(const char *str, size_t len) {
	editor_replace_key(vis, str, len);
}

/*
 * the tree of modes currently looks like this. the double line between OPERATOR-OPTION
 * and OPERATOR is only in effect once an operator is detected. that is when entering the
 * OPERATOR mode its parent is set to OPERATOR-OPTION which makes {INNER-,}TEXTOBJ
 * reachable. once the operator is processed (i.e. the OPERATOR mode is left) its parent
 * mode is reset back to MOVE.
 *
 *
 *                                         BASIC
 *                                    (arrow keys etc.)
 *                                    /      |
 *               /-------------------/       |
 *           READLINE                      MARK
 *          /        \                   (` [a-z])
 *         /          \                      |
 *        /            \                     |
 * INSERT-REGISTER    PROMPT             MARK-LINE
 * (Ctrl+R [a-z])     (history etc)      (' [a-z])
 *       |                                   |
 *       |                                   |
 *    INSERT                                MOVE
 *       |                              (h,j,k,l ...)
 *       |                                   |       \-----------------\
 *       |                                   |                         |
 *    REPLACE                            OPERATOR ======\\       INNER-TEXTOBJ
 *                                     (d,c,y,p ..)     ||    (i [wsp[]()b<>{}B"'`] )
 *                                           |          ||             |
 *                                           |          ||             |
 *                                        REGISTER      ||          TEXTOBJ
 *                                        (" [a-z])     ||    (a [wsp[]()b<>{}B"'`] )
 *                           /-----------/   |          \\
 *                          /                |           \\
 *                      VISUAL            MARK-SET        \\     OPERATOR-OPTION
 *                                        (m [a-z])        \\        (v,V)
 *                                           |              \\        //
 *                                           |               \\======//
 *                                         NORMAL
 */

static Mode vis_modes[] = {
	[VIS_MODE_BASIC] = {
		.name = "BASIC",
		.parent = NULL,
		.bindings = basic_movement,
	},
	[VIS_MODE_MARK] = {
		.name = "MARK",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_BASIC],
		.bindings = vis_marks,
	},
	[VIS_MODE_MARK_LINE] = {
		.name = "MARK-LINE",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_MARK],
		.bindings = vis_marks_line,
	},
	[VIS_MODE_MOVE] = {
		.name = "MOVE",
		.parent = &vis_modes[VIS_MODE_MARK_LINE],
		.bindings = vis_movements,
	},
	[VIS_MODE_INNER_TEXTOBJ] = {
		.name = "INNER-TEXTOBJ",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_MOVE],
		.bindings = vis_inner_textobjs,
	},
	[VIS_MODE_TEXTOBJ] = {
		.name = "TEXTOBJ",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_INNER_TEXTOBJ],
		.bindings = vis_textobjs,
	},
	[VIS_MODE_OPERATOR_OPTION] = {
		.name = "OPERATOR-OPTION",
		.parent = &vis_modes[VIS_MODE_TEXTOBJ],
		.bindings = vis_operator_options,
	},
	[VIS_MODE_OPERATOR] = {
		.name = "OPERATOR",
		.parent = &vis_modes[VIS_MODE_MOVE],
		.bindings = vis_operators,
		.enter = vis_operators_enter,
		.leave = vis_operators_leave,
		.input = operator_invalid,
	},
	[VIS_MODE_REGISTER] = {
		.name = "REGISTER",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.bindings = vis_registers,
	},
	[VIS_MODE_MARK_SET] = {
		.name = "MARK-SET",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_REGISTER],
		.bindings = vis_marks_set,
	},
	[VIS_MODE_NORMAL] = {
		.name = "NORMAL",
		.parent = &vis_modes[VIS_MODE_MARK_SET],
		.bindings = vis_normal,
	},
	[VIS_MODE_VISUAL] = {
		.name = "VISUAL",
		.parent = &vis_modes[VIS_MODE_REGISTER],
		.bindings = vis_visual,
		.enter = vis_visual_enter,
		.leave = vis_visual_leave,
	},
	[VIS_MODE_READLINE] = {
		.name = "READLINE",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.bindings = vis_readline_mode,
	},
	[VIS_MODE_PROMPT] = {
		.name = "PROMPT",
		.parent = &vis_modes[VIS_MODE_READLINE],
		.bindings = vis_prompt_mode,
		.input = vis_insert_input,
		.leave = vis_prompt_leave,
	},
	[VIS_MODE_INSERT_REGISTER] = {
		.name = "INSERT-REGISTER",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.bindings = vis_insert_register_mode,
	},
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.parent = &vis_modes[VIS_MODE_INSERT_REGISTER],
		.bindings = vis_insert_mode,
		.input = vis_insert_input,
		.idle = vis_insert_idle,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.parent = &vis_modes[VIS_MODE_INSERT],
		.bindings = vis_replace,
		.input = vis_replace_input,
		.idle = vis_insert_idle,
	},
};

/* incomplete list of useful but currently missing functionality from nano's help ^G:

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

static KeyBinding nano_keys[] = {
	{ { CONTROL('D')            }, call,     { .f = editor_delete_key      } },
	BACKSPACE(                     call,        f,     editor_backspace_key  ),
	{ { CONTROL('F')            }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { CONTROL('P')            }, movement, { .i = MOVE_LINE_UP           } },
	{ { CONTROL('N')            }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { CONTROL('Y')            }, cursor,   { .m = window_page_up         } },
	{ { KEY(F(7))               }, cursor,   { .m = window_page_up         } },
	{ { CONTROL('V')            }, cursor,   { .m = window_page_down       } },
	{ { KEY(F(8))               }, cursor,   { .m = window_page_down       } },
#if 0
	// CONTROL(' ') == 0 which signals the end of array
	{ { CONTROL(' ')            }, movement, { .i = MOVE_WORD_START_NEXT   } },
#endif
	{ { META(' ')               }, movement, { .i = MOVE_WORD_START_PREV   } },
	{ { CONTROL('A')            }, movement, { .i = MOVE_LINE_START        } },
	{ { CONTROL('E')            }, movement, { .i = MOVE_LINE_END          } },
	{ { META(']')               }, movement, { .i = MOVE_BRACKET_MATCH     } },
	{ { META(')')               }, movement, { .i = MOVE_PARAGRAPH_PREV    } },
	{ { META('(')               }, movement, { .i = MOVE_PARAGRAPH_NEXT    } },
	{ { META('\\')              }, movement, { .i = MOVE_FILE_BEGIN        } },
	{ { META('|')               }, movement, { .i = MOVE_FILE_BEGIN        } },
	{ { META('/')               }, movement, { .i = MOVE_FILE_END          } },
	{ { META('?')               }, movement, { .i = MOVE_FILE_END          } },
	{ { META('U')               }, undo,     { NULL                        } },
	{ { META('E')               }, redo,     { NULL                        } },
#if 0
	{ { CONTROL('I') },   insert,   { .s = "\t"                   } },
	/* TODO: handle this in vis to insert \n\r when appriopriate */
	{ { CONTROL('M') },   insert,   { .s = "\n"                   } },
#endif
	{ { CONTROL('L')            }, call,      { .f = editor_draw           } },
	{ /* empty last element, array terminator */                             },
};

static Mode nano[] = {
	{ .parent = NULL,     .bindings = basic_movement,  },
	{ .parent = &nano[0], .bindings = nano_keys, .input = vis_insert_input, },
};

/* list of vis configurations, first entry is default. name is matched with
 * argv[0] i.e. program name upon execution
 */
static Config editors[] = {
	{ .name = "vis",  .mode = &vis_modes[VIS_MODE_NORMAL], .statusbar = statusbar },
	{ .name = "nano", .mode = &nano[1], .statusbar = statusbar },
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
