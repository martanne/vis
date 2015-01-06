#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <sys/types.h>

#define EPOS ((size_t)-1)         /* invalid position */

typedef size_t Filepos;

typedef struct {
	size_t start, end;        /* range in bytes from start of the file */
} Filerange;

bool text_range_valid(Filerange*);
size_t text_range_size(Filerange*);
Filerange text_range_empty(void);
Filerange text_range_union(Filerange*, Filerange*);

typedef struct Text Text;
typedef struct Piece Piece;

typedef struct {
	const char *start;  /* begin of piece's data */
	const char *end;    /* pointer to the first byte after valid data i.e. [start, end) */
	const char *text;   /* current position within piece: start <= text < end */
	const Piece *piece; /* internal state do not touch! */
	size_t pos;         /* global position in bytes from start of file */
} Iterator;

#define text_iterate(txt, it, pos) \
	for (Iterator it = text_iterator_get((txt), (pos)); \
	     text_iterator_valid(&it); \
	     text_iterator_next(&it))

Text *text_load(const char *file);
Text *text_load_fd(int fd);
/* return the fd from which this text was loaded or -1 if it was
 * loaded from a filename */
int text_fd_get(Text*);
/* the filename from which this text was loaded or first saved to */
const char *text_filename_get(Text*);
/* associate a filename with the yet unnamed buffer */
void text_filename_set(Text*, const char *filename);
bool text_insert(Text*, size_t pos, const char *data, size_t len);
bool text_delete(Text*, size_t pos, size_t len);
void text_snapshot(Text*);
/* undo/redos to the last snapshoted state. returns the position where
 * the change occured or EPOS if nothing could be undo/redo. */
size_t text_undo(Text*);
size_t text_redo(Text*);

size_t text_pos_by_lineno(Text*, size_t lineno);
size_t text_lineno_by_pos(Text*, size_t pos);

bool text_byte_get(Text*, size_t pos, char *buf);
size_t text_bytes_get(Text*, size_t pos, size_t len, char *buf);

Iterator text_iterator_get(Text*, size_t pos);
bool text_iterator_valid(const Iterator*);
bool text_iterator_next(Iterator*);
bool text_iterator_prev(Iterator*);

/* get byte at current iterator position, if this is at EOF a NUL
 * byte (which is not actually part of the file) is read. */
bool text_iterator_byte_get(Iterator*, char *b);
/* advance iterator by one byte and get byte at new position. */
bool text_iterator_byte_prev(Iterator*, char *b);
/* if the new position is at EOF a NUL byte (which is not actually
 * part of the file) is read. */
bool text_iterator_byte_next(Iterator*, char *b);

bool text_iterator_char_next(Iterator*, char *c);
bool text_iterator_char_prev(Iterator*, char *c);

typedef const char* Mark;
Mark text_mark_set(Text*, size_t pos);
size_t text_mark_get(Text*, Mark);

typedef int MarkIntern;
void text_mark_intern_set(Text*, MarkIntern, size_t pos);
size_t text_mark_intern_get(Text*, MarkIntern);
void text_mark_intern_clear(Text*, MarkIntern);
void text_mark_intern_clear_all(Text*);

/* get position of change denoted by index, where 0 indicates the most recent */
size_t text_history_get(Text*, size_t index);

size_t text_size(Text*);
bool text_modified(Text*);
/* test whether the underlying file uses UNIX style \n or Windows style \r\n newlines */
bool text_newlines_crnl(Text*);
bool text_save(Text*, const char *file);
bool text_range_save(Text*, Filerange*, const char *file);
ssize_t text_write(Text*, int fd);
ssize_t text_range_write(Text*, Filerange*, int fd);
void text_free(Text*);

typedef struct Regex Regex;

typedef struct {
	size_t start; /* start of match in bytes from start of file or -1 if unused */
	size_t end;   /* end of match in bytes from start of file or -1 if unused */
} RegexMatch;

Regex *text_regex_new(void);
int text_regex_compile(Regex *r, const char *regex, int cflags);
void text_regex_free(Regex *r);
int text_search_range_forward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);
int text_search_range_backward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);

// TMP
void text_debug(Text*);

#endif
