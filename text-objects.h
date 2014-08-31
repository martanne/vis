#ifndef TEXT_OBJECTS_H
#define TEXT_OBJECTS_H

#include <stddef.h>
#include "text.h"

/* word which happens to be at pos, includes trailing white spaces. if at pos happens to
 * be a whitespace include all neighbouring leading whitespaces and the following word. */
Filerange text_object_word(Text*, size_t pos);
Filerange text_object_word_boundry(Text*, size_t pos, int (*isboundry)(int));
Filerange text_object_sentence(Text*, size_t pos);
Filerange text_object_paragraph(Text*, size_t pos);

/* the delimiters themself are not included in the range */
Filerange text_object_square_bracket(Text*, size_t pos);
Filerange text_object_curly_bracket(Text*, size_t pos);
Filerange text_object_angle_bracket(Text*, size_t pos);
Filerange text_object_paranthese(Text*, size_t pos);
Filerange text_object_quote(Text*, size_t pos);
Filerange text_object_single_quote(Text*, size_t pos);
Filerange text_object_backtick(Text*, size_t pos);

#endif
