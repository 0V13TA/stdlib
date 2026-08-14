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

/*
 * All String operations return new owned strings. The original inputs are not
 * modified unless passed to free_string or STR_REASSIGN by the caller.
 */

static String _alloc_string(size_t len) {
  String new_str = {0};
  char *ptr = malloc(sizeof(char) * (len + 1));

  if (ptr == NULL) {
    return new_str;
  }

  ptr[len] = '\0';
  new_str.string = ptr;
  new_str._length = len;
  new_str.owns_data = 1;
  return new_str;
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
  size_t i;

  if (!arr)
    return;

  for (i = 0; i < arr->length; i++) {
    free_string(&arr->string_array[i]);
  }
  free(arr->string_array);
  arr->length = 0;
  arr->string_array = NULL;
  arr->element_size = 0;
}

size_t string_length(String str) { return str._length; }

String string_create(const char *str) {
  if (!str)
    return (String){0};

  size_t str_len = strlen(str);
  String new_str = _alloc_string(str_len);

  if (new_str.string) {
    memcpy(new_str.string, str, str_len);
  }
  return new_str;
}

String string_copy(String str) {
  if (!str.string)
    return (String){0};

  String new_str = _alloc_string(str._length);
  if (new_str.string) {
    memcpy(new_str.string, str.string, str._length);
  }
  return new_str;
}

String string_concat(String str1, String str2) {
  if (!str1.string || !str2.string)
    return (String){0};

  String new_str = _alloc_string(str1._length + str2._length);
  if (new_str.string) {
    memcpy(new_str.string, str1.string, str1._length);
    memcpy(new_str.string + str1._length, str2.string, str2._length);
  }
  return new_str;
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

String string_trim_end(String str) {
  if (!str.string || str._length == 0)
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

  String new_str = _alloc_string(i);
  if (new_str.string) {
    memcpy(new_str.string, str.string, i);
  }
  return new_str;
}

String string_trim_start(String str) {
  if (!str.string || str._length == 0)
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

  String new_str = _alloc_string(str._length - i);
  if (new_str.string) {
    memcpy(new_str.string, str.string + i, new_str._length);
  }
  return new_str;
}

String string_trim(String str) {
  String trim_start = string_trim_start(str);
  if (!trim_start.string)
    return trim_start; // Propagate malloc failure

  String trim_end = string_trim_end(trim_start);

  free_string(&trim_start); // Safely clean up intermediate allocation
  return trim_end;
}

String string_pad_end(String str, size_t len, String pad_str) {
  if (!str.string || !pad_str.string || pad_str._length == 0)
    return string_copy(str);
  if (len <= str._length)
    return string_copy(str);

  String new_str = _alloc_string(len);
  if (!new_str.string)
    return new_str;

  memcpy(new_str.string, str.string, str._length);
  for (size_t i = 0; i < len - str._length; i++) {
    new_str.string[str._length + i] = pad_str.string[i % pad_str._length];
  }

  return new_str;
}

String string_pad_start(String str, size_t len, String pad_str) {
  if (!str.string || !pad_str.string || pad_str._length == 0)
    return string_copy(str);
  if (len <= str._length)
    return string_copy(str);

  String new_str = _alloc_string(len);
  if (!new_str.string)
    return new_str;

  size_t pad_len = len - str._length;
  memcpy(new_str.string + pad_len, str.string, str._length);
  for (size_t i = 0; i < pad_len; i++) {
    new_str.string[i] = pad_str.string[i % pad_str._length];
  }

  return new_str;
}

String string_uppercase(String str) {
  if (!str.string)
    return (String){0};

  String new_str = _alloc_string(str._length);
  if (!new_str.string)
    return new_str;

  for (size_t i = 0; i < str._length; i++) {
    char c = str.string[i];
    if (c >= 'a' && c <= 'z') {
      new_str.string[i] = c - ('a' - 'A');
    } else {
      new_str.string[i] = c;
    }
  }

  return new_str;
}

String string_lowercase(String str) {
  if (!str.string)
    return (String){0};

  String new_str = _alloc_string(str._length);
  if (!new_str.string)
    return new_str;

  for (size_t i = 0; i < str._length; i++) {
    char c = str.string[i];
    if (c >= 'A' && c <= 'Z') {
      new_str.string[i] = c + ('a' - 'A');
    } else {
      new_str.string[i] = c;
    }
  }

  return new_str;
}

String string_capitalize(String str) {
  if (!str.string || str._length == 0)
    return string_copy(str);

  String new_str = _alloc_string(str._length);
  if (!new_str.string)
    return new_str;

  char first_char = str.string[0];
  if (first_char >= 'a' && first_char <= 'z') {
    new_str.string[0] = first_char - ('a' - 'A');
  } else {
    new_str.string[0] = first_char;
  }

  for (size_t i = 1; i < str._length; i++) {
    char c = str.string[i];
    if (c >= 'A' && c <= 'Z') {
      new_str.string[i] = c + ('a' - 'A');
    } else {
      new_str.string[i] = c;
    }
  }

  return new_str;
}

size_t string_index_of(String str, const char *substr) {
  if (!str.string || !substr)
    return STRING_NPOS;

  char *found = strstr(str.string, substr);
  if (found) {
    return (size_t)(found - str.string);
  }
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

String string_slice_from(String str, size_t start, size_t end) {
  if (!str.string || start > str._length || end > str._length || start > end)
    return (String){0};

  size_t slice_length = end - start;
  String new_str = _alloc_string(slice_length);
  if (!new_str.string)
    return new_str;

  memcpy(new_str.string, str.string + start, slice_length);
  return new_str;
}

String string_slice(String str, size_t start) {
  if (!str.string || start > str._length)
    return (String){0};

  size_t slice_length = str._length - start;
  String new_str = _alloc_string(slice_length);
  if (!new_str.string)
    return new_str;

  memcpy(new_str.string, str.string + start, slice_length);
  return new_str;
}

String string_replace(String str, String old_substr, String new_substr) {
  if (!str.string || !old_substr.string || !new_substr.string)
    return (String){0};
  if (old_substr._length == 0)
    return string_copy(str);

  char *found = strstr(str.string, old_substr.string);
  if (!found)
    return string_copy(str);

  size_t new_length = str._length - old_substr._length + new_substr._length;
  String new_str = _alloc_string(new_length);
  if (!new_str.string)
    return new_str;

  size_t prefix_length = found - str.string;
  memcpy(new_str.string, str.string, prefix_length);
  memcpy(new_str.string + prefix_length, new_substr.string, new_substr._length);
  memcpy(new_str.string + prefix_length + new_substr._length,
         found + old_substr._length,
         str._length - prefix_length - old_substr._length);

  return new_str;
}

String string_replace_all(String str, String old_substr, String new_substr) {
  if (!str.string || !old_substr.string || !new_substr.string)
    return (String){0};
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
  String new_str = _alloc_string(new_length);
  if (!new_str.string)
    return new_str;

  char *dest = new_str.string;
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

  return new_str;
}

String cstring_replace(String str, const char *old_substr,
                       const char *new_substr) {
  if (!old_substr || !new_substr)
    return (String){0};

  String old_str = {(char *)old_substr, strlen(old_substr), 0};
  String new_str = {(char *)new_substr, strlen(new_substr), 0};

  return string_replace(str, old_str, new_str);
}

String cstring_replace_all(String str, const char *old_substr,
                           const char *new_substr) {
  if (!old_substr || !new_substr)
    return (String){0};

  String old_str = {(char *)old_substr, strlen(old_substr), 0};
  String new_str = {(char *)new_substr, strlen(new_substr), 0};
  return string_replace_all(str, old_str, new_str);
}

Array string_split(String str, String delimiter) {
  Array result = {0};
  if (!str.string || !delimiter.string || delimiter._length == 0)
    return result;

  size_t count = 0;
  char *temp = str.string;
  while ((temp = strstr(temp, delimiter.string)) != NULL) {
    count++;
    temp += delimiter._length;
  }

  result.length = count + 1;
  result.element_size = sizeof(String);
  result.string_array = malloc(result.length * sizeof(String));
  if (!result.string_array)
    return result;
  memset(result.string_array, 0, result.length * sizeof(String));

  size_t index = 0;
  char *start = str.string;
  while ((temp = strstr(start, delimiter.string)) != NULL) {
    size_t segment_length = temp - start;
    result.string_array[index] = _alloc_string(segment_length);
    if (result.string_array[index].string) {
      memcpy(result.string_array[index].string, start, segment_length);
    } else {
      array_free(&result);
      return result;
    }
    index++;
    start = temp + delimiter._length;
  }
  // Handle the last segment
  size_t segment_length = str._length - (start - str.string);
  result.string_array[index] = _alloc_string(segment_length);
  if (result.string_array[index].string) {
    memcpy(result.string_array[index].string, start, segment_length);
  } else {
    array_free(&result);
  }

  return result;
}

StringBuilder sb_create(size_t capacity) {
  StringBuilder new_sb = {0};
  new_sb.string = malloc(sizeof(char) * capacity);
  if (!new_sb.string)
    return new_sb;
  new_sb._capacity = capacity;
  new_sb._length = 0;
  return new_sb;
}

void sb_free(StringBuilder *builder) {
  if (builder) {
    builder->_length = 0;
    builder->_capacity = 0;
    free(builder->string);
  }
}

uint8_t sb_append(StringBuilder *builder, String str) {
  size_t required_space = builder->_length + str._length;
  if (required_space >= builder->_capacity) {
    builder->_capacity *= 2;
    builder->string = realloc(builder->string, builder->_capacity);
    if (!builder->string)
      return 0;
  }

  memcpy(builder->string + builder->_length, str.string, str._length);
  builder->_length += str._length;
  builder->string[builder->_length] = '\0';
  return 1;
}

uint8_t cstring_sb_append(StringBuilder *builder, const char *str) {
  String new_str = {0};
  new_str.owns_data = 0;
  new_str.string = (char *)str;
  new_str._length = strlen(str);

  return sb_append(builder, new_str);
}

uint8_t char_sb_append(StringBuilder *builder, const char c) {
  size_t required_space = builder->_length + 1;
  if (required_space >= builder->_capacity) {
    builder->_capacity *= 2;
    builder->string = realloc(builder->string, builder->_capacity);
    if (!builder->string)
      return 0;
  }

  builder->string[builder->_length] = c;
  builder->_length += 1;
  builder->string[builder->_length] = '\0';
  return 1;
}

String sb_build(StringBuilder *builder) {
  String new_str = {0};
  if (builder) {
    new_str.string = builder->string;
    new_str.owns_data = 1;
    new_str._length = builder->_length;
    sb_free(builder);
  }
  return new_str;
}
