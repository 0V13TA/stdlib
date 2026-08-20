#include "darray.h"
#include <stddef.h>
#include <stdio.h>

// --- Stateless Callbacks using ctx ---

void my_fill_func(size_t index, void *out_value, void *ctx) {
  int multiplier = *(int *)ctx;
  *(int *)out_value = (int)(index * multiplier);
}

void my_for_each_func(size_t index, void *value, void *ctx) {
  const char *prefix = (const char *)ctx;
  int val = *(int *)value;
  printf("%s[%zu] = %d\n", prefix, index, val);
}

void my_map_func(size_t index, const void *in_value, void *out_value, void *ctx) {
  int multiplier = *(int *)ctx;
  int val = *(const int *)in_value;
  *(int *)out_value = val * multiplier;
}

// --- Main Testbed ---

int main(void) {
  // 1. Creation
  DarrayResult darray_1_res = darray_create(30, sizeof(int));
  if (darray_1_res.is_error) return 1;
  Darray *darray_1 = darray_1_res.as.value;

  // 2. Filling with context
  int val30 = 30;
  int fill_mult = 2;
  darray_fill_value(darray_1, &val30, 0, darray_1->_length - 1);
  darray_fill_func(darray_1, my_fill_func, &fill_mult, 0, 20);

  // 3. Cloning
  DarrayResult clone_res = darray_clone(darray_1, 0, darray_1->_length - 1);
  Darray *darray_1_clone = clone_res.as.value;

  // 4. Structural Additions (Double Pointers)
  int val20 = 20;
  darray_set(darray_1_clone, 10, &val20); // Single ptr, no structural change
  darray_append(&darray_1_clone, &val30); // Double ptr, might realloc
  darray_prepend(&darray_1_clone, &val30);

  // 5. Iteration & Mapping
  char *prefix = "Array1";
  int map_mult = 10;
  darray_for_each(darray_1, my_for_each_func, prefix);
  
  DarrayResult double_darray_res = darray_map(darray_1, my_map_func, &map_mult);
  Darray *double_darray_1 = double_darray_res.as.value;

  // 6. Safe Removals (Out Parameters)
  int popped_val;
  if (darray_pop(&darray_1_clone, &popped_val) == DARRAY_OK) {
    printf("Popped: %d\n", popped_val);
  }

  // Remove and discard value by passing NULL
  darray_unordered_remove(&darray_1_clone, 5, NULL); 
  darray_ordered_remove(&darray_1_clone, 5, NULL);

  // 7. Slices & Concatenation
  DarraySliceResult slice_res = darray_slice(darray_1, 0, 5);
  if (!slice_res.is_error) {
    DarraySlice slice = slice_res.as.value;
    
    // Test slice concatenation
    DarrayResult concat_slice_res = darray_concat_slice(darray_1, &slice);
    if (!concat_slice_res.is_error) {
      darray_free(&concat_slice_res.as.value);
    }
  }

  // 8. Explicit Capacity Management
  darray_grow(&double_darray_1, 35);
  darray_shrink(&double_darray_1, 25);
  darray_shrink_to_fit(&double_darray_1);

  // 9. Freeing (Double pointers reset caller's pointer to NULL)
  darray_free(&darray_1);
  darray_free(&darray_1_clone);
  darray_free(&double_darray_1);

  return 0;
}
