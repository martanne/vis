/** start by reading from the top of vis.c up until config.h is included */
#define DEFAULT_TERM "xterm" /* default term to use if $TERM isn't set */
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
	VIS_MODE_VISUAL_LINE,
	VIS_MODE_READLINE,
	VIS_MODE_PROMPT,
	VIS_MODE_INSERT_REGISTER,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
	VIS_MODE_LAST,
};

static Mode vis_modes[VIS_MODE_LAST];

/* command recognized at the ':'-prompt. commands are found using a unique
 * prefix match. that is if a command should be available under an abbreviation
 * which is a prefix for another command it has to be added as an alias. the
 * long human readable name should always come first */
static Command cmds[] = {
	/* command name / optional alias, function,       options */
	{ { "bdelete"                  }, cmd_bdelete,    CMD_OPT_FORCE },
	{ { "edit"                     }, cmd_edit,       CMD_OPT_FORCE },
	{ { "new"                      }, cmd_new,        CMD_OPT_NONE  },
	{ { "open"                     }, cmd_open,       CMD_OPT_NONE  },
	{ { "qall"                     }, cmd_qall,       CMD_OPT_FORCE },
	{ { "quit", "q"                }, cmd_quit,       CMD_OPT_FORCE },
	{ { "read",                    }, cmd_read,       CMD_OPT_NONE  },
	{ { "saveas"                   }, cmd_saveas,     CMD_OPT_FORCE },
	{ { "set",                     }, cmd_set,        CMD_OPT_ARGS  },
	{ { "split"                    }, cmd_split,      CMD_OPT_NONE  },
	{ { "substitute", "s"          }, cmd_substitute, CMD_OPT_NONE  },
	{ { "vnew"                     }, cmd_vnew,       CMD_OPT_NONE  },
	{ { "vsplit",                  }, cmd_vsplit,     CMD_OPT_NONE  },
	{ { "wq",                      }, cmd_wq,         CMD_OPT_FORCE },
	{ { "write", "w"               }, cmd_write,      CMD_OPT_FORCE },
	{ { "xit",                     }, cmd_xit,        CMD_OPT_FORCE },
	{ /* array terminator */                                        },
};

/* draw a statubar, do whatever you want with win->statuswin curses window */
static void statusbar(EditorWin *win) {
	bool focused = vis->win == win || vis->prompt->editor == win;
	const char *filename = text_filename_get(win->text);
	CursorPos pos = window_cursor_getpos(win->win);
	wattrset(win->statuswin, focused ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win->statuswin, 0, 0, ' ', win->width);
	mvwprintw(win->statuswin, 0, 0, "%s %s %s %s",
	          mode->name && mode->name[0] == '-' ? mode->name : "",
	          filename ? filename : "[No Name]",
	          text_modified(win->text) ? "[+]" : "",
	          vis->recording ? "recording": "");
	char buf[win->width + 1];
	int len = snprintf(buf, win->width, "%zd, %zd", pos.line, pos.col);
	if (len > 0) {
		buf[len] = '\0';
		mvwaddstr(win->statuswin, 0, win->width - len - 1, buf);
	}
}

/* called before any other keybindings are checked, if the function returns false
 * the key is completely ignored. */
static bool vis_keypress(Key *key) {
	editor_info_hide(vis);
	if (vis->recording)
		macro_append(vis->recording, key, sizeof(*key));
	return true;
}

static KeyBinding basic_movement[] = {
	{ { CONTROL('Z')            }, suspend,  { NULL                          } },
	{ { KEY(LEFT)               }, movement, { .i = MOVE_CHAR_PREV           } },
	{ { KEY(SLEFT)              }, movement, { .i = MOVE_LONGWORD_START_PREV } },
	{ { KEY(RIGHT)              }, movement, { .i = MOVE_CHAR_NEXT           } },
	{ { KEY(SRIGHT)             }, movement, { .i = MOVE_LONGWORD_START_NEXT } },
	{ { KEY(UP)                 }, movement, { .i = MOVE_LINE_UP             } },
	{ { KEY(DOWN)               }, movement, { .i = MOVE_LINE_DOWN           } },
	{ { KEY(PPAGE)              }, wscroll,  { .i = -PAGE                    } },
	{ { KEY(NPAGE)              }, wscroll,  { .i = +PAGE                    } },
	{ { KEY(HOME)               }, movement, { .i = MOVE_LINE_START          } },
	{ { KEY(END)                }, movement, { .i = MOVE_LINE_FINISH         } },
	{ /* empty last element, array terminator */                               },
};

static KeyBinding vis_movements[] = {
	BACKSPACE(                     movement,        i,  MOVE_CHAR_PREV             ),
	{ { NONE('h')               }, movement,     { .i = MOVE_CHAR_PREV           } },
	{ { NONE(' ')               }, movement,     { .i = MOVE_CHAR_NEXT           } },
	{ { NONE('l')               }, movement,     { .i = MOVE_CHAR_NEXT           } },
	{ { NONE('k')               }, movement,     { .i = MOVE_LINE_UP             } },
	{ { CONTROL('P')            }, movement,     { .i = MOVE_LINE_UP             } },
	{ { NONE('g'), NONE('k')    }, movement,     { .i = MOVE_SCREEN_LINE_UP      } },
	{ { NONE('g'), KEY(UP)      }, movement,     { .i = MOVE_SCREEN_LINE_UP      } },
	{ { NONE('j')               }, movement,     { .i = MOVE_LINE_DOWN           } },
	{ { CONTROL('J')            }, movement,     { .i = MOVE_LINE_DOWN           } },
	{ { CONTROL('N')            }, movement,     { .i = MOVE_LINE_DOWN           } },
	{ { KEY(ENTER)              }, movement,     { .i = MOVE_LINE_DOWN           } },
	{ { NONE('g'), NONE('j')    }, movement,     { .i = MOVE_SCREEN_LINE_DOWN    } },
	{ { NONE('g'), KEY(DOWN)    }, movement,     { .i = MOVE_SCREEN_LINE_DOWN    } },
	{ { NONE('^')               }, movement,     { .i = MOVE_LINE_START          } },
	{ { NONE('g'), NONE('_')    }, movement,     { .i = MOVE_LINE_FINISH         } },
	{ { NONE('$')               }, movement,     { .i = MOVE_LINE_LASTCHAR       } },
	{ { NONE('%')               }, movement,     { .i = MOVE_BRACKET_MATCH       } },
	{ { NONE('b')               }, movement,     { .i = MOVE_WORD_START_PREV     } },
	{ { NONE('B')               }, movement,     { .i = MOVE_LONGWORD_START_PREV } },
	{ { NONE('w')               }, movement,     { .i = MOVE_WORD_START_NEXT     } },
	{ { NONE('W')               }, movement,     { .i = MOVE_LONGWORD_START_NEXT } },
	{ { NONE('g'), NONE('e')    }, movement,     { .i = MOVE_WORD_END_PREV       } },
	{ { NONE('g'), NONE('E')    }, movement,     { .i = MOVE_LONGWORD_END_PREV   } },
	{ { NONE('e')               }, movement,     { .i = MOVE_WORD_END_NEXT       } },
	{ { NONE('E')               }, movement,     { .i = MOVE_LONGWORD_END_NEXT   } },
	{ { NONE('{')               }, movement,     { .i = MOVE_PARAGRAPH_PREV      } },
	{ { NONE('}')               }, movement,     { .i = MOVE_PARAGRAPH_NEXT      } },
	{ { NONE('(')               }, movement,     { .i = MOVE_SENTENCE_PREV       } },
	{ { NONE(')')               }, movement,     { .i = MOVE_SENTENCE_NEXT       } },
	{ { NONE('g'), NONE('g')    }, gotoline,     { .i = -1                       } },
	{ { NONE('g'), NONE('0')    }, movement,     { .i = MOVE_SCREEN_LINE_BEGIN   } },
	{ { NONE('g'), NONE('m')    }, movement,     { .i = MOVE_SCREEN_LINE_MIDDLE  } },
	{ { NONE('g'), NONE('$')    }, movement,     { .i = MOVE_SCREEN_LINE_END     } },
	{ { NONE('G')               }, gotoline,     { .i = +1                       } },
	{ { NONE('|')               }, movement,     { .i = MOVE_COLUMN              } },
	{ { NONE('n')               }, movement,     { .i = MOVE_SEARCH_FORWARD      } },
	{ { NONE('N')               }, movement,     { .i = MOVE_SEARCH_BACKWARD     } },
	{ { NONE('H')               }, movement,     { .i = MOVE_WINDOW_LINE_TOP     } },
	{ { NONE('M')               }, movement,     { .i = MOVE_WINDOW_LINE_MIDDLE  } },
	{ { NONE('L')               }, movement,     { .i = MOVE_WINDOW_LINE_BOTTOM  } },
	{ { NONE('*')               }, movement,     { .i = MOVE_SEARCH_WORD_FORWARD } },
	{ { NONE('#')               }, movement,     { .i = MOVE_SEARCH_WORD_BACKWARD} },
	{ { NONE('f')               }, movement_key, { .i = MOVE_RIGHT_TO            } },
	{ { NONE('F')               }, movement_key, { .i = MOVE_LEFT_TO             } },
	{ { NONE('t')               }, movement_key, { .i = MOVE_RIGHT_TILL          } },
	{ { NONE('T')               }, movement_key, { .i = MOVE_LEFT_TILL           } },
	{ { NONE('/')               }, prompt_search,{ .s = "/"                      } },
	{ { NONE('?')               }, prompt_search,{ .s = "?"                      } },
	{ /* empty last element, array terminator */                                   },
};

static KeyBinding vis_textobjs[] = {
	{ { NONE('a'), NONE('w')  }, textobj, { .i = TEXT_OBJ_OUTER_WORD           } },
	{ { NONE('a'), NONE('W')  }, textobj, { .i = TEXT_OBJ_OUTER_LONGWORD       } },
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
	{ { NONE('i'), NONE('w')  }, textobj, { .i = TEXT_OBJ_INNER_WORD           } },
	{ { NONE('i'), NONE('W')  }, textobj, { .i = TEXT_OBJ_INNER_LONGWORD       } },
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
	{ { NONE('p')               }, put,           { .i = +1              } },
	{ { NONE('P')               }, put,           { .i = -1              } },
	{ { NONE('>')               }, operator,      { .i = OP_SHIFT_RIGHT  } },
	{ { NONE('<')               }, operator,      { .i = OP_SHIFT_LEFT   } },
	{ { NONE('g'), NONE('U')    }, changecase,    { .i = +1              } },
	{ { NONE('~')               }, changecase,    { .i =  0              } },
	{ { NONE('g'), NONE('u')    }, changecase,    { .i = -1              } },
	{ /* empty last element, array terminator */                           },
};

static void vis_mode_operator_enter(Mode *old) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_OPERATOR_OPTION];
}

static void vis_mode_operator_leave(Mode *new) {
	vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
}

static void vis_mode_operator_input(const char *str, size_t len) {
	/* invalid operator */
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
	{ { NONE('`'), NONE('<')    }, mark,     { .i = MARK_SELECTION_START } },
	{ { NONE('`'), NONE('>')    }, mark,     { .i = MARK_SELECTION_END   } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks_line[] = {
	{ { NONE('\''), NONE('a')   }, mark_line, { .i = MARK_a               } },
	{ { NONE('\''), NONE('b')   }, mark_line, { .i = MARK_b               } },
	{ { NONE('\''), NONE('c')   }, mark_line, { .i = MARK_c               } },
	{ { NONE('\''), NONE('d')   }, mark_line, { .i = MARK_d               } },
	{ { NONE('\''), NONE('e')   }, mark_line, { .i = MARK_e               } },
	{ { NONE('\''), NONE('f')   }, mark_line, { .i = MARK_f               } },
	{ { NONE('\''), NONE('g')   }, mark_line, { .i = MARK_g               } },
	{ { NONE('\''), NONE('h')   }, mark_line, { .i = MARK_h               } },
	{ { NONE('\''), NONE('i')   }, mark_line, { .i = MARK_i               } },
	{ { NONE('\''), NONE('j')   }, mark_line, { .i = MARK_j               } },
	{ { NONE('\''), NONE('k')   }, mark_line, { .i = MARK_k               } },
	{ { NONE('\''), NONE('l')   }, mark_line, { .i = MARK_l               } },
	{ { NONE('\''), NONE('m')   }, mark_line, { .i = MARK_m               } },
	{ { NONE('\''), NONE('n')   }, mark_line, { .i = MARK_n               } },
	{ { NONE('\''), NONE('o')   }, mark_line, { .i = MARK_o               } },
	{ { NONE('\''), NONE('p')   }, mark_line, { .i = MARK_p               } },
	{ { NONE('\''), NONE('q')   }, mark_line, { .i = MARK_q               } },
	{ { NONE('\''), NONE('r')   }, mark_line, { .i = MARK_r               } },
	{ { NONE('\''), NONE('s')   }, mark_line, { .i = MARK_s               } },
	{ { NONE('\''), NONE('t')   }, mark_line, { .i = MARK_t               } },
	{ { NONE('\''), NONE('u')   }, mark_line, { .i = MARK_u               } },
	{ { NONE('\''), NONE('v')   }, mark_line, { .i = MARK_v               } },
	{ { NONE('\''), NONE('w')   }, mark_line, { .i = MARK_w               } },
	{ { NONE('\''), NONE('x')   }, mark_line, { .i = MARK_x               } },
	{ { NONE('\''), NONE('y')   }, mark_line, { .i = MARK_y               } },
	{ { NONE('\''), NONE('z')   }, mark_line, { .i = MARK_z               } },
	{ { NONE('\''), NONE('<')   }, mark_line, { .i = MARK_SELECTION_START } },
	{ { NONE('\''), NONE('>')   }, mark_line, { .i = MARK_SELECTION_END   } },
	{ /* empty last element, array terminator */                            },
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

static KeyBinding vis_mode_normal[] = {
	{ { CONTROL('w'), NONE('n') }, cmd,            { .s = "open"               } },
	{ { CONTROL('w'), NONE('c') }, cmd,            { .s = "q"                  } },
	{ { CONTROL('w'), NONE('s') }, cmd,            { .s = "split"              } },
	{ { CONTROL('w'), NONE('v') }, cmd,            { .s = "vsplit"             } },
	{ { CONTROL('w'), NONE('j') }, call,           { .f = editor_window_next   } },
	{ { CONTROL('w'), NONE('l') }, call,           { .f = editor_window_next   } },
	{ { CONTROL('w'), NONE('k') }, call,           { .f = editor_window_prev   } },
	{ { CONTROL('w'), NONE('h') }, call,           { .f = editor_window_prev   } },
	{ { CONTROL('w'), CONTROL('j') }, call,        { .f = editor_window_next   } },
	{ { CONTROL('w'), CONTROL('l') }, call,        { .f = editor_window_next   } },
	{ { CONTROL('w'), CONTROL('k') }, call,        { .f = editor_window_prev   } },
	{ { CONTROL('w'), CONTROL('h') }, call,        { .f = editor_window_prev   } },
	{ { CONTROL('w'), CONTROL('w') }, call,        { .f = editor_window_next   } },
	{ { CONTROL('B')            }, wscroll,        { .i = -PAGE                } },
	{ { CONTROL('F')            }, wscroll,        { .i = +PAGE                } },
	{ { CONTROL('U')            }, wscroll,        { .i = -PAGE_HALF           } },
	{ { CONTROL('D')            }, wscroll,        { .i = +PAGE_HALF           } },
	{ { CONTROL('E')            }, wslide,         { .i = -1                   } },
	{ { CONTROL('Y')            }, wslide,         { .i = +1                   } },
	{ { CONTROL('O')            }, jumplist,       { .i = -1                   } },
	{ { CONTROL('I')            }, jumplist,       { .i = +1                   } },
	{ { NONE('g'), NONE(';')    }, changelist,     { .i = -1                   } },
	{ { NONE('g'), NONE(',')    }, changelist,     { .i = +1                   } },
	{ { NONE('a')               }, insertmode,     { .i = MOVE_CHAR_NEXT       } },
	{ { NONE('A')               }, insertmode,     { .i = MOVE_LINE_END        } },
	{ { NONE('C')               }, change,         { .i = MOVE_LINE_END        } },
	{ { NONE('D')               }, delete,         { .i = MOVE_LINE_END        } },
	{ { NONE('I')               }, insertmode,     { .i = MOVE_LINE_START      } },
	{ { NONE('.')               }, repeat,         { NULL                      } },
	{ { NONE('o')               }, openline,       { .i = MOVE_LINE_NEXT       } },
	{ { NONE('O')               }, openline,       { .i = MOVE_LINE_PREV       } },
	{ { NONE('J')               }, join,           { .i = MOVE_LINE_NEXT       } },
	{ { NONE('x')               }, delete,         { .i = MOVE_CHAR_NEXT       } },
	{ { NONE('r')               }, replace,        { NULL                      } },
	{ { NONE('i')               }, switchmode,     { .i = VIS_MODE_INSERT      } },
	{ { NONE('v')               }, switchmode,     { .i = VIS_MODE_VISUAL      } },
	{ { NONE('V')               }, switchmode,     { .i = VIS_MODE_VISUAL_LINE } },
	{ { NONE('R')               }, switchmode,     { .i = VIS_MODE_REPLACE     } },
	{ { NONE('S')               }, operator_twice, { .i = OP_CHANGE            } },
	{ { NONE('s')               }, change,         { .i = MOVE_CHAR_NEXT       } },
	{ { NONE('Y')               }, operator_twice, { .i = OP_YANK              } },
	{ { NONE('X')               }, delete,         { .i = MOVE_CHAR_PREV       } },
	{ { NONE('u')               }, undo,           { NULL                      } },
	{ { CONTROL('R')            }, redo,           { NULL                      } },
	{ { CONTROL('L')            }, call,           { .f = editor_draw          } },
	{ { NONE(':')               }, prompt_cmd,     { .s = ""                   } },
	{ { NONE('Z'), NONE('Z')    }, cmd,            { .s = "wq"                 } },
	{ { NONE('Z'), NONE('Q')    }, cmd,            { .s = "q!"                 } },
	{ { NONE('z'), NONE('t')    }, window,         { .w = window_redraw_top    } },
	{ { NONE('z'), NONE('z')    }, window,         { .w = window_redraw_center } },
	{ { NONE('z'), NONE('b')    }, window,         { .w = window_redraw_bottom } },
	{ { NONE('q')               }, macro_record,   { NULL                      } },
	{ { NONE('@')               }, macro_replay,   { NULL                      } },
	{ /* empty last element, array terminator */                                 },
};

static KeyBinding vis_mode_visual[] = {
	BACKSPACE(                     operator,          i,  OP_DELETE              ),
	{ { KEY(DC)                 }, operator,       { .i = OP_DELETE            } },
	{ { NONE(ESC)               }, switchmode,     { .i = VIS_MODE_NORMAL      } },
	{ { CONTROL('c')            }, switchmode,     { .i = VIS_MODE_NORMAL      } },
	{ { NONE('v')               }, switchmode,     { .i = VIS_MODE_NORMAL      } },
	{ { NONE('V')               }, switchmode,     { .i = VIS_MODE_VISUAL_LINE } },
	{ { NONE(':')               }, prompt_cmd,     { .s = "'<,'>"              } },
	{ { CONTROL('H')            }, operator,       { .i = OP_DELETE            } },
	{ { NONE('d')               }, operator,       { .i = OP_DELETE            } },
	{ { NONE('x')               }, operator,       { .i = OP_DELETE            } },
	{ { NONE('y')               }, operator,       { .i = OP_YANK              } },
	{ { NONE('c')               }, operator,       { .i = OP_CHANGE            } },
	{ { NONE('r')               }, operator,       { .i = OP_CHANGE            } },
	{ { NONE('s')               }, operator,       { .i = OP_CHANGE            } },
	{ { NONE('J')               }, operator,       { .i = OP_JOIN              } },
	{ { NONE('o')               }, selection_end,  { NULL                      } },
	{ /* empty last element, array terminator */                                 },
};

static void vis_mode_visual_enter(Mode *old) {
	if (!old->visual) {
		window_selection_start(vis->win->win);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
}

static void vis_mode_visual_leave(Mode *new) {
	if (!new->visual) {
		window_selection_clear(vis->win->win);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	}
}

static KeyBinding vis_mode_visual_line[] = {
	{ { NONE('v')               }, switchmode,      { .i = VIS_MODE_VISUAL   } },
	{ { NONE('V')               }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ /* empty last element, array terminator */                               },
};

static void vis_mode_visual_line_enter(Mode *old) {
	Win *win = vis->win->win;
	window_cursor_to(win, text_line_begin(vis->win->text, window_cursor_get(win)));
	if (!old->visual) {
		window_selection_start(vis->win->win);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
	movement(&(const Arg){ .i = MOVE_LINE_END });
}

static void vis_mode_visual_line_leave(Mode *new) {
	if (!new->visual) {
		window_selection_clear(vis->win->win);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	}
}

static KeyBinding vis_mode_readline[] = {
	BACKSPACE(                     call,               f,  editor_backspace_key       ),
	{ { NONE(ESC)               }, switchmode,      { .i = VIS_MODE_NORMAL          } },
	{ { CONTROL('c')            }, switchmode,      { .i = VIS_MODE_NORMAL          } },
	{ { CONTROL('D')            }, call,            { .f = editor_delete_key        } },
	{ { CONTROL('W')            }, delete,          { .i = MOVE_LONGWORD_START_PREV } },
	{ { CONTROL('U')            }, delete,          { .i = MOVE_LINE_BEGIN          } },
	{ /* empty last element, array terminator */                                      },
};

static KeyBinding vis_mode_prompt[] = {
	BACKSPACE(                     prompt_backspace,  s, NULL                  ),
	{ { KEY(ENTER)              }, prompt_enter,    { NULL                   } },
	{ { CONTROL('J')            }, prompt_enter,    { NULL                   } },
	{ { KEY(UP)                 }, prompt_up,       { NULL                   } },
	{ { KEY(DOWN)               }, prompt_down,     { NULL                   } },
	{ { KEY(HOME)               }, movement,        { .i = MOVE_FILE_BEGIN   } },
	{ { CONTROL('B')            }, movement,        { .i = MOVE_FILE_BEGIN   } },
	{ { KEY(END)                }, movement,        { .i = MOVE_FILE_END     } },
	{ { CONTROL('E')            }, movement,        { .i = MOVE_FILE_END     } },
	{ { NONE('\t')              }, NULL,            { NULL                   } },
	{ /* empty last element, array terminator */                               },
};

static void vis_mode_prompt_input(const char *str, size_t len) {
	editor_insert_key(vis, str, len);
}

static void vis_mode_prompt_enter(Mode *old) {
	if (old->isuser && old != &vis_modes[VIS_MODE_PROMPT])
		mode_before_prompt = old;
}

static void vis_mode_prompt_leave(Mode *new) {
	if (new->isuser)
		editor_prompt_hide(vis);
}

static KeyBinding vis_mode_insert_register[] = {
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

static KeyBinding vis_mode_insert[] = {
	{ { CONTROL('L')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('[')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('I')            }, insert_tab,      { NULL                   } },
	{ { CONTROL('J')            }, insert_newline,  { NULL                   } },
	{ { CONTROL('M')            }, insert_newline,  { NULL                   } },
	{ { CONTROL('O')            }, switchmode,      { .i = VIS_MODE_OPERATOR } },
	{ { CONTROL('V')            }, insert_verbatim, { NULL                   } },
	{ { CONTROL('D')            }, operator_twice,  { .i = OP_SHIFT_LEFT     } },
	{ { CONTROL('T')            }, operator_twice,  { .i = OP_SHIFT_RIGHT    } },
	{ { CONTROL('X'), CONTROL('E') }, wslide,       { .i = -1                } },
	{ { CONTROL('X'), CONTROL('Y') }, wslide,       { .i = +1                } },
	{ { NONE('\t')              }, insert_tab,      { NULL                   } },
	{ { KEY(END)                }, movement,        { .i = MOVE_LINE_END     } },
	{ /* empty last element, array terminator */                               },
};

static void vis_mode_insert_leave(Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->text);
}

static void vis_mode_insert_idle(void) {
	text_snapshot(vis->win->text);
}

static void vis_mode_insert_input(const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = window_cursor_get(vis->win->win);
	if (pos != oldpos)
		buffer_truncate(&buffer_repeat);
	buffer_append(&buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(&action_prev);
	action_prev.op = &ops[OP_REPEAT_INSERT];
	editor_insert_key(vis, str, len);
}

static KeyBinding vis_mode_replace[] = {
	{ { NONE(ESC)               }, switchmode,   { .i = VIS_MODE_NORMAL  } },
	{ /* empty last element, array terminator */                           },
};

static void vis_mode_replace_leave(Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->text);
}

static void vis_mode_replace_input(const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = window_cursor_get(vis->win->win);
	if (pos != oldpos)
		buffer_truncate(&buffer_repeat);
	buffer_append(&buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(&action_prev);
	action_prev.op = &ops[OP_REPEAT_REPLACE];
	editor_replace_key(vis, str, len);
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
 *    REPLACE                            OPERATOR ++++           INNER-TEXTOBJ
 *                                     (d,c,y,p ..)   +      (i [wsp[]()b<>{}B"'`] )
 *                                           |     \\  +               |
 *                                           |      \\  +              |
 *                                        REGISTER   \\  +          TEXTOBJ
 *                                        (" [a-z])   \\  +     (a [wsp[]()b<>{}B"'`] )
 *                           /-----------/   |         \\  +   +       |
 *                          /                |          \\  + +        |
 *                      VISUAL            MARK-SET       \\     OPERATOR-OPTION
 *                         |              (m [a-z])       \\        (v,V)
 *                         |                 |             \\        //
 *                         |                 |              \\======//
 *                   VISUAL-LINE           NORMAL
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
		.enter = vis_mode_operator_enter,
		.leave = vis_mode_operator_leave,
		.input = vis_mode_operator_input,
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
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_MARK_SET],
		.bindings = vis_mode_normal,
	},
	[VIS_MODE_VISUAL] = {
		.name = "--VISUAL--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_REGISTER],
		.bindings = vis_mode_visual,
		.enter = vis_mode_visual_enter,
		.leave = vis_mode_visual_leave,
		.visual = true,
	},
	[VIS_MODE_VISUAL_LINE] = {
		.name = "--VISUAL LINE--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_VISUAL],
		.bindings = vis_mode_visual_line,
		.enter = vis_mode_visual_line_enter,
		.leave = vis_mode_visual_line_leave,
		.visual = true,
	},
	[VIS_MODE_READLINE] = {
		.name = "READLINE",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.bindings = vis_mode_readline,
	},
	[VIS_MODE_PROMPT] = {
		.name = "PROMPT",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.bindings = vis_mode_prompt,
		.input = vis_mode_prompt_input,
		.enter = vis_mode_prompt_enter,
		.leave = vis_mode_prompt_leave,
	},
	[VIS_MODE_INSERT_REGISTER] = {
		.name = "INSERT-REGISTER",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.bindings = vis_mode_insert_register,
	},
	[VIS_MODE_INSERT] = {
		.name = "--INSERT--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_INSERT_REGISTER],
		.bindings = vis_mode_insert,
		.leave = vis_mode_insert_leave,
		.input = vis_mode_insert_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
	[VIS_MODE_REPLACE] = {
		.name = "--REPLACE--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_INSERT],
		.bindings = vis_mode_replace,
		.leave = vis_mode_replace_leave,
		.input = vis_mode_replace_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
};

/* list of vis configurations, first entry is default. name is matched with
 * argv[0] i.e. program name upon execution
 */
static Config editors[] = {
	{
		.name = "vis",
		.mode = &vis_modes[VIS_MODE_NORMAL],
		.statusbar = statusbar,
		.keypress = vis_keypress,
	},
};

/* default editor configuration to use */
static Config *config = &editors[0];

/* null terminated default settings/commands executed once on editor startup */
static const char *settings[] = {
	NULL
};

/* Color definitions for use in the sytax highlighting rules below. A fore
 * or background color of -1 specifies the default terminal color. */
enum {
	COLOR_NOHILIT,
	COLOR_SYNTAX0,
	COLOR_SYNTAX1,
	COLOR_SYNTAX2,
	COLOR_SYNTAX3,
	COLOR_SYNTAX4,
	COLOR_SYNTAX5,
	COLOR_SYNTAX6,
	COLOR_SYNTAX7,
	COLOR_SYNTAX8,
	COLOR_SYNTAX9,
	COLOR_KEYWORD      = COLOR_SYNTAX1,
	COLOR_CONSTANT     = COLOR_SYNTAX4,
	COLOR_DATATYPE     = COLOR_SYNTAX2,
	COLOR_OPERATOR     = COLOR_SYNTAX2,
	COLOR_CONTROL      = COLOR_SYNTAX3,
	COLOR_PREPROCESSOR = COLOR_SYNTAX4,
	COLOR_PRAGMA       = COLOR_SYNTAX4,
	COLOR_KEYWORD2     = COLOR_SYNTAX4,
	COLOR_BRACKETS     = COLOR_SYNTAX5,
	COLOR_STRING       = COLOR_SYNTAX6,
	COLOR_LITERAL      = COLOR_SYNTAX6,
	COLOR_VARIABLE     = COLOR_SYNTAX6,
	COLOR_TARGET       = COLOR_SYNTAX5,
	COLOR_COMMENT      = COLOR_SYNTAX7,
	COLOR_IDENTIFIER   = COLOR_SYNTAX8,
	COLOR_TYPE         = COLOR_SYNTAX9,
};

static Color colors[] = {
	[COLOR_NOHILIT] = { .fg = -1,            .bg = -1, .attr = A_NORMAL },
	[COLOR_SYNTAX0] = { .fg = COLOR_RED,     .bg = -1, .attr = A_BOLD   },
	[COLOR_SYNTAX1] = { .fg = COLOR_GREEN,   .bg = -1, .attr = A_BOLD   },
	[COLOR_SYNTAX2] = { .fg = COLOR_GREEN,   .bg = -1, .attr = A_NORMAL },
	[COLOR_SYNTAX3] = { .fg = COLOR_MAGENTA, .bg = -1, .attr = A_BOLD   },
	[COLOR_SYNTAX4] = { .fg = COLOR_MAGENTA, .bg = -1, .attr = A_NORMAL },
	[COLOR_SYNTAX5] = { .fg = COLOR_BLUE,    .bg = -1, .attr = A_BOLD   },
	[COLOR_SYNTAX6] = { .fg = COLOR_RED,     .bg = -1, .attr = A_NORMAL },
	[COLOR_SYNTAX7] = { .fg = COLOR_BLUE,    .bg = -1, .attr = A_NORMAL },
	[COLOR_SYNTAX8] = { .fg = COLOR_CYAN,    .bg = -1, .attr = A_NORMAL },
	[COLOR_SYNTAX9] = { .fg = COLOR_YELLOW,  .bg = -1, .attr = A_NORMAL },
	{ /* empty last element, array terminator */                        }
};

/* Syntax color definitions per file type. Each rule consists of a regular
 * expression, a color to apply in case of a match and boolean flag inidcating
 * whether it is a multiline rule.
 *
 * The syntax rules where initially imported from the sandy editor, written by
 * Rafael Garcia  <rafael.garcia.gallego@gmail.com>
 */
#define B "\\b"
/* Use this if \b is not in your libc's regex implementation */
// #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$"

/* common rules, used by multiple languages */

#define SYNTAX_MULTILINE_COMMENT {                                           \
	"(/\\*([^*]|\\*+[^*/])*\\*+/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)", \
	&colors[COLOR_COMMENT],                                              \
	true, /* multiline */                                                \
}

#define SYNTAX_SINGLE_LINE_COMMENT { \
	"(//.*)",                    \
	&colors[COLOR_COMMENT],      \
}

#define SYNTAX_LITERAL {                             \
	"('(\\\\.|.)')|"B"(0x[0-9A-Fa-f]+|[0-9]+)"B, \
	&colors[COLOR_LITERAL],                      \
}

#define SYNTAX_STRING {         \
	"(\"(\\\\.|[^\"])*\")", \
	&colors[COLOR_STRING],  \
	false, /* multiline */  \
}

#define SYNTAX_CONSTANT {        \
	B"[A-Z_][0-9A-Z_]+"B,    \
	&colors[COLOR_CONSTANT], \
}

#define SYNTAX_BRACKET {             \
	"(\\(|\\)|\\{|\\}|\\[|\\])", \
	&colors[COLOR_BRACKETS],     \
}

#define SYNTAX_C_PREPROCESSOR { \
	"(^#[\\t ]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma))", \
	&colors[COLOR_PREPROCESSOR], \
}

/* these rules are applied top to bottom, first match wins. Therefore more 'greedy'
 * rules such as for comments should be the first entries.
 *
 * The array of syntax definition must be terminated with an empty element.
 */
static Syntax syntaxes[] = {{
	.name = "c",
	.file = "\\.(c(pp|xx)?|h(pp|xx)?|cc)$",
	.settings = (const char*[]){
		"set number",
		"set autoindent",
		NULL
	},
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		"<[a-zA-Z0-9\\.\\-_/]+\\.(c(pp|xx)?|h(pp|xx)?|cc)>",
		&colors[COLOR_STRING],
	},
		SYNTAX_C_PREPROCESSOR,
	{
		B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|"
		"typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|"
		"namespace|template|public|protected|private|typename|this|friend|virtual|using|"
		"mutable|volatile|register|explicit)"B,
		&colors[COLOR_DATATYPE],
	},{
		B"(goto|continue|break|return)"B,
		&colors[COLOR_CONTROL],
	}}
},{
	.name = "sh",
	.file = "\\.sh$",
	.rules = {{
		"#.*$",
		&colors[COLOR_COMMENT],
	},
		SYNTAX_STRING,
	{
		"^[0-9A-Z_]+\\(\\)",
		&colors[COLOR_CONSTANT],
	},{
		"\\$\\{?[0-9A-Z_!@#$*?-]+\\}?",
		&colors[COLOR_VARIABLE],
	},{
		B"(case|do|done|elif|else|esac|exit|fi|for|function|if|in|local|read|return|select|shift|then|time|until|while)"B,
		&colors[COLOR_KEYWORD],
	},{
		"(\\{|\\}|\\(|\\)|\\;|\\]|\\[|`|\\\\|\\$|<|>|!|=|&|\\|)",
		&colors[COLOR_BRACKETS],
	}}
},{
	.name = "makefile",
	.file = "(Makefile[^/]*|\\.mk)$",
	.rules = {{
		"#.*$",
		&colors[COLOR_COMMENT],
	},{
		"\\$+[{(][a-zA-Z0-9_-]+[})]",
		&colors[COLOR_VARIABLE],
	},{
		B"(if|ifeq|else|endif)"B,
		&colors[COLOR_CONTROL],
	},{
		"^[^ 	]+:",
		&colors[COLOR_TARGET],
	},{
		"[:(+?=)]",
		&colors[COLOR_BRACKETS],
	}}
},{
	.name = "man",
	.file = "\\.[1-9]x?$",
	.rules = {{
		"\\.(BR?|I[PR]?).*$",
		&colors[COLOR_SYNTAX0],
	},{
		"\\.(S|T)H.*$",
		&colors[COLOR_SYNTAX2],
	},{
		"\\.(br|DS|RS|RE|PD)",
		&colors[COLOR_SYNTAX3],
	},{
		"(\\.(S|T)H|\\.TP)",
		&colors[COLOR_SYNTAX4],
	},{
		"\\.(BR?|I[PR]?|PP)",
		&colors[COLOR_SYNTAX5],
	},{
		"\\\\f[BIPR]",
		&colors[COLOR_SYNTAX6],
	}}
},{
	.name = "vala",
	.file = "\\.(vapi|vala)$",
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		B"(for|if|while|do|else|case|default|switch|get|set|value|out|ref|enum)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(uint|uint8|uint16|uint32|uint64|bool|byte|ssize_t|size_t|char|double|string|float|int|long|short|this|base|transient|void|true|false|null|unowned|owned)"B,
		&colors[COLOR_DATATYPE],
	},{
		B"(try|catch|throw|finally|continue|break|return|new|sizeof|signal|delegate)"B,
		&colors[COLOR_CONTROL],
	},{
		B"(abstract|class|final|implements|import|instanceof|interface|using|private|public|static|strictfp|super|throws)"B,
		&colors[COLOR_KEYWORD2],
	}}
},{
	.name = "java",
	.file = "\\.java$",
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		B"(for|if|while|do|else|case|default|switch)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(boolean|byte|char|double|float|int|long|short|transient|void|true|false|null)"B,
		&colors[COLOR_DATATYPE],
	},{
		B"(try|catch|throw|finally|continue|break|return|new)"B,
		&colors[COLOR_CONTROL],
	},{
		B"(abstract|class|extends|final|implements|import|instanceof|interface|native|package|private|protected|public|static|strictfp|this|super|synchronized|throws|volatile)"B,
		&colors[COLOR_KEYWORD2],
	}}
},{
	.name = "ruby",
	.file = "\\.rb$",
	.rules = {{
		"(#[^{].*$|#$)",
		&colors[COLOR_COMMENT],
	},{
		"(\\$|@|@@)?"B"[A-Z]+[0-9A-Z_a-z]*",
		&colors[COLOR_VARIABLE],
	},{
		B"(__FILE__|__LINE__|BEGIN|END|alias|and|begin|break|case|class|def|defined\?|do|else|elsif|end|ensure|false|for|if|in|module|next|nil|not|or|redo|rescue|retry|return|self|super|then|true|undef|unless|until|when|while|yield)"B,
		&colors[COLOR_KEYWORD],
	},{
		"([ 	]|^):[0-9A-Z_]+"B,
		&colors[COLOR_SYNTAX2],
	},{
		"(/([^/]|(\\/))*/[iomx]*|%r\\{([^}]|(\\}))*\\}[iomx]*)",
		&colors[COLOR_SYNTAX3],
	},{
		"(`[^`]*`|%x\\{[^}]*\\})",
		&colors[COLOR_SYNTAX4],
	},{
		"(\"([^\"]|(\\\\\"))*\"|%[QW]?\\{[^}]*\\}|%[QW]?\\([^)]*\\)|%[QW]?<[^>]*>|%[QW]?\\[[^]]*\\]|%[QW]?\\$[^$]*\\$|%[QW]?\\^[^^]*\\^|%[QW]?![^!]*!|\'([^\']|(\\\\\'))*\'|%[qw]\\{[^}]*\\}|%[qw]\\([^)]*\\)|%[qw]<[^>]*>|%[qw]\\[[^]]*\\]|%[qw]\\$[^$]*\\$|%[qw]\\^[^^]*\\^|%[qw]![^!]*!)",
		&colors[COLOR_SYNTAX5],
	},{
		"#\\{[^}]*\\}",
		&colors[COLOR_SYNTAX6],
	}}
},{
	.name = "python",
	.file = "\\.py$",
	.rules = {{
		"(#.*$|#$)",
		&colors[COLOR_COMMENT],
	},{
		"(\"\"\".*\"\"\")",
		&colors[COLOR_COMMENT],
		true, /* multiline */
	},{
		B"(and|class|def|not|or|return|yield|is)"B,
		&colors[COLOR_KEYWORD2],
	},{
		B"(from|import|as)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(if|elif|else|while|for|in|try|with|except|in|break|continue|finally)"B,
		&colors[COLOR_CONTROL],
	},{
		B"(int|str|float|unicode|int|bool|chr|type|list|dict|tuple)",
		&colors[COLOR_DATATYPE],
	},{
		"(True|False|None)",
		&colors[COLOR_LITERAL],
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		&colors[COLOR_LITERAL],
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		&colors[COLOR_LITERAL],
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		&colors[COLOR_STRING],
		false, /* multiline */  
	},{
		"(__init__|__str__|__unicode__|__gt__|__lt__|__eq__|__enter__|__exit__|__next__|__getattr__|__getitem__|__setitem__|__call__|__contains__|__iter__|__bool__|__all__|__name__)",
		&colors[COLOR_SYNTAX2],
	}}
},{
	.name = "php",
	.file = "\\.php$",
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_BRACKET,
	{
		"(#.*$|#$)",
		&colors[COLOR_COMMENT],
	},{
		"(\"\"\".*\"\"\")",
		&colors[COLOR_COMMENT],
		true, /* multiline */
	},{
		B"(class|interface|extends|implements|new|__construct|__destruct|use|namespace|return)"B,
		&colors[COLOR_KEYWORD2],
	},{
		B"(public|private|protected|const|parent|function|->)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(if|else|while|do|for|foreach|in|try|catch|finally|switch|case|default|break|continue|as|=>)"B,
		&colors[COLOR_CONTROL],
	},{
		B"(array|true|false|null)",
		&colors[COLOR_DATATYPE],
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		&colors[COLOR_LITERAL],
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		&colors[COLOR_LITERAL],
	},{
		"\\$[a-zA-Z0-9_\\-]+",
		&colors[COLOR_VARIABLE],
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		&colors[COLOR_STRING],
		false, /* multiline */
	},{
		"(php|echo|print|var_dump|print_r)",
		&colors[COLOR_SYNTAX2],
	}}
},{
	.name = "haskell",
	.file = "\\.hs$",
	.rules = {{
		"\\{-#.*#-\\}",
		&colors[COLOR_PRAGMA],
	},{
		"---*([^-!#$%&\\*\\+./<=>\?@\\^|~].*)?$",
		&colors[COLOR_COMMENT],
	}, {
		// These are allowed to be nested, but we can't express that
		// with regular expressions
		"\\{-.*-\\}",
		&colors[COLOR_COMMENT],
		true
	},
		SYNTAX_STRING,
		SYNTAX_C_PREPROCESSOR,
	{
		// as and hiding are only keywords when part of an import, but
		// I don't want to highlight the whole import line.
		// capture group coloring or similar would be nice
		"(^import( qualified)?)|"B"(as|hiding|infix[lr]?)"B,
		&colors[COLOR_KEYWORD2],
	},{
		B"(module|class|data|deriving|instance|default|where|type|newtype)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(do|case|of|let|in|if|then|else)"B,
		&colors[COLOR_CONTROL],
	},{
		"('(\\\\.|.)')",
		&colors[COLOR_LITERAL],
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		&colors[COLOR_LITERAL],
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		&colors[COLOR_LITERAL],
	},{
		"("B"[A-Z][a-zA-Z0-9_']*\\.)*"B"[a-zA-Z][a-zA-Z0-9_']*"B,
		&colors[COLOR_NOHILIT],
	},{
		"("B"[A-Z][a-zA-Z0-9_']*\\.)?[-!#$%&\\*\\+/<=>\\?@\\\\^|~:.][-!#$%&\\*\\+/<=>\\?@\\\\^|~:.]*",
		&colors[COLOR_OPERATOR],
	},{
		"`("B"[A-Z][a-zA-Z0-9_']*\\.)?[a-z][a-zA-Z0-9_']*`",
		&colors[COLOR_OPERATOR],
	},{
		"\\(|\\)|\\[|\\]|,|;|_|\\{|\\}",
		&colors[COLOR_BRACKETS],
	}}
},{
	.name = "markdown",
	.file = "\\.(md|mdwn)$",
	.rules = {{
		"(^#{1,6}.*$)", //titles
		&colors[COLOR_SYNTAX5],
	},{
		"((\\* *){3,}|(_ *){3,}|(- *){3,})", // horizontal rules
		&colors[COLOR_SYNTAX2],
	},{
		"(\\*\\*.*\\*\\*)|(__.*__)", // super-bolds
		&colors[COLOR_SYNTAX4],
	},{
		"(\\*.*\\*)|(_.*_)", // bolds
		&colors[COLOR_SYNTAX3],
	},{
		"(\\[.*\\]\\(.*\\))", //links
		&colors[COLOR_SYNTAX6],
	},{
		"(^ *([-\\*\\+]|[0-9]+\\.))", //lists
		&colors[COLOR_SYNTAX2],
	},{
		"(^( {4,}|\t+).*$)", // code blocks
		&colors[COLOR_SYNTAX7],
	},{
		"(`+.*`+)", // inline code
		&colors[COLOR_SYNTAX7],
	},{
		"(^>+.*)", // quotes
		&colors[COLOR_SYNTAX7],
	}}
},{
	.name = "ledger",
	.file = "\\.(journal|ledger)$",
	.rules = {
	{ /* comment */
		"^[;#].*",
		&colors[COLOR_COMMENT],
	},{ /* value tag */
		"(  |\t|^ )*; :([^ ][^:]*:)+[ \\t]*$",
		&colors[COLOR_DATATYPE],
	},{ /* typed tag */
		"(  |\t|^ )*; [^:]+::.*",
		&colors[COLOR_DATATYPE],
	},{ /* tag */
		"(  |\t|^ )*; [^:]+:.*",
		&colors[COLOR_TYPE],
	},{ /* metadata */
		"(  |\t|^ )*;.*",
		&colors[COLOR_CONSTANT],
	},{ /* date */
		"^[0-9][^ \t]+",
		&colors[COLOR_LITERAL],
	},{ /* account */
		"^[ \t]+[a-zA-Z:'!*()%&]+",
		&colors[COLOR_IDENTIFIER]
	},{ /* amount */
		"(  |\t)[^;]*",
		&colors[COLOR_LITERAL],
	},{ /* automated transaction */
		"^[=~].*",
		&colors[COLOR_TYPE],
	},{ /* directives */
		"^[!@]?(account|alias|assert|bucket|capture|check|comment|commodity|define|end|fixed|endfixed|include|payee|apply|tag|test|year|[AYNDCIiOobh])"B".*",
		&colors[COLOR_DATATYPE],
	}}
},{
	/* empty last element, array terminator */
}};
