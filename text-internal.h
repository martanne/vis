#ifndef TEXT_INTERNAL
#define TEXT_INTERNAL

#include <stdbool.h>
#include <stddef.h>
#include "text.h"

/* Block holding the file content, either readonly mmap(2)-ed from the original
 * file or heap allocated to store the modifications.
 */
typedef struct {
	size_t size;               /* maximal capacity */
	size_t len;                /* current used length / insertion position */
	char *data;                /* actual data */
	enum {                     /* type of allocation */
		BLOCK_TYPE_MMAP_ORIG, /* mmap(2)-ed from an external file */
		BLOCK_TYPE_MMAP,      /* mmap(2)-ed from a temporary file only known to this process */
		BLOCK_TYPE_MALLOC,    /* heap allocated block using malloc(3) */
	} type;
} Block;

Block *block_alloc(size_t size);
Block *block_read(size_t size, int fd);
Block *block_mmap(size_t size, int fd, off_t offset);
Block *block_load(int dirfd, const char *filename, enum TextLoadMethod method, struct stat *info);
void block_free(Block*);
bool block_capacity(Block*, size_t len);
const char *block_append(Block*, const char *data, size_t len);
bool block_insert(Block*, size_t pos, const char *data, size_t len);
bool block_delete(Block*, size_t pos, size_t len);

Block *text_block_mmaped(Text*);
void text_saved(Text*, struct stat *meta);

#endif
