#include <stdbool.h>

typedef struct Editor Editor;
typedef struct Piece Piece;

typedef struct {
	const char const *text;
	/* const */ size_t len;
	const Piece const *piece;
} Iterator;

typedef bool (*iterator_callback_t)(void *, size_t pos, const char *content, size_t len);

Editor *editor_load(const char *file);
bool editor_insert(Editor*, size_t pos, char *c);
bool editor_delete(Editor*, size_t pos, size_t len);
bool editor_replace(Editor*, size_t pos, char *c);
void editor_snapshot(Editor*);
bool editor_undo(Editor*);
bool editor_redo(Editor*);
Iterator editor_iterator_get(Editor*, size_t pos);
bool editor_iterator_valid(const Iterator*);
void editor_iterator_next(Iterator*);
void editor_iterator_prev(Iterator*);
void editor_iterate(Editor*, void *, size_t pos, iterator_callback_t);
bool editor_modified(Editor*);
int editor_save(Editor*, const char *file);
void editor_free(Editor *ed);

// TMP
void editor_debug(Editor *ed);
