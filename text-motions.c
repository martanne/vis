#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include <limits.h>
#include "text-motions.h"
#include "text-util.h"
#include "util.h"
#include "text-objects.h"

#define blank(c) ((c) == ' ' || (c) == '\t')
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

size_t text_codepoint_next(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	text_iterator_codepoint_next(&it, NULL);
	return it.pos;
}

size_t text_codepoint_prev(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	text_iterator_codepoint_prev(&it, NULL);
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
	Iterator it = text_iterator_get(txt, pos);
	text_iterator_byte_find_prev(&it, '\n');
	return it.pos;
}

size_t text_line_begin(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	return text_iterator_byte_find_prev(&it, '\n') ? it.pos+1 : it.pos;
}

size_t text_line_start(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, text_line_begin(txt, pos));
	while (text_iterator_byte_get(&it, &c) && blank(c))
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_finish(Text *txt, size_t pos) {
	char c;
	size_t end = text_line_end(txt, pos);
	Iterator it = text_iterator_get(txt, end);
	if (!text_iterator_byte_prev(&it, &c) || c == '\n')
		return end;
	while (blank(c) && text_iterator_byte_prev(&it, &c));
	return it.pos + (c == '\n');
}

size_t text_line_end(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	text_iterator_byte_find_next(&it, '\n');
	return it.pos;
}

size_t text_line_next(Text *txt, size_t pos) {
	Iterator it = text_iterator_get(txt, pos);
	if (text_iterator_byte_find_next(&it, '\n'))
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_offset(Text *txt, size_t pos, size_t off) {
	char c;
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);
	while (off-- > 0 && text_iterator_byte_get(&it, &c) && c != '\n')
		text_iterator_byte_next(&it, NULL);
	return it.pos;
}

size_t text_line_char_set(Text *txt, size_t pos, int count) {
	char c;
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);
	if (text_iterator_byte_get(&it, &c) && c != '\n')
		while (count-- > 0 && text_iterator_char_next(&it, &c) && c != '\n');
	return it.pos;
}

int text_line_char_get(Text *txt, size_t pos) {
	char c;
	int count = 0;
	size_t bol = text_line_begin(txt, pos);
	Iterator it = text_iterator_get(txt, bol);
	if (text_iterator_byte_get(&it, &c) && c != '\n') {
		while (it.pos < pos && c != '\n' && text_iterator_char_next(&it, &c))
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
		char buf[MB_LEN_MAX];
		size_t len = text_bytes_get(txt, it.pos, sizeof buf, buf);
		if (len == 0 || buf[0] == '\n')
			break;
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			ps = (mbstate_t){0};
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
		char buf[MB_LEN_MAX];
		size_t len = text_bytes_get(txt, it.pos, sizeof buf, buf);
		if (len == 0 || buf[0] == '\n')
			break;
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			ps = (mbstate_t){0};
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
	if (!text_iterator_byte_get(&it, &c) || c == '\n')
		return pos;
	text_iterator_char_next(&it, NULL);
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
	if (next == text_size(txt))
		return pos;
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
	Iterator it = text_iterator_get(txt, pos), rev = it;

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

	while (text_iterator_byte_get(&it, &c) && (c == '\n' || blank(c)))
		text_iterator_char_next(&it, NULL);
	return text_line_blank_next(txt, it.pos);
}

size_t text_paragraph_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);

	while (text_iterator_byte_get(&it, &c) && (c == '\n' || blank(c)))
		text_iterator_char_prev(&it, NULL);
	return text_line_blank_prev(txt, it.pos);
}

size_t text_line_empty_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_find_next(&it, '\n')) {
		if (text_iterator_byte_next(&it, &c) && c == '\n')
			return it.pos;
	}
	return it.pos;
}

size_t text_line_empty_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_find_prev(&it, '\n')) {
		if (text_iterator_byte_prev(&it, &c) && c == '\n')
			return it.pos + 1;
	}
	return it.pos;
}

size_t text_line_blank_next(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_find_next(&it, '\n')) {
		size_t n = it.pos;
		while (text_iterator_byte_next(&it, &c) && blank(c));
		if (c == '\n')
			return n + 1;
	}
	return it.pos;
}

size_t text_line_blank_prev(Text *txt, size_t pos) {
	char c;
	Iterator it = text_iterator_get(txt, pos);
	while (text_iterator_byte_find_prev(&it, '\n')) {
		while (text_iterator_byte_prev(&it, &c) && blank(c));
		if (c == '\n')
			return it.pos + 1;
	}
	return it.pos;
}

size_t text_block_start(Text *txt, size_t pos) {
	Filerange r = text_object_curly_bracket(txt, pos-1);
	return text_range_valid(&r) ? r.start-1 : pos;
}

size_t text_block_end(Text *txt, size_t pos) {
	Filerange r = text_object_curly_bracket(txt, pos+1);
	return text_range_valid(&r) ? r.end : pos;
}

size_t text_parenthesis_start(Text *txt, size_t pos) {
	Filerange r = text_object_parenthesis(txt, pos-1);
	return text_range_valid(&r) ? r.start-1 : pos;
}

size_t text_parenthesis_end(Text *txt, size_t pos) {
	Filerange r = text_object_parenthesis(txt, pos+1);
	return text_range_valid(&r) ? r.end : pos;
}

size_t text_bracket_match(Text *txt, size_t pos, const Filerange *limits) {
	return text_bracket_match_symbol(txt, pos, NULL, limits);
}

static size_t match_symbol(Text *txt, size_t pos, char search, int direction, const Filerange *limits) {
	char c, current;
	int count = 1;
	bool instring = false;
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &current))
		return pos;
	if (direction >= 0) { /* forward search */
		while (text_iterator_byte_next(&it, &c)) {
			if (limits && it.pos >= limits->end)
				break;
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
			if (limits && it.pos < limits->start)
				break;
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

size_t text_bracket_match_symbol(Text *txt, size_t pos, const char *symbols, const Filerange *limits) {
	int direction;
	char search, current, c;
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
	case '\'':
	{
		/* prefer matches on the same line */
		size_t fw = match_symbol(txt, pos, current, +1, limits);
		size_t bw = match_symbol(txt, pos, current, -1, limits);
		if (fw == pos)
			return bw;
		if (bw == pos)
			return fw;
		size_t line = text_lineno_by_pos(txt, pos);
		size_t line_fw = text_lineno_by_pos(txt, fw);
		size_t line_bw = text_lineno_by_pos(txt, bw);
		if (line != line_fw)
			return bw;
		if (line != line_bw)
			return fw;
		direction = +1;
		if (text_iterator_byte_next(&it, &c)) {
			/* if a single or double quote is followed by
			 * a special character, search backwards */
			char special[] = " \t\n)}]>.,:;";
			if (memchr(special, c, sizeof(special)))
				direction = -1;
		}
		return direction >= 0 ? fw : bw;
	}
	default:
		return pos;
	}

	return match_symbol(txt, pos, search, direction, limits);
}

size_t text_search_forward(Text *txt, size_t pos, Regex *regex) {
	size_t start = pos + 1;
	size_t end = text_size(txt);
	RegexMatch match[1];
	char c;
	int flags = text_byte_get(txt, pos, &c) && c == '\n' ? 0 : REG_NOTBOL;
	bool found = start < end && !text_search_range_forward(txt, start, end - start, regex, 1, match, flags);

	if (!found) {
		start = 0;
		found = !text_search_range_forward(txt, start, end - start, regex, 1, match, 0);
	}

	return found ? match[0].start : pos;
}

size_t text_search_backward(Text *txt, size_t pos, Regex *regex) {
	size_t start = 0;
	size_t end = pos;
	RegexMatch match[1];
	bool found = !text_search_range_backward(txt, start, end, regex, 1, match, REG_NOTEOL);

	if (!found) {
		end = text_size(txt);
		found = !text_search_range_backward(txt, start, end - start, regex, 1, match, 0);
	}

	return found ? match[0].start : pos;
}
