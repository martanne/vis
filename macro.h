#ifndef MACRO_H
#define MACRO_H

#include "buffer.h"

typedef Buffer Macro;
#define macro_release buffer_release
#define macro_reset buffer_truncate
#define macro_append buffer_append

#endif
