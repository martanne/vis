#include "util.h"
#include "util.c"

#include "tap.h"

#include "map.c"

static bool get(Map *map, str8 key, const void *data) {
	return vis_map_get(map, key) == data && vis_map_closest(map, key) == data;
}

static bool compare(const char *key, void *value, void *data) {
	Map *map = data;
	ok(vis_map_get(map, str8_from_c_str(key)) == value, "Compare map content");
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
	str8 key = str8("404");
	const int values[3] = { 0, 1, 2 };

	plan_no_plan();

	Map *map = map_new();

	ok(map && map_empty(map), "Creation");
	ok(vis_map_first(map, &key) == 0 && str8_equal(key, str8("404")), "First on empty map");
	ok(map_empty(vis_map_prefix(map, str8("404"))), "Empty prefix map");

	ok(!vis_map_get(map, str8("404")), "Get non-existing key");
	ok(!vis_map_closest(map, str8("404")), "Closest non-existing key");

	ok(!vis_map_put(map, str8("a"), 0) && map_empty(map) && !vis_map_get(map, str8("a")), "Put NULL value");
	ok(vis_map_put(map, str8("a"), values) && !map_empty(map) && get(map, str8("a"), &values[0]), "Put 1");
	ok(vis_map_first(map, &key) == values && str8_equal(key, str8("a")), "First on map with 1 value");
	key = str8("");
	ok(vis_map_first(vis_map_prefix(map, str8("a")), &key) == values && str8_equal(key, str8("a")), "First on prefix map");
	ok(!map_empty(vis_map_prefix(map, str8("a"))), "Contains existing key");
	ok(vis_map_closest(map, str8("a")) == values, "Closest match existing key");
	ok(!vis_map_put(map, str8("a"), values + 1) && get(map, str8("a"), values), "Put duplicate");
	ok(vis_map_put(map, str8("cafebabe"), values + 2) && get(map, str8("cafebabe"), values + 2), "Put 2");
	ok(vis_map_put(map, str8("cafe"), values + 1) && get(map, str8("cafe"), values + 1), "Put 3");
	key = str8("");
	ok(vis_map_first(vis_map_prefix(map, str8("cafe")), &key) == values + 1 && str8_equal(key, str8("cafe")), "First on prefix map with multiple suffixes");

	Map *copy = map_new();
	ok(vis_map_copy(copy, map), "Copy");
	ok(!map_empty(copy), "Not empty after copying");
	map_iterate(copy, compare, map);
	map_iterate(map, compare, copy);

	int counter = 0;
	map_iterate(copy, once, &counter);
	ok(counter == 1, "Iterate stop condition");

	ok(!vis_map_get(map, str8("ca")) && !vis_map_closest(map, str8("ca")), "Closest ambigious");

	int visited[] = { 0, 0, 0 };

	map_iterate(map, visit, &visited);
	ok(visited[0] == 1 && visited[1] == 1 && visited[2] == 1, "Iterate map");

	memset(visited, 0, sizeof visited);
	order_counter = 0;
	map_iterate(map, order, &visited);
	ok(visited[0] == 1 && visited[1] == 2 && visited[2] == 3, "Ordered iteration");

	memset(visited, 0, sizeof visited);
	map_iterate(vis_map_prefix(map, str8("ca")), visit, &visited);
	ok(visited[0] == 0 && visited[1] == 1 && visited[2] == 1, "Iterate sub map");

	memset(visited, 0, sizeof visited);
	order_counter = 0;
	map_iterate(vis_map_prefix(map, str8("ca")), order, &visited);
	ok(visited[0] == 0 && visited[1] == 1 && visited[2] == 2, "Ordered sub map iteration");

	ok(map_empty(vis_map_prefix(map, str8("404"))), "Empty map for non-existing prefix");

	ok(!vis_map_delete(map, str8("404")), "Delete non-existing key");
	ok(vis_map_delete(map, str8("cafe")) == values + 1 && !vis_map_get(map, str8("cafe")), "Delete existing key");
	ok(vis_map_closest(map, str8("cafe")) == values + 2, "Closest unambigious");
	ok(vis_map_put(map, str8("cafe"), values + 1) && get(map, str8("cafe"), values + 1), "Put 3 again");

	map_clear(map);
	ok(map_empty(map), "Empty after clean");

	map_free(map);
	map_free(copy);

	return exit_status();
}
