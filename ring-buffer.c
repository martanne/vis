#include "ring-buffer.h"
#include <stdlib.h>

struct RingBuffer {
	int cur;            /* index of current element, last added etc. */
	int start;          /* index of first/oldest element */
	int end;            /* index of reserved/empty slot */
	size_t size;        /* buffer capacity / number of slots */
	bool iterating;     /* whether we are in a sequence of prev/next calls */
	const void *data[]; /* user supplied buffer content */
};

static int ringbuf_index_prev(RingBuffer *buf, int i) {
	return (i-1+buf->size) % buf->size;
}

static int ringbuf_index_next(RingBuffer *buf, int i) {
	return (i+1) % buf->size;
}

static bool ringbuf_isfull(RingBuffer *buf) {
	return ringbuf_index_next(buf, buf->end) == buf->start;
}

static bool ringbuf_isempty(RingBuffer *buf) {
	return buf->start == buf->end;
}

static bool ringbuf_isfirst(RingBuffer *buf) {
	return buf->cur == buf->start;
}

static bool ringbuf_islast(RingBuffer *buf) {
	return ringbuf_index_next(buf, buf->cur) == buf->end;
}

const void *ringbuf_prev(RingBuffer *buf) {
	if (ringbuf_isempty(buf) || (ringbuf_isfirst(buf) && buf->iterating))
		return NULL;
	if (buf->iterating)
		buf->cur = ringbuf_index_prev(buf, buf->cur);
	buf->iterating = true;
	return buf->data[buf->cur];
}

const void *ringbuf_next(RingBuffer *buf) {
	if (ringbuf_isempty(buf) || ringbuf_islast(buf))
		return NULL;
	buf->cur = ringbuf_index_next(buf, buf->cur);
	buf->iterating = true;
	return buf->data[buf->cur];
}

void ringbuf_add(RingBuffer *buf, const void *value) {
	if (ringbuf_isempty(buf)) {
		buf->end = ringbuf_index_next(buf, buf->end);
	} else if (!ringbuf_islast(buf)) {
		buf->cur = ringbuf_index_next(buf, buf->cur);
		buf->end = ringbuf_index_next(buf, buf->cur);
	} else if (ringbuf_isfull(buf)) {
		buf->start = ringbuf_index_next(buf, buf->start);
		buf->cur = ringbuf_index_next(buf, buf->cur);
		buf->end = ringbuf_index_next(buf, buf->end);
	} else {
		buf->cur = ringbuf_index_next(buf, buf->cur);
		buf->end = ringbuf_index_next(buf, buf->end);
	}
	buf->data[buf->cur] = value;
	buf->iterating = false;
}

void ringbuf_invalidate(RingBuffer *buf) {
	buf->iterating = false;
}

RingBuffer *ringbuf_alloc(size_t size) {
	RingBuffer *buf;
	if ((buf = calloc(1, sizeof(*buf) + (++size)*sizeof(buf->data[0]))))
		buf->size = size;
	return buf;
}

void ringbuf_free(RingBuffer *buf) {
	free(buf);
}
