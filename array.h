#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdbool.h>

/* A dynamically growing array, there exist two typical ways to use it:
 *
 *  1) to hold pointers to externally allocated memory regions
 *
 *     Use array_init(...) for initialization, an element has the
 *     size of a pointer. Use the functions suffixed with `_ptr'
 *     to manage your pointers. The cleanup function array_release_full
 *     must only be used with this type of array.
 *
 *  2) to hold arbitrary sized objects
 *
 *     Use array_init_sized(...) to specify the size of a single
 *     element. Use the regular (i.e. without the `_ptr' suffix)
 *     functions to manage your objects. array_{add,set} will copy
 *     the object into the array, array_get will return a pointer
 *     to the object stored within the array.
 */
typedef struct {          /* a dynamically growing array */
	char *items;      /* NULL if empty */
	size_t elem_size; /* size of one array element */
	size_t len;       /* number of currently stored items */
	size_t count;     /* maximal capacity of the array */
} Array;

/* initalize an already allocated Array instance, storing pointers
 * (elem_size == sizeof(void*)) */
void array_init(Array*);
/* initalize an already allocated Array instance, storing arbitrary
 * sized objects */
void array_init_sized(Array*, size_t elem_size);
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
void *array_get_ptr(Array*, size_t idx);
bool array_set(Array*, size_t idx, void *item);
bool array_set_ptr(Array*, size_t idx, void *item);
/* add a new element to the end of the array */
bool array_add(Array*, void *item);
bool array_add_ptr(Array*, void *item);
/* return the number of elements currently stored in the array */
size_t array_length(Array*);

#endif
