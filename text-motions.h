#ifndef TEXT_MOTIONS_H
#define TEXT_MOTIONS_H

/* these function all take a position in bytes from the start of the file,
 * perform a certain movement and return the new postion. if the movement
 * is not possible the original position is returned unchanged. */

#include <stddef.h>
#include "text.h"

size_t text_begin(Text*, size_t pos);
size_t text_end(Text*, size_t pos);

/* move to start of next / previous UTF-8 character */
size_t text_char_next(Text*, size_t pos);
size_t text_char_prev(Text*, size_t pos);

/* find the given substring either in forward or backward direction.
 * does not wrap around at file start / end. if no match is found return
 * original position */
size_t text_find_char_next(Text*, size_t pos, const char *s, size_t len);
size_t text_find_char_prev(Text*, size_t pos, const char *s, size_t len);

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
/* A section begins after a form-feed in the first column.
size_t text_section_next(Text*, size_t pos);
size_t text_section_prev(Text*, size_t pos);
*/
/* search coresponding '(', ')', '{', '}', '[', ']', '>', '<', '"', ''' */
size_t text_bracket_match(Text*, size_t pos);

/* search the given regex pattern in either forward or backward direction,
 * starting from pos. does wrap around if no match was found. */
size_t text_search_forward(Text *txt, size_t pos, Regex *regex);
size_t text_search_backward(Text *txt, size_t pos, Regex *regex);

/* is c a special symbol delimiting a word? */
int is_word_boundry(int c);

#endif
