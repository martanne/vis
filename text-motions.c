#include <ctype.h>
#include "text-motions.h"

// TODO: consistent usage of iterators either char or byte based where appropriate.

size_t text_line_begin(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	if (c == '\r')
		text_iterator_byte_prev(&it, &c);
	if (c == '\n')
		text_iterator_byte_prev(&it, &c);
	while (text_iterator_byte_get(&it, &c)) {
		if (c == '\n' || c == '\r') {
			it.pos++;
			break;
		}
		text_iterator_byte_prev(&it, NULL);
	}
	return pos;
}

size_t text_line_start(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, text_line_begin(txt, pos));
	while (text_iterator_byte_get(&it, &c) && c != '\n' && isspace(c))
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_finish(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, text_line_end(txt, pos));
	do text_iterator_byte_prev(&it, NULL);
	while (text_iterator_byte_get(&it, &c) && c != '\n' && c != '\r' && isspace(c));
	return it.pos;
}

size_t text_line_end(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}


size_t text_word_boundry_start_next(Text *txt, size_t pos, int (*isboundry)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c) && !isboundry(c))
		text_iterator_byte_next(&it, NULL);
	while (text_iterator_byte_get(&it, &c) && isboundry(c))
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_word_boundry_start_prev(Text *txt, size_t pos, int (*isboundry)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_prev(&it, &c) && isboundry(c));
	do pos = it.pos; while (text_iterator_char_prev(&it, &c) && !isboundry(c));
	return pos;
}
 
size_t text_word_boundry_end_next(Text *txt, size_t pos, int (*isboundry)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_char_next(&it, &c) && isboundry(c));
	do pos = it.pos; while (text_iterator_char_next(&it, &c) && !isboundry(c));
	return pos;
}

size_t text_word_boundry_end_prev(Text *txt, size_t pos, int (*isboundry)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_prev(&it, &c) && !isboundry(c));
	while (text_iterator_char_prev(&it, &c) && isboundry(c));
	return it.pos;
}

size_t text_word_end_next(Text *txt, size_t pos) {
	return text_word_boundry_end_next(txt, pos, isspace);
}

size_t text_word_end_prev(Text *txt, size_t pos) {
	return text_word_boundry_end_prev(txt, pos, isspace);
}

size_t text_word_start_next(Text *txt, size_t pos) {
	return text_word_boundry_start_next(txt, pos, isspace);
}

size_t text_word_start_prev(Text *txt, size_t pos) {
	return text_word_boundry_start_prev(txt, pos, isspace);
}

static size_t text_paragraph_sentence_next(Text *txt, size_t pos, bool sentence) {
	char c;
	bool content = false, paragraph = false;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_next(&it, &c)) {
		content |= !isspace(c);
		if (sentence && (c == '.' || c == '?' || c == '!') && text_iterator_byte_next(&it, &c) && isspace(c)) {
			if (c == '\n' && text_iterator_byte_next(&it, &c)) {
				if (c == '\r')
					text_iterator_byte_next(&it, &c);
			} else {
				while (text_iterator_byte_get(&it, &c) && c == ' ')
					text_iterator_byte_next(&it, NULL);
			}
			break;
		}
		if (c == '\n' && text_iterator_byte_next(&it, &c)) {
			if (c == '\r')
				text_iterator_byte_next(&it, &c);
			content |= !isspace(c);
			if (c == '\n')
				paragraph = true;
		}
		if (content && paragraph)
			break;
	}
	return it.pos;
}

static size_t text_paragraph_sentence_prev(Text *txt, size_t pos, bool sentence) {
	char prev, c;
	bool content = false, paragraph = false;

	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &prev))
		return pos;

	while (text_iterator_byte_prev(&it, &c)) {
		content |= !isspace(c) && c != '.' && c != '?' && c != '!';
		if (sentence && content && (c == '.' || c == '?' || c == '!') && isspace(prev)) {
			do text_iterator_byte_next(&it, NULL);
			while (text_iterator_byte_get(&it, &c) && isspace(c));
			break;
		}
		if (c == '\r')
			text_iterator_byte_prev(&it, &c);
		if (c == '\n' && text_iterator_byte_prev(&it, &c)) {
			content |= !isspace(c);
			if (c == '\r')
				text_iterator_byte_prev(&it, &c);
			if (c == '\n') {
				paragraph = true;
				if (content) {
					do text_iterator_byte_next(&it, NULL);
					while (text_iterator_byte_get(&it, &c) && isspace(c));
					break;
				}
			}
		}
		if (content && paragraph) {
			do text_iterator_byte_next(&it, NULL);
			while (text_iterator_byte_get(&it, &c) && !isspace(c));
			break;
		}
		prev = c;
	}
	return it.pos;
}

size_t text_sentence_next(Text *txt, size_t pos) {
	return text_paragraph_sentence_next(txt, pos, true);
}

size_t text_sentence_prev(Text *txt, size_t pos) {
	return text_paragraph_sentence_prev(txt, pos, true);
}

size_t text_paragraph_next(Text *txt, size_t pos) {
	return text_paragraph_sentence_next(txt, pos, false);
}

size_t text_paragraph_prev(Text *txt, size_t pos) {
	return text_paragraph_sentence_prev(txt, pos, false);
}

size_t text_bracket_match(Text *txt, size_t pos) {
	int direction, count = 1;
	char search, current, c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &current))
		return pos;

	switch (current) {
	case '(': search = ')'; direction =  1; break;
	case ')': search = '('; direction = -1; break;
	case '{': search = '}'; direction =  1; break;
	case '}': search = '{'; direction = -1; break;
	case '[': search = ']'; direction =  1; break;
	case ']': search = '['; direction = -1; break;
	case '<': search = '>'; direction =  1; break;
	case '>': search = '<'; direction = -1; break;
	// TODO: heuristic basic on neighbouring chars shared with text-object
	case '"': search = '"'; direction =  1; break;
	case '\'': search = '\''; direction =  1; break;
	default: return pos;
	}

	if (direction >= 0) { /* forward search */
		while (text_iterator_byte_next(&it, &c)) {
			if (c == search && --count == 0) {
				pos = it.pos;
				break;
			} else if (c == current) {
				count++;
			}
		}
	} else { /* backwards */
		while (text_iterator_byte_prev(&it, &c)) {
			if (c == search && --count == 0) {
				pos = it.pos;
				break;
			} else if (c == current) {
				count++;
			}
		}
	}

	return pos;
}
