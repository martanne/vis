#ifndef REGISTER_H
#define REGISTER_H

#include <stddef.h>
#include <stdbool.h>
#include "buffer.h"

/* definition has to match Buffer */
typedef struct {
	char *data;    /* NULL if empty */
	size_t len;    /* current length of data */
	size_t size;   /* maximal capacity of the register */
	bool linewise; /* place register content on a new line when inserting? */
} Register;

void register_free(Register *reg);
bool register_put(Register *reg, Text *txt, Filerange *range);
bool register_append(Register *reg, Text *txt, Filerange *range);

#endif
