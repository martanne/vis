#ifndef TEXT_INTERNAL
#define TEXT_INTERNAL

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

VIS_INTERNAL Block *block_alloc(size_t size);
VIS_INTERNAL Block *block_read(size_t size, int fd);
VIS_INTERNAL Block *block_mmap(size_t size, int fd, off_t offset);
VIS_INTERNAL Block *block_load(int dirfd, const char *filename, enum TextLoadMethod method, struct stat *info);
VIS_INTERNAL void block_free(Block*);
VIS_INTERNAL bool block_capacity(Block*, size_t len);
VIS_INTERNAL const char *block_append(Block*, const char *data, size_t len);
VIS_INTERNAL bool block_insert(Block*, size_t pos, const char *data, size_t len);
VIS_INTERNAL bool block_delete(Block*, size_t pos, size_t len);

VIS_INTERNAL Block *text_block_mmaped(Text*);
VIS_INTERNAL void text_saved(Text*, struct stat *meta);

#endif
