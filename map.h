#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

/**
 * @file
 * Crit-bit tree based map which supports unique prefix queries and
 * ordered iteration.
 */

/** Opaque map type. */
typedef struct Map Map;

/** Allocate a new map. */
Map *map_new(void);
/** Lookup a value, returns ``NULL`` if not found. */
void *map_get(const Map*, const char *key);
/**
 * Get first element of the map, or ``NULL`` if empty.
 * @param key Updated with the key of the first element.
 */
void *map_first(const Map*, const char **key);
/**
 * Lookup element by unique prefix match.
 * @param prefix The prefix to search for.
 * @return The corresponding value, if the given prefix is unique.
 *         Otherwise ``NULL``. If no such prefix exists, then ``errno``
 *         is set to ``ENOENT``.
 */
void *map_closest(const Map*, const char *prefix);
/**
 * Check whether the map contains the given prefix.
 * whether it can be extended to match a key of a map element.
 */
bool map_contains(const Map*, const char *prefix);
/**
 * Store a key value pair in the map.
 * @return False if we run out of memory (``errno = ENOMEM``), or if the key
 *         already appears in the map (``errno = EEXIST``).
 */
bool map_put(Map*, const char *key, const void *value);
/**
 * Remove a map element.
 * @return The removed entry or ``NULL`` if no such element exists.
 */
void *map_delete(Map*, const char *key);
/** Copy all entries from ``src`` into ``dest``, overwrites existing entries in ``dest``. */
bool map_copy(Map *dest, Map *src);
/**
 * Ordered iteration over a map.
 * Invokes the passed callback for every map entry.
 * If ``handle`` returns false, the iteration will stop.
 * @param handle A function invoked for ever map element.
 * @param data A context pointer, passed as last argument to ``handle``.
 */
void map_iterate(const Map*, bool (*handle)(const char *key, void *value, void *data), const void *data);
/**
 * Get a sub map matching a prefix.
 * @rst
 * .. warning:: This returns a pointer into the original map.
 *              Do not alter the map while using the return value.
 * @endrst
 */
const Map *map_prefix(const Map*, const char *prefix);
/** Test whether the map is empty (contains no elements). */
bool map_empty(const Map*);
/** Empty the map. */
void map_clear(Map*);
/** Release all memory associated with this map. */
void map_free(Map*);
/**
 * Call `free(3)` for every map element, then free the map itself.
 * @rst
 * .. warning:: Assumes map elements to be pointers.
 * @endrst
 */
void map_free_full(Map*);

#endif
