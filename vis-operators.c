#include <string.h>
#include <ctype.h>
#include "vis-core.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-util.h"
#include "util.h"

static size_t op_delete(Vis *vis, Text *txt, OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_slot_put_range(vis, c->reg, c->reg_slot, txt, &c->range);
	text_delete_range(txt, &c->range);
	size_t pos = c->range.start;
	if (c->linewise && pos == text_size(txt))
		pos = text_line_begin(txt, text_line_prev(txt, pos));
	return pos;
}

static size_t op_change(Vis *vis, Text *txt, OperatorContext *c) {
	bool linewise = c->linewise || text_range_is_linewise(txt, &c->range);
	op_delete(vis, txt, c);
	size_t pos = c->range.start;
	if (linewise) {
		size_t newpos = vis_text_insert_nl(vis, txt, pos > 0 ? pos-1 : pos);
		if (pos > 0)
			pos = newpos;
	}
	return pos;
}

static size_t op_yank(Vis *vis, Text *txt, OperatorContext *c) {
	c->reg->linewise = c->linewise;
	register_slot_put_range(vis, c->reg, c->reg_slot, txt, &c->range);
	if (c->reg == &vis->registers[VIS_REG_DEFAULT]) {
		vis->registers[VIS_REG_ZERO].linewise = c->reg->linewise;
		register_slot_put_range(vis, &vis->registers[VIS_REG_ZERO], c->reg_slot, txt, &c->range);
	}
	return c->linewise ? c->pos : c->range.start;
}

static size_t op_put(Vis *vis, Text *txt, OperatorContext *c) {
	char b;
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
		else if (!sel && text_byte_get(txt, pos, &b) && b != '\n')
			pos = text_char_next(txt, pos);
		break;
	case VIS_OP_PUT_BEFORE:
	case VIS_OP_PUT_BEFORE_END:
		if (c->reg->linewise)
			pos = text_line_begin(txt, pos);
		break;
	}

	size_t len;
	const char *data = register_slot_get(vis, c->reg, c->reg_slot, &len);

	for (int i = 0; i < c->count; i++) {
		char nl;
		if (c->reg->linewise && pos > 0 && text_byte_get(txt, pos-1, &nl) && nl != '\n')
			pos += text_insert(txt, pos, "\n", 1);
		text_insert(txt, pos, data, len);
		pos += len;
		if (c->reg->linewise && pos > 0 && text_byte_get(txt, pos-1, &nl) && nl != '\n')
			pos += text_insert(txt, pos, "\n", 1);
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
	spaces[MIN(vis->win->view.tabwidth, LENGTH(spaces) - 1)] = '\0';
	const char *tab = vis->win->expandtab ? spaces : "\t";
	size_t tablen = strlen(tab);
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	size_t newpos = c->pos;

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);
	bool multiple_lines = text_line_prev(txt, pos) >= c->range.start;

	do {
		size_t end = text_line_end(txt, pos);
		prev_pos = pos = text_line_begin(txt, end);
		if ((!multiple_lines || pos != end) &&
		    text_insert(txt, pos, tab, tablen) && pos <= c->pos)
			newpos += tablen;
		pos = text_line_prev(txt, pos);
	}  while (pos >= c->range.start && pos != prev_pos);

	return newpos;
}

static size_t op_shift_left(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	size_t tabwidth = vis->win->view.tabwidth, tablen;
	size_t newpos = c->pos;

	/* if range ends at the begin of a line, skip line break */
	if (pos == c->range.end)
		pos = text_line_prev(txt, pos);

	do {
		char b;
		size_t len = 0;
		prev_pos = pos = text_line_begin(txt, pos);
		Iterator it = text_iterator_get(txt, pos);
		if (text_iterator_byte_get(&it, &b) && b == '\t') {
			len = 1;
		} else {
			for (len = 0; text_iterator_byte_get(&it, &b) && b == ' '; len++)
				text_iterator_byte_next(&it, NULL);
		}
		tablen = MIN(len, tabwidth);
		if (text_delete(txt, pos, tablen) && pos < c->pos) {
			size_t delta = c->pos - pos;
			if (delta > tablen)
				delta = tablen;
			if (delta > newpos)
				delta = newpos;
			newpos -= delta;
		}
		pos = text_line_prev(txt, pos);
	}  while (pos >= c->range.start && pos != prev_pos);

	return newpos;
}

static size_t op_cursor(Vis *vis, Text *txt, OperatorContext *c) {
	Filerange r = text_range_linewise(txt, &c->range);
	for (size_t line = text_range_line_first(txt, &r); line != EPOS; line = text_range_line_next(txt, &r, line)) {
		size_t pos;
		if (c->arg->i == VIS_OP_CURSOR_EOL)
			pos = text_line_finish(txt, line);
		else
			pos = text_line_start(txt, line);
		view_selections_new_force(&vis->win->view, pos);
	}
	return EPOS;
}

static size_t op_join(Vis *vis, Text *txt, OperatorContext *c) {
	size_t pos = text_line_begin(txt, c->range.end), prev_pos;
	Mark mark = EMARK;

	/* if operator and range are both linewise, skip last line break */
	if (c->linewise && text_range_is_linewise(txt, &c->range)) {
		size_t line_prev = text_line_prev(txt, pos);
		size_t line_prev_prev = text_line_prev(txt, line_prev);
		if (line_prev_prev >= c->range.start)
			pos = line_prev;
	}

	size_t len = c->arg->s ? strlen(c->arg->s) : 0;

	do {
		prev_pos = pos;
		size_t end = text_line_start(txt, pos);
		pos = text_line_prev(txt, end);
		if (pos < c->range.start || end <= pos)
			break;
		text_delete(txt, pos, end - pos);
		char prev, next;
		if (text_byte_get(txt, pos-1, &prev) && !isspace((unsigned char)prev) &&
		    text_byte_get(txt, pos, &next) && next != '\n')
			text_insert(txt, pos, c->arg->s, len);
		if (mark == EMARK)
			mark = text_mark_set(txt, pos);
	} while (pos != prev_pos);

	size_t newpos = text_mark_get(txt, mark);
	return newpos != EPOS ? newpos : c->range.start;
}

static size_t op_modeswitch(Vis *vis, Text *txt, OperatorContext *c) {
	return c->newpos != EPOS ? c->newpos : c->pos;
}

static size_t op_replace(Vis *vis, Text *txt, OperatorContext *c) {
	size_t count = 0;
	Iterator it = text_iterator_get(txt, c->range.start);
	while (it. pos < c->range.end && text_iterator_char_next(&it, NULL))
		count++;
	op_delete(vis, txt, c);
	size_t pos = c->range.start;
	for (size_t len = strlen(c->arg->s); count > 0; pos += len, count--)
		text_insert(txt, pos, c->arg->s, len);
	return c->range.start;
}

int vis_operator_register(Vis *vis, VisOperatorFunction *func, void *context) {
	Operator *op = calloc(1, sizeof *op);
	if (!op)
		return -1;
	op->func = func;
	op->context = context;
	if (array_add_ptr(&vis->operators, op))
		return VIS_OP_LAST + array_length(&vis->operators) - 1;
	free(op);
	return -1;
}

bool vis_operator(Vis *vis, enum VisOperator id, ...) {
	va_list ap;
	va_start(ap, id);

	switch (id) {
	case VIS_OP_MODESWITCH:
		vis->action.mode = va_arg(ap, int);
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
	case VIS_OP_JOIN:
		vis->action.arg.s = va_arg(ap, char*);
		break;
	case VIS_OP_SHIFT_LEFT:
	case VIS_OP_SHIFT_RIGHT:
		vis_motion_type(vis, VIS_MOTIONTYPE_LINEWISE);
		break;
	case VIS_OP_REPLACE:
	{
		Macro *macro = macro_get(vis, VIS_REG_DOT);
		macro_reset(macro);
		macro_append(macro, va_arg(ap, char*));
		vis->action.arg.s = macro->data;
		break;
	}
	case VIS_OP_DELETE:
	{
		enum VisMode mode = vis->mode->id;
		enum VisRegister reg = vis_register_used(vis);
		if (reg == VIS_REG_DEFAULT && (mode == VIS_MODE_INSERT || mode == VIS_MODE_REPLACE))
			vis_register(vis, VIS_REG_BLACKHOLE);
		break;
	}
	default:
		break;
	}

	const Operator *op = NULL;
	if (id < LENGTH(vis_operators))
		op = &vis_operators[id];
	else
		op = array_get_ptr(&vis->operators, id - VIS_OP_LAST);

	if (!op)
		goto err;

	if (vis->mode->visual) {
		vis->action.op = op;
		vis_do(vis);
		goto out;
	}

	/* switch to operator mode inorder to make operator options and
	 * text-object available */
	vis_mode_switch(vis, VIS_MODE_OPERATOR_PENDING);
	if (vis->action.op == op) {
		/* hacky way to handle double operators i.e. things like
		 * dd, yy etc where the second char isn't a movement */
		vis_motion_type(vis, VIS_MOTIONTYPE_LINEWISE);
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
	[VIS_OP_JOIN]        = { op_join        },
	[VIS_OP_MODESWITCH]  = { op_modeswitch  },
	[VIS_OP_REPLACE]     = { op_replace     },
	[VIS_OP_CURSOR_SOL]  = { op_cursor      },
};
