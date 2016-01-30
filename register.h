#ifndef REGISTER_H
#define REGISTER_H

#include <stddef.h>
#include <stdbool.h>
#include "buffer.h"
#include "text-util.h"

#ifndef VIS_H
typedef struct Vis Vis;
#endif

typedef struct {
	Buffer buf;
	bool linewise; /* place register content on a new line when inserting? */
	enum {
		REGISTER_NORMAL,
		REGISTER_BLACKHOLE,
		REGISTER_CLIPBOARD,
	} type;
} Register;

void register_release(Register *reg);
const char *register_get(Vis*, Register*reg, size_t *len);
bool register_put(Vis*, Register *reg, Text *txt, Filerange *range);
bool register_append(Register *reg, Text *txt, Filerange *range);

#endif
