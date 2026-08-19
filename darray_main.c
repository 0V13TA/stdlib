#include <stdio.h>

void* fill_darray_func(size_t index) {
  return (void*)(index * 2);
}

void for_each_element_func(size_t index, void* value) {
  int val = (int) *value;
  printf("val = %d\n", val);
}

void* map_each_element_func(size_t index, void* value) {
  int val = (int) *value;
  return val*2;
}

// TODO: There should be equivalent functions
// for DarraySlice e.g concat_darray_slice
// concat_slice_darray, concat_slice,
// for_each_slice_elem, map_each_slice_elem etc
int main(void) {
  DarrayResult darray_1 = create_darray(30 * size_of(int));
  fill_darray_value(darray_1, (void*)30, 0, darray_1._len-1);
  fill_darray_func(darray_1, fill_darray_func, 0, 20);
  DarrayResult darray_1_clone = from_darray(darray_1, 0, darray_1._len-1);
  darray_set(darray_1_clone, 10, (void*)20);
  darray_append(&darray_1_clone, (void*)30);
  darray_prepend(&darray_1_clone, (void*)30);
  DarraySliceResult darray_1_slice = slice_darray(darray_1, 0, 5);

  for_each_darray_elem(darray_1, for_each_element_func);
  DarrayResult double_darray_1 = map_each_darray_elem(darray_1, map_each_element_func);
  ValueResult double_darray_1_value = darray_get(double_darray_1, 10);
  // Do some error checking here

  ValueResult darray_1_clone_pop = darray_pop(darray_1_clone);
  // Do some error checking here
  ValueResult darray_1_clone_unordered_remove = darray_unordered_remove(darray_1_clone, 5); // Does swap and pop
  // Do some error checking here
  ValueResult darray_1_clone_ordered_remove = darray_ordered_remove(darray_1_clone, 5); // Does remove and shift
  // Do some error checking here


  grow_darray(&double_darray_1, 35); // Modifies the capacity
  shrink_darray(&double_darray_1, 25);
  shrink_to_fit_darray(&double_darray_1); // Simple macro that calls shrink_darray with the width of the darray as the value.

  DarrayResult concated = concat_darray(darray_1_clone, darray_1);
  free_darray(darray_1);
  free_darray(darray_1_clone);
  return 0;
}
