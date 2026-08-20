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

#define DARRAY_NPOS ((size_t)-1)

typedef enum {
  DARRAY_OK = 0,
  DARRAY_ERR_ALLOC,   // Malloc or realloc failed
  DARRAY_ERR_BOUNDS,  // Index out of bounds
  DARRAY_ERR_NULL_PTR // Null pointer passed to function
} DarrayError;

// C99 Stretchy Buffer implementation
typedef struct Darray {
  size_t _length;
  size_t _capacity;
  size_t element_size;
  uint8_t data[]; // Flexible Array Member (FAM). Must be the last member.
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
 * Inserts a value at a specific index, shifting elements right.
 * @param arr Darray** Target array.
 * @param index size_t Zero-based index to insert at.
 * @param value void* Pointer to the value to insert.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_insert(Darray **arr, size_t index, void *value);

/**
 * Creates a dynamic array.
 * Allocates the array header and data buffer in a single contiguous block.
 * @param capacity size_t Initial buffer capacity in number of elements.
 * @param element_size size_t Size of a single element in bytes.
 * @return DarrayResult An initialized Darray pointer or a DarrayError.
 */
DarrayResult darray_create(size_t capacity, size_t element_size);

/**
 * Releases a dynamic array's memory.
 * Frees the block and sets the caller's pointer to NULL.
 * @param arr Darray** Double pointer to the array to release.
 * @return void No value is returned.
 */
void darray_free(Darray **arr);

/**
 * Ensures the array has at least enough capacity for additional elements.
 * @param arr Darray** Target array.
 * @param additional_elements size_t Number of elements to reserve space for.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_reserve(Darray **arr, size_t additional_elements);

/**
 * Fills a range of the array with a specific value via memcpy.
 * @param arr Darray* Destination array.
 * @param value void* Pointer to the value to copy into the array elements.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_fill_value(Darray *arr, void *value, size_t start,
                              size_t end);

/**
 * Fills a range of the array using a callback function.
 * @param arr Darray* Destination array.
 * @param func void (*)(size_t, void*) Callback receiving the index and a
 * pointer to write the generated value to.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_fill_func(Darray *arr, void (*func)(size_t, void *),
                             size_t start, size_t end);

/**
 * Creates an owned copy of a subset of an array.
 * @param arr Darray* Source array.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_clone(Darray *arr, size_t start, size_t end);

/**
 * Sets an element at a specific index.
 * @param arr Darray* Destination array.
 * @param index size_t Zero-based index to overwrite.
 * @param value void* Pointer to the value to copy.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_set(Darray *arr, size_t index, void *value);

/**
 * Appends a value to the end of the array.
 * Reallocates the array buffer automatically when additional capacity is
 * needed.
 * @param arr Darray** Double pointer to the destination array.
 * @param value void* Pointer to the value to append.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_append(Darray **arr, void *value);

/**
 * Prepends a value to the start of the array, shifting all elements right.
 * Reallocates the array buffer automatically when additional capacity is
 * needed.
 * @param arr Darray** Double pointer to the destination array.
 * @param value void* Pointer to the value to prepend.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_prepend(Darray **arr, void *value);

/**
 * Creates a non-owning slice view into an existing array.
 * @param arr Darray* Source array.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarraySliceResult A DarraySlice or a DarrayError.
 */
DarraySliceResult darray_slice(Darray *arr, size_t start, size_t end);

/**
 * Iterates over every element in the array.
 * @param arr Darray* Source array.
 * @param func void (*)(size_t, void*) Callback receiving the index and a
 * pointer to the element.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_for_each(Darray *arr, void (*func)(size_t, void *));

/**
 * Creates a new array by applying a function to each element.
 * @param arr Darray* Source array.
 * @param func void (*)(size_t, void*, void*) Callback receiving the index,
 * input pointer, and output pointer.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_map(Darray *arr, void (*func)(size_t, void *, void *));

/**
 * Gets a pointer to the value at a specific index.
 * @param arr Darray* Source array.
 * @param index size_t Zero-based index.
 * @return ValueResult A pointer to the element or a DarrayError.
 */
ValueResult darray_get(Darray *arr, size_t index);

/**
 * Removes and returns a pointer to the last element.
 * The returned pointer points to the array's memory (which is now in the
 * capacity space). May trigger a shrink reallocation.
 * @param arr Darray** Target array.
 * @return ValueResult A pointer to the popped element or a DarrayError.
 */
ValueResult darray_pop(Darray **arr);

/**
 * Removes an element by swapping it with the last element and popping.
 * Fast O(1) removal that does not preserve array order. May trigger a shrink.
 * @param arr Darray** Target array.
 * @param index size_t Index to remove.
 * @return ValueResult A pointer to the removed element or a DarrayError.
 */
ValueResult darray_unordered_remove(Darray **arr, size_t index);

/**
 * Removes an element and shifts all subsequent elements left.
 * O(n) removal that preserves array order. May trigger a shrink.
 * @param arr Darray** Target array.
 * @param index size_t Index to remove.
 * @return ValueResult A pointer to the removed element or a DarrayError.
 */
ValueResult darray_ordered_remove(Darray **arr, size_t index);

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

/**
 * Concatenates two arrays into a newly allocated array.
 * @param arr1 Darray* First array.
 * @param arr2 Darray* Second array.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_concat(Darray *arr1, Darray *arr2);

// Additional functions for slice operations

/**
 * Concatenates a Darray and a DarraySlice into a new Darray.
 * @param arr Darray* First array.
 * @param slice DarraySlice* Slice to concatenate.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_concat_slice(Darray *arr, DarraySlice *slice);

/**
 * Concatenates a DarraySlice and a Darray into a new Darray.
 * @param slice DarraySlice* First slice.
 * @param arr Darray* Second array.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_slice_concat(DarraySlice *slice, Darray *arr);

/**
 * Concatenates two DarraySlices into a new Darray.
 * @param slice1 DarraySlice* First slice.
 * @param slice2 DarraySlice* Second slice.
 * @return DarrayResult A concatenated owned Darray pointer or a DarrayError.
 */
DarrayResult darray_slice_concat_slice(DarraySlice *slice1,
                                       DarraySlice *slice2);

/**
 * Creates a new Darray from a slice (owned copy).
 * @param slice DarraySlice* Source slice.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_from_slice(DarraySlice *slice);

/**
 * Iterates over every element in a slice.
 * @param slice DarraySlice* Source slice.
 * @param func void (*)(size_t, void*) Callback receiving the index and a
 * pointer to the element.
 * @return DarrayError DARRAY_OK on success; otherwise a DarrayError.
 */
DarrayError darray_slice_for_each(DarraySlice *slice,
                                  void (*func)(size_t, void *));

/**
 * Creates a slice of a slice (non-owning view).
 * @param slice DarraySlice* Source slice.
 * @param start size_t Inclusive zero-based start index.
 * @param end size_t Inclusive zero-based end index.
 * @return DarraySliceResult A DarraySlice or a DarrayError.
 */
DarraySliceResult darray_slice_of_slice(DarraySlice *slice, size_t start,
                                        size_t end);

/**
 * Maps a slice to a new Darray.
 * @param slice DarraySlice* Source slice.
 * @param func void (*)(size_t, void*, void*) Callback receiving the index,
 * input pointer, and output pointer.
 * @return DarrayResult A new owned Darray pointer or a DarrayError.
 */
DarrayResult darray_slice_map(DarraySlice *slice,
                              void (*func)(size_t, void *, void *));

#endif // DARRAY_H
