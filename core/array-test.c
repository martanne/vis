#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "tap.h"
#include "array.h"
#include "util.h"

typedef struct {
	char key[64];
	int value;
} Item;

static int values[] = { 2, 3, 5, 7, 11 };
static const size_t len = LENGTH(values);

static bool item_compare(Item *a, Item *b) {
	return strcmp(a->key, b->key) == 0 && a->value == b->value;
}

static void test_small_objects(void) {
	Array arr;
	array_init_sized(&arr, sizeof(int));
	ok(array_length(&arr) == 0, "Initialization");
	ok(!array_set(&arr, 0, NULL) && errno == EINVAL, "Set with invalid index");
	ok(array_get(&arr, 0) == NULL && errno == EINVAL, "Get with invalid index");
	ok(array_peek(&arr) == NULL && array_length(&arr) == 0, "Peek empty array");
	ok(array_pop(&arr) == NULL && array_length(&arr) == 0, "Pop empty array");

	for (size_t i = 0; i < len; i++) {
		int *v;
		ok(array_add(&arr, &values[i]) && array_length(&arr) == i+1,
			"Add integer: %zu = %d", i, values[i]);
		ok((v = array_get(&arr, i)) && *v == values[i],
			"Get integer: %zu = %d", i, *v);
	}

	for (size_t i = 0; i < len; i++) {
		ok(array_set(&arr, i, &values[len-i-1]) && array_length(&arr) == len,
			"Set array element: %zu = %d", i, values[len-i-1]);
	}

	for (size_t i = 0; i < len; i++) {
		int *v;
		ok((v = array_get(&arr, i)) && *v == values[len-i-1],
			"Get array element: %zu = %d", i, *v);
	}

	int *v;
	ok((v = array_peek(&arr)) && *v == values[0] && array_length(&arr) == len, "Peek populated array");
	ok((v = array_pop(&arr)) && *v == values[0] && array_length(&arr) == len-1, "Pop populated array");
	ok((v = array_peek(&arr)) && *v == values[1] && array_length(&arr) == len-1, "Peek after pop");

	array_clear(&arr);
	ok(array_length(&arr) == 0 && array_get(&arr, 0) == NULL && errno == EINVAL, "Clear");

	for (size_t i = 0; i < len; i++) {
		ok(array_add(&arr, &values[i]) && array_length(&arr) == i+1,
			"Re-add integer: %zu = %d", i, values[i]);
	}

	int old, *tmp;
	ok((tmp = array_get(&arr, 0)) && (old = *tmp) && array_set(&arr, 0, NULL) &&
	    array_get(&arr, 0) == tmp && *tmp == 0 && array_set(&arr, 0, &old) &&
	    array_get(&arr, 0) == tmp && *tmp == old, "Set array element NULL");
	ok(!array_set(&arr, array_length(&arr), &values[0]) && errno == EINVAL, "Get past end of array");
	ok(!array_get(&arr, array_length(&arr)) && errno == EINVAL, "Get past end of array");

	ok(!array_remove(&arr, array_length(&arr)) && errno == EINVAL, "Remove past end of array");

	size_t len_before = array_length(&arr);
	ok(array_remove(&arr, 2) && array_length(&arr) == len_before-1 &&
	   (v = array_get(&arr, 0)) && *v == values[0] &&
	   (v = array_get(&arr, 1)) && *v == values[1] &&
	   (v = array_get(&arr, 2)) && *v == values[3] &&
	   (v = array_get(&arr, 3)) && *v == values[4],
	   "Remove element 2");

	len_before = array_length(&arr);
	ok(array_remove(&arr, 0) && array_length(&arr) == len_before-1 &&
	   (v = array_get(&arr, 0)) && *v == values[1] &&
	   (v = array_get(&arr, 1)) && *v == values[3] &&
	   (v = array_get(&arr, 2)) && *v == values[4],
	   "Remove first element");

	len_before = array_length(&arr);
	ok(array_remove(&arr, len_before-1) && array_length(&arr) == len_before-1 &&
	   (v = array_get(&arr, 0)) && *v == values[1] &&
	   (v = array_get(&arr, 1)) && *v == values[3],
	   "Remove last element");

	array_release(&arr);
}

static void test_large_objects(void) {
	Array arr;
	array_init_sized(&arr, sizeof(Item));
	ok(array_length(&arr) == 0 && array_get(&arr, 0) == NULL && errno == EINVAL,
		"Initialization");

	Item items[len];

	for (size_t i = 0; i < len; i++) {
		snprintf(items[i].key, sizeof items[i].key, "key: %zu", i);
		items[i].value = values[i];
		Item *item;
		ok(array_add(&arr, &items[i]) && array_length(&arr) == i+1,
			"Add item: %zu = { '%s' = %d }", i, items[i].key, items[i].value);
		ok((item = array_get(&arr, i)) && item != &items[i] && item_compare(item, &items[i]),
			"Get item: %zu = { '%s' = %d }", i, item->key, item->value);
	}

	for (size_t i = 0; i < len; i++) {
		Item *item = &items[len-i-1];
		ok(array_set(&arr, i, item) && array_length(&arr) == len,
			"Set array element: %zu = { '%s' = %d }", i, item->key, item->value);
	}

	for (size_t i = 0; i < len; i++) {
		Item *item;
		ok((item = array_get(&arr, i)) && item != &items[len-i-1] && item_compare(item, &items[len-i-1]),
			"Get item: %zu = { '%s' = %d }", i, item->key, item->value);
	}

	ok(!array_add_ptr(&arr, &items[0]) && errno == ENOTSUP && array_length(&arr) == len,
		"Adding pointer to non pointer array");
	ok(!array_set_ptr(&arr, 0, &items[0]) && errno == ENOTSUP && item_compare(array_get(&arr, 0), &items[len-1]),
		"Setting pointer in non pointer array");

	array_clear(&arr);
	ok(array_length(&arr) == 0 && array_get(&arr, 0) == NULL && errno == EINVAL, "Clear");

	array_release(&arr);
}

static void test_pointers(void) {

	Array arr;

	array_init_sized(&arr, 1);
	ok(array_length(&arr) == 0 && array_get_ptr(&arr, 0) == NULL && errno == ENOTSUP,
		"Initialization with size 1");

	ok(!array_add_ptr(&arr, &arr) && errno == ENOTSUP && array_get_ptr(&arr, 0) == NULL,
		"Add pointer to non-pointer array");

	errno = 0;
	char byte = '_', *ptr;
	ok(array_add(&arr, &byte) && (ptr = array_get(&arr, 0)) && *ptr == byte,
		"Add byte element");
	ok(!array_get_ptr(&arr, 0) && errno == ENOTSUP, "Get pointer from non-pointer array");
	array_release(&arr);

	array_init(&arr);
	ok(array_length(&arr) == 0 && array_get_ptr(&arr, 0) == NULL && errno == EINVAL,
		"Initialization");

	Item *items[len];

	for (size_t i = 0; i < len; i++) {
		items[i] = malloc(sizeof(Item));
		snprintf(items[i]->key, sizeof(items[i]->key), "key: %zu", i);
		items[i]->value = values[i];
	}

	for (size_t i = 0; i < len; i++) {
		Item *item;
		ok(array_add_ptr(&arr, items[i]) && array_length(&arr) == i+1,
			"Add item: %zu = %p", i, (void*)items[i]);
		ok((item = array_get_ptr(&arr, i)) && item == items[i],
			"Get item: %zu = %p", i, (void*)item);
	}

	for (size_t i = 0; i < len; i++) {
		Item *item = items[len-i-1];
		ok(array_set_ptr(&arr, i, item) && array_length(&arr) == len,
			"Set item: %zu = %p", i, (void*)item);
	}

	for (size_t i = 0; i < len; i++) {
		Item *item;
		ok((item = array_get_ptr(&arr, i)) && item == items[len-i-1],
			"Get item: %zu = %p", i, (void*)item);
	}

	Item *tmp;
	ok((tmp = array_get_ptr(&arr, 0)) && array_set_ptr(&arr, 0, NULL) &&
	    array_get_ptr(&arr, 0) == NULL && array_set_ptr(&arr, 0, tmp) &&
	    array_get_ptr(&arr, 0) == tmp, "Set pointer NULL");
	ok(!array_set_ptr(&arr, array_length(&arr), items[0]) && errno == EINVAL, "Set pointer past end of array");
	ok(!array_get_ptr(&arr, array_length(&arr)) && errno == EINVAL, "Get pointer past end of array");

	array_clear(&arr);
	ok(array_length(&arr) == 0 && array_get_ptr(&arr, 0) == NULL && errno == EINVAL, "Clear");

	for (size_t i = 0; i < len; i++) {
		ok(array_add_ptr(&arr, items[i]) && array_length(&arr) == i+1,
			"Re-add item: %zu = %p", i, (void*)items[i]);
	}
	array_release_full(&arr);
}

int main(int argc, char *argv[]) {
	plan_no_plan();

	test_small_objects();
	test_large_objects();
	test_pointers();

	return exit_status();
}
