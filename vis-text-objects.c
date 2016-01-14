#include "vis-core.h"
#include "text-objects.h"
#include "util.h"

void vis_textobject(Vis *vis, enum VisTextObject id) {
	if (id < LENGTH(vis_textobjects)) {
		vis->action.textobj = &vis_textobjects[id];
		action_do(vis, &vis->action);
	}
}

TextObject vis_textobjects[] = {
	[VIS_TEXTOBJECT_INNER_WORD]           = { text_object_word                  },
	[VIS_TEXTOBJECT_OUTER_WORD]           = { text_object_word_outer            },
	[VIS_TEXTOBJECT_INNER_LONGWORD]       = { text_object_longword              },
	[VIS_TEXTOBJECT_OUTER_LONGWORD]       = { text_object_longword_outer        },
	[VIS_TEXTOBJECT_SENTENCE]             = { text_object_sentence              },
	[VIS_TEXTOBJECT_PARAGRAPH]            = { text_object_paragraph             },
	[VIS_TEXTOBJECT_OUTER_SQUARE_BRACKET] = { text_object_square_bracket, OUTER },
	[VIS_TEXTOBJECT_INNER_SQUARE_BRACKET] = { text_object_square_bracket, INNER },
	[VIS_TEXTOBJECT_OUTER_CURLY_BRACKET]  = { text_object_curly_bracket,  OUTER },
	[VIS_TEXTOBJECT_INNER_CURLY_BRACKET]  = { text_object_curly_bracket,  INNER },
	[VIS_TEXTOBJECT_OUTER_ANGLE_BRACKET]  = { text_object_angle_bracket,  OUTER },
	[VIS_TEXTOBJECT_INNER_ANGLE_BRACKET]  = { text_object_angle_bracket,  INNER },
	[VIS_TEXTOBJECT_OUTER_PARANTHESE]     = { text_object_paranthese,     OUTER },
	[VIS_TEXTOBJECT_INNER_PARANTHESE]     = { text_object_paranthese,     INNER },
	[VIS_TEXTOBJECT_OUTER_QUOTE]          = { text_object_quote,          OUTER },
	[VIS_TEXTOBJECT_INNER_QUOTE]          = { text_object_quote,          INNER },
	[VIS_TEXTOBJECT_OUTER_SINGLE_QUOTE]   = { text_object_single_quote,   OUTER },
	[VIS_TEXTOBJECT_INNER_SINGLE_QUOTE]   = { text_object_single_quote,   INNER },
	[VIS_TEXTOBJECT_OUTER_BACKTICK]       = { text_object_backtick,       OUTER },
	[VIS_TEXTOBJECT_INNER_BACKTICK]       = { text_object_backtick,       INNER },
	[VIS_TEXTOBJECT_OUTER_ENTIRE]         = { text_object_entire,               },
	[VIS_TEXTOBJECT_INNER_ENTIRE]         = { text_object_entire_inner,         },
	[VIS_TEXTOBJECT_OUTER_FUNCTION]       = { text_object_function,             },
	[VIS_TEXTOBJECT_INNER_FUNCTION]       = { text_object_function_inner,       },
	[VIS_TEXTOBJECT_OUTER_LINE]           = { text_object_line,                 },
	[VIS_TEXTOBJECT_INNER_LINE]           = { text_object_line_inner,           },
};

