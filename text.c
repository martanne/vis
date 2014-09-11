/*
 * Copyright (c) 2014 Marc André Tanner <mat at brain-dump.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "text.h"
#include "util.h"

#define BUFFER_SIZE (1 << 20)

struct Regex {
	const char *string;
	regex_t regex;
};

/* Buffer holding the file content, either readonly mmap(2)-ed from the original
 * file or heap allocated to store the modifications.
 */
typedef struct Buffer Buffer;
struct Buffer {
	size_t size;            /* maximal capacity */
	size_t len;             /* current used length / insertion position */
	char *data;             /* actual data */
	Buffer *next;           /* next junk */
};

/* A piece holds a reference (but doesn't itself store) a certain amount of data.
 * All active pieces chained together form the whole content of the document.
 * At the beginning there exists only one piece, spanning the whole document.
 * Upon insertion/delition new pieces will be created to represent the changes.
 * Generally pieces are never destroytxt, but kept around to peform undo/redo operations.
 */
struct Piece {
	Text *text;             /* text to which this piece belongs */
	Piece *prev, *next;     /* pointers to the logical predecessor/successor */
	Piece *global_prev;     /* double linked list in order of allocation, */
	Piece *global_next;     /* used to free individual pieces */
	const char *data;       /* pointer into a Buffer holding the data */
	size_t len;             /* the lenght in number of bytes starting from content */
	int index;              /* unique index identifiying the piece */
};

/* used to transform a global position (byte offset starting from the begining
 * of the text) into an offset relative to piece.
 */
typedef struct {
	Piece *piece;           /* piece holding the location */
	size_t off;             /* offset into the piece in bytes */
} Location;

/* A Span holds a certain range of pieces. Changes to the document are allways
 * performed by swapping out an existing span with a new one.
 */
typedef struct {
	Piece *start, *end;     /* start/end of the span */
	size_t len;             /* the sum of the lenghts of the pieces which form this span */
} Span;

/* A Change keeps all needed information to redo/undo an insertion/deletion. */
typedef struct Change Change;
struct Change {
	Span old;               /* all pieces which are being modified/swapped out by the change */
	Span new;               /* all pieces which are introduced/swapped in by the change */
	Change *next;
};

/* An Action is a list of Changes which are used to undo/redo all modifications
 * since the last snapshot operation. Actions are kept in an undo and a redo stack.
 */
typedef struct Action Action;
struct Action {
	Change *change;         /* the most recent change */
	Action *next;           /* next action in the undo/redo stack */
	time_t time;            /* when the first change of this action was performed */
};

typedef struct {
	size_t pos;             /* position in bytes from start of file */
	size_t lineno;          /* line number in file i.e. number of '\n' in [0, pos) */
} LineCache;

/* The main struct holding all information of a given file */
struct Text {
	Buffer buf;             /* original mmap(2)-ed file content at the time of load operation */
	Buffer *buffers;        /* all buffers which have been allocated to hold insertion data */
	Piece *pieces;		/* all pieces which have been allocated, used to free them */
	Piece *cache;           /* most recently modified piece */
	int piece_count;	/* number of pieces allocated, only used for debuging purposes */
	Piece begin, end;       /* sentinel nodes which always exists but don't hold any data */
	Action *redo, *undo;    /* two stacks holding all actions performed to the file */
	Action *current_action; /* action holding all file changes until a snapshot is performed */
	Action *saved_action;   /* the last action at the time of the save operation */
	size_t size;            /* current file content size in bytes */
	const char *filename;   /* filename of which data was loaded */
	struct stat info;	/* stat as proped on load time */
	int fd;                 /* the file descriptor of the original mmap-ed data */
	LineCache lines;        /* mapping between absolute pos in bytes and logical line breaks */
	const char *marks[26];  /* a mark is a pointer into an underlying buffer */
};

/* buffer management */
static Buffer *buffer_alloc(Text *txt, size_t size);
static void buffer_free(Buffer *buf);
static bool buffer_capacity(Buffer *buf, size_t len);
static const char *buffer_append(Buffer *buf, const char *data, size_t len);
static bool buffer_insert(Buffer *buf, size_t pos, const char *data, size_t len);
static bool buffer_delete(Buffer *buf, size_t pos, size_t len);
static const char *buffer_store(Text *txt, const char *data, size_t len);
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
static Change *change_alloc(Text *txt);
static void change_free(Change *c);
/* action management */
static Action *action_alloc(Text *txt);
static void action_free(Action *a);
static void action_push(Action **stack, Action *action);
static Action *action_pop(Action **stack);
/* logical line counting cache */
static void lineno_cache_invalidate(LineCache *cache);
static size_t lines_skip_forward(Text *txt, size_t pos, size_t lines);
static size_t lines_count(Text *txt, size_t pos, size_t len);

/* allocate a new buffer of MAX(size, BUFFER_SIZE) bytes */
static Buffer *buffer_alloc(Text *txt, size_t size) {
	Buffer *buf = calloc(1, sizeof(Buffer));
	if (!buf)
		return NULL;
	if (BUFFER_SIZE > size)
		size = BUFFER_SIZE;
	if (!(buf->data = malloc(size))) {
		free(buf);
		return NULL;
	}
	buf->size = size;
	buf->next = txt->buffers;
	txt->buffers = buf;
	return buf;
}

static void buffer_free(Buffer *buf) {
	if (!buf)
		return;
	free(buf->data);
	free(buf);
}

/* check whether buffer has enough free space to store len bytes */
static bool buffer_capacity(Buffer *buf, size_t len) {
	return buf->size - buf->len >= len;
}

/* append data to buffer, assumes there is enough space available */
static const char *buffer_append(Buffer *buf, const char *data, size_t len) {
	char *dest = memcpy(buf->data + buf->len, data, len);
	buf->len += len;
	return dest;
}

/* stores the given data in a buffer, allocates a new one if necessary. returns
 * a pointer to the storage location or NULL if allocation failed. */
static const char *buffer_store(Text *txt, const char *data, size_t len) {
	Buffer *buf = txt->buffers;
	if ((!buf || !buffer_capacity(buf, len)) && !(buf = buffer_alloc(txt, len)))
		return NULL;
	return buffer_append(buf, data, len);
}

/* insert data into buffer at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
static bool buffer_insert(Buffer *buf, size_t pos, const char *data, size_t len) {
	if (pos > buf->len || !buffer_capacity(buf, len))
		return false;
	if (buf->len == pos)
		return buffer_append(buf, data, len);
	char *insert = buf->data + pos;
	memmove(insert + len, insert, buf->len - pos);
	memcpy(insert, data, len);
	buf->len += len;
	return true;
}

/* delete data from a buffer at an arbitrary position, this should only be used with
 * data of the most recently created piece. */
static bool buffer_delete(Buffer *buf, size_t pos, size_t len) {
	if (pos + len > buf->len)
		return false;
	if (buf->len == pos) {
		buf->len -= len;
		return true;
	}
	char *delete = buf->data + pos;
	memmove(delete, delete + len, buf->len - pos - len);
	buf->len -= len;
	return true;
}

/* cache the given piece if it is the most recently changed one */
static void cache_piece(Text *txt, Piece *p) {
	Buffer *buf = txt->buffers;
	if (!buf || p->data < buf->data || p->data + p->len != buf->data + buf->len)
		return;
	txt->cache = p;
}

/* check whether the given piece was the most recently modified one */
static bool cache_contains(Text *txt, Piece *p) {
	Buffer *buf = txt->buffers;
	Action *a = txt->current_action;
	if (!buf || !txt->cache || txt->cache != p || !a || !a->change)
		return false;

	Piece *start = a->change->new.start;
	Piece *end = a->change->new.start;
	bool found = false;
	for (Piece *cur = start; !found; cur = cur->next) {
		if (cur == p)
			found = true;
		if (cur == end)
			break;
	}

	return found && p->data + p->len == buf->data + buf->len;
}

/* try to insert a junk of data at a given piece offset. the insertion is only
 * performed if the piece is the most recenetly changed one. the legnth of the
 * piece, the span containing it and the whole text is adjusted accordingly */
static bool cache_insert(Text *txt, Piece *p, size_t off, const char *data, size_t len) {
	if (!cache_contains(txt, p))
		return false;
	Buffer *buf = txt->buffers;
	size_t bufpos = p->data + off - buf->data;
	if (!buffer_insert(buf, bufpos, data, len))
		return false;
	p->len += len;
	txt->current_action->change->new.len += len;
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
	Buffer *buf = txt->buffers;
	size_t bufpos = p->data + off - buf->data;
	if (off + len > p->len || !buffer_delete(buf, bufpos, len))
		return false;
	p->len -= len;
	txt->current_action->change->new.len -= len;
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
	/* TODO use a balanced search tree to keep the pieces
		instead of a doubly linked list.
	 */
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

static void action_push(Action **stack, Action *action) {
	action->next = *stack;
	*stack = action;
}

static Action *action_pop(Action **stack) {
	Action *action = *stack;
	if (action)
		*stack = action->next;
	return action;
}

/* allocate a new action, empty the redo stack and push the new action onto
 * the undo stack. all further changes will be associated with this action. */
static Action *action_alloc(Text *txt) {
	Action *old, *new = calloc(1, sizeof(Action));
	if (!new)
		return NULL;
	new->time = time(NULL);
	/* throw a away all old redo operations */
	while ((old = action_pop(&txt->redo)))
		action_free(old);
	txt->current_action = new;
	action_push(&txt->undo, new);
	return new;
}

static void action_free(Action *a) {
	if (!a)
		return;
	for (Change *next, *c = a->change; c; c = next) {
		next = c->next;
		change_free(c);
	}
	free(a);
}

static Piece *piece_alloc(Text *txt) {
	Piece *p = calloc(1, sizeof(Piece));
	if (!p)
		return NULL;
	p->text = txt;
	p->index = ++txt->piece_count;
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
 * holding the text at byte offset pos. never returns a sentinel piece. */
static Location piece_get_extern(Text *txt, size_t pos) {
	size_t cur = 0;
	for (Piece *p = txt->begin.next; p->next; p = p->next) {
		if (cur <= pos && pos < cur + p->len)
			return (Location){ .piece = p, .off = pos - cur };
		cur += p->len;
	}

	return (Location){ 0 };
}

/* allocate a new change, associate it with current action or a newly
 * allocated one if none exists. */
static Change *change_alloc(Text *txt) {
	Action *a = txt->current_action;
	if (!a) {
		a = action_alloc(txt);
		if (!a)
			return NULL;
	}
	Change *c = calloc(1, sizeof(Change));
	if (!c)
		return NULL;
	c->next = a->change;
	a->change = c;
	return c;
}

static void change_free(Change *c) {
	/* only free the new part of the span, the old one is still in use */
	piece_free(c->new.start);
	if (c->new.start != c->new.end)
		piece_free(c->new.end);
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
bool text_insert_raw(Text *txt, size_t pos, const char *data, size_t len) {
	if (pos > txt->size)
		return false;
	if (pos < txt->lines.pos)
		lineno_cache_invalidate(&txt->lines);

	Location loc = piece_get_intern(txt, pos);
	Piece *p = loc.piece;
	size_t off = loc.off;
	if (cache_insert(txt, p, off, data, len))
		return true;

	Change *c = change_alloc(txt);
	if (!c)
		return false;

	if (!(data = buffer_store(txt, data, len)))
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

bool text_insert(Text *txt, size_t pos, const char *data) {
	return text_insert_raw(txt, pos, data, strlen(data));
}

/* undo all changes of the last action, return whether changes existed */
bool text_undo(Text *txt) {
	Action *a = action_pop(&txt->undo);
	if (!a)
		return false;
	for (Change *c = a->change; c; c = c->next) {
		span_swap(txt, &c->new, &c->old);
	}

	action_push(&txt->redo, a);
	lineno_cache_invalidate(&txt->lines);
	return true;
}

/* redo all changes of the last action, return whether changes existed */
bool text_redo(Text *txt) {
	Action *a = action_pop(&txt->redo);
	if (!a)
		return false;
	for (Change *c = a->change; c; c = c->next) {
		span_swap(txt, &c->old, &c->new);
	}

	action_push(&txt->undo, a);
	lineno_cache_invalidate(&txt->lines);
	return true;
}

/* save current content to given filename. the data is first saved to
 * a file called `.filename.tmp` and then atomically moved to its final
 * (possibly alredy existing) destination using rename(2).
 */
int text_save(Text *txt, const char *filename) {
	size_t len = strlen(filename) + 10;
	char tmpname[len];
	snprintf(tmpname, len, ".%s.tmp", filename);
	// TODO file ownership, permissions etc
	/* O_RDWR is needed because otherwise we can't map with MAP_SHARED */
	int fd = open(tmpname, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1)
		return -1;
	if (ftruncate(fd, txt->size) == -1)
		goto err;
	if (txt->size > 0) {
		void *buf = mmap(NULL, txt->size, PROT_WRITE, MAP_SHARED, fd, 0);
		if (buf == MAP_FAILED)
			goto err;

		char *cur = buf;
		text_iterate(txt, it, 0) {
			size_t len = it.end - it.start;
			memcpy(cur, it.start, len);
			cur += len;
		}

		if (munmap(buf, txt->size) == -1)
			goto err;
	}
	if (close(fd) == -1)
		return -1;
	if (rename(tmpname, filename) == -1)
		return -1;
	txt->saved_action = txt->undo;
	text_snapshot(txt);
	if (!txt->filename)
		txt->filename = strdup(filename);
	return 0;
err:
	close(fd);
	return -1;
}

/* load the given file as starting point for further editing operations.
 * to start with an empty document, pass NULL as filename. */
Text *text_load(const char *filename) {
	Text *txt = calloc(1, sizeof(Text));
	if (!txt)
		return NULL;
	txt->begin.index = 1;
	txt->end.index = 2;
	txt->piece_count = 2;
	piece_init(&txt->begin, NULL, &txt->end, NULL, 0);
	piece_init(&txt->end, &txt->begin, NULL, NULL, 0);
	lineno_cache_invalidate(&txt->lines);
	if (filename) {
		txt->filename = strdup(filename);
		txt->fd = open(filename, O_RDONLY);
		if (txt->fd == -1)
			goto out;
		if (fstat(txt->fd, &txt->info) == -1)
			goto out;
		if (!S_ISREG(txt->info.st_mode))
			goto out;
		// XXX: use lseek(fd, 0, SEEK_END); instead?
		txt->buf.size = txt->info.st_size;
		txt->buf.data = mmap(NULL, txt->info.st_size, PROT_READ, MAP_SHARED, txt->fd, 0);
		if (txt->buf.data == MAP_FAILED)
			goto out;

		Piece *p = piece_alloc(txt);
		if (!p)
			goto out;
		piece_init(&txt->begin, NULL, p, NULL, 0);
		piece_init(p, &txt->begin, &txt->end, txt->buf.data, txt->buf.size);
		piece_init(&txt->end, p, NULL, NULL, 0);
		txt->size = txt->buf.size;
	}
	return txt;
out:
	if (txt->fd > 2)
		close(txt->fd);
	text_free(txt);
	return NULL;
}

static void print_piece(Piece *p) {
	fprintf(stderr, "index: %d\tnext: %d\tprev: %d\t len: %d\t data: %p\n", p->index,
		p->next ? p->next->index : -1,
		p->prev ? p->prev->index : -1,
		p->len, p->data);
	fflush(stderr);
	write(2, p->data, p->len);
	write(2, "\n", 1);
}

void text_debug(Text *txt) {
	for (Piece *p = &txt->begin; p; p = p->next) {
		print_piece(p);
	}
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
	if (pos + len > txt->size)
		return false;
	if (pos < txt->lines.pos)
		lineno_cache_invalidate(&txt->lines);

	Location loc = piece_get_intern(txt, pos);
	Piece *p = loc.piece;
	size_t off = loc.off;
	if (cache_delete(txt, p, off, len))
		return true;
	size_t cur; // how much has already been deleted
	bool midway_start = false, midway_end = false;
	Change *c = change_alloc(txt);
	if (!c)
		return false;
	Piece *before, *after; // unmodified pieces before / after deletion point
	Piece *start, *end; // span which is removed
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
	} else { // cur > len
		/* deletion stops midway through a piece */
		midway_end = true;
		end = p;
		after = piece_alloc(txt);
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

/* preserve the current text content such that it can be restored by
 * means of undo/redo operations */
void text_snapshot(Text *txt) {
	txt->current_action = NULL;
	txt->cache = NULL;
}

void text_free(Text *txt) {
	if (!txt)
		return;

	Action *a;
	while ((a = action_pop(&txt->undo)))
		action_free(a);
	while ((a = action_pop(&txt->redo)))
		action_free(a);

	for (Piece *next, *p = txt->pieces; p; p = next) {
		next = p->global_next;
		piece_free(p);
	}

	for (Buffer *next, *buf = txt->buffers; buf; buf = next) {
		next = buf->next;
		buffer_free(buf);
	}

	if (txt->buf.data)
		munmap(txt->buf.data, txt->buf.size);

	free((char*)txt->filename);
	free(txt);
}

bool text_modified(Text *txt) {
	return txt->saved_action != txt->undo;
}

static bool text_iterator_init(Iterator *it, size_t pos, Piece *p, size_t off) {
	*it = (Iterator){
		.pos = pos,
		.piece = p,
		.start = p ? p->data : NULL,
		.end = p ? p->data + p->len : NULL,
		.text = p ? p->data + off : NULL,
	};
	return text_iterator_valid(it);
}

Iterator text_iterator_get(Text *txt, size_t pos) {
	Iterator it;
	Location loc = piece_get_extern(txt, pos);
	text_iterator_init(&it, pos, loc.piece, loc.off);
	return it;
}

bool text_iterator_byte_get(Iterator *it, char *b) {
	if (text_iterator_valid(it) && it->start <= it->text && it->text < it->end) {
		*b = *it->text;
		return true;
	}
	return false;
}

bool text_iterator_next(Iterator *it) {
	return text_iterator_init(it, it->pos, it->piece ? it->piece->next : NULL, 0);
}

bool text_iterator_prev(Iterator *it) {
	return text_iterator_init(it, it->pos, it->piece ? it->piece->prev : NULL, 0);
}

bool text_iterator_valid(const Iterator *it) {
	/* filter out sentinel nodes */
	return it->piece && it->piece->text;
}

bool text_iterator_byte_next(Iterator *it, char *b) {
	if (!text_iterator_valid(it))
		return false;
	it->text++;
	while (it->text == it->end) {
		if (!text_iterator_next(it))
			return false;
		it->text = it->start;
	}
	it->pos++;
	if (b)
		*b = *it->text;
	return true;
}

bool text_iterator_byte_prev(Iterator *it, char *b) {
	if (!text_iterator_valid(it))
		return false;
	while (it->text == it->start) {
		if (!text_iterator_prev(it))
			return false;
		it->text = it->end;
	}
	--it->text;
	--it->pos;
	if (b)
		*b = *it->text;
	return true;
}

bool text_iterator_char_next(Iterator *it, char *c) {
	while (text_iterator_byte_next(it, NULL)) {
		if (ISUTF8(*it->text)) {
			if (c)
				*c = *it->text;
			return true;
		}
	}
	return false;
}

bool text_iterator_char_prev(Iterator *it, char *c) {
	while (text_iterator_byte_prev(it, NULL)) {
		if (ISUTF8(*it->text)) {
			if (c)
				*c = *it->text;
			return true;
		}
	}
	return false;
}

bool text_byte_get(Text *txt, size_t pos, char *buf) {
	return text_bytes_get(txt, pos, 1, buf);
}

size_t text_bytes_get(Text *txt, size_t pos, size_t len, char *buf) {
	if (!buf)
		return 0;
	char *cur = buf;
	size_t rem = len;
	text_iterate(txt, it, pos) {
		if (rem == 0)
			break;
		size_t piece_len = it.end - it.text;
		if (piece_len > rem)
			piece_len = rem;
		memcpy(cur, it.text, piece_len);
		cur += piece_len;
		rem -= piece_len;
	}
	return len - rem;
}

size_t text_size(Text *txt) {
	return txt->size;
}

/* count the number of new lines '\n' in range [pos, pos+len) */
static size_t lines_count(Text *txt, size_t pos, size_t len) {
	size_t lines = 0;
	text_iterate(txt, it, pos) {
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
static size_t lines_skip_forward(Text *txt, size_t pos, size_t lines) {
	text_iterate(txt, it, pos) {
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

		if (lines == 0) {
			if (start < it.end && *start == '\r')
				pos++;
			break;
		}
	}
	return pos;
}

static void lineno_cache_invalidate(LineCache *cache) {
	cache->pos = 0;
	cache->lineno = 1;
}

size_t text_pos_by_lineno(Text *txt, size_t lineno) {
	LineCache *cache = &txt->lines;
	if (lineno <= 1)
		return 0;
	if (lineno > cache->lineno) {
		cache->pos = lines_skip_forward(txt, cache->pos, lineno - cache->lineno);
	} else if (lineno < cache->lineno) {
	#if 0
		// TODO does it make sense to scan memory backwards here?
		size_t diff = cache->lineno - lineno;
		if (diff < lineno) {
			lines_skip_backward(txt, cache->pos, diff);
		} else
	#endif
		cache->pos = lines_skip_forward(txt, 0, lineno - 1);
	}
	cache->lineno = lineno;
	return cache->pos;
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
	cache->pos = pos;
	return cache->lineno;
}

void text_mark_set(Text *txt, Mark mark, size_t pos) {
	if (mark < 0 || mark >= LENGTH(txt->marks))
		return;
	Location loc = piece_get_extern(txt, pos);
	if (!loc.piece)
		return;
	txt->marks[mark] = loc.piece->data + loc.off;
}

size_t text_mark_get(Text *txt, Mark mark) {
	if (mark < 0 || mark >= LENGTH(txt->marks))
		return -1;
	const char *pos = txt->marks[mark];
	size_t cur = 0;
	for (Piece *p = txt->begin.next; p->next; p = p->next) {
		if (p->data <= pos && pos < p->data + p->len)
			return cur + (pos - p->data);
		cur += p->len;
	}

	return -1;
}

void text_mark_clear(Text *txt, Mark mark) {
	if (mark < 0 || mark >= LENGTH(txt->marks))
		return;
	txt->marks[mark] = NULL;
}

void text_mark_clear_all(Text *txt) {
	for (Mark mark = 0; mark < LENGTH(txt->marks); mark++)
		text_mark_clear(txt, mark);
}

const char *text_filename(Text *txt) {
	return txt->filename;
}

Regex *text_regex_new(void) {
	Regex *r = calloc(1, sizeof(Regex));
	if (!r)
		return NULL;
	regcomp(&r->regex, "\0\0", 0); /* this should not match anything */
	return r;
}

int text_regex_compile(Regex *regex, const char *string, int cflags) {
	regex->string = string;
	return regcomp(&regex->regex, string, cflags);
}

void text_regex_free(Regex *r) {
	if (!r)
		return;
	regfree(&r->regex);
	free(r);
}

int text_search_range_forward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
	char *buf = malloc(len + 1);
	if (!buf)
		return REG_NOMATCH;
	len = text_bytes_get(txt, pos, len, buf);
	buf[len] = '\0';
	regmatch_t match[nmatch];
	int ret = regexec(&r->regex, buf, nmatch, match, eflags);
	if (!ret) {
		for (size_t i = 0; i < nmatch; i++) {
			pmatch[i].start = match[i].rm_so == -1 ? (size_t)-1 : pos + match[i].rm_so;
			pmatch[i].end = match[i].rm_eo == -1 ? (size_t)-1 : pos + match[i].rm_eo;
		}
	}
	free(buf);
	return ret;
}

int text_search_range_backward(Text *txt, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags) {
	char *buf = malloc(len + 1);
	if (!buf)
		return REG_NOMATCH;
	len = text_bytes_get(txt, pos, len, buf);
	buf[len] = '\0';
	regmatch_t match[nmatch];
	char *cur = buf;
	int ret = REG_NOMATCH;
	while (!regexec(&r->regex, cur, nmatch, match, eflags)) {
		ret = 0;
		for (size_t i = 0; i < nmatch; i++) {
			pmatch[i].start = match[i].rm_so == -1 ? (size_t)-1 : pos + (size_t)(cur - buf) + match[i].rm_so;
			pmatch[i].end = match[i].rm_eo == -1 ? (size_t)-1 : pos + (size_t)(cur - buf) + match[i].rm_eo;
		}
		cur += match[0].rm_eo;
	}
	free(buf);
	return ret;
}
