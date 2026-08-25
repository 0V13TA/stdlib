#include "darray.h"
#include "munit.h" // Ensure munit.h and munit.c are in your build path
#include <stddef.h>

#define UNUSED_TEST_ARGS                                                       \
  (void)params;                                                                \
  (void)data

// --- Stateless Callbacks using ctx ---

static void test_fill_func(size_t index, void *out_value, void *ctx) {
  int multiplier = *(int *)ctx;
  *(int *)out_value = (int)(index * multiplier);
}

static void test_for_each_sum_func(size_t index, void *value, void *ctx) {
  (void)index;
  int *sum = (int *)ctx;
  *sum += *(int *)value; // Accumulate sum for assertion
}

static void test_map_func(size_t index, const void *in_value, void *out_value,
                          void *ctx) {
  (void)index;
  int multiplier = *(int *)ctx;
  int val = *(const int *)in_value;
  *(int *)out_value = val * multiplier;
}

// --- Test Cases ---

static MunitResult test_lifecycle(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  DarrayResult res = darray_create(30, sizeof(int), NULL);
  munit_assert_false(res.is_error);

  Darray *arr = res.as.value;
  munit_assert_not_null(arr);
  munit_assert_size(darray_cap(arr), ==, 30);
  munit_assert_size(darray_len(arr), ==, 0);
  munit_assert_true(darray_empty(arr));

  darray_free(&arr);
  munit_assert_null(arr); // Free should nullify the double pointer

  return MUNIT_OK;
}

static MunitResult test_additions_and_removals(const MunitParameter params[],
                                               void *data) {
  UNUSED_TEST_ARGS;
  DarrayResult res = darray_create(5, sizeof(int), NULL);
  Darray *arr = res.as.value;

  // Test Append
  int val1 = 10, val2 = 20, val3 = 30;
  munit_assert_int(darray_append(&arr, &val1), ==, DARRAY_OK);
  munit_assert_int(darray_append(&arr, &val2), ==, DARRAY_OK);
  munit_assert_size(darray_len(arr), ==, 2);

  // Test Prepend
  munit_assert_int(darray_prepend(&arr, &val3), ==,
                   DARRAY_OK); // Arr: [30, 10, 20]

  // Test Safe Get
  int safe_val = 0;
  munit_assert_int(darray_get(arr, 0, &safe_val), ==, DARRAY_OK);
  munit_assert_int(safe_val, ==, 30);

  // Test Pointer Get
  ValueResult get_res = darray_get_ptr(arr, 0);
  munit_assert_false(get_res.is_error);
  munit_assert_int(*(int *)get_res.as.value, ==, 30);

  // Test Pop (Out Parameter)
  int popped = 0;
  munit_assert_int(darray_pop(&arr, &popped), ==, DARRAY_OK);
  munit_assert_int(popped, ==, 20);
  munit_assert_size(darray_len(arr), ==, 2);

  darray_free(&arr);
  return MUNIT_OK;
}

static MunitResult test_iteration_and_mapping(const MunitParameter params[],
                                              void *data) {
  UNUSED_TEST_ARGS;
  DarrayResult res = darray_create(10, sizeof(int), NULL);
  Darray *arr = res.as.value;

  // Setup array with [0, 2, 4, 6, 8]
  int fill_mult = 2;
  for (int i = 0; i < 5; i++) {
    darray_append(&arr, &i); // Dummy values just to advance length
  }
  darray_fill_func(arr, test_fill_func, &fill_mult, 0, 4);

  // Test For Each (Summing)
  int sum = 0;
  darray_for_each(arr, test_for_each_sum_func, &sum);
  munit_assert_int(sum, ==, 20); // 0 + 2 + 4 + 6 + 8

  // Test Map (Multiply by 10)
  int map_mult = 10;
  DarrayResult map_res = darray_map(arr, test_map_func, &map_mult, NULL);
  munit_assert_false(map_res.is_error);
  Darray *mapped_arr = map_res.as.value;

  int mapped_val = 0;
  munit_assert_int(darray_get(mapped_arr, 2, &mapped_val), ==, DARRAY_OK);
  munit_assert_int(mapped_val, ==, 40); // 4 * 10

  darray_free(&arr);
  darray_free(&mapped_arr);
  return MUNIT_OK;
}

static MunitResult test_slices(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  DarrayResult res = darray_create(10, sizeof(int), NULL);
  Darray *arr = res.as.value;

  for (int i = 0; i < 10; i++)
    darray_append(&arr, &i);

  // Slice elements 2 through 6
  DarraySliceResult slice_res = darray_slice(arr, 2, 6);
  munit_assert_false(slice_res.is_error);
  DarraySlice slice = slice_res.as.value;
  munit_assert_size(slice._length, ==, 5); // 2, 3, 4, 5, 6

  // Test Slice Gets
  int slice_val = 0;
  munit_assert_int(darray_slice_get(&slice, 1, &slice_val), ==, DARRAY_OK);
  munit_assert_int(slice_val, ==, 3);

  ValueResult slice_ptr_res = darray_slice_get_ptr(&slice, 1);
  munit_assert_false(slice_ptr_res.is_error);
  munit_assert_int(*(int *)slice_ptr_res.as.value, ==, 3);

  // Concat Slice to Array
  DarrayResult concat_res = darray_concat_slice(arr, &slice, NULL);
  munit_assert_false(concat_res.is_error);
  munit_assert_size(darray_len(concat_res.as.value), ==, 15);

  darray_free(&arr);
  darray_free(&concat_res.as.value);
  return MUNIT_OK;
}

// --- Munit Suite Registration ---

static MunitTest test_suite_tests[] = {
    {(char *)"/lifecycle", test_lifecycle, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/additions_and_removals", test_additions_and_removals, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/iteration_and_mapping", test_iteration_and_mapping, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/slices", test_slices, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // Add a NULL terminator struct to let munit know the array is done
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {
    (char *)"/darray",      // Prefix for all test names
    test_suite_tests,       // Array of tests
    NULL,                   // Array of suites (sub-suites)
    1,                      // Iterations
    MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  // Pass control to munit's test runner
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
