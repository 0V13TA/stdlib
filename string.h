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

#define STRING_NPOS ((size_t)-1)

typedef enum {
  STR_OK = 0,
  STR_ERR_ALLOC,   // Malloc or realloc failed
  STR_ERR_BOUNDS,  // Index out of bounds
  STR_ERR_NULL_PTR // Null pointer passed to function
} StringError;

typedef struct String {
  char *string;
  size_t _length;
  uint8_t owns_data;
} String;

// Tagged Union Result for String
typedef struct {
  union {
    String value;
    StringError error;
  } as;
  uint8_t is_error;
} StringResult;

typedef struct Array {
  size_t length;
  String *string_array;
  size_t element_size;
} Array;

// Tagged Union Result for Array
typedef struct {
  union {
    Array value;
    StringError error;
  } as;
  uint8_t is_error;
} ArrayResult;

typedef struct StringBuilder {
  char *string;
  size_t _length;
  size_t _capacity;
} StringBuilder;

// Tagged Union Result for StringBuilder
typedef struct {
  union {
    StringBuilder value;
    StringError error;
  } as;
  uint8_t is_error;
} SBResult;

#define STR_REASSIGN(var, expr)                                                \
  do {                                                                         \
    StringResult _tmp = (expr);                                                \
    if (!_tmp.is_error) {                                                      \
      free_string(&(var));                                                     \
      (var) = _tmp.value;                                                      \
    }                                                                          \
  } while (0)

/**
- Releases a string's owned memory.
 *
- Clears every field after releasing memory when the string owns its buffer.
 *
- @param str String* Pointer to the string to release; NULL is accepted.
- @return void No value is returned.
 *
- @example
- // Release a string created by string_create.
- free_string(&value);
 */
void free_string(String *str);

/**
- Releases an array of strings.
 *
- Frees every string in the array and resets the array fields.
 *
- @param arr Array* Pointer to the array to release; NULL is accepted.
- @return void No value is returned.
 *
- @example
- // Release an array returned by string_split.
- array_free(&parts);
 */
void array_free(Array *arr);

/**
- Gets a string's stored length.
 *
- Returns the number of characters before the terminating null byte.
 *
- @param str String String whose length is requested.
- @return size_t The string length.
 *
- @example
- // Get the length of a String value.
- size_t length = string_length(value);
 */
size_t string_length(String str);

/**
- Gets a character at an index.
 *
- Returns -1 when the string is NULL or the index is out of bounds.
 *
- @param str String String to inspect.
- @param at size_t Zero-based character index.
- @return int The unsigned character value, or -1 on failure.
 *
- @example
- // Read the first character.
- int first = string_char_at(value, 0);
 */
int string_char_at(String str, size_t at);

/**
- Compares two strings for equality.
 *
- Both length and character contents must match for strings to be equal.
 *
- @param str1 String First string to compare.
- @param str2 String Second string to compare.
- @return uint8_t 1 when equal; otherwise 0.
 *
- @example
- // Check whether two strings match.
- uint8_t equal = string_compare(left, right);
 */
uint8_t string_compare(String str1, String str2);

/**
- Finds the first occurrence of a C substring.
 *
- Searches from the start of the string.
 *
- @param str String String to search.
- @param substr const char* Null-terminated substring to find.
- @return size_t The matching index, or STRING_NPOS when absent or invalid.
 *
- @example
- // Find the first comma.
- size_t index = string_index_of(value, ",");
 */
size_t string_index_of(String str, const char *substr);

/**
- Finds the last occurrence of a C substring.
 *
- Searches all occurrences and returns the final match.
 *
- @param str String String to search.
- @param substr const char* Null-terminated substring to find.
- @return size_t The final matching index, or STRING_NPOS when absent or
invalid.
 *
- @example
- // Find the final slash in a path.
- size_t index = string_last_index_of(path, "/");
 */
size_t string_last_index_of(String str, const char *substr);

/**
- Creates an owned string from a C string.
 *
- Allocates and copies the supplied null-terminated character sequence.
 *
- @param str const char* Source C string; it must not be NULL.
- @return StringResult A new owned String or a StringError.
 *
- @example
- // Create a managed string.
- StringResult result = string_create("Hello");
 */
StringResult string_create(const char *str);

/**
- Creates an owned copy of a string.
 *
- Allocates a new buffer containing the same characters as the source.
 *
- @param str String Source string; its buffer must not be NULL.
- @return StringResult An owned copy or a StringError.
 *
- @example
- // Copy a string before modifying its lifetime.
- StringResult copy = string_copy(value);
 */
StringResult string_copy(String str);

/**
- Concatenates two strings.
 *
- Allocates a new string containing str1 followed by str2.
 *
- @param str1 String First string.
- @param str2 String Second string.
- @return StringResult The concatenated owned String or a StringError.
 *
- @example
- // Join a greeting and name.
- StringResult joined = string_concat(greeting, name);
 */
StringResult string_concat(String str1, String str2);

/**
- Removes trailing whitespace.
 *
- Removes spaces, tabs, carriage returns, and newlines from the end.
 *
- @param str String Source string.
- @return StringResult A trimmed owned String or a StringError.
 *
- @example
- // Remove whitespace after text.
- StringResult trimmed = string_trim_end(value);
 */
StringResult string_trim_end(String str);

/**
- Removes leading whitespace.
 *
- Removes spaces, tabs, carriage returns, and newlines from the beginning.
 *
- @param str String Source string.
- @return StringResult A trimmed owned String or a StringError.
 *
- @example
- // Remove whitespace before text.
- StringResult trimmed = string_trim_start(value);
 */
StringResult string_trim_start(String str);

/**
- Removes surrounding whitespace.
 *
- Applies leading and trailing whitespace trimming to a new string.
 *
- @param str String Source string.
- @return StringResult A trimmed owned String or a StringError.
 *
- @example
- // Normalize a user-entered value.
- StringResult trimmed = string_trim(value);
 */
StringResult string_trim(String str);

/**
- Pads a string on the right.
 *
- Repeats pad_str until the result reaches len characters.
 *
- @param str String Source string.
- @param len size_t Required minimum result length.
- @param pad_str String Non-empty string used for padding.
- @return StringResult A padded owned String or a StringError.
 *
- @example
- // Pad a number to five characters.
- StringResult padded = string_pad_end(value, 5, zero);
 */
StringResult string_pad_end(String str, size_t len, String pad_str);

/**
- Pads a string on the left.
 *
- Repeats pad_str before str until the result reaches len characters.
 *
- @param str String Source string.
- @param len size_t Required minimum result length.
- @param pad_str String Non-empty string used for padding.
- @return StringResult A padded owned String or a StringError.
 *
- @example
- // Left-pad a number with zeroes.
- StringResult padded = string_pad_start(value, 5, zero);
 */
StringResult string_pad_start(String str, size_t len, String pad_str);

/**
- Converts a string to uppercase.
 *
- Converts ASCII lowercase letters while leaving all other characters unchanged.
 *
- @param str String Source string.
- @return StringResult An uppercase owned String or a StringError.
 *
- @example
- // Convert text for a heading.
- StringResult upper = string_uppercase(value);
 */
StringResult string_uppercase(String str);

/**
- Converts a string to lowercase.
 *
- Converts ASCII uppercase letters while leaving all other characters unchanged.
 *
- @param str String Source string.
- @return StringResult A lowercase owned String or a StringError.
 *
- @example
- // Normalize text for comparison.
- StringResult lower = string_lowercase(value);
 */
StringResult string_lowercase(String str);

/**
- Capitalizes a string.
 *
- Uppercases the first ASCII letter and lowercases later ASCII uppercase
letters.
 *
- @param str String Source string.
- @return StringResult A capitalized owned String or a StringError.
 *
- @example
- // Format a title-like value.
- StringResult title = string_capitalize(value);
 */
StringResult string_capitalize(String str);

/**
- Extracts a bounded substring.
 *
- Copies characters in the half-open range [start, end).
 *
- @param str String Source string.
- @param start size_t Inclusive zero-based start index.
- @param end size_t Exclusive zero-based end index.
- @return StringResult The sliced owned String or a StringError.
 *
- @example
- // Extract the first five characters.
- StringResult prefix = string_slice_from(value, 0, 5);
 */
StringResult string_slice_from(String str, size_t start, size_t end);

/**
- Extracts a suffix.
 *
- Copies every character from start through the end of the string.
 *
- @param str String Source string.
- @param start size_t Inclusive zero-based start index.
- @return StringResult The sliced owned String or a StringError.
 *
- @example
- // Extract everything after an identifier.
- StringResult suffix = string_slice(value, 2);
 */
StringResult string_slice(String str, size_t start);

/**
- Replaces the first substring match.
 *
- Returns a copy unchanged when old_substr is empty or has no match.
 *
- @param str String Source string.
- @param old_substr String Substring to replace.
- @param new_substr String Replacement text.
- @return StringResult The replaced owned String or a StringError.
 *
- @example
- // Replace the first occurrence of a word.
- StringResult changed = string_replace(value, old_text, new_text);
 */
StringResult string_replace(String str, String old_substr, String new_substr);

/**
- Replaces every non-overlapping substring match.
 *
- Returns a copy unchanged when old_substr is empty or has no match.
 *
- @param str String Source string.
- @param old_substr String Substring to replace.
- @param new_substr String Replacement text.
- @return StringResult The replaced owned String or a StringError.
 *
- @example
- // Replace all occurrences of a word.
- StringResult changed = string_replace_all(value, old_text, new_text);
 */
StringResult string_replace_all(String str, String old_substr,
                                String new_substr);

/**
- Replaces the first C-string substring match.
 *
- Wraps C strings as non-owning Strings before performing the replacement.
 *
- @param str String Source string.
- @param old_substr const char* Null-terminated substring to replace.
- @param new_substr const char* Null-terminated replacement text.
- @return StringResult The replaced owned String or a StringError.
 *
- @example
- // Replace a word without constructing temporary Strings.
- StringResult changed = cstring_replace(value, "old", "new");
 */
StringResult cstring_replace(String str, const char *old_substr,
                             const char *new_substr);

/**
- Replaces every C-string substring match.
 *
- Wraps C strings as non-owning Strings before performing all replacements.
 *
- @param str String Source string.
- @param old_substr const char* Null-terminated substring to replace.
- @param new_substr const char* Null-terminated replacement text.
- @return StringResult The replaced owned String or a StringError.
 *
- @example
- // Replace all separators.
- StringResult changed = cstring_replace_all(value, "/", "-");
 */
StringResult cstring_replace_all(String str, const char *old_substr,
                                 const char *new_substr);

/**
- Splits a string around a delimiter.
 *
- Creates owned String elements and preserves empty segments between delimiters.
 *
- @param str String Source string.
- @param delimiter String Non-empty separator string.
- @return ArrayResult An Array of Strings or a StringError.
 *
- @example
- // Split a comma-separated value.
- ArrayResult parts = string_split(value, comma);
 */
ArrayResult string_split(String str, String delimiter);

/**
- Creates a string builder.
 *
- Allocates the builder buffer with the requested initial capacity.
 *
- @param capacity size_t Initial buffer capacity in bytes.
- @return SBResult An initialized StringBuilder or a StringError.
 *
- @example
- // Create a builder for incremental text.
- SBResult result = sb_create(32);
 */
SBResult sb_create(size_t capacity);

/**
- Releases a string builder.
 *
- Frees its buffer and resets every builder field.
 *
- @param builder StringBuilder* Builder to release; NULL is accepted.
- @return void No value is returned.
 *
- @example
- // Discard a builder that will not be built.
- sb_free(&builder);
 */
void sb_free(StringBuilder *builder);

/**
- Appends a String to a builder.
 *
- Enlarges the builder buffer automatically when additional capacity is needed.
 *
- @param builder StringBuilder* Destination builder.
- @param str String String to append.
- @return StringError STR_OK on success; otherwise a StringError.
 *
- @example
- // Append managed text.
- StringError error = sb_append(&builder, value);
 */
StringError sb_append(StringBuilder *builder, String str);

/**
- Appends a C string to a builder.
 *
- Treats the input as a non-owning string and appends its characters.
 *
- @param builder StringBuilder* Destination builder.
- @param str const char* Null-terminated string to append.
- @return StringError STR_OK on success; otherwise a StringError.
 *
- @example
- // Append a literal.
- StringError error = cstring_sb_append(&builder, "Hello");
 */
StringError cstring_sb_append(StringBuilder *builder, const char *str);

/**
- Appends one character to a builder.
 *
- Enlarges the builder buffer automatically when required.
 *
- @param builder StringBuilder* Destination builder.
- @param c char Character to append.
- @return StringError STR_OK on success; otherwise a StringError.
 *
- @example
- // Append a separator character.
- StringError error = char_sb_append(&builder, ',');
 */
StringError char_sb_append(StringBuilder *builder, const char c);

/**
- Transfers a builder buffer into a String.
 *
- Empties the builder and gives ownership of its buffer to the returned String.
 *
- @param builder StringBuilder* Builder containing the completed text.
- @return StringResult The completed owned String or a StringError.
 *
- @example
- // Finish building and take ownership of the text.
- StringResult result = sb_build(&builder);
 */
StringResult sb_build(StringBuilder *builder);

#endif
