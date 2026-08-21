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

#include "arena.h"

/* ==========================================================================
 * INTERNAL ALIGNMENT & HEADERS
 * ========================================================================== */

// Aligns a size to the nearest 8-byte boundary
static inline size_t align8(size_t s) { return (s + 7) & ~7; }

// By using a union with a uint64_t, we guarantee this header is exactly
// 8 bytes long, even on 32-bit systems. This guarantees that the pointer
// we hand to the user is always safely 8-byte aligned.
typedef union {
  size_t size;
  uint64_t _align;
} ArenaHeader;

/* ==========================================================================
 * INTERNAL WRAPPERS (To satisfy the Allocator interface)
 * ========================================================================== */

static void *arena_malloc_wrapper(size_t size, void *ctx) {
  return arena_alloc((Arena *)ctx, size);
}

static void *arena_realloc_wrapper(void *ptr, size_t new_size, void *ctx) {
  return arena_realloc((Arena *)ctx, ptr, new_size);
}

static void arena_free_wrapper(void *ptr, void *ctx) {
  (void)ptr;
  (void)ctx;
  // This is the absolute beauty of a linear allocator.
  // Individual frees are a pure no-op. The CPU does zero work here.
}

/* ==========================================================================
 * REGION MANAGEMENT
 * ========================================================================== */

static ArenaRegion *allocate_region(size_t capacity) {
  // Allocate the region header plus the actual memory block
  ArenaRegion *reg = malloc(sizeof(ArenaRegion) + capacity);
  if (!reg)
    return NULL;

  reg->next = NULL;
  reg->capacity = capacity;
  reg->used = 0;
  return reg;
}

/* ==========================================================================
 * LIFECYCLE
 * ========================================================================== */

ArenaResult arena_create(size_t region_size) {
  ArenaResult res = {0};

  if (region_size == 0) {
    region_size = 4096; // Sensible default fallback
  }

  Arena *a = malloc(sizeof(Arena));
  if (!a) {
    res.as.error = ARENA_ERR_ALLOC;
    res.is_error = 1;
    return res;
  }

  a->head = allocate_region(region_size);
  if (!a->head) {
    free(a);
    res.as.error = ARENA_ERR_ALLOC;
    res.is_error = 1;
    return res;
  }

  a->default_region_size = region_size;

  // Bind the generic Allocator interface to point back to this arena!
  a->allocator.malloc = arena_malloc_wrapper;
  a->allocator.realloc = arena_realloc_wrapper;
  a->allocator.free = arena_free_wrapper;
  a->allocator.ctx = a;

  res.as.value = a;
  res.is_error = 0;
  return res;
}

void arena_free(Arena **a_ptr) {
  if (!a_ptr || !*a_ptr)
    return;
  Arena *a = *a_ptr;

  // Walk the linked list and free every region back to the OS
  ArenaRegion *curr = a->head;
  while (curr) {
    ArenaRegion *next = curr->next;
    free(curr);
    curr = next;
  }

  free(a);
  *a_ptr = NULL;
}

void arena_reset(Arena *a) {
  if (!a || !a->head)
    return;

  // To cleanly reset, we free all overflow regions back to the OS,
  // but we KEEP the original starting region and just reset its 'used' counter
  // to 0. This avoids a redundant malloc() at the start of the next
  // frame/cycle.
  ArenaRegion *curr = a->head->next;
  while (curr) {
    ArenaRegion *next = curr->next;
    free(curr);
    curr = next;
  }

  a->head->next = NULL;
  a->head->used = 0;
}

/* ==========================================================================
 * DIRECT ALLOCATION API
 * ========================================================================== */

void *arena_alloc(Arena *a, size_t size) {
  if (!a || size == 0)
    return NULL;

  // Total allocation = size requested + 8 byte hidden header
  // We align the entire block to 8 bytes to maintain struct alignment.
  size_t total_size = align8(sizeof(ArenaHeader) + size);

  // If the current region is full, allocate a new one and push it to the front
  if (a->head->used + total_size > a->head->capacity) {
    size_t new_cap = a->default_region_size;
    if (total_size > new_cap) {
      new_cap =
          total_size; // Handle massive allocations that exceed the default
    }

    ArenaRegion *new_reg = allocate_region(new_cap);
    if (!new_reg)
      return NULL;

    new_reg->next = a->head;
    a->head = new_reg;
  }

  // Grab the pointer to the next free byte
  uint8_t *raw_ptr = a->head->data + a->head->used;
  a->head->used += total_size;

  // Write the hidden size header
  ArenaHeader *header = (ArenaHeader *)raw_ptr;
  header->size = size;

  // Return the pointer shifted FORWARD by 8 bytes so the user doesn't overwrite
  // the header
  return (void *)(raw_ptr + sizeof(ArenaHeader));
}

void *arena_realloc(Arena *a, void *ptr, size_t new_size) {
  if (!a)
    return NULL;

  // realloc(NULL, size) behaves exactly like malloc(size)
  if (!ptr)
    return arena_alloc(a, new_size);

  // realloc(ptr, 0) behaves like free(ptr), which in an arena is a no-op
  if (new_size == 0)
    return NULL;

  // Shift the pointer BACKWARD by 8 bytes to read our hidden header
  uint8_t *raw_ptr = (uint8_t *)ptr - sizeof(ArenaHeader);
  ArenaHeader *header = (ArenaHeader *)raw_ptr;
  size_t old_size = header->size;

  // If the new size fits inside the old block, just return the same pointer.
  // Note: We don't reclaim the unused space because it's a linear allocator!
  if (new_size <= old_size) {
    return ptr;
  }

  // Otherwise, we have to allocate a brand new block and copy the old data
  // over.
  void *new_ptr = arena_alloc(a, new_size);
  if (!new_ptr)
    return NULL;

  memcpy(new_ptr, ptr, old_size);

  return new_ptr;
}
