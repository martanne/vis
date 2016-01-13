#ifndef TEXT_REGEX_H
#define TEXT_REGEX_H

#include "text.h"

typedef struct Regex Regex;
typedef Filerange RegexMatch;

Regex *text_regex_new(void);
int text_regex_compile(Regex *r, const char *regex, int cflags);
void text_regex_free(Regex *r);
int text_search_range_forward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);
int text_search_range_backward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);

#endif
