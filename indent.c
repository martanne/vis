#include <stdlib.h>
#include <ctype.h>
#include "text-motions.h"
#include "text.h"

#define SHIFT "\t"
#define STRINGLEN(x) sizeof(x)-1

size_t insert_copy_of_range(Text *text, size_t pos, Filerange *range) {
	size_t len = range->end - range->start;
	char *buf = malloc(len);
	if (!buf)
		return pos;
	len = text_bytes_get(text, range->start, len, buf);
	bool insert_res = text_insert(text, pos, buf, len);
	free(buf);
	if (!insert_res)
		return pos;
	return pos + len;
}

size_t copy_indent_from_line(Text *text, size_t pos, size_t line) {
	size_t line_begin = text_line_begin(text, line);
	size_t line_start = text_line_start(text, line_begin);
	Filerange indent_range = { line_begin, line_start };
	return insert_copy_of_range(text, pos, &indent_range);
}

size_t delete_indent(Text *text, size_t line_begin) {
	size_t line_start = text_line_start(text, line_begin);
	size_t len = line_start - line_begin;
	text_delete(text, line_begin, len);
	return line_start;
}

size_t autoindent(Text *text, size_t line_begin, bool new_line) {
	(void)new_line;
	size_t prev_line = text_line_prev(text, line_begin);
	return copy_indent_from_line(text, line_begin, prev_line);
}

/* extraordinary lines like preprocessor directives or empty lines get no
 * indentation */
static bool line_is_extraordinary(Text *text, size_t line) {
	size_t start = text_line_start(text, line);
	char start_c;
	if (!text_byte_get(text, start, &start_c))
		return true;
	/* an empty line or one starting with '#' is extraordinary */
	return start_c == '#' || start_c == '\r' || start_c == '\n';
}

/* search previous line that is not extraordinary */
static size_t prev_normal_line(Text *text, size_t pos) {
	do {
		size_t new_pos = text_line_prev(text, pos);
		if (new_pos == pos)
			return pos;
		pos = new_pos;
	} while (line_is_extraordinary(text, pos));
	return pos;
}

static size_t comment_block_heuristic(Text *text, size_t pos, bool newline) {
	/* comment block heuristic: if previous line begins with '/''*' or "* "
	 * then start line with "* " */
	size_t prev_line = text_line_prev(text, pos);
	size_t prev_start = text_line_start(text, prev_line);
	char prev_start_c;
	char next_c;
	if (!text_byte_get(text, prev_start, &prev_start_c)
	    || (prev_start_c != '*' && prev_start_c != '/')
	    || !text_byte_get(text, prev_start+1, &next_c))
		return EPOS;

	size_t prev_last = text_line_lastchar(text, prev_start);
	char prev_last_c;
	char prev_c;
	/* exception: previous line ends with '*''/' */
	bool comment_ends = text_byte_get(text, prev_last, &prev_last_c)
	                    && prev_last_c == '/'
	                    && text_byte_get(text, prev_last-1, &prev_c)
	                    && prev_c == '*'
	                    && (prev_start_c != '/' || prev_last != prev_start+2);

	if (prev_start_c == '/' && next_c == '*' && !comment_ends) {
		pos = copy_indent_from_line(text, pos, prev_line);
		if (text_insert(text, pos, " ", 1))
			pos++;
		if (newline && text_insert(text, pos, "* ", 2))
			pos += 2;
		return pos;
	}
	if (prev_start_c == '*' && (isspace(next_c) || next_c == '/')) {
		if (comment_ends) {
			/* reduce previous indent by the last space */
			/* TODO: ensure it is actually a space and not a tab, etc. */
			size_t prev_begin = text_line_begin(text, prev_start);
			if (prev_start == prev_begin)
				return EPOS;

			Filerange range = { prev_begin, prev_start-1 };
			return insert_copy_of_range(text, pos, &range);
		} else {
			pos = copy_indent_from_line(text, pos, prev_line);
			if (newline && text_insert(text, pos, "* ", 2))
				pos += 2;
			return pos;
		}
	}
	return EPOS;
}

static bool is_label(Text *text, size_t pos) {
	size_t lastchar = text_line_lastchar(text, pos);
	char last_c;
	if (!text_byte_get(text, lastchar, &last_c))
		return false;
	/* decide whether it is a label. TODO: Would be more robust to check
	 * for identifier :/default : or case .*: at the beginning of the
	 * line. */
	return last_c == ':';
}

/* If line is a label, indent at the same level as the current block
 * opened (typically 1 indent less than normal statements) */
size_t label_heuristic(Text *text, size_t pos) {
	if (!is_label(text, pos))
		return EPOS;

	size_t block_begin = text_bracket_match_(text, pos, -1, '}', '{');
	if (block_begin == pos)
		return EPOS;
	return copy_indent_from_line(text, pos, block_begin);
}

size_t cindent(Text *text, size_t line_begin, bool newline) {
	size_t pos = line_begin;

	size_t start = text_line_start(text, pos);
	char start_c;
	if (text_byte_get(text, start, &start_c)) {
		/* if line begins with '}' copy indent from line with the
		 * opening brace. */
		if (start_c == '}') {
			size_t block_begin = text_bracket_match(text, start);
			if (block_begin != start_c)
				return copy_indent_from_line(text, pos, block_begin);
		}
	}

	/* if line is extraordinary, then reset indent to zero.
	 * newlines are not finished yet and should therefore not be considered
	 * like an empty line here. */
	if (!newline && line_is_extraordinary(text, pos))
		return pos;

	size_t comment_pos = comment_block_heuristic(text, pos, newline);
	if (comment_pos != EPOS)
		return comment_pos;

	size_t label_pos = label_heuristic(text, pos);
	if (label_pos != EPOS)
		return label_pos;

	/* if previous line ends in { or was a label, increase indent */
	size_t normal_line = prev_normal_line(text, pos);
	size_t lastchar = text_line_lastchar(text, normal_line);
	char last_c;
	if ((text_byte_get(text, lastchar, &last_c) && last_c == '{')
	    || is_label(text, normal_line)) {
		pos = copy_indent_from_line(text, pos, normal_line);
		if (text_insert(text, pos, SHIFT, STRINGLEN(SHIFT)))
			pos += STRINGLEN(SHIFT);
		return pos;
	}

	/* default: copy indent from previous (non-extraordinary) line */
	return copy_indent_from_line(text, pos, normal_line);
}
