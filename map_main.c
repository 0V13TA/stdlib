#include "map.h"
#include "munit.h"
#include <stdio.h>
#include <string.h>

#define UNUSED_TEST_ARGS                                                       \
  (void)params;                                                                \
  (void)data

// --- Custom Callbacks for String Keys ---

// Context isn't strictly needed for basic string hashing, but we must match the
// signature
static uint64_t string_hash_func(const void *key, void *ctx) {
  (void)ctx;
  const char *str = *(const char **)key; // Key is a pointer to a char*
  return map_hash_fnv1a(str, strlen(str));
}

static bool string_eq_func(const void *key_a, const void *key_b, void *ctx) {
  (void)ctx;
  const char *str_a = *(const char **)key_a;
  const char *str_b = *(const char **)key_b;
  return strcmp(str_a, str_b) == 0;
}

// --- Iteration Callbacks ---

static void sum_values_func(const void *key, void *value, void *ctx) {
  (void)key;
  int *sum = (int *)ctx;
  *sum += *(int *)value;
}

// --- Test Cases ---

static MunitResult test_lifecycle_and_defaults(const MunitParameter params[],
                                               void *data) {
  UNUSED_TEST_ARGS;

  // 1. Create a map with int keys and float values, using default raw-byte
  // hashing
  MapResult res =
      map_create(8, sizeof(int), sizeof(float), NULL, NULL, NULL, NULL);
  munit_assert_false(res.is_error);

  Map *m = res.as.value;
  munit_assert_not_null(m);
  munit_assert_true(map_empty(m));

  int k1 = 42, k2 = 99;
  float v1 = 3.14f, v2 = 2.71f;

  // 2. Insert values
  munit_assert_int(map_set(&m, &k1, &v1), ==, MAP_OK);
  munit_assert_int(map_set(&m, &k2, &v2), ==, MAP_OK);
  munit_assert_size(map_len(m), ==, 2);

  // 3. Retrieve values
  munit_assert_true(map_contains(m, &k1));
  MapValueResult get_res = map_get(m, &k1);
  munit_assert_false(get_res.is_error);
  munit_assert_float(*(float *)get_res.as.value, ==, 3.14f);

  map_free(&m);
  munit_assert_null(m);
  return MUNIT_OK;
}

static MunitResult test_custom_string_keys(const MunitParameter params[],
                                           void *data) {
  UNUSED_TEST_ARGS;

  // 1. Create a map with char* keys and int values
  MapResult res = map_create(16, sizeof(char *), sizeof(int), string_hash_func,
                             string_eq_func, NULL, NULL);
  munit_assert_false(res.is_error);
  Map *m = res.as.value;

  const char *k1 = "PlayerOne";
  const char *k2 = "EnemyBoss";
  const char *k3 = "NPC"; // <--- Add this
  int v1 = 100, v2 = 999;

  map_set(&m, &k1, &v1);
  map_set(&m, &k2, &v2);

  munit_assert_true(map_contains(m, &k1));
  munit_assert_false(map_contains(m, &k3)); // <--- Pass &k3 safely

  MapValueResult get_res = map_get(m, &k2);
  munit_assert_false(get_res.is_error);
  munit_assert_int(*(int *)get_res.as.value, ==, 999);

  map_free(&m);
  return MUNIT_OK;
}

static MunitResult test_removals_and_tombstones(const MunitParameter params[],
                                                void *data) {
  UNUSED_TEST_ARGS;

  MapResult res =
      map_create(8, sizeof(int), sizeof(int), NULL, NULL, NULL, NULL);
  Map *m = res.as.value;

  int k1 = 1, k2 = 2;
  int v1 = 10, v2 = 20;

  map_set(&m, &k1, &v1);
  map_set(&m, &k2, &v2);

  // Remove k1 and fetch the removed value
  int removed_val = 0;
  munit_assert_int(map_remove(&m, &k1, &removed_val), ==, MAP_OK);
  munit_assert_int(removed_val, ==, 10);
  munit_assert_size(map_len(m), ==, 1);

  // k1 should be gone, k2 should still be reachable past the tombstone
  munit_assert_false(map_contains(m, &k1));
  munit_assert_true(map_contains(m, &k2));

  // Removing a non-existent key should fail gracefully
  munit_assert_int(map_remove(&m, &k1, NULL), ==, MAP_ERR_NOT_FOUND);

  map_free(&m);
  return MUNIT_OK;
}

static MunitResult test_iteration_and_extraction(const MunitParameter params[],
                                                 void *data) {
  UNUSED_TEST_ARGS;

  MapResult res =
      map_create(16, sizeof(int), sizeof(int), NULL, NULL, NULL, NULL);
  Map *m = res.as.value;

  // Insert 3 pairs
  for (int i = 1; i <= 3; i++) {
    int val = i * 10;
    map_set(&m, &i, &val); // [1:10, 2:20, 3:30]
  }

  // 1. Test map_for_each
  int sum = 0;
  munit_assert_int(map_for_each(m, sum_values_func, &sum), ==, MAP_OK);
  munit_assert_int(sum, ==, 60); // 10 + 20 + 30

  // 2. Test Array Extraction
  MapArrayResult keys_res = map_keys(m, NULL);
  munit_assert_false(keys_res.is_error);
  MapArray *keys_arr = keys_res.as.value;
  munit_assert_size(keys_arr->length, ==, 3);

  MapArrayResult vals_res = map_values(m, NULL);
  munit_assert_false(vals_res.is_error);
  MapArray *vals_arr = vals_res.as.value;
  munit_assert_size(vals_arr->length, ==, 3);

  // Clean up the main map and the extracted arrays
  map_free(&m);
  map_array_free(&keys_arr);
  map_array_free(&vals_arr);

  return MUNIT_OK;
}

// --- Munit Suite Registration ---

static MunitTest test_suite_tests[] = {
    {(char *)"/lifecycle_and_defaults", test_lifecycle_and_defaults, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/custom_string_keys", test_custom_string_keys, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/removals_and_tombstones", test_removals_and_tombstones, NULL,
     NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/iteration_and_extraction", test_iteration_and_extraction, NULL,
     NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {
    (char *)"/map",         // Prefix for all test names
    test_suite_tests,       // Array of tests
    NULL,                   // Array of suites (sub-suites)
    1,                      // Iterations
    MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
