#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "vis-core.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-util.h"
#include "util.h"

static Regex *search_word(Vis *vis, Text *txt, size_t pos) {
	char expr[512];
	Filerange word = text_object_word(txt, pos);
	if (!text_range_valid(&word))
		return NULL;
	char *buf = text_bytes_alloc0(txt, word.start, text_range_size(&word));
	if (!buf)
		return NULL;
	snprintf(expr, sizeof(expr), "[[:<:]]%s[[:>:]]", buf);
	Regex *regex = vis_regex(vis, expr, strlen(expr));
	if (!regex) {
		snprintf(expr, sizeof(expr), "\\<%s\\>", buf);
		regex = vis_regex(vis, expr, strlen(expr));
	}
	free(buf);
	return regex;
}

static size_t search_word_forward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = search_word(vis, txt, pos);
	if (regex) {
		vis->search_direction = VIS_MOVE_SEARCH_REPEAT_FORWARD;
		pos = text_search_forward(txt, pos, regex);
	}
	text_regex_free(regex);
	return pos;
}

static size_t search_word_backward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = search_word(vis, txt, pos);
	if (regex) {
		vis->search_direction = VIS_MOVE_SEARCH_REPEAT_BACKWARD;
		pos = text_search_backward(txt, pos, regex);
	}
	text_regex_free(regex);
	return pos;
}

static size_t search_forward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = vis_regex(vis, NULL, 0);
	if (regex)
		pos = text_search_forward(txt, pos, regex);
	text_regex_free(regex);
	return pos;
}

static size_t search_backward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = vis_regex(vis, NULL, 0);
	if (regex)
		pos = text_search_backward(txt, pos, regex);
	text_regex_free(regex);
	return pos;
}

static size_t common_word_next(Vis *vis, Text *txt, size_t pos,
                               enum VisMotion start_next, enum VisMotion end_next,
                               int (*isboundary)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	const Movement *motion = NULL;
	int count = VIS_COUNT_DEFAULT(vis->action.count, 1);
	if (isspace((unsigned char)c)) {
		motion = &vis_motions[start_next];
	} else if (!isboundary((unsigned char)c) && text_iterator_char_next(&it, &c) && isboundary((unsigned char)c)) {
		/* we are on the last character of a word */
		if (count == 1) {
			/* map `cw` to `cl` */
			motion = &vis_motions[VIS_MOVE_CHAR_NEXT];
		} else {
			/* map `c{n}w` to `c{n-1}e` */
			count--;
			motion = &vis_motions[end_next];
		}
	} else {
		/* map `cw` to `ce` */
		motion = &vis_motions[end_next];
	}

	while (count--) {
		if (vis->interrupted)
			return pos;
		size_t newpos = motion->txt(txt, pos);
		if (newpos == pos)
			break;
		pos = newpos;
	}

	if (motion->type & INCLUSIVE)
		pos = text_char_next(txt, pos);

	return pos;
}

static size_t word_next(Vis *vis, Text *txt, size_t pos) {
	return common_word_next(vis, txt, pos, VIS_MOVE_WORD_START_NEXT,
	                        VIS_MOVE_WORD_END_NEXT, is_word_boundary);
}

static size_t longword_next(Vis *vis, Text *txt, size_t pos) {
	return common_word_next(vis, txt, pos, VIS_MOVE_LONGWORD_START_NEXT,
	                        VIS_MOVE_LONGWORD_END_NEXT, isspace);
}

static size_t to_right(Vis *vis, Text *txt, size_t pos) {
	char c;
	size_t hit = text_find_next(txt, pos+1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till_right(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to_right(vis, txt, pos+1);
	if (hit != pos)
		return text_char_prev(txt, hit);
	return pos;
}

static size_t to_left(Vis *vis, Text *txt, size_t pos) {
	return text_find_prev(txt, pos, vis->search_char);
}

static size_t till_left(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to_left(vis, txt, pos-1);
	if (hit != pos-1)
		return text_char_next(txt, hit);
	return pos;
}

static size_t to_line_right(Vis *vis, Text *txt, size_t pos) {
	char c;
	if (pos == text_line_end(txt, pos))
		return pos;
	size_t hit = text_line_find_next(txt, pos+1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till_line_right(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to_line_right(vis, txt, pos+1);
	if (pos == text_line_end(txt, pos))
		return pos;
	if (hit != pos)
		return text_char_prev(txt, hit);
	return pos;
}

static size_t to_line_left(Vis *vis, Text *txt, size_t pos) {
	return text_line_find_prev(txt, pos, vis->search_char);
}

static size_t till_line_left(Vis *vis, Text *txt, size_t pos) {
	if (pos == text_line_begin(txt, pos))
		return pos;
	size_t hit = to_line_left(vis, txt, pos-1);
	if (hit != pos-1)
		return text_char_next(txt, hit);
	return pos;
}

static size_t firstline(Text *txt, size_t pos) {
	return text_line_start(txt, 0);
}

static size_t line(Vis *vis, Text *txt, size_t pos) {
	int count = VIS_COUNT_DEFAULT(vis->action.count, 1);
	return text_line_start(txt, text_pos_by_lineno(txt, count));
}

static size_t lastline(Text *txt, size_t pos) {
	pos = text_size(txt);
	return text_line_start(txt, pos > 0 ? pos-1 : pos);
}

static size_t column(Vis *vis, Text *txt, size_t pos) {
	return text_line_offset(txt, pos, VIS_COUNT_DEFAULT(vis->action.count, 0));
}

static size_t view_lines_top(Vis *vis, View *view) {
	return view_screenline_goto(view, VIS_COUNT_DEFAULT(vis->action.count, 1));
}

static size_t view_lines_middle(Vis *vis, View *view) {
	int h = view->height;
	return view_screenline_goto(view, h/2);
}

static size_t view_lines_bottom(Vis *vis, View *view) {
	int h = view->height;
	return view_screenline_goto(view, h - VIS_COUNT_DEFAULT(vis->action.count, 0));
}

static size_t window_nop(Vis *vis, Win *win, size_t pos) {
	return pos;
}

static size_t bracket_match(Text *txt, size_t pos) {
	size_t hit = text_bracket_match_symbol(txt, pos, "(){}[]<>'\"`", NULL);
	if (hit != pos)
		return hit;
	char current;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &current)) {
		switch (current) {
		case '(':
		case ')':
		case '{':
		case '}':
		case '[':
		case ']':
		case '<':
		case '>':
		case '"':
		case '\'':
		case '`':
			return it.pos;
		}
		text_iterator_byte_next(&it, NULL);
	}
	return pos;
}

static size_t percent(Vis *vis, Text *txt, size_t pos) {
	int ratio = VIS_COUNT_DEFAULT(vis->action.count, 0);
	if (ratio > 100)
		ratio = 100;
	return text_size(txt) * ratio / 100;
}

static size_t byte(Vis *vis, Text *txt, size_t pos) {
	pos = VIS_COUNT_DEFAULT(vis->action.count, 0);
	size_t max = text_size(txt);
	return pos <= max ? pos : max;
}

static size_t byte_left(Vis *vis, Text *txt, size_t pos) {
	size_t off = VIS_COUNT_DEFAULT(vis->action.count, 1);
	return off <= pos ? pos-off : 0;
}

static size_t byte_right(Vis *vis, Text *txt, size_t pos) {
	size_t off = VIS_COUNT_DEFAULT(vis->action.count, 1);
	size_t new = pos + off;
	size_t max = text_size(txt);
	return new <= max && new > pos ? new : max;
}

void vis_motion_type(Vis *vis, enum VisMotionType type) {
	vis->action.type = type;
}

int vis_motion_register(Vis *vis, void *data, VisMotionFunction *motion) {

	Movement *move = calloc(1, sizeof *move);
	if (!move)
		return -1;

	move->user = motion;
	move->data = data;

	if (array_add_ptr(&vis->motions, move))
		return VIS_MOVE_LAST + array_length(&vis->motions) - 1;
	free(move);
	return -1;
}

bool vis_motion(Vis *vis, enum VisMotion motion, ...) {
	va_list ap;
	va_start(ap, motion);

	switch (motion) {
	case VIS_MOVE_WORD_START_NEXT:
		if (vis->action.op == &vis_operators[VIS_OP_CHANGE])
			motion = VIS_MOVE_WORD_NEXT;
		break;
	case VIS_MOVE_LONGWORD_START_NEXT:
		if (vis->action.op == &vis_operators[VIS_OP_CHANGE])
			motion = VIS_MOVE_LONGWORD_NEXT;
		break;
	case VIS_MOVE_SEARCH_FORWARD:
	case VIS_MOVE_SEARCH_BACKWARD:
	{
		const char *pattern = va_arg(ap, char*);
		size_t len = va_arg(ap, size_t);
		Regex *regex = vis_regex(vis, pattern, len);
		if (!regex) {
			vis_cancel(vis);
			goto err;
		}
		text_regex_free(regex);
		if (motion == VIS_MOVE_SEARCH_FORWARD)
			motion = VIS_MOVE_SEARCH_REPEAT_FORWARD;
		else
			motion = VIS_MOVE_SEARCH_REPEAT_BACKWARD;
		vis->search_direction = motion;
		break;
	}
	case VIS_MOVE_SEARCH_REPEAT:
	case VIS_MOVE_SEARCH_REPEAT_REVERSE:
	{
		if (!vis->search_direction)
			vis->search_direction = VIS_MOVE_SEARCH_REPEAT_FORWARD;
		if (motion == VIS_MOVE_SEARCH_REPEAT) {
			motion = vis->search_direction;
		} else {
			motion = vis->search_direction == VIS_MOVE_SEARCH_REPEAT_FORWARD ?
			                                  VIS_MOVE_SEARCH_REPEAT_BACKWARD :
			                                  VIS_MOVE_SEARCH_REPEAT_FORWARD;
		}
		break;
	}
	case VIS_MOVE_TO_RIGHT:
	case VIS_MOVE_TO_LEFT:
	case VIS_MOVE_TO_LINE_RIGHT:
	case VIS_MOVE_TO_LINE_LEFT:
	case VIS_MOVE_TILL_RIGHT:
	case VIS_MOVE_TILL_LEFT:
	case VIS_MOVE_TILL_LINE_RIGHT:
	case VIS_MOVE_TILL_LINE_LEFT:
	{
		const char *key = va_arg(ap, char*);
		if (!key)
			goto err;
		strncpy(vis->search_char, key, sizeof(vis->search_char));
		vis->search_char[sizeof(vis->search_char)-1] = '\0';
		vis->last_totill = motion;
		break;
	}
	case VIS_MOVE_TOTILL_REPEAT:
		if (!vis->last_totill)
			goto err;
		motion = vis->last_totill;
		break;
	case VIS_MOVE_TOTILL_REVERSE:
		switch (vis->last_totill) {
		case VIS_MOVE_TO_RIGHT:
			motion = VIS_MOVE_TO_LEFT;
			break;
		case VIS_MOVE_TO_LEFT:
			motion = VIS_MOVE_TO_RIGHT;
			break;
		case VIS_MOVE_TO_LINE_RIGHT:
			motion = VIS_MOVE_TO_LINE_LEFT;
			break;
		case VIS_MOVE_TO_LINE_LEFT:
			motion = VIS_MOVE_TO_LINE_RIGHT;
			break;
		case VIS_MOVE_TILL_RIGHT:
			motion = VIS_MOVE_TILL_LEFT;
			break;
		case VIS_MOVE_TILL_LEFT:
			motion = VIS_MOVE_TILL_RIGHT;
			break;
		case VIS_MOVE_TILL_LINE_RIGHT:
			motion = VIS_MOVE_TILL_LINE_LEFT;
			break;
		case VIS_MOVE_TILL_LINE_LEFT:
			motion = VIS_MOVE_TILL_LINE_RIGHT;
			break;
		default:
			goto err;
		}
		break;
	default:
		break;
	}

	if (motion < LENGTH(vis_motions))
		vis->action.movement = &vis_motions[motion];
	else
		vis->action.movement = array_get_ptr(&vis->motions, motion - VIS_MOVE_LAST);

	if (!vis->action.movement)
		goto err;

	va_end(ap);
	vis_do(vis);
	return true;
err:
	va_end(ap);
	return false;
}

const Movement vis_motions[] = {
	[VIS_MOVE_LINE_UP] = {
		.cur = view_line_up,
		.type = LINEWISE|LINEWISE_INCLUSIVE,
	},
	[VIS_MOVE_LINE_DOWN] = {
		.cur = view_line_down,
		.type = LINEWISE|LINEWISE_INCLUSIVE,
	},
	[VIS_MOVE_SCREEN_LINE_UP] = {
		.cur = view_screenline_up,
	},
	[VIS_MOVE_SCREEN_LINE_DOWN] = {
		.cur = view_screenline_down,
	},
	[VIS_MOVE_SCREEN_LINE_BEGIN] = {
		.cur = view_screenline_begin,
		.type = CHARWISE,
	},
	[VIS_MOVE_SCREEN_LINE_MIDDLE] = {
		.cur = view_screenline_middle,
		.type = CHARWISE,
	},
	[VIS_MOVE_SCREEN_LINE_END] = {
		.cur = view_screenline_end,
		.type = CHARWISE|INCLUSIVE,
	},
	[VIS_MOVE_LINE_PREV] = {
		.txt = text_line_prev,
	},
	[VIS_MOVE_LINE_BEGIN] = {
		.txt = text_line_begin,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_LINE_START] = {
		.txt = text_line_start,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_LINE_FINISH] = {
		.txt = text_line_finish,
		.type = INCLUSIVE|IDEMPOTENT,
	},
	[VIS_MOVE_LINE_END] = {
		.txt = text_line_end,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_LINE_NEXT] = {
		.txt = text_line_next,
	},
	[VIS_MOVE_LINE] = {
		.vis = line,
		.type = LINEWISE|IDEMPOTENT|JUMP,
	},
	[VIS_MOVE_COLUMN] = {
		.vis = column,
		.type = CHARWISE|IDEMPOTENT,
	},
	[VIS_MOVE_CHAR_PREV] = {
		.txt = text_char_prev,
		.type = CHARWISE,
	},
	[VIS_MOVE_CHAR_NEXT] = {
		.txt = text_char_next,
		.type = CHARWISE,
	},
	[VIS_MOVE_LINE_CHAR_PREV] = {
		.txt = text_line_char_prev,
		.type = CHARWISE,
	},
	[VIS_MOVE_LINE_CHAR_NEXT] = {
		.txt = text_line_char_next,
		.type = CHARWISE,
	},
	[VIS_MOVE_CODEPOINT_PREV] = {
		.txt = text_codepoint_prev,
		.type = CHARWISE,
	},
	[VIS_MOVE_CODEPOINT_NEXT] = {
		.txt = text_codepoint_next,
		.type = CHARWISE,
	},
	[VIS_MOVE_WORD_NEXT] = {
		.vis = word_next,
		.type = CHARWISE|IDEMPOTENT,
	},
	[VIS_MOVE_WORD_START_PREV] = {
		.txt = text_word_start_prev,
		.type = CHARWISE,
	},
	[VIS_MOVE_WORD_START_NEXT] = {
		.txt = text_word_start_next,
		.type = CHARWISE,
	},
	[VIS_MOVE_WORD_END_PREV] = {
		.txt = text_word_end_prev,
		.type = CHARWISE|INCLUSIVE,
	},
	[VIS_MOVE_WORD_END_NEXT] = {
		.txt = text_word_end_next,
		.type = CHARWISE|INCLUSIVE,
	},
	[VIS_MOVE_LONGWORD_NEXT] = {
		.vis = longword_next,
		.type = CHARWISE|IDEMPOTENT,
	},
	[VIS_MOVE_LONGWORD_START_PREV] = {
		.txt = text_longword_start_prev,
		.type = CHARWISE,
	},
	[VIS_MOVE_LONGWORD_START_NEXT] = {
		.txt = text_longword_start_next,
		.type = CHARWISE,
	},
	[VIS_MOVE_LONGWORD_END_PREV] = {
		.txt = text_longword_end_prev,
		.type = CHARWISE|INCLUSIVE,
	},
	[VIS_MOVE_LONGWORD_END_NEXT] = {
		.txt = text_longword_end_next,
		.type = CHARWISE|INCLUSIVE,
	},
	[VIS_MOVE_SENTENCE_PREV] = {
		.txt = text_sentence_prev,
		.type = CHARWISE,
	},
	[VIS_MOVE_SENTENCE_NEXT] = {
		.txt = text_sentence_next,
		.type = CHARWISE,
	},
	[VIS_MOVE_PARAGRAPH_PREV] = {
		.txt = text_paragraph_prev,
		.type = LINEWISE|JUMP,
	},
	[VIS_MOVE_PARAGRAPH_NEXT] = {
		.txt = text_paragraph_next,
		.type = LINEWISE|JUMP,
	},
	[VIS_MOVE_BLOCK_START] = {
		.txt = text_block_start,
		.type = JUMP,
	},
	[VIS_MOVE_BLOCK_END] = {
		.txt = text_block_end,
		.type = JUMP,
	},
	[VIS_MOVE_PARENTHESIS_START] = {
		.txt = text_parenthesis_start,
		.type = JUMP,
	},
	[VIS_MOVE_PARENTHESIS_END] = {
		.txt = text_parenthesis_end,
		.type = JUMP,
	},
	[VIS_MOVE_BRACKET_MATCH] = {
		.txt = bracket_match,
		.type = INCLUSIVE|JUMP,
	},
	[VIS_MOVE_FILE_BEGIN] = {
		.txt = firstline,
		.type = LINEWISE|LINEWISE_INCLUSIVE|JUMP|IDEMPOTENT,
	},
	[VIS_MOVE_FILE_END] = {
		.txt = lastline,
		.type = LINEWISE|LINEWISE_INCLUSIVE|JUMP|IDEMPOTENT,
	},
	[VIS_MOVE_TO_LEFT] = {
		.vis = to_left,
		.type = COUNT_EXACT,
	},
	[VIS_MOVE_TO_RIGHT] = {
		.vis = to_right,
		.type = INCLUSIVE|COUNT_EXACT,
	},
	[VIS_MOVE_TO_LINE_LEFT] = {
		.vis = to_line_left,
		.type = COUNT_EXACT,
	},
	[VIS_MOVE_TO_LINE_RIGHT] = {
		.vis = to_line_right,
		.type = INCLUSIVE|COUNT_EXACT,
	},
	[VIS_MOVE_TILL_LEFT] = {
		.vis = till_left,
		.type = COUNT_EXACT,
	},
	[VIS_MOVE_TILL_RIGHT] = {
		.vis = till_right,
		.type = INCLUSIVE|COUNT_EXACT,
	},
	[VIS_MOVE_TILL_LINE_LEFT] = {
		.vis = till_line_left,
		.type = COUNT_EXACT,
	},
	[VIS_MOVE_TILL_LINE_RIGHT] = {
		.vis = till_line_right,
		.type = INCLUSIVE|COUNT_EXACT,
	},
	[VIS_MOVE_SEARCH_WORD_FORWARD] = {
		.vis = search_word_forward,
		.type = JUMP,
	},
	[VIS_MOVE_SEARCH_WORD_BACKWARD] = {
		.vis = search_word_backward,
		.type = JUMP,
	},
	[VIS_MOVE_SEARCH_REPEAT_FORWARD] = {
		.vis = search_forward,
		.type = JUMP,
	},
	[VIS_MOVE_SEARCH_REPEAT_BACKWARD] = {
		.vis = search_backward,
		.type = JUMP,
	},
	[VIS_MOVE_WINDOW_LINE_TOP] = {
		.view = view_lines_top,
		.type = LINEWISE|JUMP|IDEMPOTENT,
	},
	[VIS_MOVE_WINDOW_LINE_MIDDLE] = {
		.view = view_lines_middle,
		.type = LINEWISE|JUMP|IDEMPOTENT,
	},
	[VIS_MOVE_WINDOW_LINE_BOTTOM] = {
		.view = view_lines_bottom,
		.type = LINEWISE|JUMP|IDEMPOTENT,
	},
	[VIS_MOVE_NOP] = {
		.win = window_nop,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_PERCENT] = {
		.vis = percent,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_BYTE] = {
		.vis = byte,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_BYTE_LEFT] = {
		.vis = byte_left,
		.type = IDEMPOTENT,
	},
	[VIS_MOVE_BYTE_RIGHT] = {
		.vis = byte_right,
		.type = IDEMPOTENT,
	},
};
