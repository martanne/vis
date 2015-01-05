#include <stdlib.h>
#include "text-motions.h"
#include "text.h"

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
