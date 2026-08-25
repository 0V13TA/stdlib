/* Copyright (c) 2026 OVIETA <ovieta17@gmail.com>
 *
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

#include "darray.h"
#include <stdlib.h> // For size_t (and malloc/free fallback from header)
#include <string.h> // For memcpy

/* ==========================================================================
 * LIFECYCLE (CREATION & DESTRUCTION)
 * ========================================================================== */

DarrayResult darray_create(size_t capacity, size_t element_size,
                           Allocator *alloc) {
  DarrayResult res = {0};

  if (element_size == 0) {
    res.as.error = DARRAY_ERR_ALLOC;
    res.is_error = 1;
    return res;
  }

  size_t total_size = DARRAY_CALC_SIZE(capacity, element_size);
  Darray *arr = (Darray *)internal_malloc(alloc, total_size);

  if (!arr) {
    res.as.error = DARRAY_ERR_ALLOC;
    res.is_error = 1;
    return res;
  }

  arr->_length = 0;
  arr->_capacity = capacity;
  arr->element_size = element_size;
  arr->allocator = alloc;

  res.as.value = arr;
  res.is_error = 0;
  return res;
}

DarrayResult darray_clone(const Darray *arr, size_t start, size_t end,
                          Allocator *alloc) {
  DarrayResult res = {0};

  if (!arr) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }

  if (start > end || end >= arr->_length) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  size_t clone_len = end - start + 1;
  res = darray_create(clone_len, arr->element_size, alloc);

  if (res.is_error) {
    return res; // Pass the allocation error up
  }

  Darray *new_arr = res.as.value;
  new_arr->_length = clone_len;

  // Copy the specific slice of memory over
  void *src_ptr = (void *)(arr->data + (start * arr->element_size));
  memcpy(new_arr->data, src_ptr, clone_len * arr->element_size);

  return res;
}

void darray_free(Darray **arr) {
  if (!arr || !*arr)
    return;

  internal_free((*arr)->allocator, *arr);
  *arr = NULL; // Prevent use-after-free on the caller's side
}

/* ==========================================================================
 * CAPACITY MANAGEMENT
 * ========================================================================== */

DarrayError darray_grow(Darray **arr, size_t new_capacity) {
  if (!arr || !*arr)
    return DARRAY_ERR_NULL_PTR;

  if (new_capacity <= (*arr)->_capacity) {
    return DARRAY_OK; // Nothing to do, capacity is already large enough
  }

  size_t total_size = DARRAY_CALC_SIZE(new_capacity, (*arr)->element_size);
  Darray *new_ptr =
      (Darray *)internal_realloc((*arr)->allocator, *arr, total_size);

  if (!new_ptr) {
    return DARRAY_ERR_ALLOC;
  }

  *arr = new_ptr; // Update caller's pointer
  (*arr)->_capacity = new_capacity;

  return DARRAY_OK;
}

DarrayError darray_shrink(Darray **arr, size_t new_capacity) {
  if (!arr || !*arr)
    return DARRAY_ERR_NULL_PTR;

  if (new_capacity >= (*arr)->_capacity) {
    return DARRAY_OK; // Shrink target must be smaller than current capacity
  }

  // Calculate new total size
  size_t total_size = DARRAY_CALC_SIZE(new_capacity, (*arr)->element_size);

  Darray *new_ptr =
      (Darray *)internal_realloc((*arr)->allocator, *arr, total_size);

  if (!new_ptr) {
    return DARRAY_ERR_ALLOC;
  }

  *arr = new_ptr;
  (*arr)->_capacity = new_capacity;

  // Truncate length if we shrank past the current element count
  if ((*arr)->_length > new_capacity) {
    (*arr)->_length = new_capacity;
  }

  return DARRAY_OK;
}

DarrayError darray_reserve(Darray **arr, size_t additional_elements) {
  if (!arr || !*arr)
    return DARRAY_ERR_NULL_PTR;

  size_t required_capacity = (*arr)->_length + additional_elements;

  if (required_capacity <= (*arr)->_capacity) {
    return DARRAY_OK;
  }

  // Growth Strategy: Double the current capacity, or match required, whichever
  // is bigger. Handle edge case where capacity is currently 0.
  size_t new_capacity = (*arr)->_capacity == 0 ? 4 : (*arr)->_capacity * 2;
  if (new_capacity < required_capacity) {
    new_capacity = required_capacity;
  }

  return darray_grow(arr, new_capacity);
}

/* ==========================================================================
 * ELEMENT MUTATION & ADDITION
 * ========================================================================== */

DarrayError darray_append(Darray **arr, const void *value) {
  if (!arr || !*arr || !value)
    return DARRAY_ERR_NULL_PTR;

  DarrayError err = darray_reserve(arr, 1);
  if (err != DARRAY_OK)
    return err;

  void *dest = (*arr)->data + ((*arr)->_length * (*arr)->element_size);
  memcpy(dest, value, (*arr)->element_size);
  (*arr)->_length++;

  return DARRAY_OK;
}

DarrayError darray_insert(Darray **arr, size_t index, const void *value) {
  if (!arr || !*arr || !value)
    return DARRAY_ERR_NULL_PTR;

  // Inserting exactly at _length is valid (equivalent to append)
  if (index > (*arr)->_length)
    return DARRAY_ERR_BOUNDS;

  DarrayError err = darray_reserve(arr, 1);
  if (err != DARRAY_OK)
    return err;

  uint8_t *base = (*arr)->data;
  size_t el_size = (*arr)->element_size;

  // If we aren't inserting at the very end, shift subsequent elements right
  if (index < (*arr)->_length) {
    void *src = base + (index * el_size);
    void *dest = base + ((index + 1) * el_size);
    size_t bytes_to_move = ((*arr)->_length - index) * el_size;
    memmove(dest, src, bytes_to_move);
  }

  void *target = base + (index * el_size);
  memcpy(target, value, el_size);
  (*arr)->_length++;

  return DARRAY_OK;
}

DarrayError darray_prepend(Darray **arr, const void *value) {
  return darray_insert(arr, 0, value);
}

DarrayError darray_set(Darray *arr, size_t index, const void *value) {
  if (!arr || !value)
    return DARRAY_ERR_NULL_PTR;
  if (index >= arr->_length)
    return DARRAY_ERR_BOUNDS;

  void *dest = arr->data + (index * arr->element_size);
  memcpy(dest, value, arr->element_size);

  return DARRAY_OK;
}

DarrayError darray_fill_value(Darray *arr, const void *value, size_t start,
                              size_t end) {
  if (!arr || !value)
    return DARRAY_ERR_NULL_PTR;
  if (start > end || end >= arr->_length)
    return DARRAY_ERR_BOUNDS;

  size_t el_size = arr->element_size;
  for (size_t i = start; i <= end; i++) {
    void *dest = arr->data + (i * el_size);
    memcpy(dest, value, el_size);
  }

  return DARRAY_OK;
}

DarrayError darray_fill_func(Darray *arr,
                             void (*func)(size_t index, void *out_value,
                                          void *ctx),
                             void *ctx, size_t start, size_t end) {
  if (!arr || !func)
    return DARRAY_ERR_NULL_PTR;
  if (start > end || end >= arr->_length)
    return DARRAY_ERR_BOUNDS;

  size_t el_size = arr->element_size;
  for (size_t i = start; i <= end; i++) {
    void *dest = arr->data + (i * el_size);
    func(i, dest, ctx);
  }

  return DARRAY_OK;
}

/* ==========================================================================
 * ACCESS & SEARCH
 * ========================================================================== */

DarrayError darray_get(const Darray *arr, size_t index, void *out_value) {
  if (!arr || !out_value)
    return DARRAY_ERR_NULL_PTR;

  if (index >= arr->_length)
    return DARRAY_ERR_BOUNDS;

  const void *src = arr->data + (index * arr->element_size);
  memcpy(out_value, src, arr->element_size);

  return DARRAY_OK;
}

ValueResult darray_get_ptr(const Darray *arr, size_t index) {
  ValueResult res = {0};

  if (!arr) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (index >= arr->_length) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  res.as.value = (void *)(arr->data + (index * arr->element_size));
  res.is_error = 0;
  return res;
}

/* ==========================================================================
 * ELEMENT REMOVAL
 * ========================================================================== */

// Internal helper to automatically shrink arrays that get too empty
static void internal_shrink_if_needed(Darray **arr) {
  if (!arr || !*arr)
    return;
  size_t cap = (*arr)->_capacity;
  size_t len = (*arr)->_length;

  // Hysteresis: If utilizing <= 25% of capacity, shrink it down to 50%
  if (cap > 8 && len <= cap / 4) {
    (void)darray_shrink(arr, cap / 2);
  }
}

DarrayError darray_pop(Darray **arr, void *out_value) {
  if (!arr || !*arr)
    return DARRAY_ERR_NULL_PTR;
  if ((*arr)->_length == 0)
    return DARRAY_ERR_BOUNDS;

  (*arr)->_length--;

  if (out_value) {
    void *src = (*arr)->data + ((*arr)->_length * (*arr)->element_size);
    memcpy(out_value, src, (*arr)->element_size);
  }

  internal_shrink_if_needed(arr);
  return DARRAY_OK;
}

DarrayError darray_unordered_remove(Darray **arr, size_t index,
                                    void *out_value) {
  if (!arr || !*arr)
    return DARRAY_ERR_NULL_PTR;
  if (index >= (*arr)->_length)
    return DARRAY_ERR_BOUNDS;

  size_t el_size = (*arr)->element_size;
  uint8_t *base = (*arr)->data;
  void *target = base + (index * el_size);

  if (out_value) {
    memcpy(out_value, target, el_size);
  }

  (*arr)->_length--;

  // If we didn't just remove the last element, copy the last element into the
  // removed slot
  if (index != (*arr)->_length) {
    void *last = base + ((*arr)->_length * el_size);
    memcpy(target, last, el_size);
  }

  internal_shrink_if_needed(arr);
  return DARRAY_OK;
}

DarrayError darray_ordered_remove(Darray **arr, size_t index, void *out_value) {
  if (!arr || !*arr)
    return DARRAY_ERR_NULL_PTR;
  if (index >= (*arr)->_length)
    return DARRAY_ERR_BOUNDS;

  size_t el_size = (*arr)->element_size;
  uint8_t *base = (*arr)->data;
  void *target = base + (index * el_size);

  if (out_value) {
    memcpy(out_value, target, el_size);
  }

  (*arr)->_length--;

  // Shift everything after the index one slot left to close the gap
  if (index < (*arr)->_length) {
    void *src = base + ((index + 1) * el_size);
    size_t bytes_to_move = ((*arr)->_length - index) * el_size;
    memmove(target, src, bytes_to_move);
  }

  internal_shrink_if_needed(arr);
  return DARRAY_OK;
}

/* ==========================================================================
 * ITERATION & TRANSFORMATION
 * ========================================================================== */

DarrayError darray_for_each(Darray *arr,
                            void (*func)(size_t index, void *value, void *ctx),
                            void *ctx) {
  if (!arr || !func)
    return DARRAY_ERR_NULL_PTR;

  size_t el_size = arr->element_size;
  for (size_t i = 0; i < arr->_length; i++) {
    void *val_ptr = arr->data + (i * el_size);
    func(i, val_ptr, ctx);
  }

  return DARRAY_OK;
}

DarrayResult darray_map(const Darray *arr,
                        void (*func)(size_t index, const void *in_value,
                                     void *out_value, void *ctx),
                        void *ctx, Allocator *alloc) {
  DarrayResult res = {0};
  if (!arr || !func) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }

  res = darray_create(arr->_length, arr->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = arr->_length;
  size_t el_size = arr->element_size;

  for (size_t i = 0; i < arr->_length; i++) {
    void *in_ptr = (void *)(arr->data + (i * el_size));
    void *out_ptr = new_arr->data + (i * el_size);
    func(i, in_ptr, out_ptr, ctx);
  }

  return res;
}

/* ==========================================================================
 * DARRAY CONCATENATION
 * ========================================================================== */

DarrayResult darray_concat(const Darray *arr1, const Darray *arr2,
                           Allocator *alloc) {
  DarrayResult res = {0};
  if (!arr1 || !arr2) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  // Optional but safe: Ensure element sizes match before blinding copying
  if (arr1->element_size != arr2->element_size) {
    res.as.error =
        DARRAY_ERR_BOUNDS; // Reusing bounds error for structural mismatch
    res.is_error = 1;
    return res;
  }

  size_t new_len = arr1->_length + arr2->_length;
  res = darray_create(new_len, arr1->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = new_len;
  size_t el_size = arr1->element_size;

  memcpy(new_arr->data, arr1->data, arr1->_length * el_size);
  memcpy(new_arr->data + (arr1->_length * el_size), arr2->data,
         arr2->_length * el_size);

  return res;
}

/* ==========================================================================
 * SLICE OPERATIONS
 * ========================================================================== */

DarraySliceResult darray_slice(const Darray *arr, size_t start, size_t end) {
  DarraySliceResult res = {0};
  if (!arr) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (start > end || end >= arr->_length) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  res.as.value.data = (void *)(arr->data + (start * arr->element_size));
  res.as.value._length = end - start + 1;
  res.as.value.element_size = arr->element_size;
  res.is_error = 0;

  return res;
}

DarraySliceResult darray_slice_of_slice(const DarraySlice *slice, size_t start,
                                        size_t end) {
  DarraySliceResult res = {0};
  if (!slice || !slice->data) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (start > end || end >= slice->_length) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  uint8_t *base = (uint8_t *)slice->data;
  res.as.value.data = (void *)(base + (start * slice->element_size));
  res.as.value._length = end - start + 1;
  res.as.value.element_size = slice->element_size;
  res.is_error = 0;

  return res;
}

DarrayResult darray_from_slice(const DarraySlice *slice, Allocator *alloc) {
  DarrayResult res = {0};
  if (!slice || !slice->data) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }

  res = darray_create(slice->_length, slice->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = slice->_length;
  memcpy(new_arr->data, slice->data, slice->_length * slice->element_size);

  return res;
}

DarrayError darray_slice_for_each(DarraySlice *slice,
                                  void (*func)(size_t index, void *value,
                                               void *ctx),
                                  void *ctx) {
  if (!slice || !slice->data || !func)
    return DARRAY_ERR_NULL_PTR;

  size_t el_size = slice->element_size;
  uint8_t *base = (uint8_t *)slice->data;

  for (size_t i = 0; i < slice->_length; i++) {
    func(i, (void *)(base + (i * el_size)), ctx);
  }

  return DARRAY_OK;
}

DarrayResult darray_slice_map(const DarraySlice *slice,
                              void (*func)(size_t index, const void *in_value,
                                           void *out_value, void *ctx),
                              void *ctx, Allocator *alloc) {
  DarrayResult res = {0};
  if (!slice || !slice->data || !func) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }

  res = darray_create(slice->_length, slice->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = slice->_length;
  size_t el_size = slice->element_size;
  uint8_t *base = (uint8_t *)slice->data;

  for (size_t i = 0; i < slice->_length; i++) {
    void *in_ptr = (void *)(base + (i * el_size));
    void *out_ptr = new_arr->data + (i * el_size);
    func(i, in_ptr, out_ptr, ctx);
  }

  return res;
}

DarrayResult darray_concat_slice(const Darray *arr, const DarraySlice *slice,
                                 Allocator *alloc) {
  DarrayResult res = {0};
  if (!arr || !slice || !slice->data) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (arr->element_size != slice->element_size) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  size_t new_len = arr->_length + slice->_length;
  res = darray_create(new_len, arr->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = new_len;
  size_t el_size = arr->element_size;

  memcpy(new_arr->data, arr->data, arr->_length * el_size);
  memcpy(new_arr->data + (arr->_length * el_size), slice->data,
         slice->_length * el_size);

  return res;
}

DarrayResult darray_slice_concat(const DarraySlice *slice, const Darray *arr,
                                 Allocator *alloc) {
  DarrayResult res = {0};
  if (!arr || !slice || !slice->data) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (arr->element_size != slice->element_size) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  size_t new_len = slice->_length + arr->_length;
  res = darray_create(new_len, slice->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = new_len;
  size_t el_size = slice->element_size;

  memcpy(new_arr->data, slice->data, slice->_length * el_size);
  memcpy(new_arr->data + (slice->_length * el_size), arr->data,
         arr->_length * el_size);

  return res;
}

DarrayResult darray_slice_concat_slice(const DarraySlice *slice1,
                                       const DarraySlice *slice2,
                                       Allocator *alloc) {
  DarrayResult res = {0};
  if (!slice1 || !slice1->data || !slice2 || !slice2->data) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (slice1->element_size != slice2->element_size) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }

  size_t new_len = slice1->_length + slice2->_length;
  res = darray_create(new_len, slice1->element_size, alloc);
  if (res.is_error)
    return res;

  Darray *new_arr = res.as.value;
  new_arr->_length = new_len;
  size_t el_size = slice1->element_size;

  memcpy(new_arr->data, slice1->data, slice1->_length * el_size);
  memcpy(new_arr->data + (slice1->_length * el_size), slice2->data,
         slice2->_length * el_size);

  return res;
}

DarrayError darray_slice_get(const DarraySlice *slice, size_t index,
                             void *out_value) {
  if (!slice || !slice->data || !out_value)
    return DARRAY_ERR_NULL_PTR;

  if (index >= slice->_length)
    return DARRAY_ERR_BOUNDS;

  uint8_t *base = (uint8_t *)slice->data;
  const void *src = base + (index * slice->element_size);
  memcpy(out_value, src, slice->element_size);
  return DARRAY_OK;
}

ValueResult darray_slice_get_ptr(const DarraySlice *slice, size_t index) {
  ValueResult res = {0};
  if (!slice || !slice->data) {
    res.as.error = DARRAY_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }
  if (index >= slice->_length) {
    res.as.error = DARRAY_ERR_BOUNDS;
    res.is_error = 1;
    return res;
  }
  uint8_t *base = (uint8_t *)slice->data;
  res.as.value = (void *)(base + (index * slice->element_size));
  res.is_error = 0;
  return res;
}
