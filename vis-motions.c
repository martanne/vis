#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "vis-core.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-util.h"
#include "util.h"

/** utility functions */

static Regex *search_word(Vis *vis, Text *txt, size_t pos) {
	char expr[512];
	Filerange word = text_object_word(txt, pos);
	if (!text_range_valid(&word))
		return NULL;
	char *buf = text_bytes_alloc0(txt, word.start, text_range_size(&word));
	if (!buf)
		return NULL;
	snprintf(expr, sizeof(expr), "[[:<:]]%s[[:>:]]", buf);
	Regex *regex = vis_regex(vis, expr);
	if (!regex) {
		snprintf(expr, sizeof(expr), "\\<%s\\>", buf);
		regex = vis_regex(vis, expr);
	}
	free(buf);
	return regex;
}

/** motion implementations */

static size_t search_word_forward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = search_word(vis, txt, pos);
	if (regex)
		pos = text_search_forward(txt, pos, regex);
	text_regex_free(regex);
	return pos;
}

static size_t search_word_backward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = search_word(vis, txt, pos);
	if (regex)
		pos = text_search_backward(txt, pos, regex);
	text_regex_free(regex);
	return pos;
}

static size_t search_forward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = vis_regex(vis, NULL);
	if (regex)
		pos = text_search_forward(txt, pos, regex);
	text_regex_free(regex);
	return pos;
}

static size_t search_backward(Vis *vis, Text *txt, size_t pos) {
	Regex *regex = vis_regex(vis, NULL);
	if (regex)
		pos = text_search_backward(txt, pos, regex);
	text_regex_free(regex);
	return pos;
}

static size_t mark_goto(Vis *vis, File *file, size_t pos) {
	return text_mark_get(file->text, file->marks[vis->action.mark]);
}

static size_t mark_line_goto(Vis *vis, File *file, size_t pos) {
	return text_line_start(file->text, mark_goto(vis, file, pos));
}

static size_t to(Vis *vis, Text *txt, size_t pos) {
	char c;
	if (pos == text_line_end(txt, pos))
		return pos;
	size_t hit = text_line_find_next(txt, pos+1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to(vis, txt, pos+1);
	if (pos == text_line_end(txt, pos))
		return pos;
	if (hit != pos)
		return text_char_prev(txt, hit);
	return pos;
}

static size_t to_left(Vis *vis, Text *txt, size_t pos) {
	char c;
	if (pos == text_line_begin(txt, pos))
		return pos;
	size_t hit = text_line_find_prev(txt, pos-1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till_left(Vis *vis, Text *txt, size_t pos) {
	if (pos == text_line_begin(txt, pos))
		return pos;
	size_t hit = to_left(vis, txt, pos-1);
	if (hit != pos)
		return text_char_next(txt, hit);
	return pos;
}

static size_t line(Vis *vis, Text *txt, size_t pos) {
	return text_pos_by_lineno(txt, vis_count_get_default(vis, 1));
}

static size_t column(Vis *vis, Text *txt, size_t pos) {
	return text_line_offset(txt, pos, vis_count_get_default(vis, 0));
}

static size_t view_lines_top(Vis *vis, View *view) {
	return view_screenline_goto(view, vis_count_get_default(vis, 1));
}

static size_t view_lines_middle(Vis *vis, View *view) {
	int h = view_height_get(view);
	return view_screenline_goto(view, h/2);
}

static size_t view_lines_bottom(Vis *vis, View *view) {
	int h = view_height_get(vis->win->view);
	return view_screenline_goto(vis->win->view, h - vis_count_get_default(vis, 0));
}

static size_t window_changelist_next(Vis *vis, Win *win, size_t pos) {
	ChangeList *cl = &win->changelist;
	Text *txt = win->file->text;
	time_t state = text_state(txt);
	if (cl->state != state)
		cl->index = 0;
	else if (cl->index > 0 && pos == cl->pos)
		cl->index--;
	size_t newpos = text_history_get(txt, cl->index);
	if (newpos == EPOS)
		cl->index++;
	else
		cl->pos = newpos;
	cl->state = state;
	return cl->pos;
}

static size_t window_changelist_prev(Vis *vis, Win *win, size_t pos) {
	ChangeList *cl = &win->changelist;
	Text *txt = win->file->text;
	time_t state = text_state(txt);
	if (cl->state != state)
		cl->index = 0;
	else if (pos == cl->pos)
		win->changelist.index++;
	size_t newpos = text_history_get(txt, cl->index);
	if (newpos == EPOS)
		cl->index--;
	else
		cl->pos = newpos;
	cl->state = state;
	return cl->pos;
}

static size_t window_jumplist_next(Vis *vis, Win *win, size_t cur) {
	while (win->jumplist) {
		Mark mark = ringbuf_next(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->file->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

static size_t window_jumplist_prev(Vis *vis, Win *win, size_t cur) {
	while (win->jumplist) {
		Mark mark = ringbuf_prev(win->jumplist);
		if (!mark)
			return cur;
		size_t pos = text_mark_get(win->file->text, mark);
		if (pos != EPOS && pos != cur)
			return pos;
	}
	return cur;
}

static size_t window_nop(Vis *vis, Win *win, size_t pos) {
	return pos;
}

static size_t bracket_match(Text *txt, size_t pos) {
	size_t hit = text_bracket_match_symbol(txt, pos, "(){}[]");
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
			return it.pos;
		}
		text_iterator_byte_next(&it, NULL);
	}
	return pos;
}

static size_t percent(Vis *vis, Text *txt, size_t pos) {
	int ratio = vis_count_get_default(vis, 0);
	if (ratio > 100)
		ratio = 100;
	return text_size(txt) * ratio / 100;
}

void vis_motion_type(Vis *vis, enum VisMotionType type) {
	vis->action.type = type;
}

int vis_motion_register(Vis *vis, enum VisMotionType type, void *data,
	size_t (*motion)(Vis*, Win*, void*, size_t pos)) {

	Movement *move = calloc(1, sizeof *move);
	if (!move)
		return -1;

	move->user = motion;
	move->type = type;
	move->data = data;

	if (array_add(&vis->motions, move))
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
			motion = VIS_MOVE_WORD_END_NEXT;
		break;
	case VIS_MOVE_LONGWORD_START_NEXT:
		if (vis->action.op == &vis_operators[VIS_OP_CHANGE])
			motion = VIS_MOVE_LONGWORD_END_NEXT;
		break;
	case VIS_MOVE_SEARCH_FORWARD:
	case VIS_MOVE_SEARCH_BACKWARD:
	{
		const char *pattern = va_arg(ap, char*);
		Regex *regex = vis_regex(vis, pattern);
		if (!regex) {
			vis_cancel(vis);
			goto err;
		}
		text_regex_free(regex);
		if (motion == VIS_MOVE_SEARCH_FORWARD)
			motion = VIS_MOVE_SEARCH_NEXT;
		else
			motion = VIS_MOVE_SEARCH_PREV;
		break;
	}
	case VIS_MOVE_RIGHT_TO:
	case VIS_MOVE_LEFT_TO:
	case VIS_MOVE_RIGHT_TILL:
	case VIS_MOVE_LEFT_TILL:
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
		case VIS_MOVE_RIGHT_TO:
			motion = VIS_MOVE_LEFT_TO;
			break;
		case VIS_MOVE_LEFT_TO:
			motion = VIS_MOVE_RIGHT_TO;
			break;
		case VIS_MOVE_RIGHT_TILL:
			motion = VIS_MOVE_LEFT_TILL;
			break;
		case VIS_MOVE_LEFT_TILL:
			motion = VIS_MOVE_RIGHT_TILL;
			break;
		default:
			goto err;
		}
		break;
	case VIS_MOVE_MARK:
	case VIS_MOVE_MARK_LINE:
	{
		int mark = va_arg(ap, int);
		if (VIS_MARK_a <= mark && mark < VIS_MARK_INVALID)
			vis->action.mark = mark;
		else
			goto err;
		break;
	}
	default:
		break;
	}

	if (motion < LENGTH(vis_motions))
		vis->action.movement = &vis_motions[motion];
	else
		vis->action.movement = array_get(&vis->motions, motion - VIS_MOVE_LAST);

	if (!vis->action.movement)
		goto err;

	va_end(ap);
	action_do(vis, &vis->action);
	return true;
err:
	va_end(ap);
	return false;
}

const Movement vis_motions[] = {
	[VIS_MOVE_LINE_UP]             = { .cur = view_line_up,             .type = LINEWISE                 },
	[VIS_MOVE_LINE_DOWN]           = { .cur = view_line_down,           .type = LINEWISE                 },
	[VIS_MOVE_SCREEN_LINE_UP]      = { .cur = view_screenline_up,                                        },
	[VIS_MOVE_SCREEN_LINE_DOWN]    = { .cur = view_screenline_down,                                      },
	[VIS_MOVE_SCREEN_LINE_BEGIN]   = { .cur = view_screenline_begin,    .type = CHARWISE                 },
	[VIS_MOVE_SCREEN_LINE_MIDDLE]  = { .cur = view_screenline_middle,   .type = CHARWISE                 },
	[VIS_MOVE_SCREEN_LINE_END]     = { .cur = view_screenline_end,      .type = CHARWISE|INCLUSIVE       },
	[VIS_MOVE_LINE_PREV]           = { .txt = text_line_prev,                                            },
	[VIS_MOVE_LINE_BEGIN]          = { .txt = text_line_begin,                                           },
	[VIS_MOVE_LINE_START]          = { .txt = text_line_start,                                           },
	[VIS_MOVE_LINE_FINISH]         = { .txt = text_line_finish,         .type = INCLUSIVE                },
	[VIS_MOVE_LINE_LASTCHAR]       = { .txt = text_line_lastchar,       .type = INCLUSIVE                },
	[VIS_MOVE_LINE_END]            = { .txt = text_line_end,                                             },
	[VIS_MOVE_LINE_NEXT]           = { .txt = text_line_next,                                            },
	[VIS_MOVE_LINE]                = { .vis = line,                     .type = LINEWISE|IDEMPOTENT|JUMP },
	[VIS_MOVE_COLUMN]              = { .vis = column,                   .type = CHARWISE|IDEMPOTENT      },
	[VIS_MOVE_CHAR_PREV]           = { .txt = text_char_prev,           .type = CHARWISE                 },
	[VIS_MOVE_CHAR_NEXT]           = { .txt = text_char_next,           .type = CHARWISE                 },
	[VIS_MOVE_LINE_CHAR_PREV]      = { .txt = text_line_char_prev,      .type = CHARWISE                 },
	[VIS_MOVE_LINE_CHAR_NEXT]      = { .txt = text_line_char_next,      .type = CHARWISE                 },
	[VIS_MOVE_WORD_START_PREV]     = { .txt = text_word_start_prev,     .type = CHARWISE                 },
	[VIS_MOVE_WORD_START_NEXT]     = { .txt = text_word_start_next,     .type = CHARWISE                 },
	[VIS_MOVE_WORD_END_PREV]       = { .txt = text_word_end_prev,       .type = CHARWISE|INCLUSIVE       },
	[VIS_MOVE_WORD_END_NEXT]       = { .txt = text_word_end_next,       .type = CHARWISE|INCLUSIVE       },
	[VIS_MOVE_LONGWORD_START_PREV] = { .txt = text_longword_start_prev, .type = CHARWISE                 },
	[VIS_MOVE_LONGWORD_START_NEXT] = { .txt = text_longword_start_next, .type = CHARWISE                 },
	[VIS_MOVE_LONGWORD_END_PREV]   = { .txt = text_longword_end_prev,   .type = CHARWISE|INCLUSIVE       },
	[VIS_MOVE_LONGWORD_END_NEXT]   = { .txt = text_longword_end_next,   .type = CHARWISE|INCLUSIVE       },
	[VIS_MOVE_SENTENCE_PREV]       = { .txt = text_sentence_prev,       .type = CHARWISE                 },
	[VIS_MOVE_SENTENCE_NEXT]       = { .txt = text_sentence_next,       .type = CHARWISE                 },
	[VIS_MOVE_PARAGRAPH_PREV]      = { .txt = text_paragraph_prev,      .type = LINEWISE|JUMP            },
	[VIS_MOVE_PARAGRAPH_NEXT]      = { .txt = text_paragraph_next,      .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_START_PREV] = { .txt = text_function_start_prev, .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_START_NEXT] = { .txt = text_function_start_next, .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_END_PREV]   = { .txt = text_function_end_prev,   .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_END_NEXT]   = { .txt = text_function_end_next,   .type = LINEWISE|JUMP            },
	[VIS_MOVE_BRACKET_MATCH]       = { .txt = bracket_match,            .type = INCLUSIVE|JUMP           },
	[VIS_MOVE_FILE_BEGIN]          = { .txt = text_begin,               .type = LINEWISE|JUMP            },
	[VIS_MOVE_FILE_END]            = { .txt = text_end,                 .type = LINEWISE|JUMP            },
	[VIS_MOVE_LEFT_TO]             = { .vis = to_left,                                                   },
	[VIS_MOVE_RIGHT_TO]            = { .vis = to,                       .type = INCLUSIVE                },
	[VIS_MOVE_LEFT_TILL]           = { .vis = till_left,                                                 },
	[VIS_MOVE_RIGHT_TILL]          = { .vis = till,                     .type = INCLUSIVE                },
	[VIS_MOVE_MARK]                = { .file = mark_goto,               .type = JUMP|IDEMPOTENT          },
	[VIS_MOVE_MARK_LINE]           = { .file = mark_line_goto,          .type = LINEWISE|JUMP|IDEMPOTENT },
	[VIS_MOVE_SEARCH_WORD_FORWARD] = { .vis = search_word_forward,      .type = JUMP                     },
	[VIS_MOVE_SEARCH_WORD_BACKWARD]= { .vis = search_word_backward,     .type = JUMP                     },
	[VIS_MOVE_SEARCH_NEXT]         = { .vis = search_forward,           .type = JUMP                     },
	[VIS_MOVE_SEARCH_PREV]         = { .vis = search_backward,          .type = JUMP                     },
	[VIS_MOVE_WINDOW_LINE_TOP]     = { .view = view_lines_top,          .type = LINEWISE|JUMP|IDEMPOTENT },
	[VIS_MOVE_WINDOW_LINE_MIDDLE]  = { .view = view_lines_middle,       .type = LINEWISE|JUMP|IDEMPOTENT },
	[VIS_MOVE_WINDOW_LINE_BOTTOM]  = { .view = view_lines_bottom,       .type = LINEWISE|JUMP|IDEMPOTENT },
	[VIS_MOVE_CHANGELIST_NEXT]     = { .win = window_changelist_next,   .type = INCLUSIVE                },
	[VIS_MOVE_CHANGELIST_PREV]     = { .win = window_changelist_prev,   .type = INCLUSIVE                },
	[VIS_MOVE_JUMPLIST_NEXT]       = { .win = window_jumplist_next,     .type = INCLUSIVE                },
	[VIS_MOVE_JUMPLIST_PREV]       = { .win = window_jumplist_prev,     .type = INCLUSIVE                },
	[VIS_MOVE_NOP]                 = { .win = window_nop,               .type = IDEMPOTENT               },
	[VIS_MOVE_PERCENT]             = { .vis = percent,                  .type = IDEMPOTENT               },
};

