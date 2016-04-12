#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include "text-motions.h"
#include "text-util.h"
#include "util.h"

#define space(c) (isspace((unsigned char)c))
#define boundary(c) (isboundary((unsigned char)c))

// TODO: specify this per file type?
int is_word_boundary(int c) {
	return ISASCII(c) && !(('0' <= c && c <= '9') ||
	         ('a' <= c && c <= 'z') ||
	         ('A' <= c && c <= 'Z') || c == '_');
}

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

static size_t find_next(Text *txt, size_t pos, const char *s, bool line) {
	if (!s)
		return pos;
	size_t len = strlen(s), matched = 0;
	Iterator it = text_iterator_get(txt, pos), sit;
	for (char c; matched < len && text_iterator_byte_get(&it, &c); ) {
		if (c == s[matched]) {
			if (matched == 0)
				sit = it;
			matched++;
		} else if (matched > 0) {
			it = sit;
			matched = 0;
		}
		text_iterator_byte_next(&it, NULL);
		if (line && c == '\n')
			break;
	}
	return matched == len ? it.pos - len : pos;
}

size_t text_find_next(Text *txt, size_t pos, const char *s) {
	return find_next(txt, pos, s, false);
}

size_t text_line_find_next(Text *txt, size_t pos, const char *s) {
	return find_next(txt, pos, s, true);
}

static size_t find_prev(Text *txt, size_t pos, const char *s, bool line) {
	if (!s)
		return pos;
	size_t len = strlen(s), matched = len - 1;
	Iterator it = text_iterator_get(txt, pos), sit;
	if (len == 0)
		return pos;
	for (char c; text_iterator_byte_prev(&it, &c); ) {
		if (c == s[matched]) {
			if (matched == 0)
				return it.pos;
			if (matched == len - 1)
				sit = it;
			matched--;
		} else if (matched < len - 1) {
			it = sit;
			matched = len - 1;
		}
		if (line && c == '\n')
			break;
	}
	return pos;
}

size_t text_find_prev(Text *txt, size_t pos, const char *s) {
	return find_prev(txt, pos, s, false);
}

size_t text_line_find_prev(Text *txt, size_t pos, const char *s) {
	return find_prev(txt, pos, s, true);
}

size_t text_line_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	if (c == '\n')
		text_iterator_byte_prev(&it, &c);
	if (c == '\r')
		text_iterator_byte_prev(&it, &c);
	while (text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_prev(&it, NULL);
	if (text_iterator_byte_prev(&it, &c) && c != '\r')
		text_iterator_byte_next(&it, &c);
	return it.pos;
}

size_t text_line_begin(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	if (c == '\n')
		text_iterator_byte_prev(&it, &c);
	if (c == '\r')
		text_iterator_byte_prev(&it, &c);
	while (text_iterator_byte_get(&it, &c)) {
		if (c == '\n') {
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
	while (text_iterator_byte_get(&it, &c) && c != '\n' && space(c))
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_finish(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, text_line_end(txt, pos));
	do text_iterator_char_prev(&it, NULL);
	while (text_iterator_byte_get(&it, &c) && c != '\n' && space(c));
	return it.pos;
}

size_t text_line_lastchar(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, text_line_end(txt, pos));
	if (text_iterator_char_prev(&it, &c) && c == '\n')
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_end(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c) && c != '\r' && c != '\n')
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_offset(Text *txt, size_t pos, size_t off) {
	char c;
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);
	while (off-- > 0 && text_iterator_byte_get(&it, &c) && c != '\r' && c != '\n')
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_char_set(Text *txt, size_t pos, int count) {
	char c;
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);
	while (count-- > 0 && text_iterator_byte_get(&it, &c) && c != '\r' && c != '\n')
		text_iterator_char_next(&it, NULL);
	return it.pos;
}

int text_line_char_get(Text *txt, size_t pos) {
	char c;
	int count = 0;
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);
	while (text_iterator_byte_get(&it, &c) && it.pos < pos && c != '\r' && c != '\n') {
		text_iterator_char_next(&it, NULL);
		count++;
	}
	return count;
}

int text_line_width_get(Text *txt, size_t pos) {
	int width = 0;
	mbstate_t ps = { 0 };
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);

	while (it.pos < pos) {
		char buf[MB_CUR_MAX];
		size_t len = text_bytes_get(txt, it.pos, sizeof buf, buf);
		if (len == 0 || buf[0] == '\r' || buf[0] == '\n')
			break;
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			/* assume a replacement symbol will be displayed */
			width++;
		} else if (wclen == (size_t)-2) {
			/* do nothing, advance to next character */
		} else if (wclen == 0) {
			/* assume NUL byte will be displayed as ^@ */
			width += 2;
		} else if (buf[0] == '\t') {
			width++;
		} else {
			int w = wcwidth(wc);
			if (w == -1)
				w = 2; /* assume non-printable will be displayed as ^{char} */
			width += w;
		}

		if (!text_iterator_codepoint_next(&it, NULL))
			break;
	}

	return width;
}

size_t text_line_width_set(Text *txt, size_t pos, int width) {
	int cur_width = 0;
	mbstate_t ps = { 0 };
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);

	for (;;) {
		char buf[MB_CUR_MAX];
		size_t len = text_bytes_get(txt, it.pos, sizeof buf, buf);
		if (len == 0 || buf[0] == '\r' || buf[0] == '\n')
			break;
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			/* assume a replacement symbol will be displayed */
			cur_width++;
		} else if (wclen == (size_t)-2) {
			/* do nothing, advance to next character */
		} else if (wclen == 0) {
			/* assume NUL byte will be displayed as ^@ */
			cur_width += 2;
		} else if (buf[0] == '\t') {
			cur_width++;
		} else {
			int w = wcwidth(wc);
			if (w == -1)
				w = 2; /* assume non-printable will be displayed as ^{char} */
			cur_width += w;
		}

		if (cur_width >= width || !text_iterator_codepoint_next(&it, NULL))
			break;
	}

	return it.pos;
}

size_t text_line_char_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c) || c == '\r' || c == '\n')
		return pos;
	if (!text_iterator_char_next(&it, &c) || c == '\r' || c == '\n')
		return pos;
	return it.pos;
}

size_t text_line_char_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_char_prev(&it, &c) || c == '\n')
		return pos;
	return it.pos;
}

size_t text_line_up(Text *txt, size_t pos) {
	int width = text_line_width_get(txt, pos);
	size_t prev = text_line_prev(txt, pos);
	return text_line_width_set(txt, prev, width);
}

size_t text_line_down(Text *txt, size_t pos) {
	int width = text_line_width_get(txt, pos);
	size_t next = text_line_next(txt, pos);
	return text_line_width_set(txt, next, width);
}

size_t text_range_line_first(Text *txt, Filerange *r) {
	if (!text_range_valid(r))
		return EPOS;
	return r->start;
}

size_t text_range_line_last(Text *txt, Filerange *r) {
	if (!text_range_valid(r))
		return EPOS;
	size_t pos = text_line_begin(txt, r->end);
	if (pos == r->end) {
		/* range ends at a begin of a line, skip last line ending */
		pos = text_line_prev(txt, pos);
		pos = text_line_begin(txt, pos);
	}
	return r->start <= pos ? pos : r->start;
}

size_t text_range_line_next(Text *txt, Filerange *r, size_t pos) {
	if (!text_range_contains(r, pos))
		return EPOS;
	size_t newpos = text_line_next(txt, pos);
	return newpos != pos && newpos < r->end ? newpos : EPOS;
}

size_t text_range_line_prev(Text *txt, Filerange *r, size_t pos) {
	if (!text_range_contains(r, pos))
		return EPOS;
	size_t newpos = text_line_begin(txt, text_line_prev(txt, pos));
	return newpos != pos && r->start <= newpos ? newpos : EPOS;
}

size_t text_customword_start_next(Text *txt, size_t pos, int (*isboundary)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	if (boundary(c))
		while (boundary(c) && !space(c) && text_iterator_char_next(&it, &c));
	else
		while (!boundary(c) && text_iterator_char_next(&it, &c));
	while (space(c) && text_iterator_char_next(&it, &c));
	return it.pos;
}

size_t text_customword_start_prev(Text *txt, size_t pos, int (*isboundary)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_char_prev(&it, &c) && space(c));
	if (boundary(c))
		do pos = it.pos; while (text_iterator_char_prev(&it, &c) && boundary(c) && !space(c));
	else
		do pos = it.pos; while (text_iterator_char_prev(&it, &c) && !boundary(c));
	return pos;
}

size_t text_customword_end_next(Text *txt, size_t pos, int (*isboundary)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_char_next(&it, &c) && space(c));
	if (boundary(c))
		do pos = it.pos; while (text_iterator_char_next(&it, &c) && boundary(c) && !space(c));
	else
		do pos = it.pos; while (text_iterator_char_next(&it, &c) && !isboundary(c));
	return pos;
}

size_t text_customword_end_prev(Text *txt, size_t pos, int (*isboundary)(int)) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return pos;
	if (boundary(c))
		while (boundary(c) && !space(c) && text_iterator_char_prev(&it, &c));
	else
		while (!boundary(c) && text_iterator_char_prev(&it, &c));
	while (space(c) && text_iterator_char_prev(&it, &c));
	return it.pos;
}

size_t text_longword_end_next(Text *txt, size_t pos) {
	return text_customword_end_next(txt, pos, isspace);
}

size_t text_longword_end_prev(Text *txt, size_t pos) {
	return text_customword_end_prev(txt, pos, isspace);
}

size_t text_longword_start_next(Text *txt, size_t pos) {
	return text_customword_start_next(txt, pos, isspace);
}

size_t text_longword_start_prev(Text *txt, size_t pos) {
	return text_customword_start_prev(txt, pos, isspace);
}

size_t text_word_end_next(Text *txt, size_t pos) {
	return text_customword_end_next(txt, pos, is_word_boundary);
}

size_t text_word_end_prev(Text *txt, size_t pos) {
	return text_customword_end_prev(txt, pos, is_word_boundary);
}

size_t text_word_start_next(Text *txt, size_t pos) {
	return text_customword_start_next(txt, pos, is_word_boundary);
}

size_t text_word_start_prev(Text *txt, size_t pos) {
	return text_customword_start_prev(txt, pos, is_word_boundary);
}

size_t text_sentence_next(Text *txt, size_t pos) {
	char c, prev = 'X';
	Iterator it = text_iterator_get(txt, pos), rev = text_iterator_get(txt, pos);

	if (!text_iterator_byte_get(&it, &c))
		return pos;

	while (text_iterator_byte_get(&rev, &prev) && space(prev))
		text_iterator_byte_prev(&rev, NULL);
	prev = rev.pos == 0 ? '.' : prev; /* simulate punctuation at BOF */

	do {
		if ((prev == '.' || prev == '?' || prev == '!') && space(c)) {
			do text_iterator_byte_next(&it, NULL);
			while (text_iterator_byte_get(&it, &c) && space(c));
			return it.pos;
		}
		prev = c;
	} while (text_iterator_byte_next(&it, &c));
	return it.pos;
}

size_t text_sentence_prev(Text *txt, size_t pos) {
	char c, prev = 'X';
	bool content = false;
	Iterator it = text_iterator_get(txt, pos);

	while (it.pos != 0 && text_iterator_byte_prev(&it, &c)) {
		if (content && space(prev) && (c == '.' || c == '?' || c == '!')) {
			do text_iterator_byte_next(&it, NULL);
			while (text_iterator_byte_get(&it, &c) && space(c));
			return it.pos;
		}
		content |= !space(c);
		prev = c;
	} /* The loop only ends on hitting BOF or error */
	if (content) /* starting pos was after first sentence in file => find that sentences start */
		while (text_iterator_byte_get(&it, &c) && space(c))
			text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_paragraph_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);

	while (text_iterator_byte_get(&it, &c) && (c == '\n' || c == '\r'))
		text_iterator_byte_next(&it, NULL);
	return text_line_empty_next(txt, it.pos);
}

size_t text_paragraph_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);

	/* c == \0 catches starting the search at EOF */
	while (text_iterator_byte_get(&it, &c) && (c == '\n' || c == '\r' || c == '\0'))
		text_iterator_byte_prev(&it, NULL);
	return text_line_empty_prev(txt, it.pos);
}

size_t text_line_empty_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_get(&it, &c)) {
		if (c == '\n' && text_iterator_byte_next(&it, &c)) {
			size_t match = it.pos;
			if (c == '\r')
				text_iterator_byte_next(&it, &c);
			if (c == '\n')
				return match;
		}
		text_iterator_byte_next(&it, NULL);
	}
	return it.pos;
}

size_t text_line_empty_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_prev(&it, &c)) {
		if (c == '\n' && text_iterator_byte_prev(&it, &c)) {
			if (c == '\r')
				text_iterator_byte_prev(&it, &c);
			if (c == '\n')
				return it.pos + 1;
		}
	}
	return it.pos;
}

size_t text_function_start_next(Text *txt, size_t pos) {
	size_t a = text_function_end_next(txt, pos);
	size_t b = a;
	char c;
	if (a != pos) {
		Iterator it = text_iterator_get(txt, a);
		while (text_iterator_byte_next(&it, &c) && (c == '\r' || c == '\n'));
		a = it.pos;
	}
	if (b != pos) {
		size_t match = text_bracket_match(txt, b);
		b = match != b ? text_line_next(txt, text_line_empty_prev(txt, match)) : pos;
	}
	if (a <= pos && b <= pos)
		return pos;
	else if (a <= pos)
		return b;
	else if (b <= pos)
		return a;
	else
		return MIN(a, b);
}

size_t text_function_start_prev(Text *txt, size_t pos) {
	char c;
	size_t apos = text_byte_get(txt, pos, &c) && c == '}' && pos > 0 ? pos - 1 : pos;
	size_t a = text_function_end_next(txt, apos);
	size_t b = text_function_end_prev(txt, pos);
	if (a != apos) {
		size_t match = text_bracket_match(txt, a);
		a = match != a ? text_line_next(txt, text_line_empty_prev(txt, match)) : pos;
	}
	if (b != pos) {
		size_t match = text_bracket_match(txt, b);
		b = match != b ? text_line_next(txt, text_line_empty_prev(txt, match)) : pos;
	}
	if (a >= pos && b >= pos)
		return pos;
	else if (a >= pos)
		return b;
	else if (b >= pos)
		return a;
	else
		return MAX(a, b);
}

static size_t text_function_end_direction(Text *txt, size_t pos, int direction) {
	size_t start = pos, match;
	if (direction < 0 && pos > 0)
		pos--;
	for (;;) {
		char c[3];
		if (direction > 0)
			match = text_find_next(txt, pos, "\n}");
		else
			match = text_find_prev(txt, pos, "\n}");
		if (text_bytes_get(txt, match, sizeof c, c) != 3 || c[0] != '\n' || c[1] != '}')
			break;
		if (c[2] == '\r' || c[2] == '\n')
			return match+1;
		if (match == pos)
			match += direction;
		pos = match;
	}
	return start;
}

size_t text_function_end_next(Text *txt, size_t pos) {
	return text_function_end_direction(txt, pos, +1);
}

size_t text_function_end_prev(Text *txt, size_t pos) {
	return text_function_end_direction(txt, pos, -1);
}

size_t text_bracket_match(Text *txt, size_t pos) {
	return text_bracket_match_symbol(txt, pos, NULL);
}

size_t text_bracket_match_symbol(Text *txt, size_t pos, const char *symbols) {
	int direction, count = 1;
	char search, current, c;
	bool instring = false;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &current))
		return pos;
	if (symbols && !memchr(symbols, current, strlen(symbols)))
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
		char special[] = " \n)}]>.,:;";
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
			if (c != current && c == '"')
				instring = !instring;
			if (!instring) {
				if (c == search && --count == 0)
					return it.pos;
				else if (c == current)
					count++;
			}
		}
	} else { /* backwards */
		while (text_iterator_byte_prev(&it, &c)) {
			if (c != current && c == '"')
				instring = !instring;
			if (!instring) {
				if (c == search && --count == 0)
					return it.pos;
				else if (c == current)
					count++;
			}
		}
	}

	return pos; /* no match found */
}

size_t text_search_forward(Text *txt, size_t pos, Regex *regex) {
	size_t start = pos + 1;
	size_t end = text_size(txt);
	RegexMatch match[1];
	bool found = start < end && !text_search_range_forward(txt, start, end - start, regex, 1, match, 0);

	if (!found) {
		start = 0;
		end = pos;
		found = !text_search_range_forward(txt, start, end, regex, 1, match, 0);
	}

	return found ? match[0].start : pos;
}

size_t text_search_backward(Text *txt, size_t pos, Regex *regex) {
	size_t start = 0;
	size_t end = pos;
	RegexMatch match[1];
	bool found = !text_search_range_backward(txt, start, end, regex, 1, match, 0);

	if (!found) {
		start = pos + 1;
		end = text_size(txt);
		found = start < end && !text_search_range_backward(txt, start, end - start, regex, 1, match, 0);
	}

	return found ? match[0].start : pos;
}
