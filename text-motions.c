#include <ctype.h>
#include <string.h>
#include "text-motions.h"
#include "util.h"

// TODO: consistent usage of iterators either char or byte based where appropriate.

size_t text_begin(Text *txt, size_t pos) {
	return 0;
}

size_t text_end(Text *txt, size_t pos) {
	return text_size(txt);
}

size_t text_char_next(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	text_iterator_char_next(&it, NULL);
	return it.pos;
}

size_t text_char_prev(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	text_iterator_char_prev(&it, NULL);
	return it.pos;
}

size_t text_find_char_next(Text *txt, size_t pos, const char *s, size_t len) {
	char c;
	size_t matched = 0;
	Iterator it = text_iterator_get(txt, pos);
	while (matched < len && text_iterator_byte_get(&it, &c)) {
		if (c == s[matched])
			matched++;
		else
			matched = 0;
		text_iterator_byte_next(&it, NULL);
	}
	return it.pos - (matched == len ? len : 0);
}

size_t text_find_char_prev(Text *txt, size_t pos, const char *s, size_t len) {
	char c;
	size_t matched = len - 1;
	Iterator it = text_iterator_get(txt, pos);
	if (len == 0)
		return pos;
	while (text_iterator_byte_get(&it, &c)) {
		if (c == s[matched]) {
			if (matched-- == 0)
				break;
		} else {
			matched = len - 1;
		}
		text_iterator_byte_prev(&it, NULL);
	}
	return it.pos;
}

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
	return it.pos;
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
	if (!isutf8(c))
		text_iterator_char_prev(&it, NULL);
	return it.pos;
}

size_t text_line_end(Text *txt, size_t pos) {
	return text_find_char_next(txt, pos, "\n", 1);
}

size_t text_line_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	if (text_iterator_byte_next(&it, &c) && c == '\r')
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
	while (text_iterator_byte_get(&it, &c) && !isboundry(c))
		text_iterator_byte_prev(&it, NULL);
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
	case '"':
	case '`':
	case '\'': {
		char special[] = " \n)}]>.,";
		search = current;
		direction = 1;
		if (text_iterator_byte_next(&it, &c)) {
			/* if a single or double quote is followed by
			 * a special character, search backwards */
			if (memchr(special, c, sizeof(special)))
				direction = -1;
			text_iterator_byte_prev(&it, NULL);
		}
		break;
	}
	default: return pos;
	}

	if (direction >= 0) { /* forward search */
		while (text_iterator_byte_next(&it, &c)) {
			if (c == search && --count == 0)
				return it.pos;
			else if (c == current)
				count++;
		}
	} else { /* backwards */
		while (text_iterator_byte_prev(&it, &c)) {
			if (c == search && --count == 0)
				return it.pos;
			else if (c == current)
				count++;
		}
	}

	return pos; /* no match found */
}

size_t text_search_forward(Text *txt, size_t pos, Regex *regex) {
	int start = pos + 1;
	int end = text_size(txt);
	RegexMatch match[1];
	bool found = !text_search_range_forward(txt, start, end - start, regex, 1, match, 0);

	if (!found) {
		start = 0;
		end = pos;
		found = !text_search_range_forward(txt, start, end, regex, 1, match, 0);
	}

	return found ? match[0].start : pos;
}

size_t text_search_backward(Text *txt, size_t pos, Regex *regex) {
	int start = 0;
	int end = pos;
	RegexMatch match[1];
	bool found = !text_search_range_backward(txt, start, end, regex, 1, match, 0);

	if (!found) {
		start = pos + 1;
		end = text_size(txt);
		found = !text_search_range_backward(txt, start, end - start, regex, 1, match, 0);
	}

	return found ? match[0].start : pos;
}
