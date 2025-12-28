#ifndef TEXT_H
#define TEXT_H

#include "util.h"

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

typedef struct {
	Filerange  *data;
	VisDACount  count;
	VisDACount  capacity;
} FilerangeList;

/**
 * Text object storing the buffer content being edited.
 */
typedef struct Text Text;
typedef struct Piece Piece;

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
 * @defgroup load Text Loading
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
	 * Memory map the file from disk. Use file system / virtual memory
	 * subsystem as a caching layer.
	 * @rst
	 * .. note:: Load time is (almost) independent of the file size.
	 * .. warning:: Inplace modifications of the underlying file
	 *              will be reflected in the current text content.
	 *              In particular, truncation will raise ``SIGBUS``
	 *              and result in data loss.
	 * @endrst
	 */
	TEXT_LOAD_MMAP,
};
/**
 * Create a text instance populated with the given file content.
 *
 * @rst
 * .. note:: Equivalent to ``text_load_method(vis, filename, TEXT_LOAD_AUTO)``.
 * @endrst
 */
VIS_INTERNAL Text *text_load(Vis *vis, const char *filename);
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
VIS_INTERNAL Text *text_load_method(Vis *vis, const char *filename, enum TextLoadMethod method);
VIS_INTERNAL Text *text_loadat_method(Vis *vis, int dirfd, const char *filename, enum TextLoadMethod);
/** Release all resources associated with this text instance. */
VIS_INTERNAL void text_free(Text*);
/**
 * @}
 * @defgroup state Text State
 * @{
 */
/** Return the size in bytes of the whole text. */
VIS_INTERNAL size_t text_size(const Text*);
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
VIS_INTERNAL struct stat text_stat(const Text*);
/** Query whether the text contains any unsaved modifications. */
VIS_INTERNAL bool text_modified(const Text*);
/**
 * @}
 * @defgroup modify Text Modification
 * @{
 */
/**
 * Insert data at the given byte position.
 *
 * @param vis The editor instance.
 * @param txt The text instance to modify.
 * @param pos The absolute byte position.
 * @param data The data to insert.
 * @param len The length of the data in bytes.
 * @return Whether the insertion succeeded.
 */
VIS_INTERNAL bool text_insert(Vis *vis, Text *txt, size_t pos, const char *data, size_t len);
/**
 * Delete data at given byte position.
 *
 * @param txt The text instance to modify.
 * @param pos The absolute byte position.
 * @param len The number of bytes to delete, starting from ``pos``.
 * @return Whether the deletion succeeded.
 */
VIS_INTERNAL bool text_delete(Text *txt, size_t pos, size_t len);
VIS_INTERNAL bool text_delete_range(Text *txt, const Filerange*);
VIS_INTERNAL bool text_appendf(Vis *vis, Text *txt, const char *format, ...) __attribute__((format(printf, 3, 4)));
/**
 * @}
 * @defgroup history Undo/Redo History
 * @{
 */
/**
 * Create a text snapshot, that is a vertex in the history graph.
 */
VIS_INTERNAL void text_snapshot(Text*);
/**
 * Revert to previous snapshot along the main branch.
 * @rst
 * .. note:: Takes an implicit snapshot.
 * @endrst
 * @return The position of the first change or ``EPOS``, if already at the
 *         oldest state i.e. there was nothing to undo.
 */
VIS_INTERNAL size_t text_undo(Text*);
/**
 * Reapply an older change along the main branch.
 * @rst
 * .. note:: Takes an implicit snapshot.
 * @endrst
 * @return The position of the first change or ``EPOS``, if already at the
 *         newest state i.e. there was nothing to redo.
 */
VIS_INTERNAL size_t text_redo(Text*);
VIS_INTERNAL size_t text_earlier(Text*);
VIS_INTERNAL size_t text_later(Text*);
/**
 * Restore the text to the state closest to the time given
 */
VIS_INTERNAL size_t text_restore(Text*, time_t);
/**
 * Get creation time of current state.
 * @rst
 * .. note:: TODO: This is currently not the same as the time of the last snapshot.
 * @endrst
 */
VIS_INTERNAL time_t text_state(const Text*);
/**
 * @}
 * @defgroup lines Line Operations
 * @{
 */
VIS_INTERNAL size_t text_pos_by_lineno(Text*, size_t lineno);
VIS_INTERNAL size_t text_lineno_by_pos(Text*, size_t pos);

/**
 * @}
 * @defgroup access Text Access
 * @{
 */
/**
 * Get byte stored at ``pos``.
 * @param txt The text instance to modify.
 * @param pos The absolute position.
 * @param byte Destination address to store the byte.
 * @return Whether ``pos`` was valid and ``byte`` updated accordingly.
 * @rst
 * .. note:: Unlike :c:func:`text_iterator_byte_get()` this function does not
 *           return an artificial NUL byte at EOF.
 * @endrst
 */
VIS_INTERNAL bool text_byte_get(const Text *txt, size_t pos, char *byte);
/**
 * Store at most ``len`` bytes starting from ``pos`` into ``buf``.
 * @param txt The text instance to modify.
 * @param pos The absolute starting position.
 * @param len The length in bytes.
 * @param buf The destination buffer.
 * @return The number of bytes (``<= len``) stored at ``buf``.
 * @rst
 * .. warning:: ``buf`` will not be NUL terminated.
 * @endrst
 */
VIS_INTERNAL size_t text_bytes_get(const Text *txt, size_t pos, size_t len, char *buf);
/**
 * Fetch text range into newly allocate memory region.
 * @param txt The text instance to modify.
 * @param pos The absolute starting position.
 * @param len The length in bytes.
 * @return A contiguous NUL terminated buffer holding the requested range, or
 *         ``NULL`` in error case.
 * @rst
 * .. warning:: The returned pointer must be freed by the caller.
 * @endrst
 */
VIS_INTERNAL char *text_bytes_alloc0(const Text *txt, size_t pos, size_t len);
/**
 * @}
 * @defgroup iterator Text Iterators
 * @{
 */
VIS_INTERNAL Iterator text_iterator_get(const Text*, size_t pos);
VIS_INTERNAL bool text_iterator_init(const Text*, Iterator*, size_t pos);
VIS_INTERNAL const Text *text_iterator_text(const Iterator*);
VIS_INTERNAL bool text_iterator_valid(const Iterator*);
VIS_INTERNAL bool text_iterator_has_next(const Iterator*);
VIS_INTERNAL bool text_iterator_has_prev(const Iterator*);
VIS_INTERNAL bool text_iterator_next(Iterator*);
VIS_INTERNAL bool text_iterator_prev(Iterator*);
/**
 * @}
 * @defgroup iterator_byte Byte Iterators
 * @{
 */
VIS_INTERNAL bool text_iterator_byte_get(const Iterator*, char *b);
VIS_INTERNAL bool text_iterator_byte_prev(Iterator*, char *b);
VIS_INTERNAL bool text_iterator_byte_next(Iterator*, char *b);
VIS_INTERNAL bool text_iterator_byte_find_prev(Iterator*, char b);
VIS_INTERNAL bool text_iterator_byte_find_next(Iterator*, char b);
/**
 * @}
 * @defgroup iterator_code Codepoint Iterators
 * @{
 */
VIS_INTERNAL bool text_iterator_codepoint_next(Iterator *it, char *c);
VIS_INTERNAL bool text_iterator_codepoint_prev(Iterator *it, char *c);
/**
 * @}
 * @defgroup iterator_char Character Iterators
 * @{
 */
VIS_INTERNAL bool text_iterator_char_next(Iterator*, char *c);
VIS_INTERNAL bool text_iterator_char_prev(Iterator*, char *c);
/**
 * @}
 * @defgroup mark Marks
 * @{
 */
/**
 * Set a mark.
 * @rst
 * .. note:: Setting a mark to ``text_size`` will always return the
 *           current text size upon lookup.
 * @endrst
 * @param txt The text instance to modify.
 * @param pos The position at which to store the mark.
 * @return The mark or ``EMARK`` if an invalid position was given.
 */
VIS_INTERNAL Mark text_mark_set(Text *txt, size_t pos);
/**
 * Lookup a mark.
 * @param txt The text instance to modify.
 * @param mark The mark to look up.
 * @return The byte position or ``EPOS`` for an invalid mark.
 */
VIS_INTERNAL size_t text_mark_get(const Text *txt, Mark mark);
/**
 * @}
 * @defgroup save Text Saving
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
	 *   - POSIX ACL can not be preserved (if enabled).
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

/* used to hold context between text_save_{begin,commit} calls */
typedef struct {
	enum TextSaveMethod method; /* method used to save file */
	Text *txt;                 /* text to operate on */
	const char *filename;      /* filename to save to */
	str8 tmpname;              /* temporary name used for atomic rename(2) */
	int fd;                    /* file descriptor to write data to using text_save_write */
	int dirfd;                 /* directory file descriptor, relative to which we save */
} TextSave;

#define text_save_default(...) (TextSave){.dirfd = AT_FDCWD, .fd = -1, __VA_ARGS__}

/**
 * Marks the current text revision as saved.
 */
VIS_INTERNAL void text_mark_current_revision(Text*);

/**
 * Setup a sequence of write operations.
 *
 * The returned ``TextSave`` pointer can be used to write multiple, possibly
 * non-contiguous, file ranges.
 * @rst
 * .. warning:: For every call to ``text_save_begin`` there must be exactly
 *              one matching call to either ``text_save_commit`` or
 *              ``text_save_cancel`` to release the underlying resources.
 * @endrst
 */
VIS_INTERNAL bool text_save_begin(TextSave*);
/**
 * Write file range.
 * @return The number of bytes written or ``-1`` in case of an error.
 */
VIS_INTERNAL ssize_t text_save_write_range(TextSave*, const Filerange*);
/**
 * Commit changes to disk.
 * @return Whether changes have been saved.
 * @rst
 * .. note:: Releases the underlying resources and frees the given ``TextSave``
 *           pointer which must no longer be used.
 * @endrst
 */
VIS_INTERNAL bool text_save_commit(TextSave*);
/**
 * Abort a save operation.
 * @rst
 * .. note:: Does not guarantee to undo the previous writes (they might have been
 *           performed in-place). However, it releases the underlying resources and
 *           frees the given ``TextSave`` pointer which must no longer be used.
 * @endrst
 */
VIS_INTERNAL void text_save_cancel(TextSave*);
/**
 * Write file range to file descriptor.
 * @return The number of bytes written or ``-1`` in case of an error.
 */
VIS_INTERNAL ssize_t text_write_range(const Text*, const Filerange*, int fd);
/**
 * @}
 * @defgroup misc Miscellaneous
 * @{
 */
/**
 * Check whether ``ptr`` is part of a memory mapped region associated with
 * this text instance.
 */
VIS_INTERNAL bool text_mmaped(const Text*, const char *ptr);

/**
 * Write complete buffer to file descriptor.
 * @return The number of bytes written or ``-1`` in case of an error.
 */
VIS_INTERNAL ssize_t write_all(int fd, const char *buf, size_t count);
/** @} */

/*
 * @defgroup regex Text Regular Expression Handling
 * @{
 */

/* make the REG_* constants available */
#if CONFIG_TRE
  #include <tre/tre.h>
#else
  #include <regex.h>
#endif

#define MAX_REGEX_SUB 10

typedef struct Regex Regex;
typedef Filerange RegexMatch;

VIS_INTERNAL Regex *text_regex_new(void);
VIS_INTERNAL int text_regex_compile(Regex*, const char *pattern, int cflags);
VIS_INTERNAL size_t text_regex_nsub(Regex*);
VIS_INTERNAL void text_regex_free(Regex*);
VIS_INTERNAL int text_regex_match(Regex*, const char *data, int eflags);
VIS_INTERNAL int text_search_range_forward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);
VIS_INTERNAL int text_search_range_backward(Text*, size_t pos, size_t len, Regex *r, size_t nmatch, RegexMatch pmatch[], int eflags);

/** @} */

/*
 * @defgroup motions Text Motions
 * @{
 */

/* these functions all take a position in bytes from the start of the file,
 * perform a certain movement and return the new position. If the movement
 * is not possible the original position is returned unchanged. */

/* char refers to a grapheme (might skip over multiple Unicode codepoints) */
VIS_INTERNAL size_t text_char_next(Text*, size_t pos);
VIS_INTERNAL size_t text_char_prev(Text*, size_t pos);

VIS_INTERNAL size_t text_codepoint_next(Text*, size_t pos);
VIS_INTERNAL size_t text_codepoint_prev(Text*, size_t pos);

/* find the given substring either in forward or backward direction.
 * does not wrap around at file start / end. If no match is found return
 * original position */
VIS_INTERNAL size_t text_find_next(Text*, size_t pos, const char *s);
VIS_INTERNAL size_t text_find_prev(Text*, size_t pos, const char *s);
/* same as above but limit searched range to the line containing pos */
VIS_INTERNAL size_t text_line_find_next(Text*, size_t pos, const char *s);
VIS_INTERNAL size_t text_line_find_prev(Text*, size_t pos, const char *s);

/*    begin            finish    next
 *    v                v         v
 *  \n      I am a line!       \n
 *  ^       ^                  ^
 *  prev    start              end
 */
VIS_INTERNAL size_t text_line_prev(Text*, size_t pos);
VIS_INTERNAL size_t text_line_begin(Text*, size_t pos);
VIS_INTERNAL size_t text_line_start(Text*, size_t pos);
VIS_INTERNAL size_t text_line_finish(Text*, size_t pos);
VIS_INTERNAL size_t text_line_end(Text*, size_t pos);
VIS_INTERNAL size_t text_line_next(Text*, size_t pos);
VIS_INTERNAL size_t text_line_offset(Text*, size_t pos, size_t off);
/* get grapheme count of the line upto `pos' */
VIS_INTERNAL int text_line_char_get(Text*, size_t pos);
/* get position of the `count' grapheme in the line containing `pos' */
VIS_INTERNAL size_t text_line_char_set(Text*, size_t pos, int count);
/* get display width of line upto `pos' */
VIS_INTERNAL int text_line_width_get(Text*, size_t pos);
/* get position of character being displayed at `width' in line containing `pos' */
VIS_INTERNAL size_t text_line_width_set(Text*, size_t pos, int width);
/* move to the next/previous grapheme on the same line */
VIS_INTERNAL size_t text_line_char_next(Text*, size_t pos);
VIS_INTERNAL size_t text_line_char_prev(Text*, size_t pos);
/* move to start of next/previous blank line */
VIS_INTERNAL size_t text_line_blank_next(Text*, size_t pos);
VIS_INTERNAL size_t text_line_blank_prev(Text*, size_t pos);
/* move to same offset in previous/next line */
VIS_INTERNAL size_t text_line_up(Text*, size_t pos);
VIS_INTERNAL size_t text_line_down(Text*, size_t pos);
/* functions to iterate over all line beginnings in a given range */
VIS_INTERNAL size_t text_range_line_first(Text*, Filerange*);
VIS_INTERNAL size_t text_range_line_next(Text*, Filerange*, size_t pos);
/*
 * A longword consists of a sequence of non-blank characters, separated with
 * white space. TODO?: An empty line is also considered to be a word.
 * This is equivalent to a WORD in vim terminology.
 */
VIS_INTERNAL size_t text_longword_end_next(Text*, size_t pos);
VIS_INTERNAL size_t text_longword_end_prev(Text*, size_t pos);
VIS_INTERNAL size_t text_longword_start_next(Text*, size_t pos);
VIS_INTERNAL size_t text_longword_start_prev(Text*, size_t pos);
/*
 * A word consists of a sequence of letters, digits and underscores, or a
 * sequence of other non-blank characters, separated with white space.
 * TODO?: An empty line is also considered to be a word.
 * This is equivalent to a word (lowercase) in vim terminology.
 */
VIS_INTERNAL size_t text_word_end_next(Text*, size_t pos);
VIS_INTERNAL size_t text_word_end_prev(Text*, size_t pos);
VIS_INTERNAL size_t text_word_start_next(Text*, size_t pos);
VIS_INTERNAL size_t text_word_start_prev(Text*, size_t pos);
/*
 * More general versions of the above, define your own word boundaries.
 */
VIS_INTERNAL size_t text_customword_start_next(Text*, size_t pos, int (*isboundary)(int));
VIS_INTERNAL size_t text_customword_start_prev(Text*, size_t pos, int (*isboundary)(int));
VIS_INTERNAL size_t text_customword_end_next(Text*, size_t pos, int (*isboundary)(int));
VIS_INTERNAL size_t text_customword_end_prev(Text*, size_t pos, int (*isboundary)(int));
/* TODO: implement the following semantics
 * A sentence is defined as ending at a '.', '!' or '?' followed by either the
 * end of a line, or by a space or tab.  Any number of closing ')', ']', '"'
 * and ''' characters may appear after the '.', '!' or '?' before the spaces,
 * tabs or end of line.  A paragraph and section boundary is also a sentence
 * boundary.
 */
VIS_INTERNAL size_t text_sentence_next(Text*, size_t pos);
VIS_INTERNAL size_t text_sentence_prev(Text*, size_t pos);
/* TODO: implement the following semantics
 * A paragraph begins after each empty line. A section boundary is also a
 * paragraph boundary. Note that a blank line (only containing white space)
 * is NOT a paragraph boundary.
 */
VIS_INTERNAL size_t text_paragraph_next(Text*, size_t pos);
VIS_INTERNAL size_t text_paragraph_prev(Text*, size_t pos);
/* A section begins after a form-feed in the first column.
size_t text_section_next(Text*, size_t pos);
size_t text_section_prev(Text*, size_t pos);
*/
VIS_INTERNAL size_t text_block_start(Text*, size_t pos);
VIS_INTERNAL size_t text_block_end(Text*, size_t pos);
VIS_INTERNAL size_t text_parenthesis_start(Text*, size_t pos);
VIS_INTERNAL size_t text_parenthesis_end(Text*, size_t pos);
/* search corresponding '(', ')', '{', '}', '[', ']', '>', '<', '"', ''' */
VIS_INTERNAL size_t text_bracket_match(Text*, size_t pos, const Filerange *limits);
/* same as above but explicitly specify symbols to match */
VIS_INTERNAL size_t text_bracket_match_symbol(Text*, size_t pos, const char *symbols, const Filerange *limits);

/* search the given regex pattern in either forward or backward direction,
 * starting from pos. Does wrap around if no match was found. */
VIS_INTERNAL size_t text_search_forward(Text *txt, size_t pos, Regex *regex);
VIS_INTERNAL size_t text_search_backward(Text *txt, size_t pos, Regex *regex);

/* is c a special symbol delimiting a word? */
VIS_INTERNAL int is_word_boundary(int c);

/** @} */

/*
 * @defgroup objects Text Objects
 * @{
 */

/* these functions all take a file position. If this position is part of the
 * respective text-object, a corresponding range is returned. If there is no
 * such text-object at the given location, an empty range is returned.
 */

/* return range covering the entire text */
VIS_INTERNAL Filerange text_object_entire(Text*, size_t pos);
/* word which happens to be at pos without any neighbouring white spaces */
VIS_INTERNAL Filerange text_object_word(Text*, size_t pos);
/* includes trailing white spaces. If at pos happens to be a white space
 * include all neighbouring leading white spaces and the following word. */
VIS_INTERNAL Filerange text_object_word_outer(Text*, size_t pos);
/* find next occurrence of `word' (as word not substring) in forward/backward direction */
VIS_INTERNAL Filerange text_object_word_find_next(Text*, size_t pos, const char *word);
VIS_INTERNAL Filerange text_object_word_find_prev(Text*, size_t pos, const char *word);
/* find next occurrence of a literal string (not regex) in forward/backward direction */
VIS_INTERNAL Filerange text_object_find_next(Text *txt, size_t pos, const char *search);
VIS_INTERNAL Filerange text_object_find_prev(Text *txt, size_t pos, const char *search);
/* same semantics as above but for a longword (i.e. delimited by white spaces) */
VIS_INTERNAL Filerange text_object_longword(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_longword_outer(Text*, size_t pos);

VIS_INTERNAL Filerange text_object_line(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_line_inner(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_sentence(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_paragraph(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_paragraph_outer(Text*, size_t pos);

/* these are inner text objects i.e. the delimiters themself are not
 * included in the range */
VIS_INTERNAL Filerange text_object_square_bracket(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_curly_bracket(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_angle_bracket(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_parenthesis(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_quote(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_single_quote(Text*, size_t pos);
VIS_INTERNAL Filerange text_object_backtick(Text*, size_t pos);
/* match a search term in either forward or backward direction */
VIS_INTERNAL Filerange text_object_search_forward(Text*, size_t pos, Regex*);
VIS_INTERNAL Filerange text_object_search_backward(Text*, size_t pos, Regex*);
/* match all lines with same indentation level as the current one */
VIS_INTERNAL Filerange text_object_indentation(Text*, size_t pos);

/* extend a range to cover whole lines */
VIS_INTERNAL Filerange text_range_linewise(Text*, Filerange*);
/* trim leading and trailing white spaces from range */
VIS_INTERNAL Filerange text_range_inner(Text*, Filerange*);
/* test whether a given range covers whole lines */
VIS_INTERNAL bool text_range_is_linewise(Text*, Filerange*);

/** @} */

/*
 * @defgroup util Utilities
 * @{
 */

/* test whether the given range is valid (start <= end) */
#define text_range_valid(r) ((r)->start != EPOS && (r)->end != EPOS && (r)->start <= (r)->end)
/* get the size of the range (end-start) or zero if invalid */
#define text_range_size(r) (text_range_valid(r) ? (r)->end - (r)->start : 0)
/* create an empty / invalid range of size zero */
#define text_range_empty() (Filerange){.start = EPOS, .end = EPOS}
/* merge two ranges into a new one which contains both of them */
VIS_INTERNAL Filerange text_range_union(const Filerange*, const Filerange*);
/* get intersection of two ranges */
VIS_INTERNAL Filerange text_range_intersect(const Filerange*, const Filerange*);
/* create new range [min(a,b), max(a,b)] */
VIS_INTERNAL Filerange text_range_new(size_t a, size_t b);
/* test whether two ranges are equal */
VIS_INTERNAL bool text_range_equal(const Filerange*, const Filerange*);
/* test whether two ranges overlap */
VIS_INTERNAL bool text_range_overlap(const Filerange*, const Filerange*);
/* test whether a given position is within a certain range */
VIS_INTERNAL bool text_range_contains(const Filerange*, size_t pos);
/* count the number of graphemes in data */
VIS_INTERNAL int text_char_count(const char *data, size_t len);
/* get the approximate display width of data */
VIS_INTERNAL int text_string_width(const char *data, size_t len);

/** @} */

#endif
