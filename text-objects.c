#include <ctype.h>
#include "text-motions.h"
#include "text-objects.h"

static Filerange empty = {
	.start = -1,
	.end = -1,
};

// TODO: fix problems with inclusive / exclusive
Filerange text_object_word(Text *txt, size_t pos) {
	char c;
	Filerange r;
	if (!text_byte_get(txt, pos, &c))
		return empty;
	if (!isspace(c)) {
		r.start = text_word_start_prev(txt, pos);
		r.end = text_word_end_next(txt, pos);
	} else {
		r.start = text_word_end_prev(txt, pos);
		r.end = text_word_start_next(txt, pos);
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
