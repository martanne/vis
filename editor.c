#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "editor.h"

typedef struct {
	size_t len, pos;
	char *content;
} Buffer;

typedef struct Piece Piece;
struct Piece {
	Piece *prev, *next;
	char *content;
	size_t len;
	// size_t line_count;
	int index;
};

typedef struct {
	Piece *piece;
	size_t off;
} Location;

typedef struct {
	Piece *start, *end;
	size_t len;
	// size_t line_count;
} Span;

typedef struct Change Change;
struct Change {
	Span old, new;
	Change *next;
};

typedef struct Action Action;
struct Action {
	Change *change;
	Action *next;
	time_t timestamp;
};

struct Editor {
	Buffer buf;
	Piece begin, end;
	Action *redo, *undo, *current_action;
	size_t size;
	bool modified;
	struct stat info;
	int fd;
	const char *filename;
};

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

static void span_swap(Editor *ed, Span *old, Span *new) {
	/* TODO use a balanced search tree to keep the pieces
		instead of a doubly linked list.
	 */
	if (!old || old->len == 0) {
		/* insert new span */
		new->start->prev->next = new->start;
		new->end->next->prev = new->end;
	} else if (!new || new->len == 0) {
		/* delete old span */
		old->start->prev->next = old->end->next;
		old->end->next->prev = old->start->prev;
	} else {
		/* replace old with new */
		old->start->prev->next = new->start;
		old->end->next->prev = new->end;
	}
	ed->size -= old->len;
	ed->size += new->len;
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

static Action *action_alloc(Editor *ed) {
	Action *a = calloc(1, sizeof(Action));
	if (!a)
		return NULL;
	ed->current_action = a;
	action_push(&ed->undo, a);
	return a;
}

static Piece *piece_alloc() {
	static int piece_count = 3;
	Piece *p = malloc(sizeof(Piece));
	if (!p)
		return NULL;
	p->index = piece_count++;
	return p;
}

static void piece_init(Piece *p, Piece *prev, Piece *next, char *content, size_t len) {
	p->prev = prev;
	p->next = next;
	p->content = content;
	p->len = len;
}

static Location piece_get(Editor *ed, size_t pos) {
	Location loc = {};
	// TODO: handle position at end of file: pos+1 
	size_t cur = 0;
	for (Piece *p = &ed->begin; p->next; p = p->next) {
		if (cur <= pos && pos <= cur + p->len) {
			loc.piece = p;
			loc.off = pos - cur;
			return loc;
		}
		cur += p->len;
	}

	return loc;
}

static void change_add(Action *a, Change *c) {
	c->next = a->change;
	a->change = c;
}

static Change *change_alloc(Editor *ed) {
	Action *a = ed->current_action;
	if (!a) {
		a = action_alloc(ed);
		if (!a)
			return NULL;
	}
	Change *c = calloc(1, sizeof(Change));
	if (!c)
		return NULL;
	change_add(a, c);
	return c;
}

static Piece* editor_insert_empty(Editor *ed, char *content, size_t len) {
	Piece *p;
	if (ed->size != 0 || !(p = piece_alloc()))
		return NULL;
	piece_init(&ed->begin, NULL, p, NULL, 0);
	piece_init(p, &ed->begin, &ed->end, content, len);
	piece_init(&ed->end, p, NULL, NULL, 0);
	ed->size = len;
	return p;
}

bool editor_insert(Editor *ed, size_t pos, char *text) {
	Change *c = change_alloc(ed);
	if (!c)
		return false;
	/* special case for an empty document */
	if (ed->size == 0) {
		Piece *p = editor_insert_empty(ed, text, strlen(text) /* TODO */);
		span_init(&c->new, p, p);
		span_init(&c->old, NULL, NULL);
		return true;
	}
	Location loc = piece_get(ed, pos);
	Piece *p = loc.piece;
	size_t off = loc.off;
	if (off == p->len) {
		/* insert between two existing pieces */
		Piece *new = piece_alloc();
		piece_init(new, p, p->next, text, strlen(text) /* TODO */);
		span_init(&c->new, new, new);
		span_init(&c->old, NULL, NULL);
	} else {
		/* insert into middle of an existing piece, therfore split the old
		 * piece. that is we have 3 new pieces one containing the content
		 * before the insertion point then one holding the newly inserted 
		 * text and one holding the content after the insertion point.
		 */
		Piece *before = piece_alloc();
		Piece *new = piece_alloc();
		Piece *after = piece_alloc();

		// TODO: check index calculation
		piece_init(before, p->prev, new, p->content, off);
		piece_init(new, before, after, text /* TODO */, strlen(text) /* TODO */);
		piece_init(after, new, p->next, p->content + off, p->len - off);

		span_init(&c->new, before, after);
		span_init(&c->old, p, p);
	}
	
	span_swap(ed, &c->old, &c->new);
	return true;
}

bool editor_undo(Editor *ed) {
	Action *a = action_pop(&ed->undo);
	if (!a)
		return false;
	for (Change *c = a->change; c; c = c->next) {
		span_swap(ed, &c->new, &c->old);
	}

	action_push(&ed->redo, a);	
	return true;
}

bool editor_redo(Editor *ed) {
	Action *a = action_pop(&ed->redo);
	if (!a)
		return false;
	for (Change *c = a->change; c; c = c->next) {
		span_swap(ed, &c->old, &c->new);
	}

	action_push(&ed->undo, a);
	return true;
}

Iterate copy_content(void *data, size_t pos, const char *content, size_t len) {
	char **p = (char **)data;
	memcpy(*p, content, len);
	*p += len;
	return CONTINUE;
}

int editor_save(Editor *ed, const char *filename) {
	size_t len = strlen(filename) + 10;
	char tmpname[len];
	snprintf(tmpname, len, ".%s.tmp", filename);
	// TODO file ownership, permissions etc
	/* O_RDWR is needed because otherwise we can't map with MAP_SHARED */
	int fd = open(tmpname, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1)
		return -1;
	if (ftruncate(fd, ed->size) == -1)
		goto err;
	if (ed->size > 0) {
		void *buf = mmap(NULL, ed->size, PROT_WRITE, MAP_SHARED, fd, 0);
		if (buf == MAP_FAILED)
			goto err;

		void *cur = buf;
		editor_iterate(ed, &cur, 0, copy_content);

		if (munmap(buf, ed->size) == -1)
			goto err;
	}
	if (close(fd) == -1)
		return -1;
	return rename(tmpname, filename);
err:
	close(fd);
	return -1;
}

Editor *editor_load(const char *filename) {
	Editor *ed = calloc(1, sizeof(Editor));
	if (!ed)
		return NULL;
	ed->begin.index = 1;
	ed->end.index = 2;
	piece_init(&ed->begin, NULL, &ed->end, NULL, 0);
	piece_init(&ed->end, &ed->begin, NULL, NULL, 0);
	if (filename) {
		ed->filename = filename;
		ed->fd = open(filename, O_RDONLY);
		if (ed->fd == -1)
			goto out;
		if (fstat(ed->fd, &ed->info) == -1)
			goto out;
		if (!S_ISREG(ed->info.st_mode))
			goto out;
		// XXX: use lseek(fd, 0, SEEK_END); instead?
		ed->buf.len = ed->info.st_size;
		ed->buf.content = mmap(NULL, ed->info.st_size, PROT_READ, MAP_SHARED, ed->fd, 0);
		if (ed->buf.content == MAP_FAILED)
			goto out;
		editor_insert_empty(ed, ed->buf.content, ed->buf.len);
	}
	return ed;
out:
	if (ed->fd > 2)
		close(ed->fd);
	free(ed);
	return NULL;
}

static void print_piece(Piece *p) {
	fprintf(stderr, "index: %d\tnext: %d\tprev: %d\t len: %d\t content: %p\n", p->index,
		p->next ? p->next->index : -1, 
		p->prev ? p->prev->index : -1,
		p->len, p->content);
	fflush(stderr);
	write(1, p->content, p->len);
	write(1, "\n", 1);
}

void editor_debug(Editor *ed) {
	for (Piece *p = &ed->begin; p; p = p->next) {
		print_piece(p);
	}
}

void editor_iterate(Editor *ed, void *data, size_t pos, iterator_callback_t callback) {
	Location loc = piece_get(ed, pos);
	Piece *p = loc.piece;
	if (!p)
		return;
	size_t len = p->len - loc.off;
	char *content = p->content + loc.off;
	while (p && callback(data, pos, content, len) == CONTINUE) {
		pos += len;
		p = p->next;
		if (!p)
			return;
		content = p->content;
		len = p->len;
	}
}

bool editor_delete(Editor *ed, size_t pos, size_t len) {
	if (len == 0)
		return true;
	if (pos + len > ed->size)
		return false;
	Location loc = piece_get(ed, pos);
	Piece *p = loc.piece;
	size_t off = loc.off;
	size_t cur; // how much has already been deleted
	bool midway_start = false, midway_end = false;
	Change *c = change_alloc(ed);
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
		before = piece_alloc();
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
		after = piece_alloc();
		piece_init(after, before, p->next, p->content + p->len - (cur - len), cur - len);
	}

	if (midway_start) {
		/* we finally now which piece follows our newly allocated before piece */
		piece_init(before, start->prev, after, start->content, off);
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
	span_swap(ed, &c->old, &c->new);
	return true;
}

void editor_snapshot(Editor *ed) {
	ed->current_action = NULL;
}
