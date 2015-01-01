#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Circular buffer with functions for accessing elements in order.
 * One slot always remains unused to distinguish between the empty/full case.
 */

typedef struct RingBuffer RingBuffer;

RingBuffer *ringbuf_alloc(size_t size);
void ringbuf_free(RingBuffer*);
void ringbuf_add(RingBuffer*, const void *value);
const void *ringbuf_prev(RingBuffer*);
const void *ringbuf_next(RingBuffer*);
void ringbuf_invalidate(RingBuffer*);

#endif
