/* Copyright (c) 2026 OVIETA <ovieta17@gmail.com>
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

#include "utf8.h"

/* ==========================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

// Checks if a byte is a valid UTF-8 continuation byte (matches 10xxxxxx)
static inline bool is_continuation_byte(uint8_t byte) {
  return (byte & 0xC0) == 0x80;
}

// Result constructors
static inline Utf8DecodeResult decode_err(Utf8Error err) {
  Utf8DecodeResult res = {0};
  res.as.error = err;
  res.is_error = 1;
  return res;
}

static inline Utf8DecodeResult decode_ok(uint32_t cp, size_t bytes) {
  Utf8DecodeResult res = {0};
  res.as.value.codepoint = cp;
  res.as.value.bytes_consumed = bytes;
  res.is_error = 0;
  return res;
}

static inline Utf8EncodeResult encode_err(Utf8Error err) {
  Utf8EncodeResult res = {0};
  res.as.error = err;
  res.is_error = 1;
  return res;
}

static inline Utf8EncodeResult encode_ok(size_t bytes) {
  Utf8EncodeResult res = {0};
  res.as.bytes_written = bytes;
  res.is_error = 0;
  return res;
}

/* ==========================================================================
 * DECODING & ENCODING
 * ========================================================================== */

Utf8DecodeResult utf8_decode(const char *str, size_t byte_len) {
  if (!str || byte_len == 0)
    return decode_err(UTF8_ERR_TRUNCATED);

  const uint8_t *s = (const uint8_t *)str;
  uint8_t b0 = s[0];

  // 1-byte sequence (ASCII): 0xxxxxxx
  if ((b0 & 0x80) == 0x00) {
    return decode_ok(b0, 1);
  }

  // 2-byte sequence: 110xxxxx 10xxxxxx
  if ((b0 & 0xE0) == 0xC0) {
    if (byte_len < 2)
      return decode_err(UTF8_ERR_TRUNCATED);
    if (!is_continuation_byte(s[1]))
      return decode_err(UTF8_ERR_INVALID_BYTE);

    uint32_t cp = ((b0 & 0x1F) << 6) | (s[1] & 0x3F);
    if (cp < 0x80)
      return decode_err(UTF8_ERR_OVERLONG);

    return decode_ok(cp, 2);
  }

  // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
  if ((b0 & 0xF0) == 0xE0) {
    if (byte_len < 3)
      return decode_err(UTF8_ERR_TRUNCATED);
    if (!is_continuation_byte(s[1]) || !is_continuation_byte(s[2])) {
      return decode_err(UTF8_ERR_INVALID_BYTE);
    }

    uint32_t cp = ((b0 & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);

    if (cp < 0x800)
      return decode_err(UTF8_ERR_OVERLONG);
    if (cp >= 0xD800 && cp <= 0xDFFF)
      return decode_err(UTF8_ERR_SURROGATE);

    return decode_ok(cp, 3);
  }

  // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  if ((b0 & 0xF8) == 0xF0) {
    if (byte_len < 4)
      return decode_err(UTF8_ERR_TRUNCATED);
    if (!is_continuation_byte(s[1]) || !is_continuation_byte(s[2]) ||
        !is_continuation_byte(s[3])) {
      return decode_err(UTF8_ERR_INVALID_BYTE);
    }

    uint32_t cp = ((b0 & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
                  ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);

    if (cp < 0x10000)
      return decode_err(UTF8_ERR_OVERLONG);
    if (cp > 0x10FFFF)
      return decode_err(UTF8_ERR_OUT_OF_RANGE);

    return decode_ok(cp, 4);
  }

  return decode_err(UTF8_ERR_INVALID_BYTE);
}

Utf8EncodeResult utf8_encode(uint32_t cp, char *out_buffer, size_t buffer_len) {
  if (!out_buffer)
    return encode_err(UTF8_ERR_INVALID_BYTE);

  if (cp <= 0x7F) {
    if (buffer_len < 1)
      return encode_err(UTF8_ERR_BUFFER_TOO_SMALL);
    out_buffer[0] = (char)cp;
    return encode_ok(1);
  }

  if (cp <= 0x7FF) {
    if (buffer_len < 2)
      return encode_err(UTF8_ERR_BUFFER_TOO_SMALL);
    out_buffer[0] = (char)(0xC0 | (cp >> 6));
    out_buffer[1] = (char)(0x80 | (cp & 0x3F));
    return encode_ok(2);
  }

  if (cp <= 0xFFFF) {
    if (cp >= 0xD800 && cp <= 0xDFFF)
      return encode_err(UTF8_ERR_SURROGATE);
    if (buffer_len < 3)
      return encode_err(UTF8_ERR_BUFFER_TOO_SMALL);

    out_buffer[0] = (char)(0xE0 | (cp >> 12));
    out_buffer[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out_buffer[2] = (char)(0x80 | (cp & 0x3F));
    return encode_ok(3);
  }

  if (cp <= 0x10FFFF) {
    if (buffer_len < 4)
      return encode_err(UTF8_ERR_BUFFER_TOO_SMALL);

    out_buffer[0] = (char)(0xF0 | (cp >> 18));
    out_buffer[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out_buffer[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out_buffer[3] = (char)(0x80 | (cp & 0x3F));
    return encode_ok(4);
  }

  return encode_err(UTF8_ERR_OUT_OF_RANGE);
}

/* ==========================================================================
 * UTILITIES
 * ========================================================================== */

size_t utf8_codepoint_count(const char *str, size_t byte_len) {
  if (!str || byte_len == 0)
    return 0;

  size_t count = 0;
  size_t offset = 0;

  while (offset < byte_len) {
    Utf8DecodeResult res = utf8_decode(str + offset, byte_len - offset);
    if (res.is_error) {
      // Advance by 1 byte to attempt resynchronization on invalid sequences
      offset++;
    } else {
      offset += res.as.value.bytes_consumed;
      count++;
    }
  }

  return count;
}

bool utf8_is_valid(const char *str, size_t byte_len) {
  if (!str || byte_len == 0)
    return true; // An empty string is technically valid

  size_t offset = 0;
  while (offset < byte_len) {
    Utf8DecodeResult res = utf8_decode(str + offset, byte_len - offset);
    if (res.is_error) {
      return false; // Fail immediately on the first invalid sequence
    }
    offset += res.as.value.bytes_consumed;
  }

  return true;
}
