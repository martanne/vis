#ifndef SYNTAX_H
#define SYNTAX_H

typedef struct {
	char *symbol;
	int style;
} SyntaxSymbol;

enum {
	SYNTAX_SYMBOL_SPACE,
	SYNTAX_SYMBOL_TAB,
	SYNTAX_SYMBOL_TAB_FILL,
	SYNTAX_SYMBOL_EOL,
	SYNTAX_SYMBOL_EOF,
	SYNTAX_SYMBOL_LAST,
};

#endif
