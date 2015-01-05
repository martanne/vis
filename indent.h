#ifndef INDENT_H
#define INDENT_H

#include <stdbool.h>
#include <stddef.h>
#include "text.h"

/* delete indentation of line, return previous line start. */
size_t delete_indent(Text *text, size_t line_begin);

/* indent by copying the indentation of the previous line. */
size_t autoindent(Text *text, size_t line_begin, bool newline);
/* indentation heuristic for the C language. */
size_t cindent(Text *text, size_t line_begin, bool newline);

#endif
