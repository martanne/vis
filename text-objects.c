#include <ctype.h>
#include "text-motions.h"
#include "text-objects.h"

static Filerange empty = {
	.start = -1,
	.end = -1,
};

Filerange text_object_word(Text *txt, size_t pos) {
	char c, prev = '0', next = '0';
	Filerange r;
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
