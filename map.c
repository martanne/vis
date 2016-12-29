/* Crit-bit tree based map which supports lookups based on unique
 * prefixes as well as ordered iteration.
 *
 * Based on public domain code from Rusty Russel, Adam Langley
 * and D. J. Bernstein.
 *
 * Further information about the data structure can be found at:
 *  http://cr.yp.to/critbit.html
 *  http://github.com/agl/critbit
 *  http://ccodearchive.net/info/strmap.html
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include "map.h"

typedef struct Node Node;

struct Map {     /* struct holding either an item with value v and key u.s or an internal node */
	union {
		Node *n;
		const char *s;
	} u;
	void *v; /* value stored in the map, if non NULL u.s holds the corresponding key */
};

struct Node {
	Map child[2];    /* These point to strings or nodes. */
	size_t byte_num; /* The byte number where first bit differs. */
	uint8_t bit_num; /* The bit where these children differ. */
};

/* Closest key to this in a non-empty map. */
static Map *closest(Map *n, const char *key)
{
	size_t len = strlen(key);
	const uint8_t *bytes = (const uint8_t *)key;

	/* Anything with NULL value is an internal node. */
	while (!n->v) {
		uint8_t direction = 0;

		if (n->u.n->byte_num < len) {
			uint8_t c = bytes[n->u.n->byte_num];
			direction = (c >> n->u.n->bit_num) & 1;
		}
		n = &n->u.n->child[direction];
	}
	return n;
}

void *map_get(const Map *map, const char *key)
{
	/* Not empty map? */
	if (map->u.n) {
		Map *n = closest((Map *)map, key);
		if (strcmp(key, n->u.s) == 0)
			return n->v;
	}
	return NULL;
}

void *map_closest(const Map *map, const char *prefix)
{
	errno = 0;
	void *v = map_get(map, prefix);
	if (v)
		return v;
	const Map *m = map_prefix(map, prefix);
	if (map_empty(m))
		errno = ENOENT;
	return m->v;
}

bool map_contains(const Map *map, const char *prefix)
{
	return !map_empty(map_prefix(map, prefix));
}

bool map_put(Map *map, const char *k, const void *value)
{
	size_t len = strlen(k);
	const uint8_t *bytes = (const uint8_t *)k;
	Map *n;
	Node *newn;
	size_t byte_num;
	uint8_t bit_num, new_dir;
	char *key;

	if (!value) {
		errno = EINVAL;
		return false;
	}

	if (!(key = strdup(k))) {
		errno = ENOMEM;
		return false;
	}

	/* Empty map? */
	if (!map->u.n) {
		map->u.s = key;
		map->v = (void *)value;
		return true;
	}

	/* Find closest existing key. */
	n = closest(map, key);

	/* Find where they differ. */
	for (byte_num = 0; n->u.s[byte_num] == key[byte_num]; byte_num++) {
		if (key[byte_num] == '\0') {
			/* All identical! */
			free(key);
			errno = EEXIST;
			return false;
		}
	}

	/* Find which bit differs */
	uint8_t diff = (uint8_t)n->u.s[byte_num] ^ bytes[byte_num];
	/* TODO: bit_num = 31 - __builtin_clz(diff); ? */
	for (bit_num = 0; diff >>= 1; bit_num++);

	/* Which direction do we go at this bit? */
	new_dir = ((bytes[byte_num]) >> bit_num) & 1;

	/* Allocate new node. */
	newn = malloc(sizeof(*newn));
	if (!newn) {
		free(key);
		errno = ENOMEM;
		return false;
	}
	newn->byte_num = byte_num;
	newn->bit_num = bit_num;
	newn->child[new_dir].v = (void *)value;
	newn->child[new_dir].u.s = key;

	/* Find where to insert: not closest, but first which differs! */
	n = map;
	while (!n->v) {
		uint8_t direction = 0;

		if (n->u.n->byte_num > byte_num)
			break;
		/* Subtle: bit numbers are "backwards" for comparison */
		if (n->u.n->byte_num == byte_num && n->u.n->bit_num < bit_num)
			break;

		if (n->u.n->byte_num < len) {
			uint8_t c = bytes[n->u.n->byte_num];
			direction = (c >> n->u.n->bit_num) & 1;
		}
		n = &n->u.n->child[direction];
	}

	newn->child[!new_dir] = *n;
	n->u.n = newn;
	n->v = NULL;
	return true;
}

void *map_delete(Map *map, const char *key)
{
	size_t len = strlen(key);
	const uint8_t *bytes = (const uint8_t *)key;
	Map *parent = NULL, *n;
	void *value = NULL;
	uint8_t direction;

	/* Empty map? */
	if (!map->u.n) {
		errno = ENOENT;
		return NULL;
	}

	/* Find closest, but keep track of parent. */
	n = map;
	/* Anything with NULL value is a node. */
	while (!n->v) {
		uint8_t c = 0;

		parent = n;
		if (n->u.n->byte_num < len) {
			c = bytes[n->u.n->byte_num];
			direction = (c >> n->u.n->bit_num) & 1;
		} else {
			direction = 0;
		}
		n = &n->u.n->child[direction];
	}

	/* Did we find it? */
	if (strcmp(key, n->u.s)) {
		errno = ENOENT;
		return NULL;
	}

	free((char*)n->u.s);
	value = n->v;

	if (!parent) {
		/* We deleted last node. */
		map->u.n = NULL;
	} else {
		Node *old = parent->u.n;
		/* Raise other node to parent. */
		*parent = old->child[!direction];
		free(old);
	}

	return value;
}

static bool iterate(Map n, bool (*handle)(const char *, void *, void *), const void *data)
{
	if (n.v)
		return handle(n.u.s, n.v, (void *)data);

	return iterate(n.u.n->child[0], handle, data)
		&& iterate(n.u.n->child[1], handle, data);
}

void map_iterate(const Map *map, bool (*handle)(const char *, void *, void *), const void *data)
{
	/* Empty map? */
	if (!map->u.n)
		return;

	iterate(*map, handle, data);
}

typedef struct {
	const char *key;
	void *value;
} KeyValue;

static bool first(const char *key, void *value, void *data)
{
	KeyValue *kv = data;
	kv->key = key;
	kv->value = value;
	return false;
}

void *map_first(const Map *map, const char **key)
{
	KeyValue kv = { 0 };
	map_iterate(map, first, &kv);
	if (key && kv.key)
		*key = kv.key;
	return kv.value;
}

const Map *map_prefix(const Map *map, const char *prefix)
{
	const Map *n, *top;
	size_t len = strlen(prefix);
	const uint8_t *bytes = (const uint8_t *)prefix;

	/* Empty map -> return empty map. */
	if (!map->u.n)
		return map;

	top = n = map;

	/* We walk to find the top, but keep going to check prefix matches. */
	while (!n->v) {
		uint8_t c = 0, direction;

		if (n->u.n->byte_num < len)
			c = bytes[n->u.n->byte_num];

		direction = (c >> n->u.n->bit_num) & 1;
		n = &n->u.n->child[direction];
		if (c)
			top = n;
	}

	if (strncmp(n->u.s, prefix, len)) {
		/* Convenient return for prefixes which do not appear in map. */
		static const Map empty_map;
		return &empty_map;
	}

	return top;
}

static void clear(Map n)
{
	if (!n.v) {
		clear(n.u.n->child[0]);
		clear(n.u.n->child[1]);
		free(n.u.n);
	} else {
		free((char*)n.u.s);
	}
}

void map_clear(Map *map)
{
	if (map->u.n)
		clear(*map);
	map->u.n = NULL;
	map->v = NULL;
}

static bool copy(Map *dest, Map n)
{
	if (!n.v) {
		return copy(dest, n.u.n->child[0]) &&
		       copy(dest, n.u.n->child[1]);
	} else {
		if (!map_put(dest, n.u.s, n.v) && map_get(dest, n.u.s) != n.v) {
			map_delete(dest, n.u.s);
			return map_put(dest, n.u.s, n.v);
		}
		return true;
	}
}

bool map_copy(Map *dest, Map *src)
{
	if (!src || !src->u.n)
		return true;

	return copy(dest, *src);
}

bool map_empty(const Map *map)
{
	return map->u.n == NULL;
}

Map *map_new(void)
{
	return calloc(1, sizeof(Map));
}

void map_free(Map *map)
{
	if (!map)
		return;
	map_clear(map);
	free(map);
}

static bool free_elem(const char *key, void *value, void *data)
{
	free(value);
	return true;
}

void map_free_full(Map *map)
{
	if (!map)
		return;
	map_iterate(map, free_elem, NULL);
	map_free(map);
}
