#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <wchar.h>
#include <stdint.h>
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
#include "util.h"

/* Allocate buffers holding the actual file content in junks of size: */
#define BUFFER_SIZE (1 << 20)
/* Files smaller than this value are copied on load, larger ones are mmap(2)-ed
 * directely. Hence the former can be truncated, while doing so on the latter
 * results in havoc. */
#define BUFFER_MMAP_SIZE (1 << 23)

/* Buffer holding the file content, either readonly mmap(2)-ed from the original
 * file or heap allocated to store the modifications.
 */
typedef struct Buffer Buffer;
struct Buffer {
	size_t size;               /* maximal capacity */
	size_t len;                /* current used length / insertion position */
	char *data;                /* actual data */
	enum { MMAP, MALLOC} type; /* type of allocation */
	Buffer *next;              /* next junk */
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
	const char *data;       /* pointer into a Buffer holding the data */
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
	Change *next;           /* next change which is part of the same action */
	Change *prev;           /* previous change which is part of the same action */
};

/* An Action is a list of Changes which are used to undo/redo all modifications
 * since the last snapshot operation. Actions are stored in a directed graph structure.
 */
typedef struct Action Action;
struct Action {
	Change *change;         /* the most recent change */
	Action *next;           /* the next (child) action in the undo tree */
	Action *prev;           /* the previous (parent) operation in the undo tree */
	Action *earlier;        /* the previous Action, chronologically */
	Action *later;          /* the next Action, chronologically */
	time_t time;            /* when the first change of this action was performed */
	size_t seq;             /* a unique, strictly increasing identifier */
};

typedef struct {
	size_t pos;             /* position in bytes from start of file */
	size_t lineno;          /* line number in file i.e. number of '\n' in [0, pos) */
} LineCache;

/* The main struct holding all information of a given file */
struct Text {
	Buffer *buf;            /* original file content at the time of load operation */
	Buffer *buffers;        /* all buffers which have been allocated to hold insertion data */
	Piece *pieces;          /* all pieces which have been allocated, used to free them */
	Piece *cache;           /* most recently modified piece */
	Piece begin, end;       /* sentinel nodes which always exists but don't hold any data */
	Action *history;        /* undo tree */
	Action *current_action; /* action holding all file changes until a snapshot is performed */
	Action *last_action;    /* the last action added to the tree, chronologically */
	Action *saved_action;   /* the last action at the time of the save operation */
	size_t size;            /* current file content size in bytes */
	struct stat info;       /* stat as probed at load time */
	LineCache lines;        /* mapping between absolute pos in bytes and logical line breaks */
	enum TextNewLine newlines; /* which type of new lines does the file use */
};

/* buffer management */
static Buffer *buffer_alloc(Text *txt, size_t size);
static Buffer *buffer_read(Text *txt, size_t size, int fd);
static Buffer *buffer_mmap(Text *txt, size_t size, int fd, off_t offset);
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
static Change *change_alloc(Text *txt, size_t pos);
static void change_free(Change *c);
/* action management */
static Action *action_alloc(Text *txt);
static void action_free(Action *a);
/* logical line counting cache */
static void lineno_cache_invalidate(LineCache *cache);
static size_t lines_skip_forward(Text *txt, size_t pos, size_t lines, size_t *lines_skiped);
static size_t lines_count(Text *txt, size_t pos, size_t len);

static ssize_t write_all(int fd, const char *buf, size_t count) {
	size_t rem = count;
	while (rem > 0) {
		ssize_t written = write(fd, buf, rem);
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
	buf->type = MALLOC;
	buf->size = size;
	buf->next = txt->buffers;
	txt->buffers = buf;
	return buf;
}

static Buffer *buffer_read(Text *txt, size_t size, int fd) {
	Buffer *buf = buffer_alloc(txt, size);
	if (!buf)
		return NULL;
	while (size > 0) {
		char data[4096];
		ssize_t len = read(fd, data, MIN(sizeof(data), size));
		if (len == -1) {
			txt->buffers = buf->next;
			buffer_free(buf);
			return NULL;
		} else if (len == 0) {
			break;
		} else {
			buffer_append(buf, data, len);
			size -= len;
		}
	}
	return buf;
}

static Buffer *buffer_mmap(Text *txt, size_t size, int fd, off_t offset) {
	Buffer *buf = calloc(1, sizeof(Buffer));
	if (!buf)
		return NULL;
	if (size) {
		buf->data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, offset);
		if (buf->data == MAP_FAILED) {
			free(buf);
			return NULL;
		}
	}
	buf->type = MMAP;
	buf->size = size;
	buf->len = size;
	buf->next = txt->buffers;
	txt->buffers = buf;
	return buf;
}

static void buffer_free(Buffer *buf) {
	if (!buf)
		return;
	if (buf->type == MALLOC)
		free(buf->data);
	else if (buf->type == MMAP && buf->data)
		munmap(buf->data, buf->size);
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
	Piece *end = a->change->new.end;
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

/* allocate a new action, set its pointers to the other actions in the history,
 * and set it as txt->history. All further changes will be associated with this action. */
static Action *action_alloc(Text *txt) {
	Action *new = calloc(1, sizeof(Action));
	if (!new)
		return NULL;
	new->time = time(NULL);
	txt->current_action = new;

	/* set sequence number */
	if (!txt->last_action)
		new->seq = 0;
	else
		new->seq = txt->last_action->seq + 1;

	/* set earlier, later pointers */
	if (txt->last_action)
		txt->last_action->later = new;
	new->earlier = txt->last_action;

	if (!txt->history) {
		txt->history = new;
		return new;
	}

	/* set prev, next pointers */
	new->prev = txt->history;
	txt->history->next = new;
	txt->history = new;
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

	if (pos > 0 && pos == txt->size) {
		Piece *p = txt->begin.next;
		while (p->next->next)
			p = p->next;
		return (Location){ .piece = p, .off = p->len };
	}

	for (Piece *p = txt->begin.next; p->next; p = p->next) {
		if (cur <= pos && pos < cur + p->len)
			return (Location){ .piece = p, .off = pos - cur };
		cur += p->len;
	}

	return (Location){ 0 };
}

/* allocate a new change, associate it with current action or a newly
 * allocated one if none exists. */
static Change *change_alloc(Text *txt, size_t pos) {
	Action *a = txt->current_action;
	if (!a) {
		a = action_alloc(txt);
		if (!a)
			return NULL;
	}
	Change *c = calloc(1, sizeof(Change));
	if (!c)
		return NULL;
	c->pos = pos;
	c->next = a->change;
	if (a->change)
		a->change->prev = c;
	a->change = c;
	return c;
}

static void change_free(Change *c) {
	if (!c)
		return;
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

bool text_vprintf(Text *txt, size_t pos, const char *format, va_list ap) {
	va_list ap_save;
	va_copy(ap_save, ap);
	int len = vsnprintf(NULL, 0, format, ap);
	if (len == -1)
		return false;
	char *buf = malloc(len+1);
	bool ret = buf && (vsnprintf(buf, len+1, format, ap_save) == len) && text_insert(txt, pos, buf, len);
	free(buf);
	va_end(ap_save);
	return ret;
}

size_t text_insert_newline(Text *txt, size_t pos) {
	const char *data = text_newline_char(txt);
	size_t len = strlen(data);
	return text_insert(txt, pos, data, len) ? len : 0;
}

static size_t action_undo(Text *txt, Action *a) {
	size_t pos = EPOS;
	for (Change *c = a->change; c; c = c->next) {
		span_swap(txt, &c->new, &c->old);
		pos = c->pos;
	}
	return pos;
}

static size_t action_redo(Text *txt, Action *a) {
	size_t pos = EPOS;
	Change *c = a->change;
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
	/* taking a snapshot makes sure that txt->current_action is reset */
	text_snapshot(txt);
	Action *a = txt->history->prev;
	if (!a)
		return pos;
	pos = action_undo(txt, txt->history);
	txt->history = a;
	lineno_cache_invalidate(&txt->lines);
	return pos;
}

size_t text_redo(Text *txt) {
	size_t pos = EPOS;
	/* taking a snapshot makes sure that txt->current_action is reset */
	text_snapshot(txt);
	Action *a = txt->history->next;
	if (!a)
		return pos;
	pos = action_redo(txt, a);
	txt->history = a;
	lineno_cache_invalidate(&txt->lines);
	return pos;
}

static bool history_change_branch(Action *a) {
	bool changed = false;
	while (a->prev) {
		if (a->prev->next != a) {
			a->prev->next = a;
			changed = true;
		}
		a = a->prev;
	}
	return changed;
}

static size_t history_traverse_to(Text *txt, Action *a) {
	size_t pos = EPOS;
	if (!a)
		return pos;
	bool changed = history_change_branch(a);
	if (!changed) {
		if (a->seq == txt->history->seq) {
			return txt->lines.pos;
		} else if (a->seq > txt->history->seq) {
			while (txt->history != a)
				pos = text_redo(txt);
			return pos;
		} else if (a->seq < txt->history->seq) {
			while (txt->history != a)
				pos = text_undo(txt);
			return pos;
		}
	} else {
		while (txt->history->prev && txt->history->prev->next == txt->history)
			text_undo(txt);
		pos = text_undo(txt);
		while (txt->history != a)
			pos = text_redo(txt);
		return pos;
	}
	return pos;
}

size_t text_earlier(Text *txt, int count) {
	Action *a = txt->history;
	while (count-- > 0 && a->earlier)
		a = a->earlier;
	return history_traverse_to(txt, a);
}

size_t text_later(Text *txt, int count) {
	Action *a = txt->history;
	while (count-- > 0 && a->later)
		a = a->later;
	return history_traverse_to(txt, a);
}

size_t text_restore(Text *txt, time_t time) {
	Action *a = txt->history;
	while (time < a->time && a->earlier)
		a = a->earlier;
	while (time > a->time && a->later)
		a = a->later;
	time_t diff = labs(a->time - time);
	if (a->earlier && a->earlier != txt->history && labs(a->earlier->time - time) < diff)
		a = a->earlier;
	if (a->later && a->later != txt->history && labs(a->later->time - time) < diff)
		a = a->later;
	return history_traverse_to(txt, a);
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

/* Save current content to given filename. The data is first saved to `filename~`
 * and then atomically moved to its final (possibly alredy existing) destination
 * using rename(2). This approach does not work if:
 *
 *   - the file is a symbolic link
 *   - the file is a hard link
 *   - file ownership can not be preserved
 *   - file group can not be preserved
 *   - directory permissions do not allow creation of a new file
 *   - POSXI ACL can not be preserved (if enabled)
 *   - SELinux security context can not be preserved (if enabled)
 */
static bool text_save_atomic_range(Text *txt, Filerange *range, const char *filename) {
	struct stat meta = { 0 }, oldmeta = { 0 };
	int fd = -1, oldfd = -1, saved_errno;
	char *tmpname = NULL;
	size_t size = text_range_size(range);
	size_t namelen = strlen(filename) + 1 /* ~ */ + 1 /* \0 */;

	if ((oldfd = open(filename, O_RDONLY)) == -1 && errno != ENOENT)
		goto err;
	if (oldfd != -1 && lstat(filename, &oldmeta) == -1)
		goto err;
	if (oldfd != -1) {
		if (S_ISLNK(oldmeta.st_mode)) /* symbolic link */
			goto err;
		if (oldmeta.st_nlink > 1) /* hard link */
			goto err;
	}
	if (!(tmpname = calloc(1, namelen)))
		goto err;
	snprintf(tmpname, namelen, "%s~", filename);

	/* O_RDWR is needed because otherwise we can't map with MAP_SHARED */
	if ((fd = open(tmpname, O_CREAT|O_RDWR|O_TRUNC, oldfd == -1 ? S_IRUSR|S_IWUSR : oldmeta.st_mode)) == -1)
		goto err;
	if (ftruncate(fd, size) == -1)
		goto err;
	if (oldfd != -1) {
		if (!preserve_acl(oldfd, fd) || !preserve_selinux_context(oldfd, fd))
			goto err;
		/* change owner if necessary */
		if (oldmeta.st_uid != getuid() && fchown(fd, oldmeta.st_uid, (uid_t)-1) == -1)
			goto err;
		/* change group if necessary, in case of failure some editors reset
		 * the group permissions to the same as for others */
		if (oldmeta.st_gid != getgid() && fchown(fd, (uid_t)-1, oldmeta.st_gid) == -1)
			goto err;
	}

	if (size > 0) {
		void *buf = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
		if (buf == MAP_FAILED)
			goto err;

		char *cur = buf;
		size_t rem = size;

		for (Iterator it = text_iterator_get(txt, range->start);
		     rem > 0 && text_iterator_valid(&it);
		     text_iterator_next(&it)) {
			size_t len = it.end - it.text;
			if (len > rem)
				len = rem;
			memcpy(cur, it.text, len);
			cur += len;
			rem -= len;
		}

		if (munmap(buf, size) == -1)
			goto err;
	}

	if (oldfd != -1) {
		close(oldfd);
		oldfd = -1;
	}

	if (fsync(fd) == -1)
		goto err;

	if (fstat(fd, &meta) == -1)
		goto err;

	if (close(fd) == -1) {
		fd = -1;
		goto err;
	}
	fd = -1;

	if (rename(tmpname, filename) == -1)
		goto err;

	if (meta.st_mtime)
		txt->info = meta;
	free(tmpname);
	return true;
err:
	saved_errno = errno;
	if (oldfd != -1)
		close(oldfd);
	if (fd != -1)
		close(fd);
	if (tmpname && *tmpname)
		unlink(tmpname);
	free(tmpname);
	errno = saved_errno;
	return false;
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
	struct stat meta;
	int fd = -1, newfd = -1;
	errno = 0;
	if (!filename || text_save_atomic_range(txt, range, filename))
		goto ok;
	if (errno == ENOSPC)
		goto err;
	if ((fd = open(filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR)) == -1)
		goto err;
	if (fstat(fd, &meta) == -1)
		goto err;
	if (meta.st_dev == txt->info.st_dev && meta.st_ino == txt->info.st_ino &&
	    txt->buf && txt->buf->type == MMAP && txt->buf->size) {
		/* The file we are going to overwrite is currently mmap-ed from
		 * text_load, therefore we copy the mmap-ed buffer to a temporary
		 * file and remap it at the same position such that all pointers
		 * from the various pieces are still valid.
		 */
		size_t size = txt->buf->size;
		char tmpname[32] = "/tmp/vis-XXXXXX";
		mode_t mask = umask(S_IXUSR | S_IRWXG | S_IRWXO);
		newfd = mkstemp(tmpname);
		umask(mask);
		if (newfd == -1)
			goto err;
		if (unlink(tmpname) == -1)
			goto err;
		ssize_t written = write_all(newfd, txt->buf->data, size);
		if (written == -1 || (size_t)written != size)
			goto err;
		if (munmap(txt->buf->data, size) == -1)
			goto err;

		void *data = mmap(txt->buf->data, size, PROT_READ, MAP_SHARED, newfd, 0);
		if (data == MAP_FAILED)
			goto err;
		if (data != txt->buf->data) {
			munmap(data, size);
			goto err;
		}
		if (close(newfd) == -1) {
			newfd = -1;
			goto err;
		}
		txt->buf->data = data;
		newfd = -1;
	}
	/* overwrite the exisiting file content, if somehting goes wrong
	 * here we are screwed, TODO: make a backup before? */
	if (ftruncate(fd, 0) == -1)
		goto err;
	ssize_t written = text_write_range(txt, range, fd);
	if (written == -1 || (size_t)written != text_range_size(range))
		goto err;

	if (fsync(fd) == -1)
		goto err;
	if (fstat(fd, &meta) == -1)
		goto err;
	if (close(fd) == -1)
		return false;
	txt->info = meta;
ok:
	txt->saved_action = txt->history;
	text_snapshot(txt);
	return true;
err:
	if (fd != -1)
		close(fd);
	if (newfd != -1)
		close(newfd);
	return false;
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

/* load the given file as starting point for further editing operations.
 * to start with an empty document, pass NULL as filename. */
Text *text_load(const char *filename) {
	Text *txt = calloc(1, sizeof(Text));
	if (!txt)
		return NULL;
	int fd = -1;
	piece_init(&txt->begin, NULL, &txt->end, NULL, 0);
	piece_init(&txt->end, &txt->begin, NULL, NULL, 0);
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
		size_t size = txt->info.st_size;
		if (size < BUFFER_MMAP_SIZE)
			txt->buf = buffer_read(txt, size, fd);
		else
			txt->buf = buffer_mmap(txt, size, fd, 0);
		if (!txt->buf)
			goto out;
		Piece *p = piece_alloc(txt);
		if (!p)
			goto out;
		piece_init(&txt->begin, NULL, p, NULL, 0);
		piece_init(p, &txt->begin, &txt->end, txt->buf->data, txt->buf->len);
		piece_init(&txt->end, p, NULL, NULL, 0);
		txt->size = txt->buf->len;
	}
	/* write an empty action */
	change_alloc(txt, EPOS);
	text_snapshot(txt);
	txt->saved_action = txt->history;

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
	if (pos + len > txt->size)
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
	if (txt->current_action)
		txt->last_action = txt->current_action;
	txt->current_action = NULL;
	txt->cache = NULL;
}


void text_free(Text *txt) {
	if (!txt)
		return;

	// free history
	Action *hist = txt->history;
	while (hist && hist->prev)
		hist = hist->prev;
	while (hist) {
		Action *later = hist->later;
		action_free(hist);
		hist = later;
	}

	for (Piece *next, *p = txt->pieces; p; p = next) {
		next = p->global_next;
		piece_free(p);
	}

	for (Buffer *next, *buf = txt->buffers; buf; buf = next) {
		next = buf->next;
		buffer_free(buf);
	}

	free(txt);
}

bool text_modified(Text *txt) {
	return txt->saved_action != txt->history;
}

bool text_sigbus(Text *txt, const char *addr) {
	for (Buffer *buf = txt->buffers; buf; buf = buf->next) {
		if (buf->type == MMAP && buf->data <= addr && addr < buf->data + buf->size)
			return true;
	}
	return false;
}

enum TextNewLine text_newline_type(Text *txt){
	if (!txt->newlines) {
		txt->newlines = TEXT_NEWLINE_NL; /* default to UNIX style \n new lines */
		const char *start = txt->buf ? txt->buf->data : NULL;
		if (start) {
			const char *nl = memchr(start, '\n', txt->buf->len);
			if (nl > start && nl[-1] == '\r')
				txt->newlines = TEXT_NEWLINE_CRNL;
		} else {
			char c;
			size_t nl = lines_skip_forward(txt, 0, 1, NULL);
			if (nl > 1 && text_byte_get(txt, nl-2, &c) && c == '\r')
				txt->newlines = TEXT_NEWLINE_CRNL;
		}
	}

	return txt->newlines;
}

const char *text_newline_char(Text *txt) {
	static const char *types[] = {
		[TEXT_NEWLINE_NL] = "\n",
		[TEXT_NEWLINE_CRNL] = "\r\n",
	};
	return types[text_newline_type(txt)];
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
	/* special case for advancement to EOF */
	if (it->text == it->end && !it->piece->next->text) {
		it->pos++;
		if (b)
			*b = '\0';
		return true;
	}
	while (it->text >= it->end) {
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
		char buf[MB_CUR_MAX];
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
		char buf[MB_CUR_MAX];
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
static size_t lines_skip_forward(Text *txt, size_t pos, size_t lines, size_t *lines_skipped) {
	size_t lines_old = lines;
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
	cache->pos = pos;
	return cache->lineno;
}

Mark text_mark_set(Text *txt, size_t pos) {
	if (pos == 0)
		return (Mark)&txt->begin;
	if (pos == txt->size)
		return (Mark)&txt->end;
	Location loc = piece_get_extern(txt, pos);
	if (!loc.piece)
		return NULL;
	return loc.piece->data + loc.off;
}

size_t text_mark_get(Text *txt, Mark mark) {
	size_t cur = 0;

	if (!mark)
		return EPOS;
	if (mark == (Mark)&txt->begin)
		return 0;
	if (mark == (Mark)&txt->end)
		return txt->size;

	for (Piece *p = txt->begin.next; p->next; p = p->next) {
		if (p->data <= mark && mark < p->data + p->len)
			return cur + (mark - p->data);
		cur += p->len;
	}

	return EPOS;
}

size_t text_history_get(Text *txt, size_t index) {
	for (Action *a = txt->current_action ? txt->current_action : txt->history; a; a = a->prev) {
		if (index-- == 0) {
			Change *c = a->change;
			while (c && c->next)
				c = c->next;
			return c ? c->pos : EPOS;
		}
	}
	return EPOS;
}
