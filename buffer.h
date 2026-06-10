#ifndef BUFFER_H
#define BUFFER_H

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
	char *data;   /**< Data pointer, ``NULL`` if empty. */
	s64   length; /**< Current length of data. */
	s64   size;   /**< Maximal capacity of the buffer. */
} Buffer;

/** Release all resources, reinitialize buffer. */
VIS_INTERNAL void buffer_release(Buffer*);
/** Reserve space to store at least ``size`` bytes.*/
VIS_INTERNAL bool buffer_reserve(Buffer*, s64 size);
/** Reserve space for at least ``len`` *more* bytes. */
VIS_INTERNAL bool buffer_grow(Buffer*, s64 len);
/** Set buffer content, growing the buffer as needed. */
VIS_INTERNAL bool buffer_put(Buffer*, const void *data, s64 length);
VIS_INTERNAL bool vis_buffer_put_str8(Buffer *, str8);
/** Ensures buffer is either 0 or 0 terminated. added 0 will not be included in Buffer:len */
VIS_INTERNAL void vis_buffer_terminate(Buffer *);
/** Remove ``length`` bytes starting at ``at``. */
VIS_INTERNAL bool buffer_remove(Buffer*, s64 at, s64 length);
/** Insert data at into buffer. */
VIS_INTERNAL bool vis_buffer_insert(Buffer*, s64 at, const void *data, s64 length);
/** Append further content to the end. */
VIS_INTERNAL bool buffer_append(Buffer*, const void *data, s64 length);
/** Append NUL-terminated data. */
VIS_INTERNAL bool vis_buffer_append0(Buffer*, const char *data);
/** Append formatted buffer content, ensures NUL termination on success. */
VIS_INTERNAL bool vis_buffer_appendf(Buffer*, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/**
 * Get pointer to buffer data.
 * Guaranteed to return a NUL terminated string even if buffer is empty.
 */
VIS_INTERNAL const char *buffer_content0(Buffer*);

/** ``read(3p)`` like interface for reading into a Buffer (``context``) */
VIS_INTERNAL ssize_t read_into_buffer(void *context, char *data, size_t len);

#endif
