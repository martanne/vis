#include <stdbool.h>

typedef struct Editor Editor;

typedef enum {
	CONTINUE,
	BREAK,
} Iterate;

typedef Iterate (*iterator_callback_t)(void *, size_t pos, const char *content, size_t len);


bool editor_insert(Editor*, size_t pos, char *c);
void editor_delete(Editor*, size_t start, size_t end);
bool editor_undo(Editor*);
bool editor_redo(Editor*);
void editor_snapshot(Editor*);
int editor_save(Editor*, const char *file);
Editor *editor_load(const char *file);
char *editor_get(Editor*, size_t pos, size_t len);
void editor_iterate(Editor *ed, void *, size_t pos, iterator_callback_t);


// TMP
void editor_debug(Editor *ed);
