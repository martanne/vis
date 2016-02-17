#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "array.h"
#include "util.h"

#define ARRAY_SIZE 16

void array_init(Array *arr) {
	memset(arr, 0, sizeof *arr);
}

bool array_reserve(Array *arr, size_t count) {
	if (count < ARRAY_SIZE)
		count = ARRAY_SIZE;
	if (arr->count < count) {
		count = MAX(count, arr->count*2);
		void **items = realloc(arr->items, count * sizeof(void*));
		if (!items)
			return false;
		arr->count = count;
		arr->items = items;
	}
	return true;
}

void array_release(Array *arr) {
	if (!arr)
		return;
	free(arr->items);
	array_init(arr);
}

void array_release_full(Array *arr) {
	if (!arr)
		return;
	for (size_t i = 0; i < arr->len; i++)
		free(arr->items[i]);
	array_release(arr);
}

void array_clear(Array *arr) {
	arr->len = 0;
	if (arr->items)
		memset(arr->items, 0, arr->count * sizeof(void*));
}

void *array_get(Array *arr, size_t idx) {
	if (idx >= arr->len) {
		errno = EINVAL;
		return NULL;
	}
	return arr->items[idx];
}

bool array_set(Array *arr, size_t idx, void *item) {
	if (idx >= arr->len) {
		errno = EINVAL;
		return false;
	}
	arr->items[idx] = item;
	return true;
}

bool array_add(Array *arr, void *item) {
	if (!array_reserve(arr, arr->len+1))
		return false;
	arr->items[arr->len++] = item;;
	return true;
}

size_t array_length(Array *arr) {
	return arr->len;
}
