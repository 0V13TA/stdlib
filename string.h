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

void free_string(String *str);
void array_free(Array *arr);

size_t string_length(String str);
int string_char_at(String str, size_t at);
uint8_t string_compare(String str1, String str2);
size_t string_index_of(String str, const char *substr);
size_t string_last_index_of(String str, const char *substr);

StringResult string_create(const char *str);
StringResult string_copy(String str);
StringResult string_concat(String str1, String str2);
StringResult string_trim_end(String str);
StringResult string_trim_start(String str);
StringResult string_trim(String str);
StringResult string_pad_end(String str, size_t len, String pad_str);
StringResult string_pad_start(String str, size_t len, String pad_str);
StringResult string_uppercase(String str);
StringResult string_lowercase(String str);
StringResult string_capitalize(String str);
StringResult string_slice_from(String str, size_t start, size_t end);
StringResult string_slice(String str, size_t start);
StringResult string_replace(String str, String old_substr, String new_substr);
StringResult string_replace_all(String str, String old_substr,
                                String new_substr);
StringResult cstring_replace(String str, const char *old_substr,
                             const char *new_substr);
StringResult cstring_replace_all(String str, const char *old_substr,
                                 const char *new_substr);

ArrayResult string_split(String str, String delimiter);

SBResult sb_create(size_t capacity);
void sb_free(StringBuilder *builder);
StringError sb_append(StringBuilder *builder, String str);
StringError cstring_sb_append(StringBuilder *builder, const char *str);
StringError char_sb_append(StringBuilder *builder, const char c);
StringResult sb_build(StringBuilder *builder);

#endif
