#ifndef EDITOR_H
#define EDITOR_H

#include <curses.h>
#include <regex.h>
#include "text.h"

typedef struct {
	short fg, bg;   /* fore and background color */
	int attr;       /* curses attributes */
} Color;

typedef struct {
	char *rule;     /* regex to search for */
	int cflags;     /* compilation flags (REG_*) used when compiling */
	Color color;    /* settings to apply in case of a match */
	regex_t regex;  /* compiled form of the above rule */
} SyntaxRule;

#define SYNTAX_REGEX_RULES 10

typedef struct {                              /* a syntax definition */
	char *name;                           /* syntax name */
	char *file;                           /* apply to files matching this regex */
	regex_t file_regex;                   /* compiled file name regex */
	SyntaxRule rules[SYNTAX_REGEX_RULES]; /* all rules for this file type */
} Syntax;

typedef struct Editor Editor;
typedef size_t Filepos;

typedef void (*editor_statusbar_t)(WINDOW *win, bool active, const char *filename, int line, int col);

/* initialize a new editor with the available screen size */
Editor *editor_new(int width, int height, editor_statusbar_t);
/* loads the given file and displays it in a new window. passing NULL as 
 * filename will open a new empty buffer */
bool editor_load(Editor*, const char *filename);
size_t editor_insert(Editor*, const char *c, size_t len);
size_t editor_replace(Editor *ed, const char *c, size_t len);
size_t editor_backspace(Editor*);
size_t editor_delete(Editor*);
bool editor_resize(Editor*, int width, int height);
void editor_snapshot(Editor*);
void editor_undo(Editor*);
void editor_redo(Editor*);
void editor_draw(Editor *editor);
/* flush all changes made to the ncurses windows to the screen */
void editor_update(Editor *editor);
void editor_free(Editor*);

/* cursor movements, also updates selection if one is active, returns new cursor postion */
size_t editor_file_begin(Editor *ed);
size_t editor_file_end(Editor *ed);
size_t editor_page_down(Editor *ed);
size_t editor_page_up(Editor *ed);
size_t editor_char_next(Editor*);
size_t editor_char_prev(Editor*);
size_t editor_line_goto(Editor*, size_t lineno);
size_t editor_line_down(Editor*);
size_t editor_line_up(Editor*);
size_t editor_line_begin(Editor*);
size_t editor_line_start(Editor*);
size_t editor_line_finish(Editor*);
size_t editor_line_end(Editor*);
size_t editor_word_end_next(Editor*);
size_t editor_word_end_prev(Editor *ed);
size_t editor_word_start_next(Editor*);
size_t editor_word_start_prev(Editor *ed);
size_t editor_sentence_next(Editor *ed);
size_t editor_sentence_prev(Editor *ed);
size_t editor_paragraph_next(Editor *ed);
size_t editor_paragraph_prev(Editor *ed);
size_t editor_bracket_match(Editor *ed);

// mark handling
typedef int Mark;
void editor_mark_set(Editor*, Mark, size_t pos);
void editor_mark_goto(Editor*, Mark);
void editor_mark_clear(Editor*, Mark);

// TODO comment
bool editor_syntax_load(Editor*, Syntax *syntaxes, Color *colors);
void editor_syntax_unload(Editor*);

size_t editor_cursor_get(Editor*);
void editor_scroll_to(Editor*, size_t pos);
void editor_cursor_to(Editor*, size_t pos);
Text *editor_text_get(Editor*);
// TODO void return type?
size_t editor_selection_start(Editor*);
size_t editor_selection_end(Editor*);
Filerange editor_selection_get(Editor*);
void editor_selection_clear(Editor*);

void editor_search(Editor *ed, const char *regex, int direction);

void editor_window_split(Editor *ed, const char *filename);
void editor_window_vsplit(Editor *ed, const char *filename);
void editor_window_next(Editor *ed);
void editor_window_prev(Editor *ed);

/* library initialization code, should be run at startup */
void editor_init(void);
// TODO: comment
short editor_color_reserve(short fg, short bg);
short editor_color_get(short fg, short bg);

#endif
