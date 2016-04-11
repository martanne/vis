#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "text-motions.h"
#include "text-objects.h"
#include "text-util.h"
#include "util.h"

#define space(c) (isspace((unsigned char)c))
#define boundary(c) (isboundary((unsigned char)c))

Filerange text_object_entire(Text *txt, size_t pos) {
	return text_range_new(0, text_size(txt));
}

Filerange text_object_entire_inner(Text *txt, size_t pos) {
	char c;
	Filerange r = text_object_entire(txt, pos);
	Iterator it = text_iterator_get(txt, r.start);
	while (text_iterator_byte_get(&it, &c) && (c == '\r' || c == '\n'))
		text_iterator_byte_next(&it, NULL);
	r.start = it.pos;
	it = text_iterator_get(txt, r.end);
	while (text_iterator_byte_prev(&it, &c) && (c == '\r' || c == '\n'));
	r.end = it.pos;
	return text_range_linewise(txt, &r);
}

static Filerange text_object_customword(Text *txt, size_t pos, int (*isboundary)(int)) {
	Filerange r;
	char c, prev = '0', next = '0';
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return text_range_empty();
	if (text_iterator_byte_prev(&it, &prev))
		text_iterator_byte_next(&it, NULL);
	text_iterator_byte_next(&it, &next);
	if (space(c)) {
		r.start = text_char_next(txt, text_customword_end_prev(txt, pos, isboundary));
		r.end = text_customword_start_next(txt, pos, isboundary);
	} else if (boundary(prev) && boundary(next)) {
		if (boundary(c)) {
			r.start = text_char_next(txt, text_customword_end_prev(txt, pos, isboundary));
			r.end = text_char_next(txt, text_customword_end_next(txt, pos, isboundary));
		} else {
			/* on a single character */
			r.start = pos;
			r.end = text_char_next(txt, pos);
		}
	} else if (boundary(prev)) {
		/* at start of a word */
		r.start = pos;
		r.end = text_char_next(txt, text_customword_end_next(txt, pos, isboundary));
	} else if (boundary(next)) {
		/* at end of a word */
		r.start = text_customword_start_prev(txt, pos, isboundary);
		r.end = text_char_next(txt, pos);
	} else {
		/* in the middle of a word */
		r.start = text_customword_start_prev(txt, pos, isboundary);
		r.end = text_char_next(txt, text_customword_end_next(txt, pos, isboundary));
	}

	return r;
}

Filerange text_object_word(Text *txt, size_t pos) {
	return text_object_customword(txt, pos, is_word_boundary);
}

Filerange text_object_longword(Text *txt, size_t pos) {
	return text_object_customword(txt, pos, isspace);
}

static Filerange text_object_customword_outer(Text *txt, size_t pos, int (*isboundary)(int)) {
	Filerange r;
	char c, prev = '0', next = '0';
	Iterator it = text_iterator_get(txt, pos);
	if (!text_iterator_byte_get(&it, &c))
		return text_range_empty();
	if (text_iterator_byte_prev(&it, &prev))
		text_iterator_byte_next(&it, NULL);
	text_iterator_byte_next(&it, &next);
	if (space(c)) {
		/* middle of two words, include leading white space */
		r.start = text_char_next(txt, text_customword_end_prev(txt, pos, isboundary));
		r.end = text_char_next(txt, text_customword_end_next(txt, pos, isboundary));
	} else if (boundary(prev) && boundary(next)) {
		if (boundary(c)) {
			r.start = text_char_next(txt, text_customword_end_prev(txt, pos, isboundary));
			r.end = text_word_start_next(txt, text_customword_end_next(txt, pos, isboundary));
		} else {
			/* on a single character */
			r.start = pos;
			r.end = text_customword_start_next(txt, pos, isboundary);
		}
	} else if (boundary(prev)) {
		/* at start of a word */
		r.start = pos;
		r.end = text_customword_start_next(txt, text_customword_end_next(txt, pos, isboundary), isboundary);
	} else if (boundary(next)) {
		/* at end of a word */
		r.start = text_customword_start_prev(txt, pos, isboundary);
		r.end = text_customword_start_next(txt, pos, isboundary);
	} else {
		/* in the middle of a word */
		r.start = text_customword_start_prev(txt, pos, isboundary);
		r.end = text_customword_start_next(txt, text_customword_end_next(txt, pos, isboundary), isboundary);
	}

	return r;
}

Filerange text_object_longword_outer(Text *txt, size_t pos) {
	return text_object_customword_outer(txt, pos, isspace);
}

Filerange text_object_word_outer(Text *txt, size_t pos) {
	return text_object_customword_outer(txt, pos, is_word_boundary);
}

Filerange text_object_word_find_next(Text *txt, size_t pos, const char *word) {
	size_t len = strlen(word);
	for (;;) {
		size_t match_pos = text_find_next(txt, pos, word);
		if (match_pos != pos) {
			Filerange match_word = text_object_word(txt, match_pos);
			if (text_range_size(&match_word) == len)
				return match_word;
			pos = match_word.end;
		} else {
			return text_range_empty();
		}
	}
}

Filerange text_object_word_find_prev(Text *txt, size_t pos, const char *word) {
	size_t len = strlen(word);
	for (;;) {
		size_t match_pos = text_find_prev(txt, pos, word);
		if (match_pos != pos) {
			Filerange match_word = text_object_word(txt, match_pos);
			if (text_range_size(&match_word) == len)
				return match_word;
			pos = match_pos;
		} else {
			return text_range_empty();
		}
	}
}

Filerange text_object_line(Text *txt, size_t pos) {
	Filerange r;
	r.start = text_line_begin(txt, pos);
	r.end = text_line_next(txt, pos);
	return r;
}

Filerange text_object_line_inner(Text *txt, size_t pos) {
	Filerange r = text_object_line(txt, pos);
	return text_range_inner(txt, &r);
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

static Filerange object_function(Text *txt, size_t pos) {
	size_t start_prev = text_function_start_prev(txt, pos);
	size_t end_next = text_function_end_next(txt, pos);
	size_t start = text_function_start_next(txt, start_prev);
	size_t end = text_function_end_prev(txt, end_next);
	if (start == pos)
		start_prev = pos;
	if (end == pos)
		end_next = pos;
	if (text_function_end_next(txt, start_prev) == end_next &&
	    text_function_start_prev(txt, end_next) == start_prev) {
		return text_range_new(start_prev, end_next);
	}
	return text_range_empty();
}

Filerange text_object_function(Text *txt, size_t pos) {
	Filerange r = object_function(txt, pos);
	if (!text_range_valid(&r))
		return r;
	r.end++;
	return text_range_linewise(txt, &r);
}

Filerange text_object_function_inner(Text *txt, size_t pos) {
	Filerange r = object_function(txt, pos);
	if (!text_range_valid(&r))
		return r;
	return text_range_new(text_bracket_match(txt, r.end)+1, r.end);
}

static Filerange text_object_bracket(Text *txt, size_t pos, char type) {
	char c, open, close;
	int opened = 1, closed = 1;
	Filerange r = text_range_empty();

	switch (type) {
	case '(':  case ')': open = '(';  close = ')';  break;
	case '{':  case '}': open = '{';  close = '}';  break;
	case '[':  case ']': open = '[';  close = ']';  break;
	case '<':  case '>': open = '<';  close = '>';  break;
	case '"':            open = '"';  close = '"';  break;
	case '`':            open = '`';  close = '`';  break;
	case '\'':           open = '\''; close = '\''; break;
	default: return r;
	}

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

	if (!text_range_valid(&r))
		return text_range_empty();
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

Filerange text_object_range(Text *txt, size_t pos, int (*isboundary)(int)) {
	char c;
	size_t start;
	Iterator it = text_iterator_get(txt, pos), rit = it;
	if (!text_iterator_byte_get(&rit, &c) || boundary(c))
		return text_range_empty();
	char tmp = c;
	do start = rit.pos; while (text_iterator_char_prev(&rit, &c) && !boundary(c));
	for (c = tmp; !boundary(c) && text_iterator_byte_next(&it, &c););
	return text_range_new(start, it.pos);
}

static int is_number(int c) {
	return !(c == '-' || c == 'x' || c == 'X' ||
	         ('0' <= c && c <= '9') ||
	         ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'));
}

Filerange text_object_number(Text *txt, size_t pos) {
	char *buf, *err = NULL;
	Filerange r = text_object_range(txt, pos, is_number);
	if (!text_range_valid(&r))
		return r;
	if (!(buf = text_bytes_alloc0(txt, r.start, text_range_size(&r))))
		return text_range_empty();
	errno = 0;
	strtoll(buf, &err, 0);
	if (errno || err == buf)
		r = text_range_empty();
	else
		r.end = r.start + (err - buf);
	free(buf);
	return r;
}

static int is_filename_boundary(int c) {
	switch (c) {
	case ';': case ':': case '|':
	case '"': case '\'':
	case '<': case '>':
		return true;
	default:
		return isspace(c);
	}
}

Filerange text_object_filename(Text *txt, size_t pos) {
	return text_object_range(txt, pos, is_filename_boundary);
}

Filerange text_object_search_forward(Text *txt, size_t pos, Regex *regex) {
	size_t start = pos;
	size_t end = text_size(txt);
	RegexMatch match[1];
	bool found = start < end && !text_search_range_forward(txt, start, end - start, regex, 1, match, 0);
	if (found)
		return text_range_new(match[0].start, match[0].end);
	return text_range_empty();
}

Filerange text_object_search_backward(Text *txt, size_t pos, Regex *regex) {
	size_t start = 0;
	size_t end = pos;
	RegexMatch match[1];
	bool found = !text_search_range_backward(txt, start, end, regex, 1, match, 0);
	if (found)
		return text_range_new(match[0].start, match[0].end);
	return text_range_empty();
}

Filerange text_object_indentation(Text *txt, size_t pos) {
	char c;
	size_t bol = text_line_begin(txt, pos);
	size_t sol = text_line_start(txt, bol);
	size_t start = bol;
	size_t end = text_line_next(txt, bol);
	size_t line_indent = sol - bol;
	bool line_empty = text_byte_get(txt, bol, &c) && (c == '\r' || c == '\n');

	char *buf = text_bytes_alloc0(txt, bol, line_indent);
	char *tmp = malloc(line_indent);

	if (!buf || !tmp) {
		free(buf);
		free(tmp);
		return text_range_empty();
	}

	while ((bol = text_line_begin(txt, text_line_prev(txt, start))) != start) {
		sol = text_line_start(txt, bol);
		size_t indent = sol - bol;
		if (indent < line_indent)
			break;
		bool empty = text_byte_get(txt, bol, &c) && (c == '\r' || c == '\n');
		if (line_empty && !empty)
			break;
		if (line_indent == 0 && empty)
			break;
		text_bytes_get(txt, bol, line_indent, tmp);
		if (memcmp(buf, tmp, line_indent))
			break;
		start = bol;
	}

	do {
		bol = end;
		sol = text_line_start(txt, bol);
		size_t indent = sol - bol;
		if (indent < line_indent)
			break;
		bool empty = text_byte_get(txt, bol, &c) && (c == '\r' || c == '\n');
		if (line_empty && !empty)
			break;
		if (line_indent == 0 && empty)
			break;
		text_bytes_get(txt, bol, line_indent, tmp);
		if (memcmp(buf, tmp, line_indent))
			break;
		end = text_line_next(txt, bol);
	} while (bol != end);

	free(buf);
	free(tmp);
	return text_range_new(start, end);
}

Filerange text_range_linewise(Text *txt, Filerange *rin) {
	Filerange rout = *rin;
	rout.start = text_line_begin(txt, rin->start);
	if (rin->end != text_line_begin(txt, rin->end))
		rout.end = text_line_next(txt, rin->end);
	return rout;
}

bool text_range_is_linewise(Text *txt, Filerange *r) {
	return text_range_valid(r) &&
	       r->start == text_line_begin(txt, r->start) &&
	       r->end == text_line_begin(txt, r->end);
}

Filerange text_range_inner(Text *txt, Filerange *rin) {
	char c;
	Filerange r = *rin;
	Iterator it = text_iterator_get(txt, rin->start);
	while (text_iterator_byte_get(&it, &c) && space(c))
		text_iterator_byte_next(&it, NULL);
	r.start = it.pos;
	it = text_iterator_get(txt, rin->end);
	do r.end = it.pos; while (text_iterator_byte_prev(&it, &c) && space(c));
	return r;
}
