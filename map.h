/* Copyright (c) 2026 OVIETA <ovieta17@gmail.com>
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAP_NPOS ((size_t)-1)

/* ==========================================================================
 * SHARED INTERFACES (Implicit Contract)
 * ========================================================================== */

// Guarded to prevent redefinition errors if used alongside darray.h
#ifndef OVIETA_ALLOCATOR_DEFINED
#define OVIETA_ALLOCATOR_DEFINED
typedef struct Allocator {
  void *(*malloc)(size_t size, void *ctx);
  void *(*realloc)(void *ptr, size_t new_size, void *ctx);
  void (*free)(void *ptr, void *ctx);
  void *ctx;
} Allocator;
#endif

/* ==========================================================================
 * TYPES & STRUCTURES
 * ========================================================================== */

typedef enum {
  MAP_OK = 0,
  MAP_ERR_ALLOC,
  MAP_ERR_BOUNDS,
  MAP_ERR_NULL_PTR,
  MAP_ERR_NOT_FOUND,
  MAP_ERR_FULL // Hash maps shouldn't hit this if they auto-grow, but safe to
               // have
} MapError;

// Struct of Arrays (SoA) layout with a Flexible Array Member
typedef struct Map {
  size_t _length;
  size_t _tombstone_count;

  size_t _capacity; // MUST always be a power of 2
  size_t key_size;
  size_t value_size;

  Allocator *allocator;

  // Type-agnostic hashing and equality callbacks
  uint64_t (*hash_func)(const void *key, void *ctx);
  bool (*eq_func)(const void *key_a, const void *key_b, void *ctx);
  void *ctx;

  // The FAM stores the contiguous bytes for parallel arrays:
  // [ uint64_t hashes... | padding | uint8_t keys... | padding | uint8_t
  // values... ] MSB of the hash will be used to indicate Occupied (1) or
  // Empty/Tombstone (0)
  uint8_t data[];
} Map;

// A decoupled array struct specifically for returning extracted keys/values
typedef struct MapArray {
  size_t length;
  size_t element_size;
  Allocator *allocator;
  uint8_t data[]; // FAM for the extracted elements
} MapArray;

// Tagged Unions
typedef struct {
  union {
    Map *value;
    MapError error;
  } as;
  uint8_t is_error;
} MapResult;
typedef struct {
  union {
    void *value;
    MapError error;
  } as;
  uint8_t is_error;
} MapValueResult;
typedef struct {
  union {
    MapArray *value;
    MapError error;
  } as;
  uint8_t is_error;
} MapArrayResult;

/* ==========================================================================
 * INTERNAL ALLOCATOR WRAPPERS
 * ========================================================================== */

static inline void *map_internal_malloc(Allocator *alloc, size_t size) {
  if (alloc && alloc->malloc)
    return alloc->malloc(size, alloc->ctx);
  return malloc(size);
}

static inline void map_internal_free(Allocator *alloc, void *ptr) {
  if (alloc && alloc->free)
    alloc->free(ptr, alloc->ctx);
  else
    free(ptr);
}

/* ==========================================================================
 * INLINE UTILITIES
 * ========================================================================== */

static inline size_t map_len(const Map *m) { return m ? m->_length : 0; }
static inline size_t map_cap(const Map *m) { return m ? m->_capacity : 0; }
static inline bool map_empty(const Map *m) {
  return m ? m->_length == 0 : true;
}

/* ==========================================================================
 * DEFAULT HASHING & EQUALITY
 * ========================================================================== */

/**
 * FNV-1a 64-bit hash function.
 * Fast, inline default for hashing raw bytes.
 * @param key Pointer to the data to hash.
 * @param len Number of bytes to hash.
 * @return uint64_t The 64-bit hash.
 */
static inline uint64_t map_hash_fnv1a(const void *key, size_t len) {
  const uint8_t *bytes = (const uint8_t *)key;
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (size_t i = 0; i < len; i++) {
    hash ^= bytes[i];
    hash *= 0x100000001b3ULL;
  }
  return hash;
}

/* ==========================================================================
 * LIFECYCLE
 * ========================================================================== */

/**
 * Creates a flat-array open-addressing hash map.
 * @param capacity Initial capacity (will automatically be rounded up to the
 * nearest power of 2).
 * @param key_size Size of the key type in bytes.
 * @param value_size Size of the value type in bytes.
 * @param hash_func Pointer to the hashing function.
 * @param eq_func Pointer to the key equality function.
 * @param ctx Context pointer passed to hash/eq functions.
 * @param alloc Custom memory allocator. Pass NULL for stdlib malloc.
 * @return MapResult An initialized Map pointer or a MapError.
 */
MapResult map_create(size_t capacity, size_t key_size, size_t value_size,
                     uint64_t (*hash_func)(const void *key, void *ctx),
                     bool (*eq_func)(const void *key_a, const void *key_b,
                                     void *ctx),
                     void *ctx, Allocator *alloc);

/**
 * Releases the hash map's memory.
 * @param m Map** Double pointer to the map to release. Set to NULL after free.
 */
void map_free(Map **m);

/* ==========================================================================
 * MUTATIONS (Requires Map** for Rehashing/Reallocation)
 * ========================================================================== */

/**
 * Inserts or updates a key-value pair.
 * Copies the key and value into the map's memory block via memcpy.
 * WARNING: May trigger an automatic capacity growth and complete rehash,
 * invalidating any pointers previously returned by map_get.
 * @param m Map** Double pointer to the destination map.
 * @param key const void* Pointer to the key to insert/update.
 * @param value const void* Pointer to the value to store.
 * @return MapError MAP_OK on success.
 */
MapError map_set(Map **m, const void *key, const void *value);

/**
 * Removes a key-value pair using a tombstone.
 * @param m Map** Double pointer to the map.
 * @param key const void* Pointer to the key to remove.
 * @param out_value void* Optional pointer to copy the removed value into before
 * deletion.
 * @return MapError MAP_OK if removed, MAP_ERR_NOT_FOUND if the key wasn't in
 * the map.
 */
MapError map_remove(Map **m, const void *key, void *out_value);

/* ==========================================================================
 * ACCESS & SEARCH
 * ========================================================================== */

/**
 * Safely retrieves a value by copying it into the provided out_value pointer.
 * @param m const Map* Source map.
 * @param key const void* Pointer to the key to look up.
 * @param out_value void* Pointer to destination memory to copy the value into.
 * @return MapError MAP_OK on success, or MAP_ERR_NOT_FOUND.
 */
MapError map_get(const Map *m, const void *key, void *out_value);

/**
 * Retrieves a pointer directly to the value in the map's contiguous memory.
 * WARNING: The returned pointer is strictly temporary. Any call to map_set
 * may trigger a rehash, invalidating this pointer.
 * @param m const Map* Source map.
 * @param key const void* Pointer to the key to look up.
 * @return MapValueResult A pointer to the stored value, or MAP_ERR_NOT_FOUND.
 */
MapValueResult map_get_ptr(const Map *m, const void *key);

/**
 * Checks if a key exists in the map.
 * @param m const Map* Source map.
 * @param key const void* Pointer to the key to check.
 * @return bool True if the key exists, false otherwise.
 */
bool map_contains(const Map *m, const void *key);

/* ==========================================================================
 * ITERATION
 * ========================================================================== */

/**
 * Iterates over every valid key-value pair in the map.
 * @param m Map* Source map.
 * @param func Callback receiving pointers to the key and value, and the user
 * context.
 * @param ctx void* User-provided context passed directly to the callback.
 * @return MapError MAP_OK on success.
 */
MapError map_for_each(Map *m,
                      void (*func)(const void *key, void *value, void *ctx),
                      void *ctx);

/* ==========================================================================
 * EXTRACTION & CLEANUP
 * ========================================================================== */

/**
 * Extracts all currently active keys into a new contiguous array.
 * The resulting MapArray uses the FAM pattern and must be freed with
 * map_array_free.
 * @param m const Map* Source map.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return MapArrayResult A pointer to the populated MapArray or a MapError.
 */
MapArrayResult map_keys(const Map *m, Allocator *alloc);

/**
 * Extracts all currently active values into a new contiguous array.
 * The resulting MapArray uses the FAM pattern and must be freed with
 * map_array_free.
 * @param m const Map* Source map.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return MapArrayResult A pointer to the populated MapArray or a MapError.
 */
MapArrayResult map_values(const Map *m, Allocator *alloc);

/**
 * Frees a MapArray returned by map_keys or map_values.
 * @param arr MapArray** Double pointer to the array to release.
 */
void map_array_free(MapArray **arr);
#endif // MAP_H
