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
bool editor_insert(Editor*, size_t pos, const char *data);
bool editor_insert_raw(Editor*, size_t pos, const char *data, size_t len);
bool editor_delete(Editor*, size_t pos, size_t len);
bool editor_replace(Editor*, size_t pos, const char *data);
bool editor_replace_raw(Editor*, size_t pos, const char *data, size_t len);
void editor_snapshot(Editor*);
bool editor_undo(Editor*);
bool editor_redo(Editor*);

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

size_t editor_size(Editor*);
bool editor_modified(Editor*);
int editor_save(Editor*, const char *file);
void editor_free(Editor *ed);

// TMP
void editor_debug(Editor *ed);
