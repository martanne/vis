#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file
 *
 * A dynamically growing array, there exist two typical ways to use it:
 *
 *  1. To hold pointers to externally allocated memory regions.
 *
 *     Use `array_init` for initialization, an element has the size of a
 *     pointer. Use the functions suffixed with ``_ptr`` to manage your
 *     pointers. The cleanup function `array_release_full` must only be
 *     used with this type of array.
 *
 *  2. To hold arbitrary sized objects.
 *
 *     Use `array_init_sized` to specify the size of a single element.
 *     Use the regular (i.e. without the ``_ptr`` suffix) functions to
 *     manage your objects. Functions like `array_add` and `array_set`
 *     will copy the object into the array, `array_get` will return a
 *     pointer to the object stored within the array.
 */
/** A dynamically growing array. */
typedef struct {
	char *items;      /** Data pointer, NULL if empty. */
	size_t elem_size; /** Size of one array element. */
	size_t len;       /** Number of currently stored items. */
	size_t count;     /** Maximal capacity of the array. */
} Array;

/**
 * Initialize an Array object to store pointers.
 * @rst
 * .. note:: Is equivalent to ``array_init_sized(arr, sizeof(void*))``.
 * @endrst
 */
void array_init(Array*);
/**
 * Initialize an Array object to store arbitrarily sized objects.
 */
void array_init_sized(Array*, size_t elem_size);
/** Initialize Array by using the same element size as in ``from``. */
void array_init_from(Array*, const Array *from);
/** Release storage space. Reinitializes Array object. */
void array_release(Array*);
/**
 * Release storage space and call `free(3)` for each stored pointer.
 * @rst
 * .. warning:: Assumes array elements to be pointers.
 * @endrst
 */
void array_release_full(Array*);
/** Empty array, keep allocated memory. */
void array_clear(Array*);
/** Reserve memory to store at least ``count`` elements. */
bool array_reserve(Array*, size_t count);
/**
 * Get array element.
 * @rst
 * .. warning:: Returns a pointer to the allocated array region.
 *              Operations which might cause reallocations (e.g. the insertion
 *              of new elements) might invalidate the pointer.
 * @endrst
 */
void *array_get(const Array*, size_t idx);
/**
 * Set array element.
 * @rst
 * .. note:: Copies the ``item`` into the Array. If ``item`` is ``NULL``
 *           the corresponding memory region will be cleared.
 * @endrst
 */
bool array_set(Array*, size_t idx, void *item);
/** Dereference pointer stored in array element. */
void *array_get_ptr(const Array*, size_t idx);
/** Store the address to which ``item`` points to into the array. */
bool array_set_ptr(Array*, size_t idx, void *item);
/** Add element to the end of the array. */
bool array_add(Array*, void *item);
/** Add pointer to the end of the array. */
bool array_add_ptr(Array*, void *item);
/**
 * Remove an element by index.
 * @rst
 * .. note:: Might not shrink underlying memory region.
 * @endrst
 */
bool array_remove(Array*, size_t idx);
/** Number of elements currently stored in the array. */
size_t array_length(const Array*);
/** Number of elements which can be stored without enlarging the array. */
size_t array_capacity(const Array*);
/** Remove all elements with index greater or equal to ``length``, keep allocated memory. */
bool array_truncate(Array*, size_t length);
/**
 * Change length.
 * @rst
 * .. note:: Has to be less or equal than the capacity.
 *           Newly accessible elements preserve their previous values.
 * @endrst
 */
bool array_resize(Array*, size_t length);
/**
 * Sort array, the comparision function works as for `qsort(3)`.
 */
void array_sort(Array*, int (*compar)(const void*, const void*));
/**
 * Push item onto the top of the stack.
 * @rst
 * .. note:: Is equivalent to ``array_add(arr, item)``.
 * @endrst
 */
bool array_push(Array*, void *item);
/**
 * Get and remove item at the top of the stack.
 * @rst
 * .. warning:: The same ownership rules as for ``array_get`` apply.
 * @endrst
 */
void *array_pop(Array*);
/**
 * Get item at the top of the stack without removing it.
 * @rst
 * .. warning:: The same ownership rules as for ``array_get`` apply.
 * @endrst
 */
void *array_peek(const Array*);

#endif
