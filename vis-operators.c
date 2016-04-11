#include <string.h>
#include <ctype.h>
#include "vis-core.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-util.h"
#include "util.h"

static size_t op_delete(Vis *vis, Text *txt, OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put_range(vis, c->reg, txt, &c->range);
	text_delete_range(txt, &c->range);
	size_t pos = c->range.start;
	if (c->linewise && pos == text_size(txt))
		pos = text_line_begin(txt, text_line_prev(txt, pos));
	return pos;
}

static size_t op_change(Vis *vis, Text *txt, OperatorContext *c) {
	op_delete(vis, txt, c);
	macro_operator_record(vis);
	return c->range.start;
}

static size_t op_yank(Vis *vis, Text *txt, OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_put_range(vis, c->reg, txt, &c->range);
	if (c->reg == &vis->registers[VIS_REG_DEFAULT])
		register_put_range(vis, &vis->registers[VIS_REG_ZERO], txt, &c->range);
	return c->linewise ? c->pos : c->range.start;
}

static size_t op_put(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = c->pos;
	bool sel = text_range_size(&c->range) > 0;
	bool sel_linewise = sel && text_range_is_linewise(txt, &c->range);
	if (sel) {
		text_delete_range(txt, &c->range);
		pos = c->pos = c->range.start;
	}
	switch (c->arg->i) {
	case VIS_OP_PUT_AFTER:
	case VIS_OP_PUT_AFTER_END:
		if (c->reg->linewise && !sel_linewise)
			pos = text_line_next(txt, pos);
		else if (!sel)
			pos = text_char_next(txt, pos);
		break;
	case VIS_OP_PUT_BEFORE:
	case VIS_OP_PUT_BEFORE_END:
		if (c->reg->linewise)
			pos = text_line_begin(txt, pos);
		break;
	}

	size_t len;
	const char *data = register_get(vis, c->reg, &len);

	for (int i = 0; i < c->count; i++) {
		char nl;
		if (c->reg->linewise && pos > 0 && text_byte_get(txt, pos-1, &nl) && nl != '\n')
			pos += text_insert_newline(txt, pos);
		text_insert(txt, pos, data, len);
		pos += len;
		if (c->reg->linewise && pos > 0 && text_byte_get(txt, pos-1, &nl) && nl != '\n')
			pos += text_insert_newline(txt, pos);
	}

	if (c->reg->linewise) {
		switch (c->arg->i) {
		case VIS_OP_PUT_BEFORE_END:
		case VIS_OP_PUT_AFTER_END:
			pos = text_line_start(txt, pos);
			break;
		case VIS_OP_PUT_AFTER:
			pos = text_line_start(txt, text_line_next(txt, c->pos));
			break;
		case VIS_OP_PUT_BEFORE:
			pos = text_line_start(txt, c->pos);
			break;
		}
	} else {
		switch (c->arg->i) {
		case VIS_OP_PUT_AFTER:
		case VIS_OP_PUT_BEFORE:
			pos = text_char_prev(txt, pos);
			break;
		}
	}

	return pos;
}

static size_t op_shift_right(Vis *vis, Text *txt, OperatorContext *c) {
	char spaces[9] = "         ";
	spaces[MIN(vis->tabwidth, LENGTH(spaces) - 1)] = '\0';
	const char *tab = vis->expandtab ? spaces : "\t";
	size_t tablen = strlen(tab);
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	size_t inserted = 0;

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		prev_pos = pos = text_line_begin(txt, pos);
		text_insert(txt, pos, tab, tablen);
		pos = text_line_prev(txt, pos);
		inserted += tablen;
	}  while (pos >= c->range.start && pos != prev_pos);

	return c->pos + inserted;
}

static size_t op_shift_left(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	size_t tabwidth = vis->tabwidth, tablen;
	size_t deleted = 0;

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		char c;
		size_t len = 0;
		prev_pos = pos = text_line_begin(txt, pos);
		Iterator it = text_iterator_get(txt, pos);
		if (text_iterator_byte_get(&it, &c) && c == '\t') {
			len = 1;
		} else {
			for (len = 0; text_iterator_byte_get(&it, &c) && c == ' '; len++)
				text_iterator_byte_next(&it, NULL);
		}
		tablen = MIN(len, tabwidth);
		text_delete(txt, pos, tablen);
		pos = text_line_prev(txt, pos);
		deleted += tablen;
	}  while (pos >= c->range.start && pos != prev_pos);

	return c->pos - deleted;
}

static size_t op_case_change(Vis *vis, Text *txt, OperatorContext *c) {
	size_t len = text_range_size(&c->range);
	char *buf = malloc(len);
	if (!buf)
		return c->pos;
	len = text_bytes_get(txt, c->range.start, len, buf);
	size_t rem = len;
	for (char *cur = buf; rem > 0; cur++, rem--) {
		int ch = (unsigned char)*cur;
		if (isascii(ch)) {
			if (c->arg->i == VIS_OP_CASE_SWAP)
				*cur = islower(ch) ? toupper(ch) : tolower(ch);
			else if (c->arg->i == VIS_OP_CASE_UPPER)
				*cur = toupper(ch);
			else
				*cur = tolower(ch);
		}
	}

	text_delete(txt, c->range.start, len);
	text_insert(txt, c->range.start, buf, len);
	free(buf);
	return c->pos;
}

static size_t op_cursor(Vis *vis, Text *txt, OperatorContext *c) {
	View *view = vis->win->view;
	Filerange r = text_range_linewise(txt, &c->range);
	for (size_t line = text_range_line_first(txt, &r); line != EPOS; line = text_range_line_next(txt, &r, line)) {
		size_t pos;
		if (c->arg->i == VIS_OP_CURSOR_EOL)
			pos = text_line_finish(txt, line);
		else
			pos = text_line_start(txt, line);
		view_cursors_new(view, pos);
	}
	return EPOS;
}

static size_t op_wrap_text(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_prev(txt, c->range.start), prev_pos;

	if (vis->textwidth < 1)
		return c->range.start;

	do {
		prev_pos = pos;
		size_t end = text_line_next(txt, pos);
		while (pos - prev_pos < vis->textwidth && end > pos)
			pos = text_longword_end_next(txt, pos);
		if (pos - prev_pos >= vis->textwidth) {
			size_t break_pos = text_longword_start_prev(txt, pos);
			if (break_pos == prev_pos) {
				/* vis->textwidth is longer than this word, insert line break
				 * immediately after this word */
				break_pos = pos + 1;
			} else {
				/* found an appropriate line break point just before this word */
				break_pos = break_pos - 1;
			}
			text_delete(txt, break_pos, 1);
			text_insert(txt, break_pos, "\n", 1);
			pos = break_pos + 1;
		}
	} while (pos != prev_pos && pos < c->range.end);

	return c->range.end;
}

static size_t op_join(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;

	/* if operator and range are both linewise, skip last line break */
	if (c->linewise && text_range_is_linewise(txt, &c->range)) {
		size_t line_prev = text_line_prev(txt, pos);
		size_t line_prev_prev = text_line_prev(txt, line_prev);
		if (line_prev_prev >= c->range.start)
			pos = line_prev;
	}

	do {
		prev_pos = pos;
		size_t end = text_line_start(txt, pos);
		pos = text_char_next(txt, text_line_finish(txt, text_line_prev(txt, end)));
		if (pos >= c->range.start && end > pos) {
			text_delete(txt, pos, end - pos);
			text_insert(txt, pos, " ", 1);
		} else {
			break;
		}
	} while (pos != prev_pos);

	return c->range.start;
}

static size_t op_insert(Vis *vis, Text *txt, OperatorContext *c) {
	macro_operator_record(vis);
	return c->newpos != EPOS ? c->newpos : c->pos;
}

static size_t op_replace(Vis *vis, Text *txt, OperatorContext *c) {
	macro_operator_record(vis);
	return c->newpos != EPOS ? c->newpos : c->pos;
}

static size_t op_filter(Vis *vis, Text *txt, OperatorContext *c) {
	if (!c->arg->s)
		macro_operator_record(vis);
	return text_size(txt) + 1; /* do not change cursor position, would destroy selection */
}

bool vis_operator(Vis *vis, enum VisOperator id, ...) {
	va_list ap;
	va_start(ap, id);

	switch (id) {
	case VIS_OP_CASE_LOWER:
	case VIS_OP_CASE_UPPER:
	case VIS_OP_CASE_SWAP:
		vis->action.arg.i = id;
		id = VIS_OP_CASE_SWAP;
		break;
	case VIS_OP_CURSOR_SOL:
	case VIS_OP_CURSOR_EOL:
		vis->action.arg.i = id;
		id = VIS_OP_CURSOR_SOL;
		break;
	case VIS_OP_PUT_AFTER:
	case VIS_OP_PUT_AFTER_END:
	case VIS_OP_PUT_BEFORE:
	case VIS_OP_PUT_BEFORE_END:
		vis->action.arg.i = id;
		id = VIS_OP_PUT_AFTER;
		break;
	case VIS_OP_FILTER:
		vis->action.arg.s = va_arg(ap, char*);
		/* fall through */
	case VIS_OP_SHIFT_LEFT:
	case VIS_OP_SHIFT_RIGHT:
		vis_motion_type(vis, VIS_MOTIONTYPE_LINEWISE);
		break;
	default:
		break;
	}
	if (id >= LENGTH(vis_operators))
		goto err;
	const Operator *op = &vis_operators[id];
	if (vis->mode->visual) {
		vis->action.op = op;
		action_do(vis, &vis->action);
		goto out;
	}

	/* switch to operator mode inorder to make operator options and
	 * text-object available */
	vis_mode_switch(vis, VIS_MODE_OPERATOR_PENDING);
	if (vis->action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		vis->action.type = LINEWISE;
		vis_motion(vis, VIS_MOVE_LINE_NEXT);
	} else {
		vis->action.op = op;
	}

	/* put is not a real operator, does not need a range to operate on */
	if (id == VIS_OP_PUT_AFTER)
		vis_motion(vis, VIS_MOVE_NOP);

out:
	va_end(ap);
	return true;
err:
	va_end(ap);
	return false;
}

const Operator vis_operators[] = {
	[VIS_OP_DELETE]      = { op_delete      },
	[VIS_OP_CHANGE]      = { op_change      },
	[VIS_OP_YANK]        = { op_yank        },
	[VIS_OP_PUT_AFTER]   = { op_put         },
	[VIS_OP_SHIFT_RIGHT] = { op_shift_right },
	[VIS_OP_SHIFT_LEFT]  = { op_shift_left  },
	[VIS_OP_CASE_SWAP]   = { op_case_change },
	[VIS_OP_JOIN]        = { op_join        },
	[VIS_OP_INSERT]      = { op_insert      },
	[VIS_OP_REPLACE]     = { op_replace     },
	[VIS_OP_CURSOR_SOL]  = { op_cursor      },
	[VIS_OP_FILTER]      = { op_filter      },
	[VIS_OP_WRAP_TEXT]   = { op_wrap_text   },
};
