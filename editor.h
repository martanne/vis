#include <stdbool.h>

typedef struct Editor Editor;
typedef struct Piece Piece;

typedef struct {
	const char const *start;  /* begin of piece's data */
	const char const *end;    /* pointer to the first byte after valid data i.e. [start, end) */
	const char const *text;   /* current position within piece: start <= text < end */
	const Piece const *piece; /* internal state do not touch! */
	size_t pos;               /* global position in bytes from start of file */
} Iterator;

#define editor_iterate(ed, it, pos) \
	for (Iterator it = editor_iterator_get((ed), (pos)); \
	     editor_iterator_valid(&it); \
	     editor_iterator_next(&it))

Editor *editor_load(const char *file);
const char *editor_filename(Editor*);
bool editor_insert(Editor*, size_t pos, const char *data);
bool editor_insert_raw(Editor*, size_t pos, const char *data, size_t len);
bool editor_delete(Editor*, size_t pos, size_t len);
bool editor_replace(Editor*, size_t pos, const char *data);
bool editor_replace_raw(Editor*, size_t pos, const char *data, size_t len);
void editor_snapshot(Editor*);
bool editor_undo(Editor*);
bool editor_redo(Editor*);

size_t editor_pos_by_lineno(Editor*, size_t lineno);
size_t editor_lineno_by_pos(Editor*, size_t pos);

size_t editor_bytes_get(Editor*, size_t pos, size_t len, char *buf);

Iterator editor_iterator_get(Editor*, size_t pos);
bool editor_iterator_valid(const Iterator*);
bool editor_iterator_next(Iterator*);
bool editor_iterator_prev(Iterator*);

bool editor_iterator_byte_get(Iterator *it, char *b);
bool editor_iterator_byte_next(Iterator*, char *b);
bool editor_iterator_byte_prev(Iterator*, char *b);

bool editor_iterator_char_next(Iterator *it, char *c);
bool editor_iterator_char_prev(Iterator *it, char *c);

typedef int Mark;
void editor_mark_set(Editor*, Mark, size_t pos);
size_t editor_mark_get(Editor*, Mark);
void editor_mark_clear(Editor*, Mark);
void editor_mark_clear_all(Editor*);

size_t editor_size(Editor*);
bool editor_modified(Editor*);
int editor_save(Editor*, const char *file);
void editor_free(Editor *ed);

typedef struct Regex Regex;

typedef struct {
	size_t start; /* start of match in bytes from start of file or -1 if unused */
	size_t end;   /* end of match in bytes from start of file or -1 if unused */
} RegexMatch;

Regex *editor_regex_new(void);
int editor_regex_compile(Regex *r, const char *regex, int cflags);
void editor_regex_free(Regex *r);
int editor_search_forward(Editor*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);
int editor_search_backward(Editor*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);

// TMP
void editor_debug(Editor *ed);
