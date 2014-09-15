#ifndef SYNTAX_H
#define SYNTAX_H

#include <regex.h>

#define SYNTAX_RULES 10 /* maximal number of syntax rules per file type */

typedef struct {
	short fg, bg;   /* fore and background color */
	int attr;       /* curses attributes */
} Color;

typedef struct {
	char *rule;     /* regex to search for */
	int cflags;     /* compilation flags (REG_*) used when compiling */
	Color *color;   /* settings to apply in case of a match */
	regex_t regex;  /* compiled form of the above rule */
} SyntaxRule;

typedef struct Syntax Syntax;
struct Syntax {                         /* a syntax definition */
	char *name;                     /* syntax name */
	char *file;                     /* apply to files matching this regex */
	regex_t file_regex;             /* compiled file name regex */
	SyntaxRule rules[SYNTAX_RULES]; /* all rules for this file type */
};

#endif
