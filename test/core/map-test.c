#include "util.h"

#include "tap.h"

#include "map.c"

static bool get(Map *map, const char *key, const void *data) {
	return map_get(map, key) == data && map_closest(map, key) == data;
}

static bool compare(const char *key, void *value, void *data) {
	Map *map = data;
	ok(map_get(map, key) == value, "Compare map content");
	return true;
}

static bool once(const char *key, void *value, void *data) {
	int *counter = data;
	(*counter)++;
	return false;
}

static bool visit(const char *key, void *value, void *data) {
	int *index = value;
	int *visited = data;
	visited[*index]++;
	return true;
}

static int order_counter;

static bool order(const char *key, void *value, void *data) {
	int *index = value;
	int *order = data;
	order[*index] = ++order_counter;
	return true;
}

int main(int argc, char *argv[]) {
	const char *key = "404";
	const int values[3] = { 0, 1, 2 };

	plan_no_plan();

	Map *map = map_new();

	ok(map && map_empty(map), "Creation");
	ok(map_first(map, &key) == NULL && strcmp(key, "404") == 0, "First on empty map");
	ok(map_empty(map_prefix(map, "404")), "Empty prefix map");

	ok(!map_get(map, "404"), "Get non-existing key");
	ok(!map_closest(map, "404"), "Closest non-existing key");

	ok(!map_put(map, "a", NULL) && map_empty(map) && !map_get(map, "a"), "Put NULL value");
	ok(map_put(map, "a", &values[0]) && !map_empty(map) && get(map, "a", &values[0]), "Put 1");
	ok(map_first(map, &key) == &values[0] && strcmp(key, "a") == 0, "First on map with 1 value");
	key = NULL;
	ok(map_first(map_prefix(map, "a"), &key) == &values[0] && strcmp(key, "a") == 0, "First on prefix map");
	ok(!map_empty(map_prefix(map, "a")), "Contains existing key");
	ok(map_closest(map, "a") == &values[0], "Closest match existing key");
	ok(!map_put(map, "a", &values[1]) && get(map, "a", &values[0]), "Put duplicate");
	ok(map_put(map, "cafebabe", &values[2]) && get(map, "cafebabe", &values[2]), "Put 2");
	ok(map_put(map, "cafe", &values[1]) && get(map, "cafe", &values[1]), "Put 3");
	key = NULL;
	ok(map_first(map_prefix(map, "cafe"), &key) == &values[1] && strcmp(key, "cafe") == 0, "First on prefix map with multiple suffixes");

	Map *copy = map_new();
	ok(map_copy(copy, map), "Copy");
	ok(!map_empty(copy), "Not empty after copying");
	map_iterate(copy, compare, map);
	map_iterate(map, compare, copy);

	int counter = 0;
	map_iterate(copy, once, &counter);
	ok(counter == 1, "Iterate stop condition");

	ok(!map_get(map, "ca") && !map_closest(map, "ca"), "Closest ambigious");

	int visited[] = { 0, 0, 0 };

	map_iterate(map, visit, &visited);
	ok(visited[0] == 1 && visited[1] == 1 && visited[2] == 1, "Iterate map");

	memset(visited, 0, sizeof visited);
	order_counter = 0;
	map_iterate(map, order, &visited);
	ok(visited[0] == 1 && visited[1] == 2 && visited[2] == 3, "Ordered iteration");

	memset(visited, 0, sizeof visited);
	map_iterate(map_prefix(map, "ca"), visit, &visited);
	ok(visited[0] == 0 && visited[1] == 1 && visited[2] == 1, "Iterate sub map");

	memset(visited, 0, sizeof visited);
	order_counter = 0;
	map_iterate(map_prefix(map, "ca"), order, &visited);
	ok(visited[0] == 0 && visited[1] == 1 && visited[2] == 2, "Ordered sub map iteration");

	ok(map_empty(map_prefix(map, "404")), "Empty map for non-existing prefix");

	ok(!map_delete(map, "404"), "Delete non-existing key");
	ok(map_delete(map, "cafe") == &values[1] && !map_get(map, "cafe"), "Delete existing key");
	ok(map_closest(map, "cafe") == &values[2], "Closest unambigious");
	ok(map_put(map, "cafe", &values[1]) && get(map, "cafe", &values[1]), "Put 3 again");

	map_clear(map);
	ok(map_empty(map), "Empty after clean");

	map_free(map);
	map_free(copy);

	return exit_status();
}
