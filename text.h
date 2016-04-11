#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#define EPOS ((size_t)-1)         /* invalid position */

typedef size_t Filepos;

typedef struct {
	size_t start, end;        /* range in bytes from start of the file */
} Filerange;

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

/* create a text instance populated with the given file content, if `filename'
 * is NULL the text starts out empty */
Text *text_load(const char *filename);
/* file information at time of load or last save */
struct stat text_stat(Text*);
bool text_appendf(Text*, const char *format, ...);
bool text_printf(Text*, size_t pos, const char *format, ...);
bool text_vprintf(Text*, size_t pos, const char *format, va_list ap);
/* inserts a line ending character (depending on file type) */
size_t text_insert_newline(Text*, size_t pos);
/* insert `len' bytes starting from `data' at `pos' which has to be
 * in the interval [0, text_size(txt)] */
bool text_insert(Text*, size_t pos, const char *data, size_t len);
/* delete `len' bytes starting from `pos' */
bool text_delete(Text*, size_t pos, size_t len);
bool text_delete_range(Text*, Filerange*);
/* mark the current text state, such that it can be {un,re}done */
void text_snapshot(Text*);
/* undo/redo to the last snapshotted state. returns the position where
 * the change occured or EPOS if nothing could be {un,re}done. */
size_t text_undo(Text*);
size_t text_redo(Text*);
/* move chronlogically to the `count' earlier/later revision */
size_t text_earlier(Text*, int count);
size_t text_later(Text*, int count);
/* restore the text to the state closest to the time given */
size_t text_restore(Text*, time_t);
/* get creation time of current state */
time_t text_state(Text*);

size_t text_pos_by_lineno(Text*, size_t lineno);
size_t text_lineno_by_pos(Text*, size_t pos);

/* set `buf' to the byte found at `pos' and return true, if `pos' is invalid
 * false is returned and `buf' is left unmodified */
bool text_byte_get(Text*, size_t pos, char *buf);
/* store at most `len' bytes starting from `pos' into `buf', the return value
 * indicates how many bytes were copied into `buf'. WARNING buf will not be
 * NUL terminated. */
size_t text_bytes_get(Text*, size_t pos, size_t len, char *buf);
/* allocate a NUL terminated buffer and fill at most `len' bytes
 * starting at `pos'. Freeing is the caller's responsibility! */
char *text_bytes_alloc0(Text*, size_t pos, size_t len);

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
/* move to the next/previous UTF-8 encoded Unicode codepoint
 * and set c (if it is non NULL) to the first byte */
bool text_iterator_codepoint_next(Iterator *it, char *c);
bool text_iterator_codepoint_prev(Iterator *it, char *c);
/* move to next/previous grapheme i.e. might skip over multiple
 * Unicode codepoints (e.g. for combining characters) */
bool text_iterator_char_next(Iterator*, char *c);
bool text_iterator_char_prev(Iterator*, char *c);

typedef const char* Mark;
/* mark position `pos', the returned mark can be used to later retrieve
 * the same text segment */
Mark text_mark_set(Text*, size_t pos);
/* get position of mark in bytes from start of the file or EPOS if
 * the mark is not/no longer valid e.g. if the corresponding text was
 * deleted. If the change is later restored the mark will once again be
 * valid. */
size_t text_mark_get(Text*, Mark);

/* get position of change denoted by index, where 0 indicates the most recent */
size_t text_history_get(Text*, size_t index);
/* return the size in bytes of the whole text */
size_t text_size(Text*);
/* query whether the text contains any unsaved modifications */
bool text_modified(Text*);
/* query whether `addr` is part of a memory mapped region associated with
 * this text instance */
bool text_sigbus(Text*, const char *addr);

/* which type of new lines does the text use? */
enum TextNewLine {
	TEXT_NEWLINE_NL = 1,
	TEXT_NEWLINE_CRNL,
};

enum TextNewLine text_newline_type(Text*);
const char *text_newline_char(Text*);

/* save the whole text to the given `filename'. Return true if succesful.
 * In which case an implicit snapshot is taken. The save might associate a
 * new inode to file. */
bool text_save(Text*, const char *filename);
bool text_save_range(Text*, Filerange*, const char *file);
/* write the text content to the given file descriptor `fd'. Return the
 * number of bytes written or -1 in case there was an error. */
ssize_t text_write(Text*, int fd);
ssize_t text_write_range(Text*, Filerange*, int fd);
/* release all ressources associated with this text instance */
void text_free(Text*);

#endif
