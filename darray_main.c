#include "darray.h"
#include <stddef.h>
#include <stdio.h>

// Callbacks pass data via pointers for memory safety and type casting
void my_fill_func(size_t index, void *out_value) {
  *(int *)out_value = (int)(index * 2);
}

void my_for_each_func(size_t index, void *value) {
  int val = *(int *)value;
  printf("val = %d\n", val);
}

void my_map_func(size_t index, void *in_value, void *out_value) {
  int val = *(int *)in_value;
  *(int *)out_value = val * 2;
}

int main(void) {
  // 1. Create returns a DarrayResult containing a Darray* pointer
  DarrayResult darray_1_res = darray_create(30, sizeof(int));
  // Do some error checking here...
  Darray *darray_1 = darray_1_res.as.value;

  int val30 = 30;

  // 2. Non-structural mutations take Darray* (single pointer)
  darray_fill_value(darray_1, &val30, 0, darray_1->_length - 1);
  darray_fill_func(darray_1, my_fill_func, 0, 20);

  // Cloning takes Darray* and returns a DarrayResult holding a new Darray*
  DarrayResult clone_res = darray_clone(darray_1, 0, darray_1->_length - 1);
  Darray *darray_1_clone = clone_res.as.value;

  int val20 = 20;
  darray_set(darray_1_clone, 10, &val20); // Takes Darray*

  // 3. Structural mutations (might realloc) take Darray** (double pointer)
  darray_append(&darray_1_clone, &val30);
  darray_prepend(&darray_1_clone, &val30);

  // 4. Read operations take Darray*
  DarraySliceResult slice_res = darray_slice(darray_1, 0, 5);

  darray_for_each(darray_1, my_for_each_func);

  DarrayResult double_darray_res = darray_map(darray_1, my_map_func);
  Darray *double_darray_1 = double_darray_res.as.value;

  ValueResult get_res = darray_get(double_darray_1, 10);
  // Do some error checking here

  // 5. Removals are structural and might trigger a shrink, so they take
  // Darray**
  ValueResult pop_res = darray_pop(&darray_1_clone);
  ValueResult unordered_remove_res =
      darray_unordered_remove(&darray_1_clone, 5); // Swap and pop
  ValueResult ordered_remove_res =
      darray_ordered_remove(&darray_1_clone, 5); // Remove and shift

  // 6. Explicit capacity changes take Darray**
  darray_grow(&double_darray_1, 35);
  darray_shrink(&double_darray_1, 25);
  darray_shrink_to_fit(&double_darray_1);

  // Concat takes two Darray* pointers
  DarrayResult concat_res = darray_concat(darray_1_clone, darray_1);

  // 7. Free takes Darray** to safely set the caller's pointer to NULL after
  // freeing
  darray_free(&darray_1);
  darray_free(&darray_1_clone);
  darray_free(&double_darray_1);
  if (!concat_res.is_error) {
    darray_free(&concat_res.as.value);
  }

  return 0;
}
