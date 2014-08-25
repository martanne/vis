#ifndef TEXT_OBJECTS_H
#define TEXT_OBJECTS_H

#include <stddef.h>
#include "text.h"

Filerange text_object_word(Text*, size_t pos);
Filerange text_object_word_boundry(Text*, size_t pos, int (*isboundry)(int));
Filerange text_object_char(Text*, size_t pos, char c);
Filerange text_object_sentence(Text*, size_t pos);
Filerange text_object_paragraph(Text*, size_t pos);

#endif
