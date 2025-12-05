#ifndef MAP_H
#define MAP_H

#include "util.h"

/**
 * @file
 * Crit-bit tree based map which supports unique prefix queries and
 * ordered iteration.
 */

/** Opaque map type. */
typedef struct Map Map;

/** Allocate a new map. */
VIS_INTERNAL Map *map_new(void);
/**
 * Lookup a value, returns ``NULL`` if not found.
 * @param map The map to search within.
 * @param key The key to look up.
 */
VIS_INTERNAL void *map_get(const Map *map, const char *key);
/**
 * Get first element of the map, or ``NULL`` if empty.
 * @param map The map to query.
 * @param key Updated with the key of the first element.
 */
VIS_INTERNAL void *map_first(const Map *map, const char **key);
/**
 * Lookup element by unique prefix match.
 * @param map The map to search within.
 * @param prefix The prefix to search for.
 * @return The corresponding value, if the given prefix is unique.
 * Otherwise ``NULL``.
 */
VIS_INTERNAL void *map_closest(const Map *map, const char *prefix);
/**
 * Store a key value pair in the map.
 * @param map The map to store the key-value pair in.
 * @param key The key to store.
 * @param value The value associated with the key.
 * @return False if we run out of memory, or if the key
 * already appears in the map.
 */
VIS_INTERNAL bool map_put(Map *map, const char *key, const void *value);
/**
 * Remove a map element.
 * @param map The map to remove the element from.
 * @param key The key of the element to remove.
 * @return The removed entry or ``NULL`` if no such element exists.
 */
VIS_INTERNAL void *map_delete(Map *map, const char *key);
/**
 * Copy all entries from ``src`` into ``dest``, overwrites existing entries in ``dest``.
 * @param dest The destination map.
 * @param src The source map.
 */
VIS_INTERNAL bool map_copy(Map *dest, Map *src);
/**
 * Ordered iteration over a map.
 * Invokes the passed callback for every map entry.
 * If ``handle`` returns false, the iteration will stop.
 * @param map The map to iterate over.
 * @param handle A function invoked for every map element.
 * @param data A context pointer, passed as last argument to ``handle``.
 */
VIS_INTERNAL void map_iterate(const Map *map, bool (*handle)(const char *key, void *value, void *data), const void *data);
/**
 * Get a sub map matching a prefix.
 * @param map The map to get the sub-map from.
 * @param prefix The prefix to match.
 * @rst
 * .. warning:: This returns a pointer into the original map.
 * Do not alter the map while using the return value.
 * @endrst
 */
VIS_INTERNAL const Map *map_prefix(const Map *map, const char *prefix);
/**
 * Test whether the map is empty (contains no elements).
 * @param map The map to check.
 */
VIS_INTERNAL bool map_empty(const Map *map);
/**
 * Empty the map.
 * @param map The map to clear.
 */
VIS_INTERNAL void map_clear(Map *map);
/**
 * Release all memory associated with this map.
 * @param map The map to free.
 */
VIS_INTERNAL void map_free(Map *map);

#endif
