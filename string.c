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

#include "string.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal Result Macros
#define RET_ERR(err_code) (StringResult){.as.error = (err_code), .is_error = 1}
#define RET_OK(str_val) (StringResult){.as.value = (str_val), .is_error = 0}
#define RET_ARR_ERR(err_code)                                                  \
  (ArrayResult){.as.error = (err_code), .is_error = 1}
#define RET_ARR_OK(arr_val) (ArrayResult){.as.value = (arr_val), .is_error = 0}
#define RET_SB_ERR(err_code) (SBResult){.as.error = (err_code), .is_error = 1}
#define RET_SB_OK(sb_val) (SBResult){.as.value = (sb_val), .is_error = 0}

static StringResult _alloc_string(size_t len) {
  String new_str = {0};
  char *ptr = malloc(sizeof(char) * (len + 1));
  if (ptr == NULL)
    return RET_ERR(STR_ERR_ALLOC);

  ptr[len] = '\0';
  new_str.string = ptr;
  new_str._length = len;
  new_str.owns_data = 1;
  return RET_OK(new_str);
}

void free_string(String *str) {
  if (str && str->string && str->owns_data) {
    free(str->string);
  }
  if (str) {
    str->string = NULL;
    str->_length = 0;
    str->owns_data = 0;
  }
}

void array_free(Array *arr) {
  if (!arr)
    return;
  for (size_t i = 0; i < arr->length; i++) {
    free_string(&arr->string_array[i]);
  }
  free(arr->string_array);
  arr->length = 0;
  arr->string_array = NULL;
  arr->element_size = 0;
}

size_t string_length(String str) { return str._length; }

StringResult string_create(const char *str) {
  if (!str)
    return RET_ERR(STR_ERR_NULL_PTR);
  size_t str_len = strlen(str);
  StringResult res = _alloc_string(str_len);
  if (!res.is_error) {
    memcpy(res.as.value.string, str, str_len);
  }
  return res;
}

StringResult string_copy(String str) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  StringResult res = _alloc_string(str._length);
  if (!res.is_error) {
    memcpy(res.as.value.string, str.string, str._length);
  }
  return res;
}

StringResult string_concat(String str1, String str2) {
  if (!str1.string || !str2.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  StringResult res = _alloc_string(str1._length + str2._length);
  if (!res.is_error) {
    memcpy(res.as.value.string, str1.string, str1._length);
    memcpy(res.as.value.string + str1._length, str2.string, str2._length);
  }
  return res;
}

int string_char_at(String str, size_t at) {
  if (!str.string || at >= str._length)
    return -1;
  return (unsigned char)str.string[at];
}

uint8_t string_compare(String str1, String str2) {
  if (!str1.string || !str2.string)
    return 0;
  if (str1._length != str2._length)
    return 0;
  return memcmp(str1.string, str2.string, str1._length) == 0;
}

StringResult string_trim_end(String str) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (str._length == 0)
    return string_copy(str);

  size_t i = str._length;
  while (i > 0) {
    char character = str.string[i - 1];
    if (character == ' ' || character == '\t' || character == '\n' ||
        character == '\r') {
      i--;
    } else {
      break;
    }
  }

  StringResult res = _alloc_string(i);
  if (!res.is_error) {
    memcpy(res.as.value.string, str.string, i);
  }
  return res;
}

StringResult string_trim_start(String str) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (str._length == 0)
    return string_copy(str);

  size_t i = 0;
  while (i < str._length) {
    char character = str.string[i];
    if (character == ' ' || character == '\t' || character == '\n' ||
        character == '\r') {
      i++;
    } else {
      break;
    }
  }

  StringResult res = _alloc_string(str._length - i);
  if (!res.is_error) {
    memcpy(res.as.value.string, str.string + i, res.as.value._length);
  }
  return res;
}

StringResult string_trim(String str) {
  StringResult start_res = string_trim_start(str);
  if (start_res.is_error)
    return start_res;

  StringResult end_res = string_trim_end(start_res.as.value);
  free_string(&start_res.as.value);
  return end_res;
}

StringResult string_pad_end(String str, size_t len, String pad_str) {
  if (!str.string || !pad_str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (pad_str._length == 0 || len <= str._length)
    return string_copy(str);

  StringResult res = _alloc_string(len);
  if (res.is_error)
    return res;

  memcpy(res.as.value.string, str.string, str._length);
  for (size_t i = 0; i < len - str._length; i++) {
    res.as.value.string[str._length + i] = pad_str.string[i % pad_str._length];
  }
  return res;
}

StringResult string_pad_start(String str, size_t len, String pad_str) {
  if (!str.string || !pad_str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (pad_str._length == 0 || len <= str._length)
    return string_copy(str);

  StringResult res = _alloc_string(len);
  if (res.is_error)
    return res;

  size_t pad_len = len - str._length;
  memcpy(res.as.value.string + pad_len, str.string, str._length);
  for (size_t i = 0; i < pad_len; i++) {
    res.as.value.string[i] = pad_str.string[i % pad_str._length];
  }
  return res;
}

StringResult string_uppercase(String str) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  StringResult res = _alloc_string(str._length);
  if (res.is_error)
    return res;

  for (size_t i = 0; i < str._length; i++) {
    char c = str.string[i];
    if (c >= 'a' && c <= 'z') {
      res.as.value.string[i] = c - ('a' - 'A');
    } else {
      res.as.value.string[i] = c;
    }
  }
  return res;
}

StringResult string_lowercase(String str) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  StringResult res = _alloc_string(str._length);
  if (res.is_error)
    return res;

  for (size_t i = 0; i < str._length; i++) {
    char c = str.string[i];
    if (c >= 'A' && c <= 'Z') {
      res.as.value.string[i] = c + ('a' - 'A');
    } else {
      res.as.value.string[i] = c;
    }
  }
  return res;
}

StringResult string_capitalize(String str) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (str._length == 0)
    return string_copy(str);

  StringResult res = _alloc_string(str._length);
  if (res.is_error)
    return res;

  char first_char = str.string[0];
  if (first_char >= 'a' && first_char <= 'z') {
    res.as.value.string[0] = first_char - ('a' - 'A');
  } else {
    res.as.value.string[0] = first_char;
  }
  for (size_t i = 1; i < str._length; i++) {
    char c = str.string[i];
    if (c >= 'A' && c <= 'Z') {
      res.as.value.string[i] = c + ('a' - 'A');
    } else {
      res.as.value.string[i] = c;
    }
  }
  return res;
}

size_t string_index_of(String str, const char *substr) {
  if (!str.string || !substr)
    return STRING_NPOS;
  char *found = strstr(str.string, substr);
  if (found)
    return (size_t)(found - str.string);
  return STRING_NPOS;
}

size_t string_last_index_of(String str, const char *substr) {
  if (!str.string || !substr)
    return STRING_NPOS;
  size_t last_index = STRING_NPOS;
  char *found = strstr(str.string, substr);
  while (found) {
    last_index = (size_t)(found - str.string);
    found = strstr(found + 1, substr);
  }
  return last_index;
}

StringResult string_slice_from(String str, size_t start, size_t end) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (start > str._length || end > str._length || start > end)
    return RET_ERR(STR_ERR_BOUNDS);

  size_t slice_length = end - start;
  StringResult res = _alloc_string(slice_length);
  if (!res.is_error) {
    memcpy(res.as.value.string, str.string + start, slice_length);
  }
  return res;
}

StringResult string_slice(String str, size_t start) {
  if (!str.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (start > str._length)
    return RET_ERR(STR_ERR_BOUNDS);

  size_t slice_length = str._length - start;
  StringResult res = _alloc_string(slice_length);
  if (!res.is_error) {
    memcpy(res.as.value.string, str.string + start, slice_length);
  }
  return res;
}

StringResult string_replace(String str, String old_substr, String new_substr) {
  if (!str.string || !old_substr.string || !new_substr.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (old_substr._length == 0)
    return string_copy(str);

  char *found = strstr(str.string, old_substr.string);
  if (!found)
    return string_copy(str);

  size_t new_length = str._length - old_substr._length + new_substr._length;
  StringResult res = _alloc_string(new_length);
  if (res.is_error)
    return res;

  size_t prefix_length = found - str.string;
  memcpy(res.as.value.string, str.string, prefix_length);
  memcpy(res.as.value.string + prefix_length, new_substr.string,
         new_substr._length);
  memcpy(res.as.value.string + prefix_length + new_substr._length,
         found + old_substr._length,
         str._length - prefix_length - old_substr._length);

  return res;
}

StringResult string_replace_all(String str, String old_substr,
                                String new_substr) {
  if (!str.string || !old_substr.string || !new_substr.string)
    return RET_ERR(STR_ERR_NULL_PTR);
  if (old_substr._length == 0)
    return string_copy(str);

  size_t count = 0;
  char *temp = str.string;
  while ((temp = strstr(temp, old_substr.string)) != NULL) {
    count++;
    temp += old_substr._length;
  }
  if (count == 0)
    return string_copy(str);

  size_t new_length;
  if (new_substr._length >= old_substr._length) {
    new_length =
        str._length + count * (new_substr._length - old_substr._length);
  } else {
    new_length =
        str._length - count * (old_substr._length - new_substr._length);
  }

  StringResult res = _alloc_string(new_length);
  if (res.is_error)
    return res;

  char *dest = res.as.value.string;
  char *current_src = str.string;
  while ((temp = strstr(current_src, old_substr.string)) != NULL) {
    size_t prefix_length = temp - current_src;
    memcpy(dest, current_src, prefix_length);
    dest += prefix_length;
    memcpy(dest, new_substr.string, new_substr._length);
    dest += new_substr._length;
    current_src = temp + old_substr._length;
  }

  size_t remaining_length = str._length - (current_src - str.string);
  memcpy(dest, current_src, remaining_length);
  return res;
}

StringResult cstring_replace(String str, const char *old_substr,
                             const char *new_substr) {
  if (!old_substr || !new_substr)
    return RET_ERR(STR_ERR_NULL_PTR);
  String old_str = {(char *)old_substr, strlen(old_substr), 0};
  String new_str = {(char *)new_substr, strlen(new_substr), 0};
  return string_replace(str, old_str, new_str);
}

StringResult cstring_replace_all(String str, const char *old_substr,
                                 const char *new_substr) {
  if (!old_substr || !new_substr)
    return RET_ERR(STR_ERR_NULL_PTR);
  String old_str = {(char *)old_substr, strlen(old_substr), 0};
  String new_str = {(char *)new_substr, strlen(new_substr), 0};
  return string_replace_all(str, old_str, new_str);
}

ArrayResult string_split(String str, String delimiter) {
  if (!str.string || !delimiter.string)
    return RET_ARR_ERR(STR_ERR_NULL_PTR);
  if (delimiter._length == 0)
    return RET_ARR_ERR(STR_ERR_BOUNDS);

  size_t count = 0;
  char *temp = str.string;
  while ((temp = strstr(temp, delimiter.string)) != NULL) {
    count++;
    temp += delimiter._length;
  }

  Array result = {0};
  result.length = count + 1;
  result.element_size = sizeof(String);
  result.string_array = malloc(result.length * sizeof(String));

  if (!result.string_array)
    return RET_ARR_ERR(STR_ERR_ALLOC);
  memset(result.string_array, 0, result.length * sizeof(String));

  size_t index = 0;
  char *start = str.string;
  while ((temp = strstr(start, delimiter.string)) != NULL) {
    size_t segment_length = temp - start;
    StringResult seg_res = _alloc_string(segment_length);
    if (seg_res.is_error) {
      array_free(&result);
      return RET_ARR_ERR(STR_ERR_ALLOC);
    }
    result.string_array[index] = seg_res.as.value;
    memcpy(result.string_array[index].string, start, segment_length);
    index++;
    start = temp + delimiter._length;
  }

  size_t segment_length = str._length - (start - str.string);
  StringResult last_seg = _alloc_string(segment_length);
  if (last_seg.is_error) {
    array_free(&result);
    return RET_ARR_ERR(STR_ERR_ALLOC);
  }
  result.string_array[index] = last_seg.as.value;
  memcpy(result.string_array[index].string, start, segment_length);

  return RET_ARR_OK(result);
}

SBResult sb_create(size_t capacity) {
  StringBuilder new_sb = {0};
  new_sb.string = malloc(sizeof(char) * capacity);
  if (!new_sb.string)
    return RET_SB_ERR(STR_ERR_ALLOC);
  new_sb._capacity = capacity;
  new_sb._length = 0;
  return RET_SB_OK(new_sb);
}

void sb_free(StringBuilder *builder) {
  if (builder) {
    free(builder->string);
    builder->string = NULL;
    builder->_length = 0;
    builder->_capacity = 0;
  }
}

StringError sb_append(StringBuilder *builder, String str) {
  if (!builder || !str.string)
    return STR_ERR_NULL_PTR;
  size_t required_space = builder->_length + str._length + 1;
  if (required_space > builder->_capacity) {
    size_t new_capacity = builder->_capacity ? builder->_capacity : 16;
    while (new_capacity < required_space)
      new_capacity *= 2;
    char *new_ptr = realloc(builder->string, new_capacity);
    if (!new_ptr)
      return STR_ERR_ALLOC;
    builder->string = new_ptr;
    builder->_capacity = new_capacity;
  }
  memcpy(builder->string + builder->_length, str.string, str._length);
  builder->_length += str._length;
  builder->string[builder->_length] = '\0';
  return STR_OK;
}

StringError cstring_sb_append(StringBuilder *builder, const char *str) {
  if (!str)
    return STR_ERR_NULL_PTR;
  String new_str = {0};
  new_str.owns_data = 0;
  new_str.string = (char *)str;
  new_str._length = strlen(str);
  return sb_append(builder, new_str);
}

StringError char_sb_append(StringBuilder *builder, const char c) {
  if (!builder)
    return STR_ERR_NULL_PTR;
  size_t required_space = builder->_length + 2;
  if (required_space > builder->_capacity) {
    size_t new_capacity = builder->_capacity ? builder->_capacity : 16;
    while (new_capacity < required_space)
      new_capacity *= 2;
    char *new_ptr = realloc(builder->string, new_capacity);
    if (!new_ptr)
      return STR_ERR_ALLOC;
    builder->string = new_ptr;
    builder->_capacity = new_capacity;
  }
  builder->string[builder->_length] = c;
  builder->_length += 1;
  builder->string[builder->_length] = '\0';
  return STR_OK;
}

StringResult sb_build(StringBuilder *builder) {
  if (!builder || !builder->string)
    return RET_ERR(STR_ERR_NULL_PTR);

  String new_str = {0};
  new_str.string = builder->string;
  new_str.owns_data = 1;
  new_str._length = builder->_length;

  builder->string = NULL;
  builder->_length = 0;
  builder->_capacity = 0;

  return RET_OK(new_str);
}
