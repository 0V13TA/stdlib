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

#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

/**
 * Sentinel returned by search functions when a substring is not found.
 */
#define STRING_NPOS ((size_t)-1)

/**
 * Owned or borrowed string value.
 *
 * `string` is always null-terminated when non-null. `_length` does not include
 * the null terminator. `owns_data` controls whether `free_string` frees the
 * pointer.
 */
typedef struct String {
  char *string;
  size_t _length;
  uint8_t owns_data;
} String;

/**
 * Dynamic array of String values returned by string_split.
 */
typedef struct Array {
  size_t length;
  String *string_array;
  size_t element_size;
} Array;

/**
 * Replaces a String variable with a newly computed String and frees the old
 * value.
 *
 * @param var String variable to replace.
 * @param expr Expression that returns the replacement String.
 */
#define STR_REASSIGN(var, expr)                                                \
  do {                                                                         \
    String _tmp = (expr);                                                      \
    free_string(&(var));                                                       \
    (var) = _tmp;                                                              \
  } while (0)

/**
 * Frees a String's owned data and resets it to an empty null state.
 *
 * Borrowed strings are reset but their backing data is not freed.
 *
 * @param str String to free. May be NULL.
 */
void free_string(String *str);

/**
 * Frees every String in an Array and resets the Array to an empty null state.
 *
 * @param arr Array to free. May be NULL.
 */
void array_free(Array *arr);

/**
 * Returns the length of a String, excluding the null terminator.
 *
 * @param str String to measure.
 * @returns Number of bytes in the String.
 */
size_t string_length(String str);

/**
 * Creates an owned String from a null-terminated C string.
 *
 * @param str Source C string.
 * @returns Owned String, or a null String if str is NULL or allocation fails.
 */
String string_create(const char *str);

/**
 * Creates an owned copy of a String.
 *
 * @param str Source String.
 * @returns Owned copy, or a null String if input is null or allocation fails.
 */
String string_copy(String str);

/**
 * Concatenates two Strings into a new owned String.
 *
 * @param str1 Left-hand String.
 * @param str2 Right-hand String.
 * @returns Concatenated String, or a null String on invalid input or allocation
 * failure.
 */
String string_concat(String str1, String str2);

/**
 * Returns the character byte at an index.
 *
 * @param str Source String.
 * @param at Zero-based byte index.
 * @returns Character byte as an int, or -1 if the index is out of bounds.
 */
int string_char_at(String str, size_t at);

/**
 * Compares two Strings for exact byte equality.
 *
 * @param str1 First String.
 * @param str2 Second String.
 * @returns 1 when equal, otherwise 0.
 */
uint8_t string_compare(String str1, String str2);

/**
 * Removes trailing ASCII whitespace from a String.
 *
 * Whitespace includes space, tab, newline, and carriage return.
 *
 * @param str Source String.
 * @returns Trimmed owned String, or a null String on invalid input or
 * allocation failure.
 */
String string_trim_end(String str);

/**
 * Removes leading ASCII whitespace from a String.
 *
 * Whitespace includes space, tab, newline, and carriage return.
 *
 * @param str Source String.
 * @returns Trimmed owned String, or a null String on invalid input or
 * allocation failure.
 */
String string_trim_start(String str);

/**
 * Removes leading and trailing ASCII whitespace from a String.
 *
 * Whitespace includes space, tab, newline, and carriage return.
 *
 * @param str Source String.
 * @returns Trimmed owned String, or a null String on invalid input or
 * allocation failure.
 */
String string_trim(String str);

/**
 * Pads the end of a String until it reaches a target length.
 *
 * The pad String is repeated and truncated as needed.
 *
 * @param str Source String.
 * @param len Target byte length.
 * @param pad_str Padding String.
 * @returns Padded owned String. If no padding is needed, returns a copy.
 */
String string_pad_end(String str, size_t len, String pad_str);

/**
 * Pads the start of a String until it reaches a target length.
 *
 * The pad String is repeated and truncated as needed.
 *
 * @param str Source String.
 * @param len Target byte length.
 * @param pad_str Padding String.
 * @returns Padded owned String. If no padding is needed, returns a copy.
 */
String string_pad_start(String str, size_t len, String pad_str);

/**
 * Converts ASCII lowercase letters in a String to uppercase.
 *
 * @param str Source String.
 * @returns Uppercase owned String, or a null String on invalid input or
 * allocation failure.
 */
String string_uppercase(String str);

/**
 * Converts ASCII uppercase letters in a String to lowercase.
 *
 * @param str Source String.
 * @returns Lowercase owned String, or a null String on invalid input or
 * allocation failure.
 */
String string_lowercase(String str);

/**
 * Capitalizes a String by uppercasing the first ASCII letter and lowercasing
 * the remaining ASCII uppercase letters.
 *
 * @param str Source String.
 * @returns Capitalized owned String, or a null String on allocation failure.
 */
String string_capitalize(String str);

/**
 * Finds the first occurrence of a C substring.
 *
 * @param str Source String.
 * @param substr Null-terminated substring to find.
 * @returns Zero-based byte index, or STRING_NPOS when not found.
 */
size_t string_index_of(String str, const char *substr);

/**
 * Finds the last occurrence of a C substring.
 *
 * @param str Source String.
 * @param substr Null-terminated substring to find.
 * @returns Zero-based byte index, or STRING_NPOS when not found.
 */
size_t string_last_index_of(String str, const char *substr);

/**
 * Creates a slice from start up to, but not including, end.
 *
 * Empty ranges are valid and return an owned empty String.
 *
 * @param str Source String.
 * @param start Zero-based starting byte index.
 * @param end Zero-based ending byte index, exclusive.
 * @returns Owned slice, or a null String when the range is invalid or
 * allocation fails.
 */
String string_slice_from(String str, size_t start, size_t end);

/**
 * Creates a slice from start through the end of a String.
 *
 * Slicing at exactly the String length returns an owned empty String.
 *
 * @param str Source String.
 * @param start Zero-based starting byte index.
 * @returns Owned slice, or a null String when the range is invalid or
 * allocation fails.
 */
String string_slice(String str, size_t start);

/**
 * Replaces the first occurrence of a substring with another String.
 *
 * @param str Source String.
 * @param old_substr Substring to replace.
 * @param new_substr Replacement String.
 * @returns Owned String with the first replacement applied. If no match is
 * found, returns a copy of str.
 */
String string_replace(String str, String old_substr, String new_substr);

/**
 * Replaces all non-overlapping occurrences of a substring with another String.
 *
 * @param str Source String.
 * @param old_substr Substring to replace.
 * @param new_substr Replacement String.
 * @returns Owned String with all replacements applied. If no match is found,
 * returns a copy of str.
 */
String string_replace_all(String str, String old_substr, String new_substr);

/**
 * Replaces the first occurrence of a C substring with another C string.
 *
 * @param str Source String.
 * @param old_substr Null-terminated substring to replace.
 * @param new_substr Null-terminated replacement string.
 * @returns Owned String with the first replacement applied.
 */
String cstring_replace(String str, const char *old_substr,
                       const char *new_substr);

/**
 * Replaces all non-overlapping occurrences of a C substring with another C
 * string.
 *
 * @param str Source String.
 * @param old_substr Null-terminated substring to replace.
 * @param new_substr Null-terminated replacement string.
 * @returns Owned String with all replacements applied.
 */
String cstring_replace_all(String str, const char *old_substr,
                           const char *new_substr);

/**
 * Splits a String using a delimiter String.
 *
 * Empty segments are preserved. The returned Array must be released with
 * array_free.
 *
 * @param str Source String.
 * @param delimiter Delimiter String.
 * @returns Array of owned String segments, or an empty Array on invalid input
 * or allocation failure.
 */
Array string_split(String str, String delimiter);

#endif
