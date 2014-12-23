#ifndef TEXT_OBJECTS_H
#define TEXT_OBJECTS_H

/* these functions all take a file position. if this position is part of the
 * respective text-object, a corresponding range is returned. if there is no
 * such text-object at the given location, an empty range is returned.
 */

#include <stddef.h>
#include "text.h"

/* word which happens to be at pos without any neighbouring white spaces */
Filerange text_object_word(Text*, size_t pos);
/* includes trailing white spaces. if at pos happens to be a white space
 * include all neighbouring leading white spaces and the following word. */
Filerange text_object_word_outer(Text*, size_t pos);
/* same semantics as above but for a longword (i.e. delimited by white spaces) */
Filerange text_object_longword(Text*, size_t pos);
Filerange text_object_longword_outer(Text*, size_t pos);

Filerange text_object_line(Text*, size_t pos);
Filerange text_object_sentence(Text*, size_t pos);
Filerange text_object_paragraph(Text*, size_t pos);

/* these are inner text objects i.e. the delimiters themself are not
 * included in the range */
Filerange text_object_square_bracket(Text*, size_t pos);
Filerange text_object_curly_bracket(Text*, size_t pos);
Filerange text_object_angle_bracket(Text*, size_t pos);
Filerange text_object_paranthese(Text*, size_t pos);
Filerange text_object_quote(Text*, size_t pos);
Filerange text_object_single_quote(Text*, size_t pos);
Filerange text_object_backtick(Text*, size_t pos);

#endif
