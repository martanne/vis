#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"

/**
 * @file
 * A dynamically growing buffer storing arbitrary data.
 * @rst
 * .. note:: Used for Register, *not* Text content.
 * @endrst
 */

/** A dynamically growing buffer storing arbitrary data. */
typedef struct {
	char *data;    /**< Data pointer, ``NULL`` if empty. */
	size_t len;    /**< Current length of data. */
	size_t size;   /**< Maximal capacity of the buffer. */
} Buffer;

/** Initialize a Buffer object. */
void buffer_init(Buffer*);
/** Release all resources, reinitialize buffer. */
void buffer_release(Buffer*);
/** Set buffer length to zero, keep allocated memory. */
void buffer_clear(Buffer*);
/** Reserve space to store at least ``size`` bytes.*/
bool buffer_reserve(Buffer*, size_t size);
/** Reserve space for at least ``len`` *more* bytes. */
bool buffer_grow(Buffer*, size_t len);
/** If buffer is non-empty, make sure it is ``NUL`` terminated. */
bool buffer_terminate(Buffer*);
/** Set buffer content, growing the buffer as needed. */
bool buffer_put(Buffer*, const void *data, size_t len);
/** Set buffer content to ``NUL`` terminated data. */
bool buffer_put0(Buffer*, const char *data);
/** Remove ``len`` bytes starting at ``pos``. */
bool buffer_remove(Buffer*, size_t pos, size_t len);
/** Insert ``len`` bytes of ``data`` at ``pos``. */
bool buffer_insert(Buffer*, size_t pos, const void *data, size_t len);
/** Insert NUL-terminated data at pos. */
bool buffer_insert0(Buffer*, size_t pos, const char *data);
/** Append further content to the end. */
bool buffer_append(Buffer*, const void *data, size_t len);
/** Append NUL-terminated data. */
bool buffer_append0(Buffer*, const char *data);
/** Insert ``len`` bytes of ``data`` at the start. */
bool buffer_prepend(Buffer*, const void *data, size_t len);
/** Insert NUL-terminated data at the start. */
bool buffer_prepend0(Buffer*, const char *data);
/** Set formatted buffer content, ensures NUL termination on success. */
bool buffer_printf(Buffer*, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/** Append formatted buffer content, ensures NUL termination on success. */
bool buffer_appendf(Buffer*, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/** Return length of a buffer without trailing NUL byte. */
size_t buffer_length0(Buffer*);
/** Return length of a buffer including possible NUL byte. */
size_t buffer_length(Buffer*);
/** Return current maximal capacity in bytes of this buffer. */
size_t buffer_capacity(Buffer*);
/**
 * Get pointer to buffer data.
 * Guaranteed to return a NUL terminated string even if buffer is empty.
 */
const char *buffer_content0(Buffer*);
/**
 * Get pointer to buffer data.
 * @rst
 * .. warning:: Might be NULL, if empty. Might not be NUL terminated.
 * @endrst
 */
const char *buffer_content(Buffer*);
/**
 * Borrow underlying buffer data.
 * @rst
 * .. warning:: The caller is responsible to ``free(3)`` it.
 * @endrst
 */
char *buffer_move(Buffer*);

#endif
