#ifndef TEXT_MOTIONS_H
#define TEXT_MOTIONS_H

/* these function all take a position in bytes from the start of the file,
 * perform a certain movement and return the new postion. if the movement
 * is not possible the original position is returned unchanged. */

#include <stddef.h>
#include "text.h"
#include "text-regex.h"

size_t text_begin(Text*, size_t pos);
size_t text_end(Text*, size_t pos);

/* char refers to a grapheme (might skip over multiple Unicode codepoints) */
size_t text_char_next(Text*, size_t pos);
size_t text_char_prev(Text*, size_t pos);

/* find the given substring either in forward or backward direction.
 * does not wrap around at file start / end. if no match is found return
 * original position */
size_t text_find_next(Text*, size_t pos, const char *s);
size_t text_find_prev(Text*, size_t pos, const char *s);
/* same as above but limit searched range to the line containing pos */
size_t text_line_find_next(Text*, size_t pos, const char *s);
size_t text_line_find_prev(Text*, size_t pos, const char *s);

/*        begin            finish  end   next
 *        v                v       v     v
 *  [\r]\n      I am a line!       [\r]\n
 *  ^           ^                 ^
 *  prev        start             lastchar
 */
size_t text_line_prev(Text*, size_t pos);
size_t text_line_begin(Text*, size_t pos);
size_t text_line_start(Text*, size_t pos);
size_t text_line_finish(Text*, size_t pos);
size_t text_line_lastchar(Text*, size_t pos);
size_t text_line_end(Text*, size_t pos);
size_t text_line_next(Text*, size_t pos);
size_t text_line_offset(Text*, size_t pos, size_t off);
/* get grapheme count of the line upto `pos' */
int text_line_char_get(Text*, size_t pos);
/* get position of the `count' grapheme in the line containing `pos' */
size_t text_line_char_set(Text*, size_t pos, int count);
/* get display width of line upto `pos' */
int text_line_width_get(Text*, size_t pos);
/* get position of character being displayed at `width' in line containing `pos' */
size_t text_line_width_set(Text*, size_t pos, int width);
/* move to the next/previous grapheme on the same line */
size_t text_line_char_next(Text*, size_t pos);
size_t text_line_char_prev(Text*, size_t pos);
/* move to the next/previous empty line */
size_t text_line_empty_next(Text*, size_t pos);
size_t text_line_empty_prev(Text*, size_t pos);
/* move to same offset in previous/next line */
size_t text_line_up(Text*, size_t pos);
size_t text_line_down(Text*, size_t pos);
/* functions to iterate over all line beginnings in a given range */
size_t text_range_line_first(Text*, Filerange*);
size_t text_range_line_last(Text*, Filerange*);
size_t text_range_line_next(Text*, Filerange*, size_t pos);
size_t text_range_line_prev(Text*, Filerange*, size_t pos);
/*
 * A longword consists of a sequence of non-blank characters, separated with
 * white space. TODO?: An empty line is also considered to be a word.
 * This is equivalant to a WORD in vim terminology.
 */
size_t text_longword_end_next(Text*, size_t pos);
size_t text_longword_end_prev(Text*, size_t pos);
size_t text_longword_start_next(Text*, size_t pos);
size_t text_longword_start_prev(Text*, size_t pos);
/*
 * A word consists of a sequence of letters, digits and underscores, or a
 * sequence of other non-blank characters, separated with white space.
 * TODO?: An empty line is also considered to be a word.
 * This is equivalant to a word (lowercase) in vim terminology.
 */
size_t text_word_end_next(Text*, size_t pos);
size_t text_word_end_prev(Text*, size_t pos);
size_t text_word_start_next(Text*, size_t pos);
size_t text_word_start_prev(Text*, size_t pos);
/*
 * More general versions of the above, define your own word boundaries.
 */
size_t text_customword_start_next(Text*, size_t pos, int (*isboundary)(int));
size_t text_customword_start_prev(Text*, size_t pos, int (*isboundary)(int));
size_t text_customword_end_next(Text*, size_t pos, int (*isboundary)(int));
size_t text_customword_end_prev(Text*, size_t pos, int (*isboundary)(int));
/* TODO: implement the following semantics
 * A sentence is defined as ending at a '.', '!' or '?' followed by either the
 * end of a line, or by a space or tab.  Any number of closing ')', ']', '"'
 * and ''' characters may appear after the '.', '!' or '?' before the spaces,
 * tabs or end of line.  A paragraph and section boundary is also a sentence
 * boundary.
 */
size_t text_sentence_next(Text*, size_t pos);
size_t text_sentence_prev(Text*, size_t pos);
/* TODO: implement the following semantics
 * A paragraph begins after each empty line. A section boundary is also a
 * paragraph boundary. Note that a blank line (only containing white space)
 * is NOT a paragraph boundary.
 */
size_t text_paragraph_next(Text*, size_t pos);
size_t text_paragraph_prev(Text*, size_t pos);
/* Find next/previous start/end of a C like function definition */
size_t text_function_start_next(Text*, size_t pos);
size_t text_function_start_prev(Text*, size_t pos);
size_t text_function_end_next(Text*, size_t pos);
size_t text_function_end_prev(Text*, size_t pos);
/* A section begins after a form-feed in the first column.
size_t text_section_next(Text*, size_t pos);
size_t text_section_prev(Text*, size_t pos);
*/
/* search coresponding '(', ')', '{', '}', '[', ']', '>', '<', '"', ''' */
size_t text_bracket_match(Text*, size_t pos);
/* same as above but explicitly specify symbols to match */
size_t text_bracket_match_symbol(Text*, size_t pos, const char *symbols);

/* search the given regex pattern in either forward or backward direction,
 * starting from pos. does wrap around if no match was found. */
size_t text_search_forward(Text *txt, size_t pos, Regex *regex);
size_t text_search_backward(Text *txt, size_t pos, Regex *regex);

/* is c a special symbol delimiting a word? */
int is_word_boundary(int c);

#endif
