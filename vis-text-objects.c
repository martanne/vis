#include "vis-core.h"
#include "text-objects.h"
#include "util.h"

int vis_textobject_register(Vis *vis, int type, void *data, VisTextObjectFunction *textobject) {

	TextObject *obj = calloc(1, sizeof *obj);
	if (!obj)
		return -1;

	obj->user = textobject;
	obj->type = type;
	obj->data = data;

	if (array_add_ptr(&vis->textobjects, obj))
		return LENGTH(vis_textobjects) + array_length(&vis->textobjects) - 1;
	free(obj);
	return -1;
}

bool vis_textobject(Vis *vis, enum VisTextObject id) {
	if (id < LENGTH(vis_textobjects))
		vis->action.textobj = &vis_textobjects[id];
	else
		vis->action.textobj = array_get_ptr(&vis->textobjects, id - LENGTH(vis_textobjects));
	if (!vis->action.textobj)
		return false;
	vis_do(vis);
	return true;
}

static Filerange search_forward(Vis *vis, Text *txt, size_t pos) {
	Filerange range = text_range_empty();
	Regex *regex = vis_regex(vis, NULL);
	if (regex)
		range = text_object_search_forward(txt, pos, regex);
	text_regex_free(regex);
	return range;
}

static Filerange search_backward(Vis *vis, Text *txt, size_t pos) {
	Filerange range = text_range_empty();
	Regex *regex = vis_regex(vis, NULL);
	if (regex)
		range = text_object_search_backward(txt, pos, regex);
	text_regex_free(regex);
	return range;
}

static Filerange object_unpaired(Text *txt, size_t pos, char obj) {
	char c;
	bool before = false;
	Iterator it = text_iterator_get(txt, pos), rit = it;

	while (text_iterator_byte_get(&rit, &c) && c != '\n') {
		if (c == obj) {
			before = true;
			break;
		}
		text_iterator_byte_prev(&rit, NULL);
	}

	/* if there is no previous occurrence on the same line, advance starting position */
	if (!before) {
		while (text_iterator_byte_get(&it, &c) && c != '\n') {
			if (c == obj) {
				pos = it.pos;
				break;
			}
			text_iterator_byte_next(&it, NULL);
		}
	}

	switch (obj) {
	case '"':
		return text_object_quote(txt, pos);
	case '\'':
		return text_object_single_quote(txt, pos);
	case '`':
		return text_object_backtick(txt, pos);
	default:
		return text_range_empty();
	}
}

static Filerange object_quote(Text *txt, size_t pos) {
	return object_unpaired(txt, pos, '"');
}

static Filerange object_single_quote(Text *txt, size_t pos) {
	return object_unpaired(txt, pos, '\'');
}

static Filerange object_backtick(Text *txt, size_t pos) {
	return object_unpaired(txt, pos, '`');
}

const TextObject vis_textobjects[] = {
	[VIS_TEXTOBJECT_INNER_WORD] = {
		.txt = text_object_word,
	},
	[VIS_TEXTOBJECT_OUTER_WORD] = {
		.txt = text_object_word_outer,
	},
	[VIS_TEXTOBJECT_INNER_LONGWORD] = {
		.txt = text_object_longword,
	},
	[VIS_TEXTOBJECT_OUTER_LONGWORD] = {
		.txt = text_object_longword_outer,
	},
	[VIS_TEXTOBJECT_SENTENCE] = {
		.txt = text_object_sentence,
	},
	[VIS_TEXTOBJECT_PARAGRAPH] = {
		.txt = text_object_paragraph,
	},
	[VIS_TEXTOBJECT_PARAGRAPH_OUTER] = {
		.txt = text_object_paragraph_outer,
	},
	[VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET] = {
		.txt = text_object_square_bracket,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_SQUARE_BRACKET] = {
		.txt = text_object_square_bracket,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_CURLY_BRACKET] = {
		.txt = text_object_curly_bracket,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_CURLY_BRACKET] = {
		.txt = text_object_curly_bracket,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET] = {
		.txt = text_object_angle_bracket,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_ANGLE_BRACKET] = {
		.txt = text_object_angle_bracket,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_PARENTHESIS] = {
		.txt = text_object_parenthesis,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_PARENTHESIS] = {
		.txt = text_object_parenthesis,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_QUOTE] = {
		.txt = object_quote,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_QUOTE] = {
		.txt = object_quote,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE] = {
		.txt = object_single_quote,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_SINGLE_QUOTE] = {
		.txt = object_single_quote,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_BACKTICK] = {
		.txt = object_backtick,
		.type = TEXTOBJECT_DELIMITED_OUTER,
	},
	[VIS_TEXTOBJECT_INNER_BACKTICK] = {
		.txt = object_backtick,
		.type = TEXTOBJECT_DELIMITED_INNER,
	},
	[VIS_TEXTOBJECT_OUTER_LINE] = {
		.txt = text_object_line,
	},
	[VIS_TEXTOBJECT_INNER_LINE] = {
		.txt = text_object_line_inner,
	},
	[VIS_TEXTOBJECT_INDENTATION] = {
		.txt = text_object_indentation,
	},
	[VIS_TEXTOBJECT_SEARCH_FORWARD] = {
		.vis = search_forward,
		.type = TEXTOBJECT_NON_CONTIGUOUS|TEXTOBJECT_EXTEND_FORWARD,
	},
	[VIS_TEXTOBJECT_SEARCH_BACKWARD] = {
		.vis = search_backward,
		.type = TEXTOBJECT_NON_CONTIGUOUS|TEXTOBJECT_EXTEND_BACKWARD,
	},
};

