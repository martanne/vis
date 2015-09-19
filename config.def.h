/** start by reading from the top of vis.c up until config.h is included */

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
	VIS_MODE_MOVE,
	VIS_MODE_TEXTOBJ,
	VIS_MODE_OPERATOR,
	VIS_MODE_OPERATOR_OPTION,
	VIS_MODE_NORMAL,
	VIS_MODE_VISUAL,
	VIS_MODE_VISUAL_LINE,
	VIS_MODE_READLINE,
	VIS_MODE_PROMPT,
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
	{ { "read",                    }, cmd_read,       CMD_OPT_FORCE },
	{ { "saveas"                   }, cmd_saveas,     CMD_OPT_FORCE },
	{ { "set",                     }, cmd_set,        CMD_OPT_ARGS  },
	{ { "split"                    }, cmd_split,      CMD_OPT_NONE  },
	{ { "substitute", "s"          }, cmd_substitute, CMD_OPT_NONE  },
	{ { "vnew"                     }, cmd_vnew,       CMD_OPT_NONE  },
	{ { "vsplit",                  }, cmd_vsplit,     CMD_OPT_NONE  },
	{ { "wq",                      }, cmd_wq,         CMD_OPT_FORCE },
	{ { "write", "w"               }, cmd_write,      CMD_OPT_FORCE },
	{ { "xit",                     }, cmd_xit,        CMD_OPT_FORCE },
	{ { "earlier"                  }, cmd_earlier_later, CMD_OPT_NONE },
	{ { "later"                    }, cmd_earlier_later, CMD_OPT_NONE },
	{ { "!",                       }, cmd_filter,     CMD_OPT_NONE  },
	{ /* array terminator */                                        },
};

/* called before any other keybindings are checked, if the function returns false
 * the key is completely ignored. */
static bool vis_keypress(const char *key) {
	editor_info_hide(vis);
	if (vis->recording && key)
		macro_append(vis->recording, key);
	return true;
}

static KeyBinding basic_movement[] = {
	{ "<C-z>"                    , suspend,  { NULL                          } },
	{ "<Left>"                   , movement, { .i = MOVE_CHAR_PREV           } },
	{ "<S-Left>"                 , movement, { .i = MOVE_LONGWORD_START_PREV } },
	{ "<Right>"                  , movement, { .i = MOVE_CHAR_NEXT           } },
	{ "<S-Right>"                , movement, { .i = MOVE_LONGWORD_START_NEXT } },
	{ "<Up>"                     , movement, { .i = MOVE_LINE_UP             } },
	{ "<Down>"                   , movement, { .i = MOVE_LINE_DOWN           } },
	{ "<PageUp>"                 , wscroll,  { .i = -PAGE                    } },
	{ "<PageDown>"               , wscroll,  { .i = +PAGE                    } },
	{ "<Home>"                   , movement, { .i = MOVE_LINE_START          } },
	{ "<End>"                    , movement, { .i = MOVE_LINE_FINISH         } },
	{ /* empty last element, array terminator */                               },
};

static KeyBinding vis_movements[] = {
	{ "<Backspace>"              , movement,     { .i = MOVE_CHAR_PREV           } },
	{ "h"                        , movement,     { .i = MOVE_CHAR_PREV           } },
	{ " "                        , movement,     { .i = MOVE_CHAR_NEXT           } },
	{ "l"                        , movement,     { .i = MOVE_CHAR_NEXT           } },
	{ "k"                        , movement,     { .i = MOVE_LINE_UP             } },
	{ "C-p"                      , movement,     { .i = MOVE_LINE_UP             } },
	{ "gk"                       , movement,     { .i = MOVE_SCREEN_LINE_UP      } },
	{ "g<Up>"                    , movement,     { .i = MOVE_SCREEN_LINE_UP      } },
	{ "j"                        , movement,     { .i = MOVE_LINE_DOWN           } },
	{ "<C-j>"                    , movement,     { .i = MOVE_LINE_DOWN           } },
	{ "<C-n>"                    , movement,     { .i = MOVE_LINE_DOWN           } },
	{ "<Enter>"                  , movement,     { .i = MOVE_LINE_DOWN           } },
	{ "gj"                       , movement,     { .i = MOVE_SCREEN_LINE_DOWN    } },
	{ "g<Down>"                  , movement,     { .i = MOVE_SCREEN_LINE_DOWN    } },
	{ "^"                        , movement,     { .i = MOVE_LINE_START          } },
	{ "g_"                       , movement,     { .i = MOVE_LINE_FINISH         } },
	{ "$"                        , movement,     { .i = MOVE_LINE_LASTCHAR       } },
	{ "%"                        , movement,     { .i = MOVE_BRACKET_MATCH       } },
	{ "b"                        , movement,     { .i = MOVE_WORD_START_PREV     } },
	{ "B"                        , movement,     { .i = MOVE_LONGWORD_START_PREV } },
	{ "w"                        , movement,     { .i = MOVE_WORD_START_NEXT     } },
	{ "W"                        , movement,     { .i = MOVE_LONGWORD_START_NEXT } },
	{ "ge"                       , movement,     { .i = MOVE_WORD_END_PREV       } },
	{ "gE"                       , movement,     { .i = MOVE_LONGWORD_END_PREV   } },
	{ "e"                        , movement,     { .i = MOVE_WORD_END_NEXT       } },
	{ "E"                        , movement,     { .i = MOVE_LONGWORD_END_NEXT   } },
	{ "{"                        , movement,     { .i = MOVE_PARAGRAPH_PREV      } },
	{ "}"                        , movement,     { .i = MOVE_PARAGRAPH_NEXT      } },
	{ "("                        , movement,     { .i = MOVE_SENTENCE_PREV       } },
	{ ")"                        , movement,     { .i = MOVE_SENTENCE_NEXT       } },
	{ "[["                       , movement,     { .i = MOVE_FUNCTION_START_PREV } },
	{ "[]"                       , movement,     { .i = MOVE_FUNCTION_END_PREV   } },
	{ "]["                       , movement,     { .i = MOVE_FUNCTION_START_NEXT } },
	{ "]]"                       , movement,     { .i = MOVE_FUNCTION_END_NEXT   } },
	{ "gg"                       , gotoline,     { .i = -1                       } },
	{ "g0"                       , movement,     { .i = MOVE_SCREEN_LINE_BEGIN   } },
	{ "gm"                       , movement,     { .i = MOVE_SCREEN_LINE_MIDDLE  } },
	{ "g$"                       , movement,     { .i = MOVE_SCREEN_LINE_END     } },
	{ "G"                        , gotoline,     { .i = +1                       } },
	{ "|"                        , movement,     { .i = MOVE_COLUMN              } },
	{ "n"                        , movement,     { .i = MOVE_SEARCH_FORWARD      } },
	{ "N"                        , movement,     { .i = MOVE_SEARCH_BACKWARD     } },
	{ "H"                        , movement,     { .i = MOVE_WINDOW_LINE_TOP     } },
	{ "M"                        , movement,     { .i = MOVE_WINDOW_LINE_MIDDLE  } },
	{ "L"                        , movement,     { .i = MOVE_WINDOW_LINE_BOTTOM  } },
	{ "*"                        , movement,     { .i = MOVE_SEARCH_WORD_FORWARD } },
	{ "#"                        , movement,     { .i = MOVE_SEARCH_WORD_BACKWARD} },
	{ "f"                        , movement_key, { .i = MOVE_RIGHT_TO            } },
	{ "F"                        , movement_key, { .i = MOVE_LEFT_TO             } },
	{ "t"                        , movement_key, { .i = MOVE_RIGHT_TILL          } },
	{ "T"                        , movement_key, { .i = MOVE_LEFT_TILL           } },
	{ ";"                        , totill_repeat, { NULL                         } },
	{ ","                        , totill_reverse,{ NULL                         } },
	{ "/"                        , prompt_search,{ .s = "/"                      } },
	{ "?"                        , prompt_search,{ .s = "?"                      } },
	{ "`"                        , mark,         { NULL                          } },
	{ "'"                        , mark_line,    { NULL                          } },
	{ /* empty last element, array terminator */                                   },
};

static KeyBinding vis_textobjs[] = {
	{ "aw",  textobj, { .i = TEXT_OBJ_OUTER_WORD           } },
	{ "aW",  textobj, { .i = TEXT_OBJ_OUTER_LONGWORD       } },
	{ "as",  textobj, { .i = TEXT_OBJ_SENTENCE             } },
	{ "ap",  textobj, { .i = TEXT_OBJ_PARAGRAPH            } },
	{ "a[",  textobj, { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ "a]",  textobj, { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ "a(",  textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ "a)",  textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ "ab",  textobj, { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ "a<",  textobj, { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ "a>",  textobj, { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ "a{",  textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ "a}",  textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ "aB",  textobj, { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ "a\"", textobj, { .i = TEXT_OBJ_OUTER_QUOTE          } },
	{ "a\'", textobj, { .i = TEXT_OBJ_OUTER_SINGLE_QUOTE   } },
	{ "a`",  textobj, { .i = TEXT_OBJ_OUTER_BACKTICK       } },
	{ "ae",  textobj, { .i = TEXT_OBJ_OUTER_ENTIRE         } },
	{ "af",  textobj, { .i = TEXT_OBJ_OUTER_FUNCTION       } },
	{ "al",  textobj, { .i = TEXT_OBJ_OUTER_LINE           } },
	{ "iw",  textobj, { .i = TEXT_OBJ_INNER_WORD           } },
	{ "iW",  textobj, { .i = TEXT_OBJ_INNER_LONGWORD       } },
	{ "is",  textobj, { .i = TEXT_OBJ_SENTENCE             } },
	{ "ip",  textobj, { .i = TEXT_OBJ_PARAGRAPH            } },
	{ "i[",  textobj, { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ "i]",  textobj, { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ "i(",  textobj, { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ "i)",  textobj, { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ "ib",  textobj, { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ "i<",  textobj, { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ "i>",  textobj, { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ "i{",  textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ "i}",  textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ "iB",  textobj, { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ "i\"", textobj, { .i = TEXT_OBJ_INNER_QUOTE          } },
	{ "i\'", textobj, { .i = TEXT_OBJ_INNER_SINGLE_QUOTE   } },
	{ "i`",  textobj, { .i = TEXT_OBJ_INNER_BACKTICK       } },
	{ "ie",  textobj, { .i = TEXT_OBJ_INNER_ENTIRE         } },
	{ "if",  textobj, { .i = TEXT_OBJ_INNER_FUNCTION       } },
	{ "il",  textobj, { .i = TEXT_OBJ_INNER_LINE           } },
	{ /* empty last element, array terminator */             },
};

static KeyBinding vis_operators[] = {
	{ "0"               , zero,          { NULL                 } },
	{ "1"               , count,         { .i = 1               } },
	{ "2"               , count,         { .i = 2               } },
	{ "3"               , count,         { .i = 3               } },
	{ "4"               , count,         { .i = 4               } },
	{ "5"               , count,         { .i = 5               } },
	{ "6"               , count,         { .i = 6               } },
	{ "7"               , count,         { .i = 7               } },
	{ "8"               , count,         { .i = 8               } },
	{ "9"               , count,         { .i = 9               } },
	{ "d"               , operator,      { .i = OP_DELETE       } },
	{ "c"               , operator,      { .i = OP_CHANGE       } },
	{ "y"               , operator,      { .i = OP_YANK         } },
	{ "p"               , put,           { .i = PUT_AFTER       } },
	{ "P"               , put,           { .i = PUT_BEFORE      } },
	{ "gp"              , put,           { .i = PUT_AFTER_END   } },
	{ "gP"              , put,           { .i = PUT_BEFORE_END  } },
	{ ">"               , operator,      { .i = OP_SHIFT_RIGHT  } },
	{ "<"               , operator,      { .i = OP_SHIFT_LEFT   } },
	{ "gU"              , changecase,    { .i = +1              } },
	{ "~"               , changecase,    { .i =  0              } },
	{ "g~"              , changecase,    { .i =  0              } },
	{ "gu"              , changecase,    { .i = -1              } },
	{ "\""              , reg,           { NULL                 } },
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
	action_reset(&vis->action);
	switchmode_to(vis->mode_prev);
}

static KeyBinding vis_operator_options[] = {
	{ "v"                 , motiontype,    { .i = CHARWISE        } },
	{ "V"                 , motiontype,    { .i = LINEWISE        } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_mode_normal[] = {
	{ "<Escape>",         cursors_clear,  {                           } },
	{ "<C-k>",            cursors_new,    { .i = -1                   } },
	{ "<C-j>",            cursors_new,    { .i = +1                   } },
	{ "<C-a>",            cursors_align,  {                           } },
	{ "<C-n>",            cursors_select, {                           } },
	{ "<C-p>",            cursors_remove, {                           } },
	{ "<C-w>n",           cmd,            { .s = "open"               } },
	{ "<C-w>c",           cmd,            { .s = "q"                  } },
	{ "<C-w>s",           cmd,            { .s = "split"              } },
	{ "<C-w>v",           cmd,            { .s = "vsplit"             } },
	{ "<C-w>j",           call,           { .f = editor_window_next   } },
	{ "<C-w>l",           call,           { .f = editor_window_next   } },
	{ "<C-w>k",           call,           { .f = editor_window_prev   } },
	{ "<C-w>h",           call,           { .f = editor_window_prev   } },
	{ "<C-w><C-j>",       call,           { .f = editor_window_next   } },
	{ "<C-w><C-l>",       call,           { .f = editor_window_next   } },
	{ "<C-w><C-k>",       call,           { .f = editor_window_prev   } },
	{ "<C-w><C-w>",       call,           { .f = editor_window_next   } },
	{ "<C-w><C-h>",       call,           { .f = editor_window_prev   } },
	{ "<C-w><Backspace>", call,           { .f = editor_window_prev   } },
	{ "<C-b>",            wscroll,        { .i = -PAGE                } },
	{ "<C-f>",            wscroll,        { .i = +PAGE                } },
	{ "<C-u>",            wscroll,        { .i = -PAGE_HALF           } },
	{ "<C-d>",            wscroll,        { .i = +PAGE_HALF           } },
	{ "<C-e>",            wslide,         { .i = -1                   } },
	{ "<C-y>",            wslide,         { .i = +1                   } },
	{ "<C-o>",            jumplist,       { .i = -1                   } },
	{ "<C-i>",            jumplist,       { .i = +1                   } },
	{ "g;",               changelist,     { .i = -1                   } },
	{ "g,",               changelist,     { .i = +1                   } },
	{ "a",                insertmode,     { .i = MOVE_CHAR_NEXT       } },
	{ "A",                insertmode,     { .i = MOVE_LINE_END        } },
	{ "C",                change,         { .i = MOVE_LINE_END        } },
	{ "D",                delete,         { .i = MOVE_LINE_END        } },
	{ "I",                insertmode,     { .i = MOVE_LINE_START      } },
	{ ".",                repeat,         { NULL                      } },
	{ "o",                openline,       { .i = MOVE_LINE_NEXT       } },
	{ "O",                openline,       { .i = MOVE_LINE_PREV       } },
	{ "J",                join,           { .i = MOVE_LINE_NEXT       } },
	{ "x",                delete,         { .i = MOVE_CHAR_NEXT       } },
	{ "r",                replace,        { NULL                      } },
	{ "i",                switchmode,     { .i = VIS_MODE_INSERT      } },
	{ "v",                switchmode,     { .i = VIS_MODE_VISUAL      } },
	{ "V",                switchmode,     { .i = VIS_MODE_VISUAL_LINE } },
	{ "R",                switchmode,     { .i = VIS_MODE_REPLACE     } },
	{ "S",                operator_twice, { .i = OP_CHANGE            } },
	{ "s",                change,         { .i = MOVE_CHAR_NEXT       } },
	{ "Y",                operator_twice, { .i = OP_YANK              } },
	{ "X",                delete,         { .i = MOVE_CHAR_PREV       } },
	{ "u",                undo,           { NULL                      } },
	{ "<C-r>",            redo,           { NULL                      } },
	{ "g+",               later,          { NULL                      } },
	{ "g-",               earlier,        { NULL                      } },
	{ "<C-l>",            call,           { .f = editor_draw          } },
	{ ":",                prompt_cmd,     { .s = ""                   } },
	{ "ZZ",               cmd,            { .s = "wq"                 } },
	{ "ZQ",               cmd,            { .s = "q!"                 } },
	{ "zt",               window,         { .w = view_redraw_top      } },
	{ "zz",               window,         { .w = view_redraw_center   } },
	{ "zb",               window,         { .w = view_redraw_bottom   } },
	{ "q",                macro_record,   { NULL                      } },
	{ "@",                macro_replay,   { NULL                      } },
	{ "gv",               selection_restore, { NULL                   } },
	{ "m",                mark_set,       { NULL                      } },
	{ /* empty last element, array terminator */                        },
};

static KeyBinding vis_mode_visual[] = {
	{ "<C-n>",              cursors_select_next, {                      } },
	{ "<C-x>",              cursors_select_skip, {                      } },
	{ "<C-p>",              cursors_remove, {                           } },
	{ "I",                  cursors_split,  { .i = -1                   } },
	{ "A",                  cursors_split,  { .i = +1                   } },
	{ "<Backspace>",        operator,       { .i = OP_DELETE            } },
	{ "<DEL>",              operator,       { .i = OP_DELETE            } },
	{ "<Escape>",           switchmode,     { .i = VIS_MODE_NORMAL      } },
	{ "<C-c>",              switchmode,     { .i = VIS_MODE_NORMAL      } },
	{ "v",                  switchmode,     { .i = VIS_MODE_NORMAL      } },
	{ "V",                  switchmode,     { .i = VIS_MODE_VISUAL_LINE } },
	{ ":",                  prompt_cmd,     { .s = "'<,'>"              } },
	{ "<C-h>",              operator,       { .i = OP_DELETE            } },
	{ "d",                  operator,       { .i = OP_DELETE            } },
	{ "x",                  operator,       { .i = OP_DELETE            } },
	{ "y",                  operator,       { .i = OP_YANK              } },
	{ "c",                  operator,       { .i = OP_CHANGE            } },
	{ "r",                  operator,       { .i = OP_CHANGE            } },
	{ "s",                  operator,       { .i = OP_CHANGE            } },
	{ "J",                  operator,       { .i = OP_JOIN              } },
	{ "o",                  selection_end,  { NULL                      } },
	{ /* empty last element, array terminator */                                 },
};

static void vis_mode_visual_enter(Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
}

static void vis_mode_visual_leave(Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	}
}

static KeyBinding vis_mode_visual_line[] = {
	{ "v",                switchmode,      { .i = VIS_MODE_VISUAL   } },
	{ "V",                switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ /* empty last element, array terminator */                      },
};

static void vis_mode_visual_line_enter(Mode *old) {
	if (!old->visual) {
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c))
			view_cursors_selection_start(c);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_TEXTOBJ];
	}
	movement(NULL, &(const Arg){ .i = MOVE_LINE_END });
}

static void vis_mode_visual_line_leave(Mode *new) {
	if (!new->visual) {
		view_selections_clear(vis->win->view);
		vis_modes[VIS_MODE_OPERATOR].parent = &vis_modes[VIS_MODE_MOVE];
	} else {
		view_cursor_to(vis->win->view, view_cursor_get(vis->win->view));
	}
}

static KeyBinding vis_mode_readline[] = {
	{ "<Backspace>",    delete,          { .i = MOVE_CHAR_PREV           } },
	{ "<Escape>",       switchmode,      { .i = VIS_MODE_NORMAL          } },
	{ "<C-c>",          switchmode,      { .i = VIS_MODE_NORMAL          } },
	{ "<C-d>",          delete ,         { .i = MOVE_CHAR_NEXT           } },
	{ "<C-w>",          delete,          { .i = MOVE_LONGWORD_START_PREV } },
	{ "<C-u>",          delete,          { .i = MOVE_LINE_BEGIN          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_mode_prompt[] = {
	{ "<Backspace>",    prompt_backspace,{ .s = NULL              } },
	{ "<Enter>",        prompt_enter,    { NULL                   } },
	{ "<C-j>",          prompt_enter,    { NULL                   } },
	{ "<Up>",           prompt_up,       { NULL                   } },
	{ "<Down>",         prompt_down,     { NULL                   } },
	{ "<Home>",         movement,        { .i = MOVE_FILE_BEGIN   } },
	{ "<C-b>",          movement,        { .i = MOVE_FILE_BEGIN   } },
	{ "<End>",          movement,        { .i = MOVE_FILE_END     } },
	{ "<C-e>",          movement,        { .i = MOVE_FILE_END     } },
	{ "<Tab>",          NULL,            { NULL                   } },
	{ /* empty last element, array terminator */                    },
};

static void vis_mode_prompt_input(const char *str, size_t len) {
	editor_insert_key(vis, str, len);
}

static void vis_mode_prompt_enter(Mode *old) {
	if (old->isuser && old != &vis_modes[VIS_MODE_PROMPT])
		vis->mode_before_prompt = old;
}

static void vis_mode_prompt_leave(Mode *new) {
	if (new->isuser)
		editor_prompt_hide(vis);
}

static KeyBinding vis_mode_insert[] = {
	{ "<Escape>",           switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ "<C-l>",              switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ "<C-[>",              switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ "<C-i>",              insert_tab,      { NULL                   } },
	{ "<C-j>",              insert_newline,  { NULL                   } },
	{ "<C-m>",              insert_newline,  { NULL                   } },
	{ "<Enter>",            insert_newline,  { NULL                   } },
	{ "<C-o>",              switchmode,      { .i = VIS_MODE_OPERATOR } },
	{ "<C-v>",              insert_verbatim, { NULL                   } },
	{ "<C-d>",              operator_twice,  { .i = OP_SHIFT_LEFT     } },
	{ "<C-t>",              operator_twice,  { .i = OP_SHIFT_RIGHT    } },
	{ "<C-x><C-e>",         wslide,          { .i = -1                } },
	{ "<C-x><C-y>",         wslide,          { .i = +1                } },
	{ "<Tab>",              insert_tab,      { NULL                   } },
	{ "<End>",              movement,        { .i = MOVE_LINE_END     } },
	{ "<C-r>",              insert_register, { NULL                   } },
	{ /* empty last element, array terminator */                        },
};

static void vis_mode_insert_leave(Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_idle(void) {
	text_snapshot(vis->win->file->text);
}

static void vis_mode_insert_input(const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = view_cursor_get(vis->win->view);
	if (pos != oldpos)
		buffer_truncate(&vis->buffer_repeat);
	buffer_append(&vis->buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(&vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_INSERT];
	editor_insert_key(vis, str, len);
}

static KeyBinding vis_mode_replace[] = {
	{ "<Escape>",                  switchmode,   { .i = VIS_MODE_NORMAL  } },
	{ /* empty last element, array terminator */                           },
};

static void vis_mode_replace_leave(Mode *old) {
	/* make sure we can recover the current state after an editing operation */
	text_snapshot(vis->win->file->text);
}

static void vis_mode_replace_input(const char *str, size_t len) {
	static size_t oldpos = EPOS;
	size_t pos = view_cursor_get(vis->win->view);
	if (pos != oldpos)
		buffer_truncate(&vis->buffer_repeat);
	buffer_append(&vis->buffer_repeat, str, len);
	oldpos = pos + len;
	action_reset(&vis->action_prev);
	vis->action_prev.op = &ops[OP_REPEAT_REPLACE];
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
 *            READLINE                      MOVE
 *            /       \                 (h,j,k,l ...)
 *           /         \                     |       \-----------------\
 *          /           \                    |                         |
 *       INSERT       PROMPT             OPERATOR ++++           INNER-TEXTOBJ
 *          |      (history etc)       (d,c,y,p ..)   +      (i [wsp[]()b<>{}B"'`] )
 *          |                                |     \\  +               |
 *          |                                |      \\  +              |
 *       REPLACE                           NORMAL    \\  +          TEXTOBJ
 *                                           |        \\  +     (a [wsp[]()b<>{}B"'`] )
 *                                           |         \\  +   +       |
 *                                           |          \\  + +        |
 *                                         VISUAL        \\     OPERATOR-OPTION
 *                                           |            \\        (v,V)
 *                                           |             \\        //
 *                                           |              \\======//
 *                                      VISUAL-LINE
 */

static Mode vis_modes[] = {
	[VIS_MODE_BASIC] = {
		.name = "BASIC",
		.parent = NULL,
		.default_bindings = basic_movement,
	},
	[VIS_MODE_MOVE] = {
		.name = "MOVE",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.default_bindings = vis_movements,
	},
	[VIS_MODE_TEXTOBJ] = {
		.name = "TEXTOBJ",
		.common_prefix = true,
		.parent = &vis_modes[VIS_MODE_MOVE],
		.default_bindings = vis_textobjs,
	},
	[VIS_MODE_OPERATOR_OPTION] = {
		.name = "OPERATOR-OPTION",
		.parent = &vis_modes[VIS_MODE_TEXTOBJ],
		.default_bindings = vis_operator_options,
	},
	[VIS_MODE_OPERATOR] = {
		.name = "OPERATOR",
		.parent = &vis_modes[VIS_MODE_MOVE],
		.default_bindings = vis_operators,
		.enter = vis_mode_operator_enter,
		.leave = vis_mode_operator_leave,
		.input = vis_mode_operator_input,
	},
	[VIS_MODE_NORMAL] = {
		.name = "NORMAL",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.default_bindings = vis_mode_normal,
	},
	[VIS_MODE_VISUAL] = {
		.name = "--VISUAL--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.default_bindings = vis_mode_visual,
		.enter = vis_mode_visual_enter,
		.leave = vis_mode_visual_leave,
		.visual = true,
	},
	[VIS_MODE_VISUAL_LINE] = {
		.name = "--VISUAL LINE--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_VISUAL],
		.default_bindings = vis_mode_visual_line,
		.enter = vis_mode_visual_line_enter,
		.leave = vis_mode_visual_line_leave,
		.visual = true,
	},
	[VIS_MODE_READLINE] = {
		.name = "READLINE",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.default_bindings = vis_mode_readline,
	},
	[VIS_MODE_PROMPT] = {
		.name = "PROMPT",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.default_bindings = vis_mode_prompt,
		.input = vis_mode_prompt_input,
		.enter = vis_mode_prompt_enter,
		.leave = vis_mode_prompt_leave,
	},
	[VIS_MODE_INSERT] = {
		.name = "--INSERT--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_READLINE],
		.default_bindings = vis_mode_insert,
		.leave = vis_mode_insert_leave,
		.input = vis_mode_insert_input,
		.idle = vis_mode_insert_idle,
		.idle_timeout = 3,
	},
	[VIS_MODE_REPLACE] = {
		.name = "--REPLACE--",
		.isuser = true,
		.parent = &vis_modes[VIS_MODE_INSERT],
		.default_bindings = vis_mode_replace,
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
	COLOR_WHITESPACE   = COLOR_COMMENT,
	COLOR_SPACES       = COLOR_WHITESPACE,
	COLOR_TABS         = COLOR_WHITESPACE,
	COLOR_EOL          = COLOR_WHITESPACE,
	COLOR_EOF          = COLOR_WHITESPACE,
};

static Color colors[] = {
	[COLOR_NOHILIT] = { .fg = UI_COLOR_DEFAULT, .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	[COLOR_SYNTAX0] = { .fg = UI_COLOR_RED,     .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_BOLD   },
	[COLOR_SYNTAX1] = { .fg = UI_COLOR_GREEN,   .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_BOLD   },
	[COLOR_SYNTAX2] = { .fg = UI_COLOR_GREEN,   .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	[COLOR_SYNTAX3] = { .fg = UI_COLOR_MAGENTA, .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_BOLD   },
	[COLOR_SYNTAX4] = { .fg = UI_COLOR_MAGENTA, .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	[COLOR_SYNTAX5] = { .fg = UI_COLOR_BLUE,    .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_BOLD   },
	[COLOR_SYNTAX6] = { .fg = UI_COLOR_RED,     .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	[COLOR_SYNTAX7] = { .fg = UI_COLOR_BLUE,    .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	[COLOR_SYNTAX8] = { .fg = UI_COLOR_CYAN,    .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	[COLOR_SYNTAX9] = { .fg = UI_COLOR_YELLOW,  .bg = UI_COLOR_DEFAULT, .attr = UI_ATTR_NORMAL },
	{ /* empty last element, array terminator */                                               }
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

#define SYNTAX_MULTILINE_COMMENT {    \
	"(/\\*([^*]|\\*+[^*/])*\\*+/|/\\*([^*]|\\*+[^*/])*$|^([^/]|/+[^/*])*\\*/)", \
	&colors[COLOR_COMMENT],       \
	true, /* multiline */         \
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
	"(^#[\\t ]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma)?)", \
	&colors[COLOR_PREPROCESSOR], \
}

#define SYNTAX_SPACES    { "\xC2\xB7",     &colors[COLOR_SPACES] }
#define SYNTAX_TABS      { "\xE2\x96\xB6", &colors[COLOR_TABS]   }
#define SYNTAX_TABS_FILL { " ",            &colors[COLOR_TABS]   }
#define SYNTAX_EOL       { "\xE2\x8F\x8E", &colors[COLOR_EOL]    }
#define SYNTAX_EOF       { "~",            &colors[COLOR_EOF]    }

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
		"set show spaces=0 tabs=1 newlines=1",
		NULL
	},
	.symbols = {
		SYNTAX_SPACES,
		SYNTAX_TABS,
		SYNTAX_TABS_FILL,
		SYNTAX_EOL,
		SYNTAX_EOF,
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
		"\\$[?!@#$?*-]",
		&colors[COLOR_VARIABLE],
	},{
		"\\$\\{[A-Za-z_][0-9A-Za-z_]+\\}",
		&colors[COLOR_VARIABLE],
	},{
		"\\$[A-Za-z_][0-9A-Za-z_]+",
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
	.name = "javascript",
	.file = "\\.(js|json)$",
	.rules = {
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_BRACKET,
	{
		B"(true|false|null|undefined)"B,
		&colors[COLOR_DATATYPE],
	},{
		B"(NaN|Infinity)"B,
		&colors[COLOR_LITERAL],
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		&colors[COLOR_STRING],
	},{
		B"(for|if|while|do|in|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(continue|break|return)"B,
		&colors[COLOR_CONTROL],
	},{
		B"(case|class|const|debugger|default|enum|export|extends|finally|function|implements|import|instanceof|let|this|typeof|var|with|yield)"B,
		&colors[COLOR_KEYWORD2],
	}}
},{
	.name = "lua",
	.file = "\\.lua$",
	.settings = (const char*[]){
		"set number",
		"set autoindent",
		NULL
	},
	.rules = {{
		"--\\[(=*)\\[([^]]*)\\](=*)\\]",
		&colors[COLOR_COMMENT],
		true,
	},{
		"--.*$",
		&colors[COLOR_COMMENT],
	},{
		"(\\[(=*)\\[([^]]*)\\](=*)\\]|^([^][]*)\\](=*)\\])",
		&colors[COLOR_STRING],
		true,
	},
		SYNTAX_STRING,
	{
		B"([0-9]*\\.)?[0-9]+([eE]([\\+-])?[0-9]+)?"B,
		&colors[COLOR_LITERAL],
	},{
		B"0x[0-9a-fA-F]+"B,
		&colors[COLOR_LITERAL],
	},{
		B"(false|nil|true)"B,
		&colors[COLOR_CONSTANT],
	},{
		"(\\.\\.\\.)",
		&colors[COLOR_CONSTANT],
	},{
		B"(break|do|else|elseif|end|for|function|if|in|local|repeat|return|then|until|while)"B,
		&colors[COLOR_KEYWORD],
	},{
		B"(and|not|or)"B,
		&colors[COLOR_OPERATOR],
	},{
		"(\\+|-|\\*|/|%|\\^|#|[=~<>]=|<|>|\\.\\.)",
		&colors[COLOR_OPERATOR],
	},
		SYNTAX_BRACKET,
	}
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
	.name = "apl",
	.file = "\\.apl$",
	.settings = (const char*[]){
		"set number",
		NULL
	},
	.rules = {{
		"(⍝|#).*$",
		&colors[COLOR_COMMENT],
	},{
		"('([^']|'')*')|(\"([^\"]|\"\")*\")",
		&colors[COLOR_STRING],
	},{
		"^ *(∇|⍫)",
		&colors[COLOR_SYNTAX9],
	},{
		"(⎕[a-zA-Z]*)|[⍞χ⍺⍶⍵⍹]",
		&colors[COLOR_KEYWORD],
	},{
		"[∆⍙_a-zA-Z][∆⍙_¯a-zA-Z0-9]* *:",
		&colors[COLOR_SYNTAX2],
	},{
		"[∆⍙_a-zA-Z][∆⍙_¯a-zA-Z0-9]*",
		&colors[COLOR_IDENTIFIER],
	},{
		"¯?(([0-9]+(\\.[0-9]+)?)|\\.[0-9]+)([eE]¯?[0-9]+)?([jJ]¯?(([0-9]+(\\.[0-9]+)?)|\\.[0-9]+)([eE]¯?[0-9]+)?)?",
		&colors[COLOR_CONSTANT],
	},{
		"[][(){}]",
		&colors[COLOR_BRACKETS],
	},{
		"[←→◊]",
		&colors[COLOR_SYNTAX3],
	}}
},{
	/* empty last element, array terminator */
}};
