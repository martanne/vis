#include <stdbool.h>

typedef struct Editor Editor;
typedef struct Piece Piece;

typedef struct {
	const char const *start;
	const char const *end;
	const char const *text;
	const Piece const *piece;
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
bool editor_iterator_valid(const Iterator*);
Iterator editor_iterator_get(Editor*, size_t pos);
bool editor_iterator_next(Iterator*);
bool editor_iterator_prev(Iterator*);
Iterator editor_iterator_byte_get(Editor*, size_t pos, char *byte);
bool editor_iterator_byte_next(Iterator*, char *b);
bool editor_iterator_byte_prev(Iterator*, char *b);
bool editor_iterator_byte_peek(Iterator *it, char *b);

size_t editor_size(Editor*);
bool editor_modified(Editor*);
int editor_save(Editor*, const char *file);
void editor_free(Editor *ed);

// TMP
void editor_debug(Editor *ed);
