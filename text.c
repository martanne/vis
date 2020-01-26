#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* memrchr(3) is non-standard */
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <wchar.h>
#include <stdint.h>
#include <libgen.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#if CONFIG_ACL
#include <sys/acl.h>
#endif
#if CONFIG_SELINUX
#include <selinux/selinux.h>
#endif

#include "text.h"
#include "text-util.h"
#include "text-motions.h"
#include "util.h"

/* Allocate blocks holding the actual file content in junks of size: */
#ifndef BLOCK_SIZE
#define BLOCK_SIZE (1 << 20)
#endif
/* Files smaller than this value are copied on load, larger ones are mmap(2)-ed
 * directely. Hence the former can be truncated, while doing so on the latter
 * results in havoc. */
#define BLOCK_MMAP_SIZE (1 << 26)

/* Block holding the file content, either readonly mmap(2)-ed from the original
 * file or heap allocated to store the modifications.
 */
typedef struct Block Block;
struct Block {
	size_t size;               /* maximal capacity */
	size_t len;                /* current used length / insertion position */
	char *data;                /* actual data */
	enum {                     /* type of allocation */
		MMAP_ORIG,         /* mmap(2)-ed from an external file */
		MMAP,              /* mmap(2)-ed from a temporary file only known to this process */
		MALLOC,            /* heap allocated block using malloc(3) */
	} type;
	Block *next;               /* next junk */
};

/* A piece holds a reference (but doesn't itself store) a certain amount of data.
 * All active pieces chained together form the whole content of the document.
 * At the beginning there exists only one piece, spanning the whole document.
 * Upon insertion/deletion new pieces will be created to represent the changes.
 * Generally pieces are never destroyed, but kept around to peform undo/redo
 * operations.
 */
struct Piece {
	Text *text;             /* text to which this piece belongs */
	Piece *prev, *next;     /* pointers to the logical predecessor/successor */
	Piece *global_prev;     /* double linked list in order of allocation, */
	Piece *global_next;     /* used to free individual pieces */
	const char *data;       /* pointer into a Block holding the data */
	size_t len;             /* the length in number of bytes of the data */
};

/* used to transform a global position (byte offset starting from the beginning
 * of the text) into an offset relative to a piece.
 */
typedef struct {
	Piece *piece;           /* piece holding the location */
	size_t off;             /* offset into the piece in bytes */
} Location;

/* A Span holds a certain range of pieces. Changes to the document are always
 * performed by swapping out an existing span with a new one.
 */
typedef struct {
	Piece *start, *end;     /* start/end of the span */
	size_t len;             /* the sum of the lengths of the pieces which form this span */
} Span;

/* A Change keeps all needed information to redo/undo an insertion/deletion. */
typedef struct Change Change;
struct Change {
	Span old;               /* all pieces which are being modified/swapped out by the change */
	Span new;               /* all pieces which are introduced/swapped in by the change */
	size_t pos;             /* absolute position at which the change occured */
	Change *next;           /* next change which is part of the same revision */
	Change *prev;           /* previous change which is part of the same revision */
};

/* A Revision is a list of Changes which are used to undo/redo all modifications
 * since the last snapshot operation. Revisions are stored in a directed graph structure.
 */
typedef struct Revision Revision;
struct Revision {
	Change *change;         /* the most recent change */
	Revision *next;         /* the next (child) revision in the undo tree */
	Revision *prev;         /* the previous (parent) revision in the undo tree */
	Revision *earlier;      /* the previous Revision, chronologically */
	Revision *later;        /* the next Revision, chronologically */
	time_t time;            /* when the first change of this revision was performed */
	size_t seq;             /* a unique, strictly increasing identifier */
};

typedef struct {
	size_t pos;             /* position in bytes from start of file */
	size_t lineno;          /* line number in file i.e. number of '\n' in [0, pos) */
} LineCache;

/* The main struct holding all information of a given file */
struct Text {
	Block *block;           /* original file content at the time of load operation */
	Block *blocks;          /* all blocks which have been allocated to hold insertion data */
	Piece *pieces;          /* all pieces which have been allocated, used to free them */
	Piece *cache;           /* most recently modified piece */
	Piece begin, end;       /* sentinel nodes which always exists but don't hold any data */
	Revision *history;        /* undo tree */
	Revision *current_revision; /* revision holding all file changes until a snapshot is performed */
	Revision *last_revision;    /* the last revision added to the tree, chronologically */
	Revision *saved_revision;   /* the last revision at the time of the save operation */
	size_t size;            /* current file content size in bytes */
	struct stat info;       /* stat as probed at load time */
	LineCache lines;        /* mapping between absolute pos in bytes and logical line breaks */
};

struct TextSave {                  /* used to hold context between text_save_{begin,commit} calls */
	Text *txt;                 /* text to operate on */
	char *filename;            /* filename to save to as given to text_save_begin */
	char *tmpname;             /* temporary name used for atomic rename(2) */
	int fd;                    /* file descriptor to write data to using text_save_write */
	enum TextSaveMethod type;  /* method used to save file */
};

/* block management */
static Block *block_alloc(Text*, size_t size);
static Block *block_read(Text*, size_t size, int fd);
static Block *block_mmap(Text*, size_t size, int fd, off_t offset);
static void block_free(Block*);
static bool block_capacity(Block*, size_t len);
static const char *block_append(Block*, const char *data, size_t len);
static bool block_insert(Block*, size_t pos, const char *data, size_t len);
static bool block_delete(Block*, size_t pos, size_t len);
static const char *block_store(Text*, const char *data, size_t len);
/* cache layer */
static void cache_piece(Text *txt, Piece *p);
static bool cache_contains(Text *txt, Piece *p);
static bool cache_insert(Text *txt, Piece *p, size_t off, const char *data, size_t len);
static bool cache_delete(Text *txt, Piece *p, size_t off, size_t len);
/* piece management */
static Piece *piece_alloc(Text *txt);
static void piece_free(Piece *p);
static void piece_init(Piece *p, Piece *prev, Piece *next, const char *data, size_t len);
static Location piece_get_intern(Text *txt, size_t pos);
static Location piece_get_extern(Text *txt, size_t pos);
/* span management */
static void span_init(Span *span, Piece *start, Piece *end);
static void span_swap(Text *txt, Span *old, Span *new);
/* change management */
static Change *change_alloc(Text *txt, size_t pos);
static void change_free(Change *c);
/* revision management */
static Revision *revision_alloc(Text *txt);
static void revision_free(Revision *rev);
/* logical line counting cache */
static void lineno_cache_invalidate(LineCache *cache);
static size_t lines_skip_forward(Text *txt, size_t pos, size_t lines, size_t *lines_skiped);
static size_t lines_count(Text *txt, size_t pos, size_t len);

static ssize_t write_all(int fd, const char *buf, size_t count) {
	size_t rem = count;
	while (rem > 0) {
		ssize_t written = write(fd, buf, rem > INT_MAX ? INT_MAX : rem);
		if (written < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return -1;
		} else if (written == 0) {
			break;
		}
		rem -= written;
		buf += written;
	}
	return count - rem;
}

/* allocate a new block of MAX(size, BLOCK_SIZE) bytes */
static Block *block_alloc(Text *txt, size_t size) {
	Block *blk = calloc(1, sizeof *blk);
	if (!blk)
		return NULL;
	if (BLOCK_SIZE > size)
		size = BLOCK_SIZE;
	if (!(blk->data = malloc(size))) {
		free(blk);
		return NULL;
	}
	blk->type = MALLOC;
	blk->size = size;
	blk->next = txt->blocks;
	txt->blocks = blk;
	return blk;
}

static Block *block_read(Text *txt, size_t size, int fd) {
	Block *blk = block_alloc(txt, size);
	if (!blk)
		return NULL;
	while (size > 0) {
		char data[4096];
		ssize_t len = read(fd, data, MIN(sizeof(data), size));
		if (len == -1) {
			txt->blocks = blk->next;
			block_free(blk);
			return NULL;
		} else if (len == 0) {
			break;
		} else {
			block_append(blk, data, len);
			size -= len;
		}
	}
	return blk;
}

static Block *block_mmap(Text *txt, size_t size, int fd, off_t offset) {
	Block *blk = calloc(1, sizeof *blk);
	if (!blk)
		return NULL;
	if (size) {
		blk->data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, offset);
		if (blk->data == MAP_FAILED) {
			free(blk);
			return NULL;
		}
	}
	blk->type = MMAP_ORIG;
	blk->size = size;
	blk->len = size;
	blk->next = txt->blocks;
	txt->blocks = blk;
	return blk;
}

static void block_free(Block *blk) {
	if (!blk)
		return;
	if (blk->type == MALLOC)
		free(blk->data);
	else if ((blk->type == MMAP_ORIG || blk->type == MMAP) && blk->data)
		munmap(blk->data, blk->size);
	free(blk);
}

/* check whether block has enough free space to store len bytes */
static bool block_capacity(Block *blk, size_t len) {
	return blk->size - blk->len >= len;
}

/* append data to block, assumes there is enough space available */
static const char *block_append(Block *blk, const char *data, size_t len) {
	char *dest = memcpy(blk->data + blk->len, data, len);
	blk->len += len;
	return dest;
}

/* stores the given data in a block, allocates a new one if necessary. returns
 * a pointer to the storage location or NULL if allocation failed. */
static const char *block_store(Text *txt, const char *data, size_t len) {
	Block *blk = txt->blocks;
	if ((!blk || !block_capacity(blk, len)) && !(blk = block_alloc(txt, len)))
		return NULL;
	return block_append(blk, data, len);
}

/* insert data into block at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
static bool block_insert(Block *blk, size_t pos, const char *data, size_t len) {
	if (pos > blk->len || !block_capacity(blk, len))
		return false;
	if (blk->len == pos)
		return block_append(blk, data, len);
	char *insert = blk->data + pos;
	memmove(insert + len, insert, blk->len - pos);
	memcpy(insert, data, len);
	blk->len += len;
	return true;
}

/* delete data from a block at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
static bool block_delete(Block *blk, size_t pos, size_t len) {
	size_t end;
	if (!addu(pos, len, &end) || end > blk->len)
		return false;
	if (blk->len == pos) {
		blk->len -= len;
		return true;
	}
	char *delete = blk->data + pos;
	memmove(delete, delete + len, blk->len - pos - len);
	blk->len -= len;
	return true;
}

/* cache the given piece if it is the most recently changed one */
static void cache_piece(Text *txt, Piece *p) {
	Block *blk = txt->blocks;
	if (!blk || p->data < blk->data || p->data + p->len != blk->data + blk->len)
		return;
	txt->cache = p;
}

/* check whether the given piece was the most recently modified one */
static bool cache_contains(Text *txt, Piece *p) {
	Block *blk = txt->blocks;
	Revision *rev = txt->current_revision;
	if (!blk || !txt->cache || txt->cache != p || !rev || !rev->change)
		return false;

	Piece *start = rev->change->new.start;
	Piece *end = rev->change->new.end;
	bool found = false;
	for (Piece *cur = start; !found; cur = cur->next) {
		if (cur == p)
			found = true;
		if (cur == end)
			break;
	}

	return found && p->data + p->len == blk->data + blk->len;
}

/* try to insert a junk of data at a given piece offset. the insertion is only
 * performed if the piece is the most recenetly changed one. the legnth of the
 * piece, the span containing it and the whole text is adjusted accordingly */
static bool cache_insert(Text *txt, Piece *p, size_t off, const char *data, size_t len) {
	if (!cache_contains(txt, p))
		return false;
	Block *blk = txt->blocks;
	size_t bufpos = p->data + off - blk->data;
	if (!block_insert(blk, bufpos, data, len))
		return false;
	p->len += len;
	txt->current_revision->change->new.len += len;
	txt->size += len;
	return true;
}

/* try to delete a junk of data at a given piece offset. the deletion is only
 * performed if the piece is the most recenetly changed one and the whole
 * affected range lies within it. the legnth of the piece, the span containing it
 * and the whole text is adjusted accordingly */
static bool cache_delete(Text *txt, Piece *p, size_t off, size_t len) {
	if (!cache_contains(txt, p))
		return false;
	Block *blk = txt->blocks;
	size_t end;
	size_t bufpos = p->data + off - blk->data;
	if (!addu(off, len, &end) || end > p->len || !block_delete(blk, bufpos, len))
		return false;
	p->len -= len;
	txt->current_revision->change->new.len -= len;
	txt->size -= len;
	return true;
}

/* initialize a span and calculate its length */
static void span_init(Span *span, Piece *start, Piece *end) {
	size_t len = 0;
	span->start = start;
	span->end = end;
	for (Piece *p = start; p; p = p->next) {
		len += p->len;
		if (p == end)
			break;
	}
	span->len = len;
}

/* swap out an old span and replace it with a new one.
 *
 *  - if old is an empty span do not remove anything, just insert the new one
 *  - if new is an empty span do not insert anything, just remove the old one
 *
 * adjusts the document size accordingly.
 */
static void span_swap(Text *txt, Span *old, Span *new) {
	if (old->len == 0 && new->len == 0) {
		return;
	} else if (old->len == 0) {
		/* insert new span */
		new->start->prev->next = new->start;
		new->end->next->prev = new->end;
	} else if (new->len == 0) {
		/* delete old span */
		old->start->prev->next = old->end->next;
		old->end->next->prev = old->start->prev;
	} else {
		/* replace old with new */
		old->start->prev->next = new->start;
		old->end->next->prev = new->end;
	}
	txt->size -= old->len;
	txt->size += new->len;
}

/* Allocate a new revision and place it in the revision graph.
 * All further changes will be associated with this revision. */
static Revision *revision_alloc(Text *txt) {
	Revision *rev = calloc(1, sizeof *rev);
	if (!rev)
		return NULL;
	rev->time = time(NULL);
	txt->current_revision = rev;

	/* set sequence number */
	if (!txt->last_revision)
		rev->seq = 0;
	else
		rev->seq = txt->last_revision->seq + 1;

	/* set earlier, later pointers */
	if (txt->last_revision)
		txt->last_revision->later = rev;
	rev->earlier = txt->last_revision;

	if (!txt->history) {
		txt->history = rev;
		return rev;
	}

	/* set prev, next pointers */
	rev->prev = txt->history;
	txt->history->next = rev;
	txt->history = rev;
	return rev;
}

static void revision_free(Revision *rev) {
	if (!rev)
		return;
	for (Change *next, *c = rev->change; c; c = next) {
		next = c->next;
		change_free(c);
	}
	free(rev);
}

static Piece *piece_alloc(Text *txt) {
	Piece *p = calloc(1, sizeof *p);
	if (!p)
		return NULL;
	p->text = txt;
	p->global_next = txt->pieces;
	if (txt->pieces)
		txt->pieces->global_prev = p;
	txt->pieces = p;
	return p;
}

static void piece_free(Piece *p) {
	if (!p)
		return;
	if (p->global_prev)
		p->global_prev->global_next = p->global_next;
	if (p->global_next)
		p->global_next->global_prev = p->global_prev;
	if (p->text->pieces == p)
		p->text->pieces = p->global_next;
	if (p->text->cache == p)
		p->text->cache = NULL;
	free(p);
}

static void piece_init(Piece *p, Piece *prev, Piece *next, const char *data, size_t len) {
	p->prev = prev;
	p->next = next;
	p->data = data;
	p->len = len;
}

/* returns the piece holding the text at byte offset pos. if pos happens to
 * be at a piece boundry i.e. the first byte of a piece then the previous piece
 * to the left is returned with an offset of piece->len. this is convenient for
 * modifications to the piece chain where both pieces (the returned one and the
 * one following it) are needed, but unsuitable as a public interface.
 *
 * in particular if pos is zero, the begin sentinel piece is returned.
 */
static Location piece_get_intern(Text *txt, size_t pos) {
	size_t cur = 0;
	for (Piece *p = &txt->begin; p->next; p = p->next) {
		if (cur <= pos && pos <= cur + p->len)
			return (Location){ .piece = p, .off = pos - cur };
		cur += p->len;
	}

	return (Location){ 0 };
}

/* similiar to piece_get_intern but usable as a public API. returns the piece
 * holding the text at byte offset pos. never returns a sentinel piece.
 * it pos is the end of file (== text_size()) and the file is not empty then
 * the last piece holding data is returned.
 */
static Location piece_get_extern(Text *txt, size_t pos) {
	size_t cur = 0;
	Piece *p;

	for (p = txt->begin.next; p->next; p = p->next) {
		if (cur <= pos && pos < cur + p->len)
			return (Location){ .piece = p, .off = pos - cur };
		cur += p->len;
	}

	if (cur == pos)
		return (Location){ .piece = p->prev, .off = p->prev->len };

	return (Location){ 0 };
}

/* allocate a new change, associate it with current revision or a newly
 * allocated one if none exists. */
static Change *change_alloc(Text *txt, size_t pos) {
	Revision *rev = txt->current_revision;
	if (!rev) {
		rev = revision_alloc(txt);
		if (!rev)
			return NULL;
	}
	Change *c = calloc(1, sizeof *c);
	if (!c)
		return NULL;
	c->pos = pos;
	c->next = rev->change;
	if (rev->change)
		rev->change->prev = c;
	rev->change = c;
	return c;
}

static void change_free(Change *c) {
	if (!c)
		return;
	/* only free the new part of the span, the old one is still in use */
	if (c->new.start != c->new.end)
		piece_free(c->new.end);
	piece_free(c->new.start);
	free(c);
}

/* When inserting new data there are 2 cases to consider.
 *
 *  - in the first the insertion point falls into the middle of an exisiting
 *    piece which is replaced by three new pieces:
 *
 *      /-+ --> +---------------+ --> +-\
 *      | |     | existing text |     | |
 *      \-+ <-- +---------------+ <-- +-/
 *                         ^
 *                         Insertion point for "demo "
 *
 *      /-+ --> +---------+ --> +-----+ --> +-----+ --> +-\
 *      | |     | existing|     |demo |     |text |     | |
 *      \-+ <-- +---------+ <-- +-----+ <-- +-----+ <-- +-/
 *
 *  - the second case deals with an insertion point at a piece boundry:
 *
 *      /-+ --> +---------------+ --> +-\
 *      | |     | existing text |     | |
 *      \-+ <-- +---------------+ <-- +-/
 *            ^
 *            Insertion point for "short"
 *
 *      /-+ --> +-----+ --> +---------------+ --> +-\
 *      | |     |short|     | existing text |     | |
 *      \-+ <-- +-----+ <-- +---------------+ <-- +-/
 */
bool text_insert(Text *txt, size_t pos, const char *data, size_t len) {
	if (len == 0)
		return true;
	if (pos > txt->size)
		return false;
	if (pos < txt->lines.pos)
		lineno_cache_invalidate(&txt->lines);

	Location loc = piece_get_intern(txt, pos);
	Piece *p = loc.piece;
	if (!p)
		return false;
	size_t off = loc.off;
	if (cache_insert(txt, p, off, data, len))
		return true;

	Change *c = change_alloc(txt, pos);
	if (!c)
		return false;

	if (!(data = block_store(txt, data, len)))
		return false;

	Piece *new = NULL;

	if (off == p->len) {
		/* insert between two existing pieces, hence there is nothing to
		 * remove, just add a new piece holding the extra text */
		if (!(new = piece_alloc(txt)))
			return false;
		piece_init(new, p, p->next, data, len);
		span_init(&c->new, new, new);
		span_init(&c->old, NULL, NULL);
	} else {
		/* insert into middle of an existing piece, therfore split the old
		 * piece. that is we have 3 new pieces one containing the content
		 * before the insertion point then one holding the newly inserted
		 * text and one holding the content after the insertion point.
		 */
		Piece *before = piece_alloc(txt);
		new = piece_alloc(txt);
		Piece *after = piece_alloc(txt);
		if (!before || !new || !after)
			return false;
		piece_init(before, p->prev, new, p->data, off);
		piece_init(new, before, after, data, len);
		piece_init(after, new, p->next, p->data + off, p->len - off);

		span_init(&c->new, before, after);
		span_init(&c->old, p, p);
	}

	cache_piece(txt, new);
	span_swap(txt, &c->old, &c->new);
	return true;
}

static bool text_vprintf(Text *txt, size_t pos, const char *format, va_list ap) {
	va_list ap_save;
	va_copy(ap_save, ap);
	int len = vsnprintf(NULL, 0, format, ap);
	if (len == -1) {
		va_end(ap_save);
		return false;
	}
	char *buf = malloc(len+1);
	bool ret = buf && (vsnprintf(buf, len+1, format, ap_save) == len) && text_insert(txt, pos, buf, len);
	free(buf);
	va_end(ap_save);
	return ret;
}

bool text_appendf(Text *txt, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	bool ret = text_vprintf(txt, text_size(txt), format, ap);
	va_end(ap);
	return ret;
}

bool text_printf(Text *txt, size_t pos, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	bool ret = text_vprintf(txt, pos, format, ap);
	va_end(ap);
	return ret;
}

static size_t revision_undo(Text *txt, Revision *rev) {
	size_t pos = EPOS;
	for (Change *c = rev->change; c; c = c->next) {
		span_swap(txt, &c->new, &c->old);
		pos = c->pos;
	}
	return pos;
}

static size_t revision_redo(Text *txt, Revision *rev) {
	size_t pos = EPOS;
	Change *c = rev->change;
	while (c->next)
		c = c->next;
	for ( ; c; c = c->prev) {
		span_swap(txt, &c->old, &c->new);
		pos = c->pos;
		if (c->new.len > c->old.len)
			pos += c->new.len - c->old.len;
	}
	return pos;
}

size_t text_undo(Text *txt) {
	size_t pos = EPOS;
	/* taking rev snapshot makes sure that txt->current_revision is reset */
	text_snapshot(txt);
	Revision *rev = txt->history->prev;
	if (!rev)
		return pos;
	pos = revision_undo(txt, txt->history);
	txt->history = rev;
	lineno_cache_invalidate(&txt->lines);
	return pos;
}

size_t text_redo(Text *txt) {
	size_t pos = EPOS;
	/* taking a snapshot makes sure that txt->current_revision is reset */
	text_snapshot(txt);
	Revision *rev = txt->history->next;
	if (!rev)
		return pos;
	pos = revision_redo(txt, rev);
	txt->history = rev;
	lineno_cache_invalidate(&txt->lines);
	return pos;
}

static bool history_change_branch(Revision *rev) {
	bool changed = false;
	while (rev->prev) {
		if (rev->prev->next != rev) {
			rev->prev->next = rev;
			changed = true;
		}
		rev = rev->prev;
	}
	return changed;
}

static size_t history_traverse_to(Text *txt, Revision *rev) {
	size_t pos = EPOS;
	if (!rev)
		return pos;
	bool changed = history_change_branch(rev);
	if (!changed) {
		if (rev->seq == txt->history->seq) {
			return txt->lines.pos;
		} else if (rev->seq > txt->history->seq) {
			while (txt->history != rev)
				pos = text_redo(txt);
			return pos;
		} else if (rev->seq < txt->history->seq) {
			while (txt->history != rev)
				pos = text_undo(txt);
			return pos;
		}
	} else {
		while (txt->history->prev && txt->history->prev->next == txt->history)
			text_undo(txt);
		pos = text_undo(txt);
		while (txt->history != rev)
			pos = text_redo(txt);
		return pos;
	}
	return pos;
}

size_t text_earlier(Text *txt) {
	return history_traverse_to(txt, txt->history->earlier);
}

size_t text_later(Text *txt) {
	return history_traverse_to(txt, txt->history->later);
}

size_t text_restore(Text *txt, time_t time) {
	Revision *rev = txt->history;
	while (time < rev->time && rev->earlier)
		rev = rev->earlier;
	while (time > rev->time && rev->later)
		rev = rev->later;
	time_t diff = labs(rev->time - time);
	if (rev->earlier && rev->earlier != txt->history && labs(rev->earlier->time - time) < diff)
		rev = rev->earlier;
	if (rev->later && rev->later != txt->history && labs(rev->later->time - time) < diff)
		rev = rev->later;
	return history_traverse_to(txt, rev);
}

time_t text_state(Text *txt) {
	return txt->history->time;
}

static bool preserve_acl(int src, int dest) {
#if CONFIG_ACL
	acl_t acl = acl_get_fd(src);
	if (!acl)
		return errno == ENOTSUP ? true : false;
	if (acl_set_fd(dest, acl) == -1) {
		acl_free(acl);
		return false;
	}
	acl_free(acl);
#endif /* CONFIG_ACL */
	return true;
}

static bool preserve_selinux_context(int src, int dest) {
#if CONFIG_SELINUX
	char *context = NULL;
	if (!is_selinux_enabled())
		return true;
	if (fgetfilecon(src, &context) == -1)
		return errno == ENOTSUP ? true : false;
	if (fsetfilecon(dest, context) == -1) {
		freecon(context);
		return false;
	}
	freecon(context);
#endif /* CONFIG_SELINUX */
	return true;
}

/* Create a new file named `.filename.vis.XXXXXX` (where `XXXXXX` is a
 * randomly generated, unique suffix) and try to preserve all important
 * meta data. After the file content has been written to this temporary
 * file, text_save_commit_atomic will atomically move it to  its final
 * (possibly already existing) destination using rename(2).
 *
 * This approach does not work if:
 *
 *   - the file is a symbolic link
 *   - the file is a hard link
 *   - file ownership can not be preserved
 *   - file group can not be preserved
 *   - directory permissions do not allow creation of a new file
 *   - POSXI ACL can not be preserved (if enabled)
 *   - SELinux security context can not be preserved (if enabled)
 */
static bool text_save_begin_atomic(TextSave *ctx) {
	int oldfd, saved_errno;
	if ((oldfd = open(ctx->filename, O_RDONLY)) == -1 && errno != ENOENT)
		goto err;
	struct stat oldmeta = { 0 };
	if (oldfd != -1 && lstat(ctx->filename, &oldmeta) == -1)
		goto err;
	if (oldfd != -1) {
		if (S_ISLNK(oldmeta.st_mode)) /* symbolic link */
			goto err;
		if (oldmeta.st_nlink > 1) /* hard link */
			goto err;
	}

	char suffix[] = ".vis.XXXXXX";
	size_t len = strlen(ctx->filename) + sizeof("./.") + sizeof(suffix);
	char *dir = strdup(ctx->filename);
	char *base = strdup(ctx->filename);

	if (!(ctx->tmpname = malloc(len)) || !dir || !base) {
		free(dir);
		free(base);
		goto err;
	}

	snprintf(ctx->tmpname, len, "%s/.%s%s", dirname(dir), basename(base), suffix);
	free(dir);
	free(base);

	if ((ctx->fd = mkstemp(ctx->tmpname)) == -1)
		goto err;

	if (oldfd == -1) {
		mode_t mask = umask(0);
		umask(mask);
		if (fchmod(ctx->fd, 0666 & ~mask) == -1)
			goto err;
	} else {
		if (fchmod(ctx->fd, oldmeta.st_mode) == -1)
			goto err;
		if (!preserve_acl(oldfd, ctx->fd) || !preserve_selinux_context(oldfd, ctx->fd))
			goto err;
		/* change owner if necessary */
		if (oldmeta.st_uid != getuid() && fchown(ctx->fd, oldmeta.st_uid, (uid_t)-1) == -1)
			goto err;
		/* change group if necessary, in case of failure some editors reset
		 * the group permissions to the same as for others */
		if (oldmeta.st_gid != getgid() && fchown(ctx->fd, (uid_t)-1, oldmeta.st_gid) == -1)
			goto err;
		close(oldfd);
	}

	ctx->type = TEXT_SAVE_ATOMIC;
	return true;
err:
	saved_errno = errno;
	if (oldfd != -1)
		close(oldfd);
	if (ctx->fd != -1)
		close(ctx->fd);
	ctx->fd = -1;
	free(ctx->tmpname);
	ctx->tmpname = NULL;
	errno = saved_errno;
	return false;
}

static bool text_save_commit_atomic(TextSave *ctx) {
	if (fsync(ctx->fd) == -1)
		return false;

	struct stat meta = { 0 };
	if (fstat(ctx->fd, &meta) == -1)
		return false;

	bool close_failed = (close(ctx->fd) == -1);
	ctx->fd = -1;
	if (close_failed)
		return false;

	if (rename(ctx->tmpname, ctx->filename) == -1)
		return false;

	free(ctx->tmpname);
	ctx->tmpname = NULL;

	int dir = open(dirname(ctx->filename), O_DIRECTORY|O_RDONLY);
	if (dir == -1)
		return false;

	if (fsync(dir) == -1 && errno != EINVAL) {
		close(dir);
		return false;
	}

	if (close(dir) == -1)
		return false;

	if (meta.st_mtime)
		ctx->txt->info = meta;
	return true;
}

static bool text_save_begin_inplace(TextSave *ctx) {
	Text *txt = ctx->txt;
	struct stat meta = { 0 };
	int newfd = -1, saved_errno;
	if ((ctx->fd = open(ctx->filename, O_CREAT|O_WRONLY, 0666)) == -1)
		goto err;
	if (fstat(ctx->fd, &meta) == -1)
		goto err;
	if (meta.st_dev == txt->info.st_dev && meta.st_ino == txt->info.st_ino &&
	    txt->block && txt->block->type == MMAP_ORIG && txt->block->size) {
		/* The file we are going to overwrite is currently mmap-ed from
		 * text_load, therefore we copy the mmap-ed block to a temporary
		 * file and remap it at the same position such that all pointers
		 * from the various pieces are still valid.
		 */
		size_t size = txt->block->size;
		char tmpname[32] = "/tmp/vis-XXXXXX";
		newfd = mkstemp(tmpname);
		if (newfd == -1)
			goto err;
		if (unlink(tmpname) == -1)
			goto err;
		ssize_t written = write_all(newfd, txt->block->data, size);
		if (written == -1 || (size_t)written != size)
			goto err;
		if (munmap(txt->block->data, size) == -1)
			goto err;

		void *data = mmap(txt->block->data, size, PROT_READ, MAP_SHARED, newfd, 0);
		if (data == MAP_FAILED)
			goto err;
		if (data != txt->block->data) {
			munmap(data, size);
			goto err;
		}
		bool close_failed = (close(newfd) == -1);
		newfd = -1;
		if (close_failed)
			goto err;
		txt->block->data = data;
		txt->block->type = MMAP;
		newfd = -1;
	}
	/* overwrite the existing file content, if something goes wrong
	 * here we are screwed, TODO: make a backup before? */
	if (ftruncate(ctx->fd, 0) == -1)
		goto err;
	ctx->type = TEXT_SAVE_INPLACE;
	return true;
err:
	saved_errno = errno;
	if (newfd != -1)
		close(newfd);
	if (ctx->fd != -1)
		close(ctx->fd);
	ctx->fd = -1;
	errno = saved_errno;
	return false;
}

static bool text_save_commit_inplace(TextSave *ctx) {
	if (fsync(ctx->fd) == -1)
		return false;
	struct stat meta = { 0 };
	if (fstat(ctx->fd, &meta) == -1)
		return false;
	if (close(ctx->fd) == -1)
		return false;
	ctx->txt->info = meta;
	return true;
}

TextSave *text_save_begin(Text *txt, const char *filename, enum TextSaveMethod type) {
	if (!filename)
		return NULL;
	TextSave *ctx = calloc(1, sizeof *ctx);
	if (!ctx)
		return NULL;
	ctx->txt = txt;
	ctx->fd = -1;
	if (!(ctx->filename = strdup(filename)))
		goto err;
	errno = 0;
	if ((type == TEXT_SAVE_AUTO || type == TEXT_SAVE_ATOMIC) && text_save_begin_atomic(ctx))
		return ctx;
	if (errno == ENOSPC)
		goto err;
	if ((type == TEXT_SAVE_AUTO || type == TEXT_SAVE_INPLACE) && text_save_begin_inplace(ctx))
		return ctx;
err:
	text_save_cancel(ctx);
	return NULL;
}

bool text_save_commit(TextSave *ctx) {
	if (!ctx)
		return true;
	bool ret;
	Text *txt = ctx->txt;
	switch (ctx->type) {
	case TEXT_SAVE_ATOMIC:
		ret = text_save_commit_atomic(ctx);
		break;
	case TEXT_SAVE_INPLACE:
		ret = text_save_commit_inplace(ctx);
		break;
	default:
		ret = false;
		break;
	}

	if (ret) {
		txt->saved_revision = txt->history;
		text_snapshot(txt);
	}
	text_save_cancel(ctx);
	return ret;
}

void text_save_cancel(TextSave *ctx) {
	if (!ctx)
		return;
	int saved_errno = errno;
	if (ctx->fd != -1)
		close(ctx->fd);
	if (ctx->tmpname && ctx->tmpname[0])
		unlink(ctx->tmpname);
	free(ctx->tmpname);
	free(ctx->filename);
	free(ctx);
	errno = saved_errno;
}

bool text_save(Text *txt, const char *filename) {
	Filerange r = (Filerange){ .start = 0, .end = text_size(txt) };
	return text_save_range(txt, &r, filename);
}

/* First try to save the file atomically using rename(2) if this does not
 * work overwrite the file in place. However if something goes wrong during
 * this overwrite the original file is permanently damaged.
 */
bool text_save_range(Text *txt, Filerange *range, const char *filename) {
	if (!filename) {
		txt->saved_revision = txt->history;
		text_snapshot(txt);
		return true;
	}
	TextSave *ctx = text_save_begin(txt, filename, TEXT_SAVE_AUTO);
	if (!ctx)
		return false;
	ssize_t written = text_write_range(txt, range, ctx->fd);
	if (written == -1 || (size_t)written != text_range_size(range)) {
		text_save_cancel(ctx);
		return false;
	}
	return text_save_commit(ctx);
}

ssize_t text_save_write_range(TextSave *ctx, Filerange *range) {
	return text_write_range(ctx->txt, range, ctx->fd);
}

ssize_t text_write(Text *txt, int fd) {
	Filerange r = (Filerange){ .start = 0, .end = text_size(txt) };
	return text_write_range(txt, &r, fd);
}

ssize_t text_write_range(Text *txt, Filerange *range, int fd) {
	size_t size = text_range_size(range), rem = size;
	for (Iterator it = text_iterator_get(txt, range->start);
	     rem > 0 && text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		size_t prem = it.end - it.text;
		if (prem > rem)
			prem = rem;
		ssize_t written = write_all(fd, it.text, prem);
		if (written == -1)
			return -1;
		rem -= written;
		if ((size_t)written != prem)
			break;
	}
	return size - rem;
}

Text *text_load(const char *filename) {
	return text_load_method(filename, TEXT_LOAD_AUTO);
}

Text *text_load_method(const char *filename, enum TextLoadMethod method) {
	int fd = -1;
	size_t size = 0;
	Text *txt = calloc(1, sizeof *txt);
	if (!txt)
		return NULL;
	Piece *p = piece_alloc(txt);
	if (!p)
		goto out;
	lineno_cache_invalidate(&txt->lines);
	if (filename) {
		if ((fd = open(filename, O_RDONLY)) == -1)
			goto out;
		if (fstat(fd, &txt->info) == -1)
			goto out;
		if (!S_ISREG(txt->info.st_mode)) {
			errno = S_ISDIR(txt->info.st_mode) ? EISDIR : ENOTSUP;
			goto out;
		}
		// XXX: use lseek(fd, 0, SEEK_END); instead?
		size = txt->info.st_size;
		if (size > 0) {
			if (method == TEXT_LOAD_READ || (method == TEXT_LOAD_AUTO && size < BLOCK_MMAP_SIZE))
				txt->block = block_read(txt, size, fd);
			else
				txt->block = block_mmap(txt, size, fd, 0);
			if (!txt->block)
				goto out;
			piece_init(p, &txt->begin, &txt->end, txt->block->data, txt->block->len);
		}
	}

	if (size == 0)
		piece_init(p, &txt->begin, &txt->end, "\0", 0);

	piece_init(&txt->begin, NULL, p, NULL, 0);
	piece_init(&txt->end, p, NULL, NULL, 0);
	txt->size = p->len;
	/* write an empty revision */
	change_alloc(txt, EPOS);
	text_snapshot(txt);
	txt->saved_revision = txt->history;

	if (fd != -1)
		close(fd);
	return txt;
out:
	if (fd != -1)
		close(fd);
	text_free(txt);
	return NULL;
}

struct stat text_stat(Text *txt) {
	return txt->info;
}

/* A delete operation can either start/stop midway through a piece or at
 * a boundry. In the former case a new piece is created to represent the
 * remaining text before/after the modification point.
 *
 *      /-+ --> +---------+ --> +-----+ --> +-----+ --> +-\
 *      | |     | existing|     |demo |     |text |     | |
 *      \-+ <-- +---------+ <-- +-----+ <-- +-----+ <-- +-/
 *                   ^                         ^
 *                   |------ delete range -----|
 *
 *      /-+ --> +----+ --> +--+ --> +-\
 *      | |     | exi|     |t |     | |
 *      \-+ <-- +----+ <-- +--+ <-- +-/
 */
bool text_delete(Text *txt, size_t pos, size_t len) {
	if (len == 0)
		return true;
	size_t pos_end;
	if (!addu(pos, len, &pos_end) || pos_end > txt->size)
		return false;
	if (pos < txt->lines.pos)
		lineno_cache_invalidate(&txt->lines);

	Location loc = piece_get_intern(txt, pos);
	Piece *p = loc.piece;
	if (!p)
		return false;
	size_t off = loc.off;
	if (cache_delete(txt, p, off, len))
		return true;
	Change *c = change_alloc(txt, pos);
	if (!c)
		return false;

	bool midway_start = false, midway_end = false; /* split pieces? */
	Piece *before, *after; /* unmodified pieces before/after deletion point */
	Piece *start, *end;    /* span which is removed */
	size_t cur;            /* how much has already been deleted */

	if (off == p->len) {
		/* deletion starts at a piece boundry */
		cur = 0;
		before = p;
		start = p->next;
	} else {
		/* deletion starts midway through a piece */
		midway_start = true;
		cur = p->len - off;
		start = p;
		before = piece_alloc(txt);
		if (!before)
			return false;
	}

	/* skip all pieces which fall into deletion range */
	while (cur < len) {
		p = p->next;
		cur += p->len;
	}

	if (cur == len) {
		/* deletion stops at a piece boundry */
		end = p;
		after = p->next;
	} else {
		/* cur > len: deletion stops midway through a piece */
		midway_end = true;
		end = p;
		after = piece_alloc(txt);
		if (!after)
			return false;
		piece_init(after, before, p->next, p->data + p->len - (cur - len), cur - len);
	}

	if (midway_start) {
		/* we finally know which piece follows our newly allocated before piece */
		piece_init(before, start->prev, after, start->data, off);
	}

	Piece *new_start = NULL, *new_end = NULL;
	if (midway_start) {
		new_start = before;
		if (!midway_end)
			new_end = before;
	}
	if (midway_end) {
		if (!midway_start)
			new_start = after;
		new_end = after;
	}

	span_init(&c->new, new_start, new_end);
	span_init(&c->old, start, end);
	span_swap(txt, &c->old, &c->new);
	return true;
}

bool text_delete_range(Text *txt, Filerange *r) {
	if (!text_range_valid(r))
		return false;
	return text_delete(txt, r->start, text_range_size(r));
}

/* preserve the current text content such that it can be restored by
 * means of undo/redo operations */
void text_snapshot(Text *txt) {
	if (txt->current_revision)
		txt->last_revision = txt->current_revision;
	txt->current_revision = NULL;
	txt->cache = NULL;
}


void text_free(Text *txt) {
	if (!txt)
		return;

	// free history
	Revision *hist = txt->history;
	while (hist && hist->prev)
		hist = hist->prev;
	while (hist) {
		Revision *later = hist->later;
		revision_free(hist);
		hist = later;
	}

	for (Piece *next, *p = txt->pieces; p; p = next) {
		next = p->global_next;
		piece_free(p);
	}

	for (Block *next, *blk = txt->blocks; blk; blk = next) {
		next = blk->next;
		block_free(blk);
	}

	free(txt);
}

bool text_modified(Text *txt) {
	return txt->saved_revision != txt->history;
}

bool text_mmaped(Text *txt, const char *ptr) {
	uintptr_t addr = (uintptr_t)ptr;
	for (Block *blk = txt->blocks; blk; blk = blk->next) {
		if ((blk->type == MMAP_ORIG || blk->type == MMAP) &&
		    (uintptr_t)(blk->data) <= addr && addr < (uintptr_t)(blk->data + blk->size))
			return true;
	}
	return false;
}

static bool text_iterator_init(Iterator *it, size_t pos, Piece *p, size_t off) {
	Iterator iter = (Iterator){
		.pos = pos,
		.piece = p,
		.start = p ? p->data : NULL,
		.end = p ? p->data + p->len : NULL,
		.text = p ? p->data + off : NULL,
	};
	*it = iter;
	return text_iterator_valid(it);
}

Iterator text_iterator_get(Text *txt, size_t pos) {
	Iterator it;
	Location loc = piece_get_extern(txt, pos);
	text_iterator_init(&it, pos, loc.piece, loc.off);
	return it;
}

bool text_iterator_byte_get(Iterator *it, char *b) {
	if (text_iterator_valid(it)) {
		if (it->start <= it->text && it->text < it->end) {
			*b = *it->text;
			return true;
		} else if (it->pos == it->piece->text->size) { /* EOF */
			*b = '\0';
			return true;
		}
	}
	return false;
}

bool text_iterator_next(Iterator *it) {
	size_t rem = it->end - it->text;
	return text_iterator_init(it, it->pos+rem, it->piece ? it->piece->next : NULL, 0);
}

bool text_iterator_prev(Iterator *it) {
	size_t off = it->text - it->start;
	size_t len = it->piece && it->piece->prev ? it->piece->prev->len : 0;
	return text_iterator_init(it, it->pos-off, it->piece ? it->piece->prev : NULL, len);
}

bool text_iterator_valid(const Iterator *it) {
	/* filter out sentinel nodes */
	return it->piece && it->piece->text;
}

bool text_iterator_byte_next(Iterator *it, char *b) {
	if (!it->piece || !it->piece->next)
		return false;
	bool eof = true;
	if (it->text < it->end) {
		it->text++;
		it->pos++;
		eof = false;
	} else if (!it->piece->prev) {
		eof = false;
	}

	while (it->text == it->end) {
		if (!text_iterator_next(it)) {
			if (eof)
				return false;
			if (b)
				*b = '\0';
			return text_iterator_prev(it);
		}
	}

	if (b)
		*b = *it->text;
	return true;
}

bool text_iterator_byte_prev(Iterator *it, char *b) {
	if (!it->piece || !it->piece->prev)
		return false;
	bool eof = !it->piece->next;
	while (it->text == it->start) {
		if (!text_iterator_prev(it)) {
			if (!eof)
				return false;
			if (b)
				*b = '\0';
			return text_iterator_next(it);
		}
	}

	--it->text;
	--it->pos;

	if (b)
		*b = *it->text;
	return true;
}

bool text_iterator_byte_find_prev(Iterator *it, char b) {
	while (it->text) {
		const char *match = memrchr(it->start, b, it->text - it->start);
		if (match) {
			it->pos -= it->text - match;
			it->text = match;
			return true;
		}
		text_iterator_prev(it);
	}
	text_iterator_next(it);
	return false;
}

bool text_iterator_byte_find_next(Iterator *it, char b) {
	while (it->text) {
		const char *match = memchr(it->text, b, it->end - it->text);
		if (match) {
			it->pos += match - it->text;
			it->text = match;
			return true;
		}
		text_iterator_next(it);
	}
	text_iterator_prev(it);
	return false;
}

bool text_iterator_codepoint_next(Iterator *it, char *c) {
	while (text_iterator_byte_next(it, NULL)) {
		if (ISUTF8(*it->text)) {
			if (c)
				*c = *it->text;
			return true;
		}
	}
	return false;
}

bool text_iterator_codepoint_prev(Iterator *it, char *c) {
	while (text_iterator_byte_prev(it, NULL)) {
		if (ISUTF8(*it->text)) {
			if (c)
				*c = *it->text;
			return true;
		}
	}
	return false;
}

bool text_iterator_char_next(Iterator *it, char *c) {
	if (!text_iterator_codepoint_next(it, c))
		return false;
	mbstate_t ps = { 0 };
	for (;;) {
		char buf[MB_LEN_MAX];
		size_t len = text_bytes_get(it->piece->text, it->pos, sizeof buf, buf);
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			return true;
		} else if (wclen == (size_t)-2) {
			return false;
		} else if (wclen == 0) {
			return true;
		} else {
			int width = wcwidth(wc);
			if (width != 0)
				return true;
			if (!text_iterator_codepoint_next(it, c))
				return false;
		}
	}
	return true;
}

bool text_iterator_char_prev(Iterator *it, char *c) {
	if (!text_iterator_codepoint_prev(it, c))
		return false;
	for (;;) {
		char buf[MB_LEN_MAX];
		size_t len = text_bytes_get(it->piece->text, it->pos, sizeof buf, buf);
		wchar_t wc;
		mbstate_t ps = { 0 };
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			return true;
		} else if (wclen == (size_t)-2) {
			return false;
		} else if (wclen == 0) {
			return true;
		} else {
			int width = wcwidth(wc);
			if (width != 0)
				return true;
			if (!text_iterator_codepoint_prev(it, c))
				return false;
		}
	}
	return true;
}

bool text_byte_get(Text *txt, size_t pos, char *byte) {
	return text_bytes_get(txt, pos, 1, byte);
}

size_t text_bytes_get(Text *txt, size_t pos, size_t len, char *buf) {
	if (!buf)
		return 0;
	char *cur = buf;
	size_t rem = len;
	for (Iterator it = text_iterator_get(txt, pos);
	     text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		if (rem == 0)
			break;
		size_t piece_len = it.end - it.text;
		if (piece_len > rem)
			piece_len = rem;
		if (piece_len) {
			memcpy(cur, it.text, piece_len);
			cur += piece_len;
			rem -= piece_len;
		}
	}
	return len - rem;
}

char *text_bytes_alloc0(Text *txt, size_t pos, size_t len) {
	if (len == SIZE_MAX)
		return NULL;
	char *buf = malloc(len+1);
	if (!buf)
		return NULL;
	len = text_bytes_get(txt, pos, len, buf);
	buf[len] = '\0';
	return buf;
}

size_t text_size(Text *txt) {
	return txt->size;
}

/* count the number of new lines '\n' in range [pos, pos+len) */
static size_t lines_count(Text *txt, size_t pos, size_t len) {
	size_t lines = 0;
	for (Iterator it = text_iterator_get(txt, pos);
	     text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		const char *start = it.text;
		while (len > 0 && start < it.end) {
			size_t n = MIN(len, (size_t)(it.end - start));
			const char *end = memchr(start, '\n', n);
			if (!end) {
				len -= n;
				break;
			}
			lines++;
			len -= end - start + 1;
			start = end + 1;
		}

		if (len == 0)
			break;
	}
	return lines;
}

/* skip n lines forward and return position afterwards */
static size_t lines_skip_forward(Text *txt, size_t pos, size_t lines, size_t *lines_skipped) {
	size_t lines_old = lines;
	for (Iterator it = text_iterator_get(txt, pos);
	     text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		const char *start = it.text;
		while (lines > 0 && start < it.end) {
			size_t n = it.end - start;
			const char *end = memchr(start, '\n', n);
			if (!end) {
				pos += n;
				break;
			}
			pos += end - start + 1;
			start = end + 1;
			lines--;
		}

		if (lines == 0)
			break;
	}
	if (lines_skipped)
		*lines_skipped = lines_old - lines;
	return pos;
}

static void lineno_cache_invalidate(LineCache *cache) {
	cache->pos = 0;
	cache->lineno = 1;
}

size_t text_pos_by_lineno(Text *txt, size_t lineno) {
	size_t lines_skipped;
	LineCache *cache = &txt->lines;
	if (lineno <= 1)
		return 0;
	if (lineno > cache->lineno) {
		cache->pos = lines_skip_forward(txt, cache->pos, lineno - cache->lineno, &lines_skipped);
		cache->lineno += lines_skipped;
	} else if (lineno < cache->lineno) {
	#if 0
		// TODO does it make sense to scan memory backwards here?
		size_t diff = cache->lineno - lineno;
		if (diff < lineno) {
			lines_skip_backward(txt, cache->pos, diff);
		} else
	#endif
		cache->pos = lines_skip_forward(txt, 0, lineno - 1, &lines_skipped);
		cache->lineno = lines_skipped + 1;
	}
	return cache->lineno == lineno ? cache->pos : EPOS;
}

size_t text_lineno_by_pos(Text *txt, size_t pos) {
	LineCache *cache = &txt->lines;
	if (pos > txt->size)
		pos = txt->size;
	if (pos < cache->pos) {
		size_t diff = cache->pos - pos;
		if (diff < pos)
			cache->lineno -= lines_count(txt, pos, diff);
		else
			cache->lineno = lines_count(txt, 0, pos) + 1;
	} else if (pos > cache->pos) {
		cache->lineno += lines_count(txt, cache->pos, pos - cache->pos);
	}
	cache->pos = text_line_begin(txt, pos);
	return cache->lineno;
}

Mark text_mark_set(Text *txt, size_t pos) {
	if (pos == txt->size)
		return (Mark)&txt->end;
	Location loc = piece_get_extern(txt, pos);
	if (!loc.piece)
		return EMARK;
	return (Mark)(loc.piece->data + loc.off);
}

size_t text_mark_get(Text *txt, Mark mark) {
	size_t cur = 0;

	if (mark == EMARK)
		return EPOS;
	if (mark == (Mark)&txt->end)
		return txt->size;

	for (Piece *p = txt->begin.next; p->next; p = p->next) {
		Mark start = (Mark)(p->data);
		Mark end = start + p->len;
		if (start <= mark && mark < end)
			return cur + (mark - start);
		cur += p->len;
	}

	return EPOS;
}
