#ifndef REGISTER_H
#define REGISTER_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"

typedef struct Register Register;

bool register_alloc(Register *reg, size_t size);
void register_free(Register *reg); 
bool register_store(Register *reg, const char *data, size_t len);
bool register_put(Register *reg, Text *txt, Filerange *range);
bool register_append(Register *reg, Text *txt, Filerange *range); 

#endif
