#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {       /* a dynamically growing array */
	void **items;  /* NULL if empty */
	size_t len;    /* number of currently stored items */
	size_t count;  /* maximal capacity of the array */
} Array;

/* initalize a (stack allocated) Array instance */
void array_init(Array*);
/* release/free the storage space used to hold items, reset size to zero */
void array_release(Array*);
/* like above but also call free(3) for each stored pointer */
void array_release_full(Array*);
/* remove all elements, set array length to zero, keep allocated memory */
void array_clear(Array*);
/* reserve space to store at least `count' elements in this aray */
bool array_reserve(Array*, size_t count);
/* get/set the i-th (zero based) element of the array */
void *array_get(Array*, size_t idx);
bool array_set(Array*, size_t idx, void *item);
/* add a new element to the end of the array */
bool array_add(Array*, void *item);
/* return the number of elements currently stored in the array */
size_t array_length(Array*);

#endif
