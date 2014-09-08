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

static Mode *mode, *mode_prev;
static Vis *vis;
static Mode vis_modes[];
static Action action, action_prev;

static void switchmode(const Arg *arg);
static void switchmode_to(Mode *new_mode);

enum {
	VIS_MODE_BASIC,
	VIS_MODE_MARK,
	VIS_MODE_MARK_LINE,
	VIS_MODE_MARK_SET,
	VIS_MODE_MOVE,
	VIS_MODE_TEXTOBJ,
	VIS_MODE_OPERATOR,
	VIS_MODE_OPERATOR_OPTION,
	VIS_MODE_REGISTER,
	VIS_MODE_NORMAL,
	VIS_MODE_VISUAL,
	VIS_MODE_READLINE,
	VIS_MODE_PROMPT,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
};

enum {
	OP_DELETE,
	OP_CHANGE,
	OP_YANK,
	OP_PASTE,
};

void op_delete(OperatorContext *c) {
	size_t len = c->range.end - c->range.start;
	c->reg->linewise = c->linewise;
	register_put(c->reg, vis->win->text, &c->range);
	vis_delete(vis, c->range.start, len);
	window_cursor_to(vis->win->win, c->range.start);
}

void op_change(OperatorContext *c) {
	op_delete(c);
	switchmode(&(const Arg){ .i = VIS_MODE_INSERT });
}

void op_yank(OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put(c->reg, vis->win->text, &c->range);
}

void op_paste(OperatorContext *c) {
	size_t pos = window_cursor_get(vis->win->win);
	if (c->reg->linewise)
		pos = text_line_next(vis->win->text, pos);
	vis_insert(vis, pos, c->reg->data, c->reg->len);
	window_cursor_to(vis->win->win, pos + c->reg->len);
}

static size_t search_forward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	return text_search_forward(vis->win->text, pos, vis->search_pattern);
}

static size_t search_backward(const Arg *arg) {
	size_t pos = window_cursor_get(vis->win->win);
	return text_search_backward(vis->win->text, pos, vis->search_pattern);
}

static void mark_set(const Arg *arg) {
	text_mark_set(vis->win->text, arg->i, window_cursor_get(vis->win->win));
}

static size_t mark_goto(const Arg *arg) {
	return text_mark_get(vis->win->text, action.mark);
}

static size_t mark_line_goto(const Arg *arg) {
	return text_line_start(vis->win->text, mark_goto(arg));
}

static size_t to(const Arg *arg) {
	return text_find_char_next(vis->win->text, window_cursor_get(vis->win->win) + 1,
		action.key.str, strlen(action.key.str));
}

static size_t till(const Arg *arg) {
	return text_char_prev(vis->win->text, to(arg));
}

static size_t to_left(const Arg *arg) {
	return text_find_char_prev(vis->win->text, window_cursor_get(vis->win->win) - 1,
		action.key.str, strlen(action.key.str));
}

static size_t till_left(const Arg *arg) {
	return text_char_next(vis->win->text, to_left(arg));
}

static size_t line(const Arg *arg) {
	if (action.count == 0)
		return text_size(vis->win->text);
	size_t pos = text_pos_by_lineno(vis->win->text, action.count);
	action.count = 0;
	return pos;
}

static size_t column(const Arg *arg) {
	char c;
	VisWin *win = vis->win;
	size_t pos = window_cursor_get(win->win);
	Iterator it = text_iterator_get(win->text, text_line_begin(win->text, pos));
	while (action.count-- > 0 && text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	action.count = 0;
	return it.pos;
}

static void insert(const Arg *arg) {
	vis_insert_key(vis, arg->s, arg->s ? strlen(arg->s) : 0);
}

static void insert_tab(const Arg *arg) {
	insert(&(const Arg){ .s = "\t" });
}

static void insert_newline(const Arg *arg) {
	// TODO determine file type to insert \n\r or \n
	insert(&(const Arg){ .s = "\n" });
}

static Operator ops[] = {
	[OP_DELETE] = { op_delete, false },
	[OP_CHANGE] = { op_change, false },
	[OP_YANK]   = { op_yank,   false },
	[OP_PASTE]  = { op_paste,  true  },
};

enum {
	MOVE_LINE_UP,
	MOVE_LINE_DOWN,
	MOVE_LINE_BEGIN,
	MOVE_LINE_START,
	MOVE_LINE_FINISH,
	MOVE_LINE_END,
	MOVE_LINE,
	MOVE_COLUMN,
	MOVE_CHAR_PREV,
	MOVE_CHAR_NEXT,
	MOVE_WORD_START_PREV,
	MOVE_WORD_START_NEXT,
	MOVE_WORD_END_PREV,
	MOVE_WORD_END_NEXT,
	MOVE_SENTENCE_PREV,
	MOVE_SENTENCE_NEXT,
	MOVE_PARAGRAPH_PREV,
	MOVE_PARAGRAPH_NEXT,
	MOVE_BRACKET_MATCH,
	MOVE_LEFT_TO,
	MOVE_RIGHT_TO,
	MOVE_LEFT_TILL,
	MOVE_RIGHT_TILL,
	MOVE_FILE_BEGIN,
	MOVE_FILE_END,
	MOVE_MARK,
	MOVE_MARK_LINE,
	MOVE_SEARCH_FORWARD,
	MOVE_SEARCH_BACKWARD,
};

static Movement moves[] = {
	[MOVE_LINE_UP]         = { .win = window_line_up                                   },
	[MOVE_LINE_DOWN]       = { .win = window_line_down                                 },
	[MOVE_LINE_BEGIN]      = { .txt = text_line_begin,      .type = LINEWISE           },
	[MOVE_LINE_START]      = { .txt = text_line_start,      .type = LINEWISE           },
	[MOVE_LINE_FINISH]     = { .txt = text_line_finish,     .type = LINEWISE           },
	[MOVE_LINE_END]        = { .txt = text_line_end,        .type = LINEWISE           },
	[MOVE_LINE]            = { .cmd = line,                 .type = LINEWISE           },
	[MOVE_COLUMN]          = { .cmd = column,               .type = CHARWISE           },
	[MOVE_CHAR_PREV]       = { .win = window_char_prev                                 },
	[MOVE_CHAR_NEXT]       = { .win = window_char_next                                 },
	[MOVE_WORD_START_PREV] = { .txt = text_word_start_prev, .type = CHARWISE           },
	[MOVE_WORD_START_NEXT] = { .txt = text_word_start_next, .type = CHARWISE           },
	[MOVE_WORD_END_PREV]   = { .txt = text_word_end_prev,   .type = CHARWISE|INCLUSIVE },
	[MOVE_WORD_END_NEXT]   = { .txt = text_word_end_next,   .type = CHARWISE|INCLUSIVE },
	[MOVE_SENTENCE_PREV]   = { .txt = text_sentence_prev,   .type = LINEWISE           },
	[MOVE_SENTENCE_NEXT]   = { .txt = text_sentence_next,   .type = LINEWISE           },
	[MOVE_PARAGRAPH_PREV]  = { .txt = text_paragraph_prev,  .type = LINEWISE           },
	[MOVE_PARAGRAPH_NEXT]  = { .txt = text_paragraph_next,  .type = LINEWISE           },
	[MOVE_BRACKET_MATCH]   = { .txt = text_bracket_match,   .type = LINEWISE|INCLUSIVE },
	[MOVE_FILE_BEGIN]      = { .txt = text_begin,           .type = LINEWISE           },
	[MOVE_FILE_END]        = { .txt = text_end,             .type = LINEWISE           },
	[MOVE_LEFT_TO]         = { .cmd = to_left,              .type = LINEWISE           },
	[MOVE_RIGHT_TO]        = { .cmd = to,                   .type = LINEWISE           },
	[MOVE_LEFT_TILL]       = { .cmd = till_left,            .type = LINEWISE           },
	[MOVE_RIGHT_TILL]      = { .cmd = till,                 .type = LINEWISE           },
	[MOVE_MARK]            = { .cmd = mark_goto,            .type = LINEWISE           },
	[MOVE_MARK_LINE]       = { .cmd = mark_line_goto,       .type = LINEWISE           },
	[MOVE_SEARCH_FORWARD]  = { .cmd = search_forward,       .type = LINEWISE           },
	[MOVE_SEARCH_BACKWARD] = { .cmd = search_backward,      .type = LINEWISE           },
};

enum {
	TEXT_OBJ_WORD,
	TEXT_OBJ_LINE_UP,
	TEXT_OBJ_LINE_DOWN,
	TEXT_OBJ_SENTENCE,
	TEXT_OBJ_PARAGRAPH,
	TEXT_OBJ_OUTER_SQUARE_BRACKET,
	TEXT_OBJ_INNER_SQUARE_BRACKET,
	TEXT_OBJ_OUTER_CURLY_BRACKET,
	TEXT_OBJ_INNER_CURLY_BRACKET,
	TEXT_OBJ_OUTER_ANGLE_BRACKET,
	TEXT_OBJ_INNER_ANGLE_BRACKET,
	TEXT_OBJ_OUTER_PARANTHESE,
	TEXT_OBJ_INNER_PARANTHESE,
	TEXT_OBJ_OUTER_QUOTE,
	TEXT_OBJ_INNER_QUOTE,
	TEXT_OBJ_OUTER_SINGLE_QUOTE,
	TEXT_OBJ_INNER_SINGLE_QUOTE,
	TEXT_OBJ_OUTER_BACKTICK,
	TEXT_OBJ_INNER_BACKTICK,
};

static TextObject textobjs[] = {
	[TEXT_OBJ_WORD]                 = { text_object_word                  },
	[TEXT_OBJ_LINE_UP]              = { text_object_line                  },
	[TEXT_OBJ_LINE_DOWN]            = { text_object_line                  },
	[TEXT_OBJ_SENTENCE]             = { text_object_sentence              },
	[TEXT_OBJ_PARAGRAPH]            = { text_object_paragraph             },
	[TEXT_OBJ_OUTER_SQUARE_BRACKET] = { text_object_square_bracket, OUTER },
	[TEXT_OBJ_INNER_SQUARE_BRACKET] = { text_object_square_bracket, INNER },
	[TEXT_OBJ_OUTER_CURLY_BRACKET]  = { text_object_curly_bracket,  OUTER },
	[TEXT_OBJ_INNER_CURLY_BRACKET]  = { text_object_curly_bracket,  INNER },
	[TEXT_OBJ_OUTER_ANGLE_BRACKET]  = { text_object_angle_bracket,  OUTER },
	[TEXT_OBJ_INNER_ANGLE_BRACKET]  = { text_object_angle_bracket,  INNER },
	[TEXT_OBJ_OUTER_PARANTHESE]     = { text_object_paranthese,     OUTER },
	[TEXT_OBJ_INNER_PARANTHESE]     = { text_object_paranthese,     INNER },
	[TEXT_OBJ_OUTER_QUOTE]          = { text_object_quote,          OUTER },
	[TEXT_OBJ_INNER_QUOTE]          = { text_object_quote,          INNER },
	[TEXT_OBJ_OUTER_SINGLE_QUOTE]   = { text_object_single_quote,   OUTER },
	[TEXT_OBJ_INNER_SINGLE_QUOTE]   = { text_object_single_quote,   INNER },
	[TEXT_OBJ_OUTER_BACKTICK]       = { text_object_backtick,       OUTER },
	[TEXT_OBJ_INNER_BACKTICK]       = { text_object_backtick,       INNER },
};

static TextObject *moves_linewise[] = {
	[MOVE_LINE_UP]   = &textobjs[TEXT_OBJ_LINE_UP],
	[MOVE_LINE_DOWN] = &textobjs[TEXT_OBJ_LINE_DOWN],
};

/* draw a statubar, do whatever you want with the given curses window */
static void statusbar(WINDOW *win, bool active, const char *filename, size_t line, size_t col) {
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

static void quit(const Arg *arg) {
	endwin();
	exit(0);
}

static void split(const Arg *arg) {
	vis_window_split(vis, arg->s);
}

void action_do(Action *a); 
void action_reset(Action *a);

static void snapshot(void) {
	text_snapshot(vis->win->text);
}

static void repeat(const Arg *arg) {
	action = action_prev;
	action_do(&action);
}

static void count(const Arg *arg) {
	action.count = action.count * 10 + arg->i;
}

static void linewise(const Arg *arg) {
	action.linewise = arg->b;
}

static void operator(const Arg *arg) {
	Operator *op = &ops[arg->i];
	if (mode == &vis_modes[VIS_MODE_VISUAL]) {
		action.op = op;
		action_do(&action);
		return;
	}

	switchmode(&(const Arg){ .i = VIS_MODE_OPERATOR });
	if (action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		action.linewise = true;
		action.textobj = moves_linewise[MOVE_LINE_DOWN];
		action_do(&action);
	} else {
		action.op = op;
		if (op->selfcontained)
			action_do(&action);
	}
}

static bool operator_invalid(const char *str, size_t len) {
	action_reset(&action);
	switchmode_to(mode_prev);
	return false;
}

static void movement_key(const Arg *arg) {
	Key k = getkey();
	if (!k.str[0]) {
		action_reset(&action);
		return;
	}
	action.key = k;
	action.movement = &moves[arg->i];
	action_do(&action);
}

static void movement(const Arg *arg) {
	if (action.linewise && arg->i < LENGTH(moves_linewise))
		action.textobj = moves_linewise[arg->i];
	else
		action.movement = &moves[arg->i];
	action_do(&action);
}

static void textobj(const Arg *arg) {
	action.textobj = &textobjs[arg->i];
	action_do(&action);
}

static void reg(const Arg *arg) {
	action.reg = &vis->registers[arg->i];
}

static void mark(const Arg *arg) {
	action.mark = arg->i;
	action.movement = &moves[MOVE_MARK];
	action_do(&action);
}

static void mark_line(const Arg *arg) {
	action.mark = arg->i;
	action.movement = &moves[MOVE_MARK_LINE];
	action_do(&action);
}

void action_reset(Action *a) {
	a->count = 0;
	a->linewise = false;
	a->op = NULL;
	a->movement = NULL;
	a->textobj = NULL;
	a->reg = NULL;
}

void action_do(Action *a) {
	Text *txt = vis->win->text;
	Win *win = vis->win->win;
	size_t pos = window_cursor_get(win);
	if (a->count == 0)
		a->count = 1;
	OperatorContext c = {
		.count = a->count,
		.pos = pos,
		.reg = a->reg ? a->reg : &vis->registers[REG_DEFAULT],
		.linewise = a->linewise,
	};

	if (a->movement) {
		size_t start = pos;
		for (int i = 0; i < a->count; i++) {
			if (a->movement->txt)
				pos = a->movement->txt(txt, pos);
			else if (a->movement->win)
				pos = a->movement->win(win);
			else
				pos = a->movement->cmd(&a->arg);
			if (pos == (size_t)-1)
				break;
		}

		if (pos == (size_t)-1) {
			c.range.start = start;
			c.range.end = start;
		} else {
			c.range.start = MIN(start, pos);
			c.range.end = MAX(start, pos);
		}

		if (!a->op) {
			if (a->movement->type & CHARWISE)
				window_scroll_to(win, pos);
			else
				window_cursor_to(win, pos);
		} else if (a->movement->type & INCLUSIVE) {
			Iterator it = text_iterator_get(txt, c.range.end);
			text_iterator_char_next(&it, NULL);
			c.range.end = it.pos;
		}
	} else if (a->textobj) {
		Filerange r;
		c.range.start = c.range.end = pos;
		for (int i = 0; i < a->count; i++) {
			r = a->textobj->range(txt, pos);
			// TODO range_valid?
			if (r.start == (size_t)-1 || r.end == (size_t)-1)
				break;
			if (a->textobj->type == OUTER) {
				r.start--;
				r.end++;
			}
			// TODO c.range = range_union(&c.range, &r);
			c.range.start = MIN(c.range.start, r.start);
			c.range.end = MAX(c.range.end, r.end);
			if (i < a->count - 1) {
				if (a->textobj == &textobjs[TEXT_OBJ_LINE_UP]) {
					pos = c.range.start - 1;
				} else {
					pos = c.range.end + 1;
				}
			}
		}
	} else if (mode == &vis_modes[VIS_MODE_VISUAL]) {
		c.range = window_selection_get(win);
		if (c.range.start == (size_t)-1 || c.range.end == (size_t)-1)
			c.range.start = c.range.end = pos;
	}

	if (a->op) {
		a->op->func(&c);
		if (mode == &vis_modes[VIS_MODE_OPERATOR])
			switchmode_to(mode_prev);
		else if (mode == &vis_modes[VIS_MODE_VISUAL])
			switchmode(&(const Arg){ .i = VIS_MODE_NORMAL });
		snapshot();
	}

	if (a != &action_prev) {
		if (a->op)
			action_prev = *a;
		action_reset(a);
	}
}

static void undo(const Arg *arg) {
	if (text_undo(vis->win->text))
		vis_draw(vis);
}

static void redo(const Arg *arg) {
	if (text_redo(vis->win->text))
		vis_draw(vis);
}

static void zero(const Arg *arg) {
	if (action.count == 0)
		movement(&(const Arg){ .i = MOVE_LINE_BEGIN });
	else
		count(&(const Arg){ .i = 0 });
}

static void delete_word(const Arg *arg) {
	operator(&(const Arg){ .i = OP_DELETE });
	movement(&(const Arg){ .i = MOVE_WORD_START_PREV });
}

static void insert_register(const Arg *arg) {
	Register *reg = &vis->registers[arg->i];
	vis_insert(vis, window_cursor_get(vis->win->win), reg->data, reg->len);
}

static void prompt(const Arg *arg) {
	vis_prompt_show(vis, arg->s);
	switchmode(&(const Arg){ .i = VIS_MODE_PROMPT });
}

static void prompt_enter(const Arg *arg) {
	char *s = vis_prompt_get(vis);
	fprintf(stderr, "prompt: %s\n", s);
	switchmode(&(const Arg){ .i = VIS_MODE_NORMAL });
	switch (vis->prompt->title[0]) {
	case '/':
	case '?':
		text_regex_free(vis->search_pattern);
		if (!(vis->search_pattern = text_regex_new()) ||
		    text_regex_compile(vis->search_pattern, s, REG_EXTENDED)) {
			action_reset(&action);
		} else {
			movement(&(const Arg){ .i = vis->prompt->title[0] == '/' ?
				MOVE_SEARCH_FORWARD : MOVE_SEARCH_BACKWARD });
		}
		break;
	case ':':
		/* TODO : parse command */
		break;
	}
	free(s);
}

static void prompt_up(const Arg *arg) {
	
}

static void prompt_down(const Arg *arg) {

}

static void insert_verbatim(const Arg *arg) {
	int value = 0;
	for (int i = 0; i < 3; i++) {
		Key k = getkey();
		if (k.str[0] < '0' || k.str[0] > '9')
			return;
		value = value * 10 + k.str[0] - '0';
	}
	char v = value;
	vis_insert(vis, window_cursor_get(vis->win->win), &v, 1);
}

/* use vim's  
   :help motion
   :h operator
   :h text-objects
 as reference 
*/

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

// TODO: factor out prefix [ia] into spearate mode which sets a flag
static KeyBinding vis_textobjs[] = {
	{ { NONE('a'), NONE('w')    }, textobj,  { .i = TEXT_OBJ_WORD                 } },
	{ { NONE('i'), NONE('w')    }, textobj,  { .i = TEXT_OBJ_WORD                 } },
	{ { NONE('a'), NONE('s')    }, textobj,  { .i = TEXT_OBJ_SENTENCE             } },
	{ { NONE('i'), NONE('s')    }, textobj,  { .i = TEXT_OBJ_SENTENCE             } },
	{ { NONE('a'), NONE('p')    }, textobj,  { .i = TEXT_OBJ_PARAGRAPH            } },
	{ { NONE('i'), NONE('p')    }, textobj,  { .i = TEXT_OBJ_PARAGRAPH            } },
	{ { NONE('a'), NONE('[')    }, textobj,  { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ { NONE('a'), NONE(']')    }, textobj,  { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ { NONE('i'), NONE('[')    }, textobj,  { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ { NONE('i'), NONE(']')    }, textobj,  { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ { NONE('a'), NONE('(')    }, textobj,  { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE(')')    }, textobj,  { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE('b')    }, textobj,  { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('i'), NONE('(')    }, textobj,  { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE(')')    }, textobj,  { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE('b')    }, textobj,  { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('a'), NONE('<')    }, textobj,  { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ { NONE('a'), NONE('>')    }, textobj,  { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ { NONE('i'), NONE('<')    }, textobj,  { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ { NONE('i'), NONE('>')    }, textobj,  { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ { NONE('a'), NONE('{')    }, textobj,  { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('}')    }, textobj,  { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('B')    }, textobj,  { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('{')    }, textobj,  { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('}')    }, textobj,  { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('B')    }, textobj,  { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('"')    }, textobj,  { .i = TEXT_OBJ_OUTER_QUOTE          } },
	{ { NONE('i'), NONE('"')    }, textobj,  { .i = TEXT_OBJ_INNER_QUOTE          } },
	{ { NONE('a'), NONE('\'')   }, textobj,  { .i = TEXT_OBJ_OUTER_SINGLE_QUOTE   } },
	{ { NONE('i'), NONE('\'')   }, textobj,  { .i = TEXT_OBJ_INNER_SINGLE_QUOTE   } },
	{ { NONE('a'), NONE('`')    }, textobj,  { .i = TEXT_OBJ_OUTER_BACKTICK       } },
	{ { NONE('i'), NONE('`')    }, textobj,  { .i = TEXT_OBJ_INNER_BACKTICK       } },
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

static KeyBinding vis_operator_options[] = {
	{ { NONE('v')               }, linewise,      { .b = false           } },
	{ { NONE('V')               }, linewise,      { .b = true            } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_registers[] = { /* {a-zA-Z0-9.%#:-"} */
	{ { NONE('"'), NONE('a')    }, reg,           { .i = REG_a           } },
	{ { NONE('"'), NONE('b')    }, reg,           { .i = REG_b           } },
	{ { NONE('"'), NONE('c')    }, reg,           { .i = REG_c           } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks[] = { /* {a-zA-Z} */
	{ { NONE('`'), NONE('a')    }, mark,          { .i = MARK_a          } },
	{ { NONE('`'), NONE('b')    }, mark,          { .i = MARK_b          } },
	{ { NONE('`'), NONE('c')    }, mark,          { .i = MARK_c          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks_line[] = { /* {a-zA-Z} */
	{ { NONE('\''), NONE('a')   }, mark_line,     { .i = MARK_a          } },
	{ { NONE('\''), NONE('b')   }, mark_line,     { .i = MARK_b          } },
	{ { NONE('\''), NONE('c')   }, mark_line,     { .i = MARK_c          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks_set[] = { /* {a-zA-Z} */
	{ { NONE('m'), NONE('a')    }, mark_set,      { .i = MARK_a          } },
	{ { NONE('m'), NONE('b')    }, mark_set,      { .i = MARK_b          } },
	{ { NONE('m'), NONE('c')    }, mark_set,      { .i = MARK_c          } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_normal[] = {
	{ { CONTROL('w'), NONE('c') }, split,    { .s = NULL                   } },
	{ { CONTROL('w'), NONE('j') }, call,     { .f = vis_window_next     } },
	{ { CONTROL('w'), NONE('k') }, call,     { .f = vis_window_prev     } },
	{ { CONTROL('F')            }, cursor,   { .m = window_page_up         } },
	{ { CONTROL('B')            }, cursor,   { .m = window_page_down       } },
	{ { NONE('.')               }, repeat,   {                             } },
	{ { NONE('n')               }, movement, { .i = MOVE_SEARCH_FORWARD    } },
	{ { NONE('N')               }, movement, { .i = MOVE_SEARCH_BACKWARD   } },
	{ { NONE('x')               }, call,          { .f = vis_delete_key   } },
	{ { NONE('i')               }, switchmode,    { .i = VIS_MODE_INSERT } },
	{ { NONE('v')               }, switchmode,    { .i = VIS_MODE_VISUAL } },
	{ { NONE('R')               }, switchmode,    { .i = VIS_MODE_REPLACE} },
	{ { NONE('u')               }, undo,          { NULL                   } },
	{ { CONTROL('R')            }, redo,          { NULL                   } },
	{ { CONTROL('L')            }, call,          { .f = vis_draw     } },
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
	BACKSPACE(                     call,               f,  vis_backspace_key   ),
	{ { CONTROL('D')            }, call,            { .f = vis_delete_key    } },
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
	if (new != &vis_modes[VIS_MODE_OPERATOR])
		vis_prompt_hide(vis);
}

static KeyBinding vis_insert_mode[] = {
	{ { CONTROL('L')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('[')            }, switchmode,      { .i = VIS_MODE_NORMAL   } },
	{ { CONTROL('I')            }, insert_tab,      { NULL                   } },
	{ { CONTROL('J')            }, insert_newline,  { NULL                   } },
	{ { CONTROL('M')            }, insert_newline,  { NULL                   } },
	{ { CONTROL('O')            }, switchmode,      { .i = VIS_MODE_OPERATOR } },
	{ { CONTROL('R'), NONE('a') }, insert_register, { .i = REG_a             } },
	{ { CONTROL('R'), NONE('b') }, insert_register, { .i = REG_b             } },
	{ { CONTROL('R'), NONE('c') }, insert_register, { .i = REG_c             } },
	{ { CONTROL('V')            }, insert_verbatim, { NULL                   } },
	{ /* empty last element, array terminator */                               },
};

static bool vis_insert_input(const char *str, size_t len) {
	vis_insert_key(vis, str, len);
	return true;
}

static KeyBinding vis_replace[] = {
	{ { NONE(ESC)               }, switchmode,   { .i = VIS_MODE_NORMAL  } },
	{ /* empty last element, array terminator */                           },
};

static bool vis_replace_input(const char *str, size_t len) {
	vis_replace_key(vis, str, len);
	return true;
}

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
	[VIS_MODE_TEXTOBJ] = { 
		.name = "TEXTOBJ",
		.parent = &vis_modes[VIS_MODE_MOVE],
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
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.parent = &vis_modes[VIS_MODE_READLINE],
		.bindings = vis_insert_mode,
		.input = vis_insert_input,
		.idle = snapshot,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.parent = &vis_modes[VIS_MODE_INSERT],
		.bindings = vis_replace,
		.input = vis_replace_input,
		.idle = snapshot,
	},
};

static void switchmode_to(Mode *new_mode) {
	if (mode == new_mode)
		return;
	if (mode->leave)
		mode->leave(new_mode);
	mode_prev = mode;
	//fprintf(stderr, "%s -> %s\n", mode_prev->name, new_mode->name);
	mode = new_mode;
	if (mode->enter)
		mode->enter(mode_prev);
	// TODO display mode name somewhere?

}

static void switchmode(const Arg *arg) {
	switchmode_to(&vis_modes[arg->i]);
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
static KeyBinding nano_keys[] = {
	{ { CONTROL('D')            }, call,           { .f = vis_delete_key   } },
	BACKSPACE(                     call,              f,  vis_backspace_key  ),
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
	{ { CONTROL('L')            }, call,      { .f = vis_draw              } },
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
