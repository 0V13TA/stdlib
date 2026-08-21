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

#ifndef ARENA_H
#define ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
 * SHARED INTERFACES (Implicit Contract)
 * ========================================================================== */

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

typedef enum { ARENA_OK = 0, ARENA_ERR_ALLOC, ARENA_ERR_NULL_PTR } ArenaError;

// A single contiguous block of memory.
// If the arena fills up, it will allocate a new region and link it here.
typedef struct ArenaRegion {
  struct ArenaRegion *next;
  size_t capacity;
  size_t used;
  uint8_t data[]; // FAM for the actual memory block
} ArenaRegion;

typedef struct Arena {
  ArenaRegion *head;          // The current active memory region
  size_t default_region_size; // How big new regions should be
  Allocator allocator;        // The interface passed to Darray/Map/String
} Arena;

// Tagged Union Result
typedef struct {
  union {
    Arena *value;
    ArenaError error;
  } as;
  uint8_t is_error;
} ArenaResult;

/* ==========================================================================
 * LIFECYCLE
 * ========================================================================== */

/**
 * Creates a new memory arena.
 * @param region_size The capacity of the first memory block (e.g., 4096 bytes).
 * @return ArenaResult An initialized Arena pointer or an ArenaError.
 */
ArenaResult arena_create(size_t region_size);

/**
 * Destroys the arena, returning all regions back to the OS.
 * @param a Arena** Double pointer to the arena to release.
 */
void arena_free(Arena **a);

/**
 * reclaims all memory allocated from this arena.
 * Instead of freeing back to the OS, it just resets the internal pointers to 0.
 * Future allocations will overwrite the old data.
 * @param a Arena* The arena to reset.
 */
void arena_reset(Arena *a);

/* ==========================================================================
 * DIRECT ALLOCATION API
 * ========================================================================== */

/**
 * Allocates memory directly from the arena.
 * @param a Arena* Source arena.
 * @param size size_t Number of bytes to allocate.
 * @return void* Pointer to the allocated memory, or NULL if out of OS memory.
 */
void *arena_alloc(Arena *a, size_t size);

/**
 * Reallocates memory within the arena.
 * @param a Arena* Source arena.
 * @param ptr void* Pointer to existing arena allocation.
 * @param new_size size_t New requested size.
 * @return void* Pointer to the new memory block, or NULL if out of OS memory.
 */
void *arena_realloc(Arena *a, void *ptr, size_t new_size);

#endif // ARENA_H
