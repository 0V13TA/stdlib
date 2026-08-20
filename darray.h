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

#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DARRAY_NPOS ((size_t)-1)

/* ==========================================================================
 * TYPES & STRUCTURES
 * ========================================================================== */

typedef enum {
  DARRAY_OK = 0,
  DARRAY_ERR_ALLOC,   // Malloc or realloc failed
  DARRAY_ERR_BOUNDS,  // Index out of bounds
  DARRAY_ERR_NULL_PTR // Null pointer passed to function
} DarrayError;

// The Allocator interface
typedef struct Allocator {
  void *(*malloc)(size_t size, void *ctx);
  void *(*realloc)(void *ptr, size_t new_size, void *ctx);
  void (*free)(void *ptr, void *ctx);
  void *ctx; // Pointer to the actual arena/memory pool instance
} Allocator;

// C99 Stretchy Buffer implementation
typedef struct Darray {
  size_t _length;
  size_t _capacity;
  size_t element_size;
  Allocator *allocator; // The allocator that owns this memory block
  uint8_t data[];       // Flexible Array Member (FAM). Must be the last member.
} Darray;

typedef struct DarraySlice {
  void *data;
  size_t _length;
  size_t element_size;
} DarraySlice;

// Tagged Union Result for Darray (Now holds a pointer)
typedef struct {
  union {
    Darray *value;
    DarrayError error;
  } as;
  uint8_t is_error;
} DarrayResult;

// Tagged Union Result for DarraySlice
typedef struct {
  union {
    DarraySlice value;
    DarrayError error;
  } as;
  uint8_t is_error;
} DarraySliceResult;

// Tagged Union Result for retrieving single values (returns a pointer)
typedef struct {
  union {
    void *value;
    DarrayError error;
  } as;
  uint8_t is_error;
} ValueResult;

/* ==========================================================================
 * INLINE UTILITIES
 * ========================================================================== */

/**
 * Returns the length of the array.
 * @param arr const Darray* Source array.
 * @return size_t Number of elements in the array.
 */
static inline size_t darray_len(const Darray *arr) {
  return arr ? arr->_length : 0;
}

/**
 * Returns the capacity of the array.
 * @param arr const Darray* Source array.
 * @return size_t Current capacity in elements.
 */
static inline size_t darray_cap(const Darray *arr) {
  return arr ? arr->_capacity : 0;
}

/**
 * Checks if the array is empty.
 * @param arr const Darray* Source array.
 * @return int 1 if empty, 0 otherwise.
 */
static inline int darray_empty(const Darray *arr) {
  return arr ? arr->_length == 0 : 1;
}

/**
 * Helper to calculate the total byte size of a Darray including its FAM.
 * Casts to size_t to guard against integer overflow on 32-bit architectures.
 */
#define DARRAY_CALC_SIZE(cap, el_size)                                         \
  (sizeof(Darray) + (size_t)(cap) * (size_t)(el_size))

/* ==========================================================================
 * LIFECYCLE (CREATION & DESTRUCTION)
 * ========================================================================== */

/**
 * Creates a dynamic array.
 * Allocates the array header and data buffer in a single contiguous block.
 * @param capacity size_t Initial buffer capacity in number of elements.
 * @param element_size size_t Size of a single element in bytes.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult An initialized Darray pointer or a DarrayError.
 */
DarrayResult darray_create(size_t capacity, size_t element_size,
                           Allocator *alloc);

/**
 * Creates an owned copy of a subset of an array.
 * @param arr const Darray* Source array.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_clone(const Darray *arr, size_t start, size_t end,
                          Allocator *alloc);

/**
 * Releases a dynamic array's memory.
 * Frees the block and sets the caller's pointer to NULL.
 * @param arr Darray** Double pointer to the array to release.
 * @return void No value is returned.
 */
void darray_free(Darray **arr);

/* ==========================================================================
 * INTERNAL ALLOCATOR WRAPPERS
 * ========================================================================== */

static inline void *internal_malloc(Allocator *alloc, size_t size) {
  if (alloc && alloc->malloc) {
    return alloc->malloc(size, alloc->ctx);
  }
  return malloc(size);
}

static inline void *internal_realloc(Allocator *alloc, void *ptr,
                                     size_t new_size) {
  if (alloc && alloc->realloc) {
    return alloc->realloc(ptr, new_size, alloc->ctx);
  }
  return realloc(ptr, new_size);
}

static inline void internal_free(Allocator *alloc, void *ptr) {
  if (alloc && alloc->free) {
    alloc->free(ptr, alloc->ctx);
  } else {
    free(ptr);
  }
}

/* ==========================================================================
 * CAPACITY MANAGEMENT (STRUCTURAL - REQUIRES DARRAY**)
 * ========================================================================== */

/**
 * Ensures the array has at least enough capacity for additional elements.
 * @param arr Darray** Target array.
 * @param additional_elements size_t Number of elements to reserve space for.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_reserve(Darray **arr, size_t additional_elements);

/**
 * Reallocates the array buffer to a new capacity if the new capacity is larger.
 * @param arr Darray** Target array.
 * @param new_capacity size_t The target capacity.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_grow(Darray **arr, size_t new_capacity);

/**
 * Reallocates the array buffer to a smaller capacity, truncating length if
 * necessary.
 * @param arr Darray** Target array.
 * @param new_capacity size_t The target capacity.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_shrink(Darray **arr, size_t new_capacity);

/**
 * Macro to shrink the array's capacity to exactly match its current length.
 * Expects a Darray** (double pointer) as the argument.
 */
#define darray_shrink_to_fit(arr_ptr)                                          \
  darray_shrink((arr_ptr), (*(arr_ptr))->_length)

/* ==========================================================================
 * ELEMENT MUTATION & ADDITION
 * ========================================================================== */

/**
 * Appends a value to the end of the array.
 * Reallocates the array buffer automatically when additional capacity is
 * needed.
 * @param arr Darray** Double pointer to the destination array.
 * @param value const void* Pointer to the value to append.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_append(Darray **arr, const void *value);

/**
 * Prepends a value to the start of the array, shifting all elements right.
 * Reallocates the array buffer automatically when additional capacity is
 * needed.
 * @param arr Darray** Double pointer to the destination array.
 * @param value const void* Pointer to the value to prepend.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_prepend(Darray **arr, const void *value);

/**
 * Inserts a value at a specific index, shifting elements right.
 * @param arr Darray** Target array.
 * @param index size_t Zero-based index to insert at.
 * @param value const void* Pointer to the value to insert.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_insert(Darray **arr, size_t index, const void *value);

/**
 * Sets an element at a specific index.
 * @param arr Darray* Destination array.
 * @param index size_t Zero-based index to overwrite.
 * @param value const void* Pointer to the value to copy.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_set(Darray *arr, size_t index, const void *value);

/**
 * Fills a range of the array with a specific value via memcpy.
 * @param arr Darray* Destination array.
 * @param value const void* Pointer to the value to copy into the array
 * elements.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_fill_value(Darray *arr, const void *value, size_t start,
                              size_t end);

/**
 * Fills a range of the array using a callback function.
 * @param arr Darray* Destination array.
 * @param func Callback receiving the index, output pointer, and user context.
 * @param ctx void* User-provided context passed directly to the callback.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_fill_func(Darray *arr,
                             void (*func)(size_t index, void *out_value,
                                          void *ctx),
                             void *ctx, size_t start, size_t end);

/* ==========================================================================
 * ELEMENT REMOVAL (STRUCTURAL - REQUIRES DARRAY**)
 * ========================================================================== */

/**
 * Removes the last element and copies its data to out_value.
 * WARNING: This may trigger an internal shrink reallocation, which will
 * invalidate any pointers previously obtained via darray_get.
 * @param arr Darray** Target array double pointer.
 * @param out_value void* Pointer to destination memory. If NULL, the value is
 * discarded.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_pop(Darray **arr, void *out_value);

/**
 * Removes an element by swapping it with the last element and popping.
 * Fast O(1) removal that does not preserve array order.
 * WARNING: This may trigger an internal shrink reallocation, which will
 * invalidate any pointers previously obtained via darray_get.
 * @param arr Darray** Target array double pointer.
 * @param index size_t Index to remove.
 * @param out_value void* Pointer to destination memory. If NULL, the value is
 * discarded.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_unordered_remove(Darray **arr, size_t index,
                                    void *out_value);

/**
 * Removes an element and shifts all subsequent elements left.
 * O(n) removal that preserves array order.
 * WARNING: This may trigger an internal shrink reallocation, which will
 * invalidate any pointers previously obtained via darray_get.
 * @param arr Darray** Target array double pointer.
 * @param index size_t Index to remove.
 * @param out_value void* Pointer to destination memory. If NULL, the value is
 * discarded.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_ordered_remove(Darray **arr, size_t index, void *out_value);

/* ==========================================================================
 * ACCESS & SEARCH
 * ========================================================================== */

/**
 * Gets a pointer to the value at a specific index.
 * WARNING: The returned pointer points directly into the array's heap memory.
 * It must NOT be stored long-term. Any operation that adds or removes elements
 * (e.g., append, pop, remove, grow, shrink) may trigger a memory reallocation,
 * which will immediately invalidate this pointer. Always call darray_get right
 * before use.
 * @param arr const Darray* Source array.
 * @param index size_t Zero-based index.
 * @return ValueResult A pointer to the element or a DarrayError.
 */
ValueResult darray_get(const Darray *arr, size_t index);

/* ==========================================================================
 * ITERATION & TRANSFORMATION
 * ========================================================================== */

/**
 * Iterates over every element in the array.
 * @param arr Darray* Source array.
 * @param func Callback receiving the index, element pointer, and user context.
 * @param ctx void* User-provided context passed directly to the callback.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_for_each(Darray *arr,
                            void (*func)(size_t index, void *value, void *ctx),
                            void *ctx);

/**
 * Creates a new array by applying a function to each element.
 * @param arr const Darray* Source array.
 * @param func Callback receiving index, input pointer, output pointer, and
 * context.
 * @param ctx void* User-provided context passed directly to the callback.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_map(const Darray *arr,
                        void (*func)(size_t index, const void *in_value,
                                     void *out_value, void *ctx),
                        void *ctx, Allocator *alloc);

/* ==========================================================================
 * DARRAY CONCATENATION
 * ========================================================================== */

/**
 * Concatenates two arrays into a newly allocated array.
 * @param arr1 const Darray* First array.
 * @param arr2 const Darray* Second array.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_concat(const Darray *arr1, const Darray *arr2,
                           Allocator *alloc);

/* ==========================================================================
 * SLICE OPERATIONS
 * ========================================================================== */

/**
 * Creates a non-owning slice view into an existing array.
 * @param arr const Darray* Source array.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarraySliceResult A DarraySlice or a DarrayError.
 */
DarraySliceResult darray_slice(const Darray *arr, size_t start, size_t end);

/**
 * Creates a slice of a slice (non-owning view).
 * @param slice const DarraySlice* Source slice.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarraySliceResult A DarraySlice or a DarrayError.
 */
DarraySliceResult darray_slice_of_slice(const DarraySlice *slice, size_t start,
                                        size_t end);

/**
 * Creates a new Darray from a slice (owned copy).
 * @param slice const DarraySlice* Source slice.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_from_slice(const DarraySlice *slice, Allocator *alloc);

/**
 * Iterates over every element in a slice.
 * @param slice DarraySlice* Source slice.
 * @param func Callback receiving the index, pointer to the element, and user
 * context.
 * @param ctx void* User-provided context passed directly to the callback.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_slice_for_each(DarraySlice *slice,
                                  void (*func)(size_t index, void *value,
                                               void *ctx),
                                  void *ctx);

/**
 * Maps a slice to a new Darray.
 * @param slice const DarraySlice* Source slice.
 * @param func Callback receiving the index, input pointer, output pointer, and
 * user context.
 * @param ctx void* User-provided context passed directly to the callback.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_slice_map(const DarraySlice *slice,
                              void (*func)(size_t index, const void *in_value,
                                           void *out_value, void *ctx),
                              void *ctx, Allocator *alloc);

/**
 * Concatenates a Darray and a DarraySlice into a new Darray.
 * @param arr const Darray* First array.
 * @param slice const DarraySlice* Slice to concatenate.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_concat_slice(const Darray *arr, const DarraySlice *slice,
                                 Allocator *alloc);

/**
 * Concatenates a DarraySlice and a Darray into a new Darray.
 * @param slice const DarraySlice* First slice.
 * @param arr const Darray* Second array.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_slice_concat(const DarraySlice *slice, const Darray *arr,
                                 Allocator *alloc);

/**
 * Concatenates two DarraySlices into a new Darray.
 * @param slice1 const DarraySlice* First slice.
 * @param slice2 const DarraySlice* Second slice.
 * @param alloc Allocator* Custom memory allocator. Pass NULL for stdlib malloc.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_slice_concat_slice(const DarraySlice *slice1,
                                       const DarraySlice *slice2,
                                       Allocator *alloc);

#endif // DARRAY_H
