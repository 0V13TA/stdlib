#include "arena.h"
#include "darray.h"
#include "map.h"
#include "munit.h"
#include <stdio.h>

#define UNUSED_TEST_ARGS                                                       \
  (void)params;                                                                \
  (void)data

// ---------------------------------------------------------
// Test 1: Basic Arena Lifecycle
// ---------------------------------------------------------
static MunitResult test_arena_basic(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;

  // Create an arena with a 1024-byte region
  ArenaResult res = arena_create(1024);
  munit_assert_false(res.is_error);
  Arena *a = res.as.value;

  munit_assert_not_null(a);
  munit_assert_not_null(a->head);
  munit_assert_size(a->head->capacity, >=, 1024);
  munit_assert_size(a->head->used, ==, 0);

  // Allocate 128 bytes
  void *ptr1 = arena_alloc(a, 128);
  munit_assert_not_null(ptr1);
  munit_assert_size(a->head->used, >, 0);

  // Reset the arena (instantly reclaims memory without OS free)
  arena_reset(a);
  munit_assert_size(a->head->used, ==, 0);

  // Free back to the OS
  arena_free(&a);
  munit_assert_null(a);

  return MUNIT_OK;
}

// ---------------------------------------------------------
// Test 2: Arena + Dynamic Array Integration
// ---------------------------------------------------------
static MunitResult test_arena_darray(const MunitParameter params[],
                                     void *data) {
  UNUSED_TEST_ARGS;
  Arena *a = arena_create(4096).as.value;

  // Pass the Arena's allocator interface into Darray!
  DarrayResult d_res = darray_create(5, sizeof(int), &a->allocator);
  munit_assert_false(d_res.is_error);
  Darray *arr = d_res.as.value;

  // Append 100 items. This will force multiple darray_grow (realloc) calls,
  // entirely managed by the arena.
  for (int i = 0; i < 100; i++) {
    munit_assert_int(darray_append(&arr, &i), ==, DARRAY_OK);
  }
  munit_assert_size(darray_len(arr), ==, 100);

  // Test Safe Get
  int arena_safe_val = 0;
  munit_assert_int(darray_get(arr, 50, &arena_safe_val), ==, DARRAY_OK);
  munit_assert_int(arena_safe_val, ==, 50);

  // Test Pointer Get
  ValueResult get_res = darray_get_ptr(arr, 50);
  munit_assert_false(get_res.is_error);
  munit_assert_int(*(int *)get_res.as.value, ==, 50);

  // darray_free routes to arena_free (which does nothing for individual
  // pointers), but it safely nullifies the 'arr' double pointer.
  darray_free(&arr);
  munit_assert_null(arr);

  arena_free(&a); // Clean up the actual memory
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Test 3: Arena + Hash Map Integration
// ---------------------------------------------------------
static MunitResult test_arena_map(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;

  Arena *a = arena_create(4096).as.value;

  // Pass the Arena's allocator interface into Map!
  MapResult m_res =
      map_create(8, sizeof(int), sizeof(int), NULL, NULL, NULL, &a->allocator);
  munit_assert_false(m_res.is_error);
  Map *m = m_res.as.value;

  // Insert 50 elements. This will force the map to hit its 75% load factor
  // multiple times, allocating new blocks and rehashing via the arena.
  for (int i = 0; i < 50; i++) {
    int val = i * 10;
    munit_assert_int(map_set(&m, &i, &val), ==, MAP_OK);
  }

  munit_assert_size(map_len(m), ==, 50);

  int search_key = 25;

  // Test Safe Get
  int arena_out_val = 0;
  munit_assert_int(map_get(m, &search_key, &arena_out_val), ==, MAP_OK);
  munit_assert_int(arena_out_val, ==, 250);

  // Test Pointer Get
  MapValueResult get_res = map_get_ptr(m, &search_key);
  munit_assert_false(get_res.is_error);
  munit_assert_int(*(int *)get_res.as.value, ==, 250);

  map_free(&m);
  arena_free(&a);
  return MUNIT_OK;
}

// --- Munit Suite Registration ---

static MunitTest test_suite_tests[] = {
    {(char *)"/basic", test_arena_basic, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/darray_integration", test_arena_darray, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/map_integration", test_arena_map, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {
    (char *)"/arena",       // Prefix for all test names
    test_suite_tests,       // Array of tests
    NULL,                   // Array of suites (sub-suites)
    1,                      // Iterations
    MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
