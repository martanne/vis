#include "vis-core.h"
#include "text-objects.h"
#include "util.h"

int vis_textobject_register(Vis *vis, int type, void *data,
	Filerange (*textobject)(Vis*, Win*, void*, size_t pos)) {

	TextObject *obj = calloc(1, sizeof *obj);
	if (!obj)
		return -1;

	obj->user = textobject;
	obj->type = type;
	obj->data = data;

	if (array_add(&vis->textobjects, obj))
		return LENGTH(vis_textobjects) + array_length(&vis->textobjects) - 1;
	free(obj);
	return -1;
}

bool vis_textobject(Vis *vis, enum VisTextObject id) {
	if (id < LENGTH(vis_textobjects))
		vis->action.textobj = &vis_textobjects[id];
	else
		vis->action.textobj = array_get(&vis->textobjects, id - LENGTH(vis_textobjects));
	if (!vis->action.textobj)
		return false;
	action_do(vis, &vis->action);
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

const TextObject vis_textobjects[] = {
	[VIS_TEXTOBJECT_INNER_WORD]           = { .txt = text_object_word                          },
	[VIS_TEXTOBJECT_OUTER_WORD]           = { .txt = text_object_word_outer                    },
	[VIS_TEXTOBJECT_INNER_LONGWORD]       = { .txt = text_object_longword                      },
	[VIS_TEXTOBJECT_OUTER_LONGWORD]       = { .txt = text_object_longword_outer                },
	[VIS_TEXTOBJECT_SENTENCE]             = { .txt = text_object_sentence                      },
	[VIS_TEXTOBJECT_PARAGRAPH]            = { .txt = text_object_paragraph                     },
	[VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET] = { .txt = text_object_square_bracket, .type = OUTER },
	[VIS_TEXTOBJECT_INNER_SQUARE_BRACKET] = { .txt = text_object_square_bracket, .type = INNER },
	[VIS_TEXTOBJECT_OUTER_CURLY_BRACKET]  = { .txt = text_object_curly_bracket,  .type = OUTER },
	[VIS_TEXTOBJECT_INNER_CURLY_BRACKET]  = { .txt = text_object_curly_bracket,  .type = INNER },
	[VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET]  = { .txt = text_object_angle_bracket,  .type = OUTER },
	[VIS_TEXTOBJECT_INNER_ANGLE_BRACKET]  = { .txt = text_object_angle_bracket,  .type = INNER },
	[VIS_TEXTOBJECT_OUTER_PARANTHESE]     = { .txt = text_object_paranthese,     .type = OUTER },
	[VIS_TEXTOBJECT_INNER_PARANTHESE]     = { .txt = text_object_paranthese,     .type = INNER },
	[VIS_TEXTOBJECT_OUTER_QUOTE]          = { .txt = text_object_quote,          .type = OUTER },
	[VIS_TEXTOBJECT_INNER_QUOTE]          = { .txt = text_object_quote,          .type = INNER },
	[VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE]   = { .txt = text_object_single_quote,   .type = OUTER },
	[VIS_TEXTOBJECT_INNER_SINGLE_QUOTE]   = { .txt = text_object_single_quote,   .type = INNER },
	[VIS_TEXTOBJECT_OUTER_BACKTICK]       = { .txt = text_object_backtick,       .type = OUTER },
	[VIS_TEXTOBJECT_INNER_BACKTICK]       = { .txt = text_object_backtick,       .type = INNER },
	[VIS_TEXTOBJECT_OUTER_ENTIRE]         = { .txt = text_object_entire,                       },
	[VIS_TEXTOBJECT_INNER_ENTIRE]         = { .txt = text_object_entire_inner,                 },
	[VIS_TEXTOBJECT_OUTER_FUNCTION]       = { .txt = text_object_function,                     },
	[VIS_TEXTOBJECT_INNER_FUNCTION]       = { .txt = text_object_function_inner,               },
	[VIS_TEXTOBJECT_OUTER_LINE]           = { .txt = text_object_line,                         },
	[VIS_TEXTOBJECT_INNER_LINE]           = { .txt = text_object_line_inner,                   },
	[VIS_TEXTOBJECT_INDENTATION]          = { .txt = text_object_indentation,                  },
	[VIS_TEXTOBJECT_SEARCH_FORWARD]       = { .vis = search_forward,             .type = SPLIT },
	[VIS_TEXTOBJECT_SEARCH_BACKWARD]      = { .vis = search_backward,            .type = SPLIT },
};

