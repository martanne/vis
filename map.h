#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

typedef struct Map Map;

/* Allocate a new map. */
Map *map_new(void);
/* Retrieves a value, or NULL if it isn't in the map */
void *map_get(const Map*, const char *key);
/* Returns the corresponding value if the given prefix is unique.
 * Otherwise NULL, if no such prefix exists then errno is set to ENOENT. */
void *map_closest(const Map*, const char *prefix);
/* Place a member in the map. This returns false if we run out of memory
 * (errno = ENOMEM), or if that key already appears in the map (errno = EEXIST). */
bool map_put(Map*, const char *key, const void *value);
/* Remove a member from the map. Returns the removed entry or NULL
 * if there was no entry found using the given key*/
void *map_delete(Map*, const char *key);
/* Ordered iteration over a map, call handle for every entry. If handle
 * returns false, the iteration will stop. */
void map_iterate(const Map*, bool (*handle)(const char *key, void *value, void *data), const void *data);
/* Return a submap matching a prefix. This returns a pointer into the
 * original map, so don't alter the map while using the return value. */
const Map *map_prefix(const Map*, const char *prefix);
/* Test whether the map is empty i.e. contains no elements */
bool map_empty(const Map*);
/* Remove every member from the map. The map will be empty after this. */
void map_clear(Map*);
/* Release all memory associated with this map */
void map_free(Map*);

#endif
