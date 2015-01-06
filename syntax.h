#ifndef SYNTAX_H
#define SYNTAX_H

#include <regex.h>

typedef struct {
	short fg, bg;   /* fore and background color */
	int attr;       /* curses attributes */
} Color;

typedef struct {
	char *rule;     /* regex to search for */
	Color *color;   /* settings to apply in case of a match */
	bool multiline; /* whether . should match new lines */
	regex_t regex;  /* compiled form of the above rule */
} SyntaxRule;

typedef struct Syntax Syntax;
struct Syntax {               /* a syntax definition */
	char *name;           /* syntax name */
	char *file;           /* apply to files matching this regex */
	regex_t file_regex;   /* compiled file name regex */
	const char **settings;/* settings associated with this file type */
	SyntaxRule rules[24]; /* all rules for this file type */
};

#endif
