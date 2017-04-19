#ifndef REGISTER_H
#define REGISTER_H

#include <stddef.h>
#include <stdbool.h>
#include "vis.h"
#include "array.h"
#include "text-util.h"

typedef struct {
	Array values;
	bool linewise; /* place register content on a new line when inserting? */
	bool append;
	enum {
		REGISTER_NORMAL,
		REGISTER_BLACKHOLE,
		REGISTER_CLIPBOARD,
	} type;
} Register;

bool register_init(Register*);
void register_release(Register*);

const char *register_get(Vis*, Register*, size_t *len);
const char *register_slot_get(Vis*, Register*, size_t slot, size_t *len);

bool register_put0(Vis*, Register*, const char *data);
bool register_put(Vis*, Register*, const char *data, size_t len);
bool register_slot_put(Vis*, Register*, size_t slot, const char *data, size_t len);

bool register_put_range(Vis*, Register*, Text*, Filerange*);
bool register_slot_put_range(Vis*, Register*, size_t slot, Text*, Filerange*);

size_t register_count(Register*);
bool register_resize(Register*, size_t count);

#endif
