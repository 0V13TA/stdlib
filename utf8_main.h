#include "utf8.h"
#include "munit.h"
#include <string.h>

#define UNUSED_TEST_ARGS \
  (void)params;          \
  (void)data

// ---------------------------------------------------------
// Test 1: Valid Decoding (ASCII to Emoji)
// ---------------------------------------------------------
static MunitResult test_utf8_decode_valid(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;

  // 1-byte: 'A'
  const char *ascii = "A";
  Utf8DecodeResult res1 = utf8_decode(ascii, 1);
  munit_assert_false(res1.is_error);
  munit_assert_uint32(res1.as.value.codepoint, ==, 0x0041);
  munit_assert_size(res1.as.value.bytes_consumed, ==, 1);

  // 2-byte: 'ñ' (U+00F1)
  const char *latin = "ñ";
  Utf8DecodeResult res2 = utf8_decode(latin, 2);
  munit_assert_false(res2.is_error);
  munit_assert_uint32(res2.as.value.codepoint, ==, 0x00F1);
  munit_assert_size(res2.as.value.bytes_consumed, ==, 2);

  // 3-byte: '世' (U+4E16)
  const char *kanji = "世";
  Utf8DecodeResult res3 = utf8_decode(kanji, 3);
  munit_assert_false(res3.is_error);
  munit_assert_uint32(res3.as.value.codepoint, ==, 0x4E16);
  munit_assert_size(res3.as.value.bytes_consumed, ==, 3);

  // 4-byte: '🚀' (U+1F680)
  const char *emoji = "🚀";
  Utf8DecodeResult res4 = utf8_decode(emoji, 4);
  munit_assert_false(res4.is_error);
  munit_assert_uint32(res4.as.value.codepoint, ==, 0x1F680);
  munit_assert_size(res4.as.value.bytes_consumed, ==, 4);

  return MUNIT_OK;
}

// ---------------------------------------------------------
// Test 2: Invalid Decoding (Security & Bounds)
// ---------------------------------------------------------
static MunitResult test_utf8_decode_invalid(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;

  // Truncated sequence (Starts a 3-byte sequence, but only provides 2 bytes)
  const char *truncated = "\xE4\xB8"; 
  Utf8DecodeResult res_trunc = utf8_decode(truncated, 2);
  munit_assert_true(res_trunc.is_error);
  munit_assert_int(res_trunc.as.error, ==, UTF8_ERR_TRUNCATED);

  // Overlong encoding (Encoding the ASCII '/' character using 2 bytes: C0 AF)
  const char *overlong = "\xC0\xAF"; 
  Utf8DecodeResult res_overlong = utf8_decode(overlong, 2);
  munit_assert_true(res_overlong.is_error);
  munit_assert_int(res_overlong.as.error, ==, UTF8_ERR_OVERLONG);

  return MUNIT_OK;
}

// ---------------------------------------------------------
// Test 3: Encoding & Validation
// ---------------------------------------------------------
static MunitResult test_utf8_encode_and_count(const MunitParameter params[], void *data) {
  UNUSED_TEST_ARGS;

  // Encode a Rocket Emoji
  char buffer[4] = {0};
  Utf8EncodeResult enc_res = utf8_encode(0x1F680, buffer, 4);
  munit_assert_false(enc_res.is_error);
  munit_assert_size(enc_res.as.bytes_written, ==, 4);
  munit_assert_memory_equal(4, buffer, "🚀");

  // Buffer too small test
  Utf8EncodeResult short_res = utf8_encode(0x1F680, buffer, 3);
  munit_assert_true(short_res.is_error);
  munit_assert_int(short_res.as.error, ==, UTF8_ERR_BUFFER_TOO_SMALL);

  // Count & Validation
  const char *mixed = "Añ世🚀"; // 1 + 2 + 3 + 4 = 10 bytes, 4 characters
  munit_assert_true(utf8_is_valid(mixed, 10));
  munit_assert_size(utf8_codepoint_count(mixed, 10), ==, 4);

  return MUNIT_OK;
}

// --- Munit Suite Registration ---

static MunitTest test_suite_tests[] = {
  { (char *)"/decode_valid", test_utf8_decode_valid, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char *)"/decode_invalid", test_utf8_decode_invalid, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char *)"/encode_and_count", test_utf8_encode_and_count, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char *)"/utf8",        // Prefix for all test names
  test_suite_tests,       // Array of tests
  NULL,                   // Array of suites (sub-suites)
  1,                      // Iterations 
  MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
