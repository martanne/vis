#include <ccan/tap/tap.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "array.h"
#include "../../util.h"

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

	array_clear(&arr);
	ok(array_length(&arr) == 0 && array_get(&arr, 0) == NULL && errno == EINVAL, "Clear");

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
