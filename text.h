#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
	size_t start, end; /* range in bytes from start of file */
} Filerange;

typedef struct Text Text;
typedef struct Piece Piece;

typedef struct {
	const char const *start;  /* begin of piece's data */
	const char const *end;    /* pointer to the first byte after valid data i.e. [start, end) */
	const char const *text;   /* current position within piece: start <= text < end */
	const Piece const *piece; /* internal state do not touch! */
	size_t pos;               /* global position in bytes from start of file */
} Iterator;

#define text_iterate(ed, it, pos) \
	for (Iterator it = text_iterator_get((ed), (pos)); \
	     text_iterator_valid(&it); \
	     text_iterator_next(&it))

Text *text_load(const char *file);
const char *text_filename(Text*);
bool text_insert(Text*, size_t pos, const char *data);
bool text_insert_raw(Text*, size_t pos, const char *data, size_t len);
bool text_delete(Text*, size_t pos, size_t len);
void text_snapshot(Text*);
bool text_undo(Text*);
bool text_redo(Text*);

size_t text_pos_by_lineno(Text*, size_t lineno);
size_t text_lineno_by_pos(Text*, size_t pos);

bool text_byte_get(Text *ed, size_t pos, char *buf);
size_t text_bytes_get(Text*, size_t pos, size_t len, char *buf);

Iterator text_iterator_get(Text*, size_t pos);
bool text_iterator_valid(const Iterator*);
bool text_iterator_next(Iterator*);
bool text_iterator_prev(Iterator*);

bool text_iterator_byte_get(Iterator *it, char *b);
bool text_iterator_byte_next(Iterator*, char *b);
bool text_iterator_byte_prev(Iterator*, char *b);

bool text_iterator_char_next(Iterator *it, char *c);
bool text_iterator_char_prev(Iterator *it, char *c);

typedef int Mark;
void text_mark_set(Text*, Mark, size_t pos);
size_t text_mark_get(Text*, Mark);
void text_mark_clear(Text*, Mark);
void text_mark_clear_all(Text*);

size_t text_size(Text*);
bool text_modified(Text*);
int text_save(Text*, const char *file);
void text_free(Text*);

typedef struct Regex Regex;

typedef struct {
	size_t start; /* start of match in bytes from start of file or -1 if unused */
	size_t end;   /* end of match in bytes from start of file or -1 if unused */
} RegexMatch;

Regex *text_regex_new(void);
int text_regex_compile(Regex *r, const char *regex, int cflags);
void text_regex_free(Regex *r);
int text_search_forward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);
int text_search_backward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);

// TMP
void text_debug(Text *ed);

#endif
