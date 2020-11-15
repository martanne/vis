#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

/** A mark. */
typedef uintptr_t Mark;

/** An invalid mark, lookup of which will yield ``EPOS``. */
#define EMARK ((Mark)0)
/** An invalid position. */
#define EPOS ((size_t)-1)

/** A range. */
typedef struct {
	size_t start;  /**< Absolute byte position. */
	size_t end;    /**< Absolute byte position. */
} Filerange;

/**
 * Text object storing the buffer content being edited.
 */
typedef struct Text Text;
typedef struct Piece Piece;
typedef struct TextSave TextSave;

/** A contiguous part of the text. */
typedef struct {
	const char *data; /**< Content, might not be NUL-terminated. */
	size_t len;       /**< Length in bytes. */
} TextString;

/**
 * Iterator used to navigate the buffer content.
 *
 * Captures the position within a Piece.
 *
 * @rst
 * .. warning:: Any change to the Text will invalidate the iterator state.
 * .. note:: Should be treated as an opaque type.
 * @endrst
 */
typedef struct {
	const char *start;  /**< Start of the piece data. */
	const char *end;    /**< End of piece data. Addressable range is ``[start, end)``. */
	const char *text;   /**< Current position within piece. Invariant ``start <= text < end`` holds. */
	const Piece *piece; /**< Internal state of current piece. */
	size_t pos;         /**< Absolute position in bytes from start of buffer. */
} Iterator;

/**
 * @defgroup load
 * @{
 */
/**
 * Method used to load existing file content.
 */
enum TextLoadMethod {
	/** Automatically chose best option. */
	TEXT_LOAD_AUTO,
	/**
	 * Read file content and copy it to an in-memory buffer.
	 * Subsequent changes to the underlying file will have no
	 * effect on this text instance.
	 *
	 * @rst
	 * .. note:: Load time is linear in the file size.
	 * @endrst
	 */
	TEXT_LOAD_READ,
	/**
	 * Memory map the the file from disk. Use file system / virtual memory
	 * subsystem as a caching layer.
	 * @rst
	 * .. note:: Load time is (almost) independent of the file size.
	 * .. warning:: Inplace modifications of the underlying file
	 *              will be reflected in the current text content.
	 *              In particular, truncatenation will raise ``SIGBUS``
	 *              and result in data loss.
	 * @endrst
	 */
	TEXT_LOAD_MMAP,
};
/**
 * Create a text instance populated with the given file content.
 *
 * @rst
 * .. note:: Equivalent to ``text_load_method(filename, TEXT_LOAD_AUTO)``.
 * @endrst
 */
Text *text_load(const char *filename);
Text *text_loadat(int dirfd, const char *filename);
/**
 * Create a text instance populated with the given file content.
 *
 * @param filename The name of the file to load, if ``NULL`` an empty text is created.
 * @param method How the file content should be loaded.
 * @return The new Text object or ``NULL`` in case of an error.
 * @rst
 * .. note:: When attempting to load a non-regular file, ``errno`` will be set to:
 *
 *    - ``EISDIR`` for a directory.
 *    - ``ENOTSUP`` otherwise.
 * @endrst
 */
Text *text_load_method(const char *filename, enum TextLoadMethod);
Text *text_loadat_method(int dirfd, const char *filename, enum TextLoadMethod);
/** Release all ressources associated with this text instance. */
void text_free(Text*);
/**
 * @}
 * @defgroup state
 * @{
 */
/** Return the size in bytes of the whole text. */
size_t text_size(const Text*);
/**
 * Get file information at time of load or last save, whichever happened more
 * recently.
 * @rst
 * .. note:: If an empty text instance was created using ``text_load(NULL)``
 *           and it has not yet been saved, an all zero ``struct stat`` will
 *           be returned.
 * @endrst
 * @return See ``stat(2)`` for details.
 */
struct stat text_stat(const Text*);
/** Query whether the text contains any unsaved modifications. */
bool text_modified(const Text*);
/**
 * @}
 * @defgroup modify
 * @{
 */
/**
 * Insert data at the given byte position.
 *
 * @param pos The absolute byte position.
 * @param data The data to insert.
 * @param len The length of the data in bytes.
 * @return Whether the insertion succeeded.
 */
bool text_insert(Text*, size_t pos, const char *data, size_t len);
/**
 * Delete data at given byte position.
 *
 * @param pos The absolute byte position.
 * @param len The number of bytes to delete, starting from ``pos``.
 * @return Whether the deletion succeeded.
 */
bool text_delete(Text*, size_t pos, size_t len);
bool text_delete_range(Text*, const Filerange*);
bool text_printf(Text*, size_t pos, const char *format, ...) __attribute__((format(printf, 3, 4)));
bool text_appendf(Text*, const char *format, ...) __attribute__((format(printf, 2, 3)));
/**
 * @}
 * @defgroup history
 * @{
 */
/**
 * Create a text snapshot, that is a vertice in the history graph.
 */
bool text_snapshot(Text*);
/**
 * Revert to previous snapshot along the main branch.
 * @rst
 * .. note:: Takes an implicit snapshot.
 * @endrst
 * @return The position of the first change or ``EPOS``, if already at the
 *         oldest state i.e. there was nothing to undo.
 */
size_t text_undo(Text*);
/**
 * Reapply an older change along the main brach.
 * @rst
 * .. note:: Takes an implicit snapshot.
 * @endrst
 * @return The position of the first change or ``EPOS``, if already at the
 *         newest state i.e. there was nothing to redo.
 */
size_t text_redo(Text*);
size_t text_earlier(Text*);
size_t text_later(Text*);
/**
 * Restore the text to the state closest to the time given
 */
size_t text_restore(Text*, time_t);
/**
 * Get creation time of current state.
 * @rst
 * .. note:: TODO: This is currently not the same as the time of the last snapshot.
 * @endrst
 */
time_t text_state(const Text*);
/**
 * @}
 * @defgroup lines
 * @{
 */
size_t text_pos_by_lineno(Text*, size_t lineno);
size_t text_lineno_by_pos(Text*, size_t pos);

/**
 * @}
 * @defgroup access
 * @{
 */
/**
 * Get byte stored at ``pos``.
 * @param pos The absolute position.
 * @param byte Destination address to store the byte.
 * @return Whether ``pos`` was valid and ``byte`` updated accordingly.
 * @rst
 * .. note:: Unlike :c:func:`text_iterator_byte_get()` this function does not
 *           return an artificial NUL byte at EOF.
 * @endrst
 */
bool text_byte_get(const Text*, size_t pos, char *byte);
/**
 * Store at most ``len`` bytes starting from ``pos`` into ``buf``.
 * @param pos The absolute starting position.
 * @param len The length in bytes.
 * @param buf The destination buffer.
 * @return The number of bytes (``<= len``) stored at ``buf``.
 * @rst
 * .. warning:: ``buf`` will not be NUL terminated.
 * @endrst
 */
size_t text_bytes_get(const Text*, size_t pos, size_t len, char *buf);
/**
 * Fetch text range into newly allocate memory region.
 * @param pos The absolute starting position.
 * @param len The length in bytes.
 * @return A contigious NUL terminated buffer holding the requested range, or
 *         ``NULL`` in error case.
 * @rst
 * .. warning:: The returned pointer must be freed by the caller.
 * @endrst
 */
char *text_bytes_alloc0(const Text*, size_t pos, size_t len);
/**
 * @}
 * @defgroup iterator
 * @{
 */
Iterator text_iterator_get(const Text*, size_t pos);
bool text_iterator_init(const Text*, Iterator*, size_t pos);
const Text *text_iterator_text(const Iterator*);
bool text_iterator_valid(const Iterator*);
bool text_iterator_has_next(const Iterator*);
bool text_iterator_has_prev(const Iterator*);
bool text_iterator_next(Iterator*);
bool text_iterator_prev(Iterator*);
/**
 * @}
 * @defgroup iterator_byte
 * @{
 */
bool text_iterator_byte_get(const Iterator*, char *b);
bool text_iterator_byte_prev(Iterator*, char *b);
bool text_iterator_byte_next(Iterator*, char *b);
bool text_iterator_byte_find_prev(Iterator*, char b);
bool text_iterator_byte_find_next(Iterator*, char b);
/**
 * @}
 * @defgroup iterator_code
 * @{
 */
bool text_iterator_codepoint_next(Iterator *it, char *c);
bool text_iterator_codepoint_prev(Iterator *it, char *c);
/**
 * @}
 * @defgroup iterator_char
 * @{
 */
bool text_iterator_char_next(Iterator*, char *c);
bool text_iterator_char_prev(Iterator*, char *c);
/**
 * @}
 * @defgroup mark
 * @{
 */
/**
 * Set a mark.
 * @rst
 * .. note:: Setting a mark to ``text_size`` will always return the
 *           current text size upon lookup.
 * @endrst
 * @param pos The position at which to store the mark.
 * @return The mark or ``EMARK`` if an invalid position was given.
 */
Mark text_mark_set(Text*, size_t pos);
/**
 * Lookup a mark.
 * @param mark The mark to look up.
 * @return The byte position or ``EPOS`` for an invalid mark.
 */
size_t text_mark_get(const Text*, Mark);
/**
 * @}
 * @defgroup save
 * @{
 */
/**
 * Method used to save the text.
 */
enum TextSaveMethod {
	/** Automatically chose best option. */
	TEXT_SAVE_AUTO,
	/**
	 * Save file atomically using ``rename(2)``.
	 *
	 * Creates a temporary file, restores all important meta data,
	 * before moving it atomically to its final (possibly already
	 * existing) destination using ``rename(2)``. For new files,
	 * permissions are set to ``0666 & ~umask``.
	 *
	 * @rst
	 * .. warning:: This approach does not work if:
	 *
	 *   - The file is a symbolic link.
	 *   - The file is a hard link.
	 *   - File ownership can not be preserved.
	 *   - File group can not be preserved.
	 *   - Directory permissions do not allow creation of a new file.
	 *   - POSXI ACL can not be preserved (if enabled).
	 *   - SELinux security context can not be preserved (if enabled).
	 * @endrst
	 */
	TEXT_SAVE_ATOMIC,
	/**
	 * Overwrite file in place.
	 * @rst
	 * .. warning:: I/O failure might cause data loss.
	 * @endrst
	 */
	TEXT_SAVE_INPLACE,
};

/**
 * Save the whole text to the given file name.
 *
 * @rst
 * .. note:: Equivalent to ``text_save_method(filename, TEXT_SAVE_AUTO)``.
 * @endrst
 */
bool text_save(Text*, const char *filename);
bool text_saveat(Text*, int dirfd, const char *filename);
/**
 * Save the whole text to the given file name, using the specified method.
 */
bool text_save_method(Text*, const char *filename, enum TextSaveMethod);
bool text_saveat_method(Text*, int dirfd, const char *filename, enum TextSaveMethod);

/**
 * Setup a sequence of write operations.
 *
 * The returned ``TextSave`` pointer can be used to write multiple, possibly
 * non-contigious, file ranges.
 * @rst
 * .. warning:: For every call to ``text_save_begin`` there must be exactly
 *              one matching call to either ``text_save_commit`` or
 *              ``text_save_cancel`` to release the underlying resources.
 * @endrst
 */
TextSave *text_save_begin(Text*, int dirfd, const char *filename, enum TextSaveMethod);
/**
 * Write file range.
 * @return The number of bytes written or ``-1`` in case of an error.
 */
ssize_t text_save_write_range(TextSave*, const Filerange*);
/**
 * Commit changes to disk.
 * @return Whether changes have been saved.
 * @rst
 * .. note:: Releases the underlying resources and frees the given ``TextSave``
 *           pointer which must no longer be used.
 * @endrst
 */
bool text_save_commit(TextSave*);
/**
 * Abort a save operation.
 * @rst
 * .. note:: Does not guarantee to undo the previous writes (they might have been
 *           performed in-place). However, it releases the underlying resources and
 *           frees the given ``TextSave`` pointer which must no longer be used.
 * @endrst
 */
void text_save_cancel(TextSave*);
/**
 * Write whole text content to file descriptor.
 * @return The number of bytes written or ``-1`` in case of an error.
 */
ssize_t text_write(const Text*, int fd);
/**
 * Write file range to file descriptor.
 * @return The number of bytes written or ``-1`` in case of an error.
 */
ssize_t text_write_range(const Text*, const Filerange*, int fd);
/**
 * @}
 * @defgroup misc
 * @{
 */
/**
 * Check whether ``ptr`` is part of a memory mapped region associated with
 * this text instance.
 */
bool text_mmaped(const Text*, const char *ptr);
/** @} */

#endif
