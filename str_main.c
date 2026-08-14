#include "munit.h"
#include "string.h"

#define UNUSED_TEST_ARGS                                                       \
  (void)params;                                                                \
  (void)data

// ---------------------------------------------------------
// Tests for Creation, Copying, and Freeing
// ---------------------------------------------------------
static MunitResult test_creation(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult res = string_create("Hello, Munit!");
  munit_assert_false(res.is_error);
  String s = res.as.value;

  munit_assert_ptr_not_null(s.string);
  munit_assert_size(s._length, ==, 13);
  munit_assert_uint8(s.owns_data, ==, 1);
  munit_assert_string_equal(s.string, "Hello, Munit!");

  free_string(&s);
  munit_assert_null(s.string);
  munit_assert_size(s._length, ==, 0);
  return MUNIT_OK;
}

static MunitResult test_concat(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult s1_res = string_create("Hello ");
  StringResult s2_res = string_create("World");
  munit_assert_false(s1_res.is_error);
  munit_assert_false(s2_res.is_error);

  String s1 = s1_res.as.value;
  String s2 = s2_res.as.value;

  StringResult res = string_concat(s1, s2);
  munit_assert_false(res.is_error);
  String result = res.as.value;

  munit_assert_string_equal(result.string, "Hello World");
  munit_assert_size(result._length, ==, 11);

  free_string(&s1);
  free_string(&s2);
  free_string(&result);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for Trimming and Padding
// ---------------------------------------------------------
static MunitResult test_trim(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult s1_res = string_create("   spaced   ");
  munit_assert_false(s1_res.is_error);
  String s1 = s1_res.as.value;

  StringResult res = string_trim(s1);
  munit_assert_false(res.is_error);
  String result = res.as.value;

  munit_assert_string_equal(result.string, "spaced");
  munit_assert_size(result._length, ==, 6);

  free_string(&s1);
  free_string(&result);
  return MUNIT_OK;
}

static MunitResult test_pad(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult s1_res = string_create("42");
  StringResult pad_res = string_create("0");
  munit_assert_false(s1_res.is_error);
  munit_assert_false(pad_res.is_error);

  String s1 = s1_res.as.value;
  String pad = pad_res.as.value;

  StringResult res_start = string_pad_start(s1, 5, pad);
  munit_assert_false(res_start.is_error);
  String result_start = res_start.as.value;
  munit_assert_string_equal(result_start.string, "00042");

  StringResult res_end = string_pad_end(s1, 5, pad);
  munit_assert_false(res_end.is_error);
  String result_end = res_end.as.value;
  munit_assert_string_equal(result_end.string, "42000");

  free_string(&s1);
  free_string(&pad);
  free_string(&result_start);
  free_string(&result_end);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for Case Conversion
// ---------------------------------------------------------
static MunitResult test_case_conversion(const MunitParameter params[],
                                        void *data) {
  UNUSED_TEST_ARGS;
  StringResult s_res = string_create("hElLo 123!");
  munit_assert_false(s_res.is_error);
  String s = s_res.as.value;

  StringResult upper_res = string_uppercase(s);
  munit_assert_false(upper_res.is_error);
  String upper = upper_res.as.value;
  munit_assert_string_equal(upper.string, "HELLO 123!");

  StringResult lower_res = string_lowercase(s);
  munit_assert_false(lower_res.is_error);
  String lower = lower_res.as.value;
  munit_assert_string_equal(lower.string, "hello 123!");

  StringResult cap_res = string_capitalize(s);
  munit_assert_false(cap_res.is_error);
  String cap = cap_res.as.value;
  munit_assert_string_equal(cap.string, "Hello 123!");

  free_string(&s);
  free_string(&upper);
  free_string(&lower);
  free_string(&cap);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for Searching and Slicing
// ---------------------------------------------------------
static MunitResult test_indexing(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult s_res = string_create("banana");
  munit_assert_false(s_res.is_error);
  String s = s_res.as.value;

  size_t idx1 = string_index_of(s, "na");
  munit_assert_size(idx1, ==, 2);

  size_t idx2 = string_last_index_of(s, "na");
  munit_assert_size(idx2, ==, 4);

  size_t idx_none = string_index_of(s, "apple");
  munit_assert_size(idx_none, ==, STRING_NPOS);

  free_string(&s);
  return MUNIT_OK;
}

static MunitResult test_slice(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult s_res = string_create("hello world");
  munit_assert_false(s_res.is_error);
  String s = s_res.as.value;

  StringResult slice1_res = string_slice_from(s, 0, 5);
  munit_assert_false(slice1_res.is_error);
  String slice1 = slice1_res.as.value;
  munit_assert_string_equal(slice1.string, "hello");

  StringResult slice2_res = string_slice(s, 6);
  munit_assert_false(slice2_res.is_error);
  String slice2 = slice2_res.as.value;
  munit_assert_string_equal(slice2.string, "world");

  free_string(&s);
  free_string(&slice1);
  free_string(&slice2);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for Splitting
// ---------------------------------------------------------
static MunitResult test_split(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  StringResult s_res = string_create("apple,banana,cherry");
  StringResult delim_res = string_create(",");
  munit_assert_false(s_res.is_error);
  munit_assert_false(delim_res.is_error);

  String s = s_res.as.value;
  String delim = delim_res.as.value;

  ArrayResult arr_res = string_split(s, delim);
  munit_assert_false(arr_res.is_error);
  Array arr = arr_res.as.value;

  munit_assert_size(arr.length, ==, 3);
  munit_assert_string_equal(arr.string_array[0].string, "apple");
  munit_assert_string_equal(arr.string_array[1].string, "banana");
  munit_assert_string_equal(arr.string_array[2].string, "cherry");

  array_free(&arr);
  munit_assert_size(arr.length, ==, 0);
  munit_assert_null(arr.string_array);

  free_string(&s);
  free_string(&delim);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for String Replacement
// ---------------------------------------------------------
static MunitResult test_replace(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  String s = string_create("hello world").as.value;
  String old_sub = string_create("world").as.value;
  String new_sub = string_create("there").as.value;

  String result = string_replace(s, old_sub, new_sub).as.value;
  munit_assert_string_equal(result.string, "hello there");
  munit_assert_size(result._length, ==, 11);

  String missing_sub = string_create("xyz").as.value;
  String result_no_match = string_replace(s, missing_sub, new_sub).as.value;
  munit_assert_string_equal(result_no_match.string, "hello world");

  String c_res = cstring_replace(s, "hello", "hi").as.value;
  munit_assert_string_equal(c_res.string, "hi world");

  free_string(&s);
  free_string(&old_sub);
  free_string(&new_sub);
  free_string(&missing_sub);
  free_string(&result);
  free_string(&result_no_match);
  free_string(&c_res);
  return MUNIT_OK;
}

static MunitResult test_replace_all(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;
  String s = string_create("foo bar foo baz foo").as.value;
  String old_sub = string_create("foo").as.value;
  String new_sub = string_create("qux").as.value;

  String result = string_replace_all(s, old_sub, new_sub).as.value;
  munit_assert_string_equal(result.string, "qux bar qux baz qux");

  String long_sub = string_create("TARGET").as.value;
  String result_long = string_replace_all(s, old_sub, long_sub).as.value;
  munit_assert_string_equal(result_long.string, "TARGET bar TARGET baz TARGET");

  String empty_sub = string_create("").as.value;
  String result_short = string_replace_all(s, old_sub, empty_sub).as.value;
  munit_assert_string_equal(result_short.string, " bar  baz ");

  String c_res = cstring_replace_all(s, "foo", "123").as.value;
  munit_assert_string_equal(c_res.string, "123 bar 123 baz 123");

  free_string(&s);
  free_string(&old_sub);
  free_string(&new_sub);
  free_string(&long_sub);
  free_string(&empty_sub);
  free_string(&result);
  free_string(&result_long);
  free_string(&result_short);
  free_string(&c_res);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for Edge Cases (Split, Replace, Slice, Search)
// ---------------------------------------------------------
static MunitResult test_split_edge_cases(const MunitParameter params[],
                                         void *data) {
  UNUSED_TEST_ARGS;
  String delim = string_create(",").as.value;
  String s1 = string_create(",a,,b,").as.value;

  Array arr = string_split(s1, delim).as.value;

  munit_assert_size(arr.length, ==, 5);
  munit_assert_string_equal(arr.string_array[0].string, "");
  munit_assert_string_equal(arr.string_array[1].string, "a");
  munit_assert_string_equal(arr.string_array[2].string, "");
  munit_assert_string_equal(arr.string_array[3].string, "b");
  munit_assert_string_equal(arr.string_array[4].string, "");

  array_free(&arr);
  free_string(&s1);
  free_string(&delim);
  return MUNIT_OK;
}

static MunitResult test_replace_edge_cases(const MunitParameter params[],
                                           void *data) {
  UNUSED_TEST_ARGS;
  String s = string_create("a").as.value;

  String res_contains = cstring_replace_all(s, "a", "aa").as.value;
  munit_assert_string_equal(res_contains.string, "aa");

  String res_longer = cstring_replace(s, "a", "longer").as.value;
  munit_assert_string_equal(res_longer.string, "longer");

  String long_str = string_create("longer").as.value;
  String res_shorter = cstring_replace(long_str, "longer", "a").as.value;
  munit_assert_string_equal(res_shorter.string, "a");

  String res_empty = cstring_replace(long_str, "longer", "").as.value;
  munit_assert_string_equal(res_empty.string, "");

  free_string(&s);
  free_string(&res_contains);
  free_string(&res_longer);
  free_string(&long_str);
  free_string(&res_shorter);
  free_string(&res_empty);
  return MUNIT_OK;
}

static MunitResult test_search_edge_cases(const MunitParameter params[],
                                          void *data) {
  UNUSED_TEST_ARGS;
  String s = string_create("hello").as.value;
  munit_assert_size(string_index_of(s, "he"), ==, 0);
  munit_assert_size(string_index_of(s, "lo"), ==, 3);
  munit_assert_size(string_index_of(s, "xyz"), ==, STRING_NPOS);
  free_string(&s);
  return MUNIT_OK;
}

static MunitResult test_slice_edge_cases(const MunitParameter params[],
                                         void *data) {
  UNUSED_TEST_ARGS;
  String s = string_create("hello").as.value;

  String slice1 = string_slice_from(s, 0, 5).as.value;
  munit_assert_string_equal(slice1.string, "hello");

  String slice2 = string_slice(s, 5).as.value;
  munit_assert_ptr_not_null(slice2.string);
  munit_assert_string_equal(slice2.string, "");
  munit_assert_size(slice2._length, ==, 0);

  String slice3 = string_slice_from(s, 2, 2).as.value;
  munit_assert_ptr_not_null(slice3.string);
  munit_assert_string_equal(slice3.string, "");

  // Testing the actual explicit error return now!
  StringResult invalid = string_slice(s, 6);
  munit_assert_true(invalid.is_error);
  munit_assert_int(invalid.as.error, ==, STR_ERR_BOUNDS);

  free_string(&s);
  free_string(&slice1);
  free_string(&slice2);
  free_string(&slice3);
  return MUNIT_OK;
}

static MunitResult test_public_helpers(const MunitParameter params[],
                                       void *data) {
  UNUSED_TEST_ARGS;
  String s = string_create("abc").as.value;
  String same = string_create("abc").as.value;
  String other = string_create("abd").as.value;
  String empty = string_create("").as.value;

  munit_assert_size(string_length(s), ==, 3);
  munit_assert_int(string_char_at(s, 1), ==, 'b');
  munit_assert_int(string_char_at(s, 3), ==, -1);
  munit_assert_uint8(string_compare(s, same), ==, 1);
  munit_assert_uint8(string_compare(s, other), ==, 0);
  munit_assert_ptr_not_null(empty.string);
  munit_assert_size(empty._length, ==, 0);

  free_string(&s);
  free_string(&same);
  free_string(&other);
  free_string(&empty);
  return MUNIT_OK;
}

// ---------------------------------------------------------
// Tests for String Builder
// ---------------------------------------------------------
static MunitResult test_string_builder_basic(const MunitParameter params[],
                                             void *data) {
  UNUSED_TEST_ARGS;
  SBResult sb_res = sb_create(8);
  munit_assert_false(sb_res.is_error);
  StringBuilder sb = sb_res.as.value;

  munit_assert_ptr_not_null(sb.string);
  munit_assert_size(sb._length, ==, 0);
  munit_assert_size(sb._capacity, ==, 8);

  StringError ok1 = cstring_sb_append(&sb, "Hello");
  munit_assert_int(ok1, ==, STR_OK);
  munit_assert_size(sb._length, ==, 5);
  munit_assert_string_equal(sb.string, "Hello");

  StringError ok2 = char_sb_append(&sb, ' ');
  munit_assert_int(ok2, ==, STR_OK);
  munit_assert_size(sb._length, ==, 6);

  String s = string_create("World!").as.value;
  StringError ok3 = sb_append(&sb, s);
  munit_assert_int(ok3, ==, STR_OK);
  munit_assert_size(sb._length, ==, 12);
  munit_assert_string_equal(sb.string, "Hello World!");

  StringResult result_res = sb_build(&sb);
  munit_assert_false(result_res.is_error);
  String result = result_res.as.value;

  munit_assert_ptr_not_null(result.string);
  munit_assert_string_equal(result.string, "Hello World!");
  munit_assert_size(result._length, ==, 12);
  munit_assert_uint8(result.owns_data, ==, 1);

  munit_assert_null(sb.string);
  munit_assert_size(sb._length, ==, 0);

  free_string(&s);
  free_string(&result);
  return MUNIT_OK;
}

static MunitResult test_string_builder_growth(const MunitParameter params[],
                                              void *data) {
  UNUSED_TEST_ARGS;
  StringBuilder sb = sb_create(2).as.value;

  cstring_sb_append(&sb, "A quick brown fox ");
  cstring_sb_append(&sb, "jumps over the lazy dog.");

  String result = sb_build(&sb).as.value;
  munit_assert_string_equal(result.string,
                            "A quick brown fox jumps over the lazy dog.");
  munit_assert_size(result._length, ==, 42);

  free_string(&result);
  return MUNIT_OK;
}

static MunitResult test_string_builder_free(const MunitParameter params[],
                                            void *data) {
  UNUSED_TEST_ARGS;
  StringBuilder sb = sb_create(16).as.value;
  cstring_sb_append(&sb, "temporary data");
  munit_assert_size(sb._length, ==, 14);

  sb_free(&sb);
  munit_assert_null(sb.string);
  munit_assert_size(sb._length, ==, 0);
  munit_assert_size(sb._capacity, ==, 0);

  return MUNIT_OK;
}

// ---------------------------------------------------------
// Test Suite Setup
// ---------------------------------------------------------
static MunitTest test_suite_tests[] = {
    {(char *)"/creation", test_creation, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/helpers", test_public_helpers, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/concat", test_concat, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/trim", test_trim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/pad", test_pad, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/case", test_case_conversion, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/indexing", test_indexing, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/slice", test_slice, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/split", test_split, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/replace", test_replace, NULL, NULL, MUNIT_TEST_OPTION_NONE,
     NULL},
    {(char *)"/replace_all", test_replace_all, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/edge/split", test_split_edge_cases, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/edge/replace", test_replace_edge_cases, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/edge/search", test_search_edge_cases, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/edge/slice", test_slice_edge_cases, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/string_builder/basic", test_string_builder_basic, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/string_builder/growth", test_string_builder_growth, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/string_builder/free", test_string_builder_free, NULL, NULL,
     MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {(char *)"/string_library",
                                      test_suite_tests, NULL, 1,
                                      MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, (void *)" nit", argc, argv);
}
