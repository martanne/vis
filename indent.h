#ifndef INDENT_H
#define INDENT_H

/* delete indentation of line, return previous line start. */
size_t delete_indent(Text *text, size_t line_begin);

/* indent by copying the indentation of the previous line. */
size_t autoindent(Text *text, size_t line_begin, bool new_line);

#endif
