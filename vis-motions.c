#include "vis-core.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-util.h"

/** utility functions */

static char *get_word_at(Text *txt, size_t pos) {
	Filerange word = text_object_word(txt, pos);
	if (!text_range_valid(&word))
		return NULL;
	size_t len = word.end - word.start;
	char *buf = malloc(len+1);
	if (!buf)
		return NULL;
	len = text_bytes_get(txt, word.start, len, buf);
	buf[len] = '\0';
	return buf;
}

/** motion implementations */

static size_t search_word_forward(Vis *vis, Text *txt, size_t pos) {
	char *word = get_word_at(txt, pos);
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_forward(txt, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_word_backward(Vis *vis, Text *txt, size_t pos) {
	char *word = get_word_at(txt, pos);
	if (word && !text_regex_compile(vis->search_pattern, word, REG_EXTENDED))
		pos = text_search_backward(txt, pos, vis->search_pattern);
	free(word);
	return pos;
}

static size_t search_forward(Vis *vis, Text *txt, size_t pos) {
	return text_search_forward(txt, pos, vis->search_pattern);
}

static size_t search_backward(Vis *vis, Text *txt, size_t pos) {
	return text_search_backward(txt, pos, vis->search_pattern);
}

static size_t mark_goto(Vis *vis, File *file, size_t pos) {
	return text_mark_get(file->text, file->marks[vis->action.mark]);
}

static size_t mark_line_goto(Vis *vis, File *file, size_t pos) {
	return text_line_start(file->text, mark_goto(vis, file, pos));
}

static size_t to(Vis *vis, Text *txt, size_t pos) {
	char c;
	size_t hit = text_line_find_next(txt, pos+1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to(vis, txt, pos);
	if (hit != pos)
		return text_char_prev(txt, hit);
	return pos;
}

static size_t to_left(Vis *vis, Text *txt, size_t pos) {
	char c;
	if (pos == 0)
		return pos;
	size_t hit = text_line_find_prev(txt, pos-1, vis->search_char);
	if (!text_byte_get(txt, hit, &c) || c != vis->search_char[0])
		return pos;
	return hit;
}

static size_t till_left(Vis *vis, Text *txt, size_t pos) {
	size_t hit = to_left(vis, txt, pos);
	if (hit != pos)
		return text_char_next(txt, hit);
	return pos;
}

static size_t line(Vis *vis, Text *txt, size_t pos) {
	return text_pos_by_lineno(txt, vis->action.count);
}

static size_t column(Vis *vis, Text *txt, size_t pos) {
	return text_line_offset(txt, pos, vis->action.count);
}

static size_t view_lines_top(Vis *vis, View *view) {
	return view_screenline_goto(view, vis->action.count);
}

static size_t view_lines_middle(Vis *vis, View *view) {
	int h = view_height_get(view);
	return view_screenline_goto(view, h/2);
}

static size_t view_lines_bottom(Vis *vis, View *view) {
	int h = view_height_get(vis->win->view);
	return view_screenline_goto(vis->win->view, h - vis->action.count);
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

Movement moves[] = {
	[VIS_MOVE_LINE_UP]             = { .cur = view_line_up,            .type = LINEWISE                  },
	[VIS_MOVE_LINE_DOWN]           = { .cur = view_line_down,          .type = LINEWISE                  },
	[VIS_MOVE_SCREEN_LINE_UP]      = { .cur = view_screenline_up,                                        },
	[VIS_MOVE_SCREEN_LINE_DOWN]    = { .cur = view_screenline_down,                                      },
	[VIS_MOVE_SCREEN_LINE_BEGIN]   = { .cur = view_screenline_begin,   .type = CHARWISE                  },
	[VIS_MOVE_SCREEN_LINE_MIDDLE]  = { .cur = view_screenline_middle,  .type = CHARWISE                  },
	[VIS_MOVE_SCREEN_LINE_END]     = { .cur = view_screenline_end,     .type = CHARWISE|INCLUSIVE        },
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
	[VIS_MOVE_SENTENCE_PREV]       = { .txt = text_sentence_prev,       .type = LINEWISE                 },
	[VIS_MOVE_SENTENCE_NEXT]       = { .txt = text_sentence_next,       .type = LINEWISE                 },
	[VIS_MOVE_PARAGRAPH_PREV]      = { .txt = text_paragraph_prev,      .type = LINEWISE|JUMP            },
	[VIS_MOVE_PARAGRAPH_NEXT]      = { .txt = text_paragraph_next,      .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_START_PREV] = { .txt = text_function_start_prev, .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_START_NEXT] = { .txt = text_function_start_next, .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_END_PREV]   = { .txt = text_function_end_prev,   .type = LINEWISE|JUMP            },
	[VIS_MOVE_FUNCTION_END_NEXT]   = { .txt = text_function_end_next,   .type = LINEWISE|JUMP            },
	[VIS_MOVE_BRACKET_MATCH]       = { .txt = text_bracket_match,       .type = INCLUSIVE|JUMP           },
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
};

