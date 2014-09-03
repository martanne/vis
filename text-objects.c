#include <ctype.h>
#include "text-motions.h"
#include "text-objects.h"
#include "util.h"

static Filerange empty = {
	.start = -1,
	.end = -1,
};

Filerange text_object_word(Text *txt, size_t pos) {
	Filerange r;
	char c, prev = '0', next = '0';
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return empty;
	if (text_iterator_byte_prev(&it, &prev))
		text_iterator_byte_next(&it, NULL);
	text_iterator_byte_next(&it, &next);
	if (isspace(c)) {
		/* middle of two words, include leading white space */
		r.start = text_char_next(txt, text_word_end_prev(txt, pos));
		r.end = text_char_next(txt, text_word_end_next(txt, pos));
	} else if (isspace(prev) && isspace(next)) {
		/* on a single character */
		r.start = pos;
		r.end = text_word_start_next(txt, pos);
	} else if (isspace(prev)) {
		/* at start of a word */
		r.start = pos;
		r.end = text_word_start_next(txt, text_word_end_next(txt, pos));
	} else if (isspace(next)) {
		/* at end of a word */
		r.start = text_word_start_prev(txt, pos);
		r.end = text_word_start_next(txt, pos);
	} else {
		/* in the middle of a word */
		r.start = text_word_start_prev(txt, pos);
		r.end = text_word_start_next(txt, text_word_end_next(txt, pos));
	}
	return r;
}

Filerange text_object_line(Text *txt, size_t pos) {
	char c;
	Filerange r;
	r.start = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, text_line_end(txt, pos));
	if (text_iterator_byte_get(&it, &c) && c == '\n')
		text_iterator_byte_next(&it, NULL);
	if (text_iterator_byte_get(&it, &c) && c == '\r')
		text_iterator_byte_next(&it, NULL);
	r.end = it.pos;
	return r;
}

Filerange text_object_sentence(Text *txt, size_t pos) {
	Filerange r;
	r.start = text_sentence_prev(txt, pos);
	r.end = text_sentence_next(txt, pos);
	return r;
}

Filerange text_object_paragraph(Text *txt, size_t pos) {
	Filerange r;
	r.start = text_paragraph_prev(txt, pos);
	r.end = text_paragraph_next(txt, pos);
	return r;
}

static Filerange text_object_bracket(Text *txt, size_t pos, char type) {
	char c, open, close;
	int opened = 1, closed = 1;

	switch (type) {
	case '(':  case ')': open = '(';  close = ')';  break;
	case '{':  case '}': open = '{';  close = '}';  break;
	case '[':  case ']': open = '[';  close = ']';  break;
	case '<':  case '>': open = '<';  close = '>';  break;
	case '"':            open = '"';  close = '"';  break;
	case '`':            open = '`';  close = '`';  break;
	case '\'':           open = '\''; close = '\''; break;
	default: return empty;
	}

	Filerange r = empty;
	Iterator it = text_iterator_get(txt, pos);

	if (open == close && text_iterator_byte_get(&it, &c) && (c == '"' || c == '`' || c == '\'')) {
		size_t match = text_bracket_match(txt, pos);
		r.start = MIN(pos, match) + 1;
		r.end = MAX(pos, match);
		return r;
	}

	while (text_iterator_byte_get(&it, &c)) {
		if (c == open && --opened == 0) {
			r.start = it.pos + 1;
			break;
		} else if (c == close && it.pos != pos) {
			opened++;
		}
		text_iterator_byte_prev(&it, NULL);
	}

	it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c)) {
		if (c == close && --closed == 0) {
			r.end = it.pos;
			break;
		} else if (c == open && it.pos != pos) {
			closed++;
		}
		text_iterator_byte_next(&it, NULL);
	}

	if (r.start == (size_t)-1 || r.end == (size_t)-1 || r.start > r.end)
		return empty;
	return r;
}

Filerange text_object_square_bracket(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, ']');
}

Filerange text_object_curly_bracket(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, '}');
}

Filerange text_object_angle_bracket(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, '>');
}

Filerange text_object_paranthese(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, ')');
}

Filerange text_object_quote(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, '"');
}

Filerange text_object_single_quote(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, '\'');
}

Filerange text_object_backtick(Text *txt, size_t pos) {
	return text_object_bracket(txt, pos, '`');
}
