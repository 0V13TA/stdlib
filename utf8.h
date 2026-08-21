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

#ifndef UTF8_H
#define UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UTF8_MAX_BYTES 4

// The standard replacement character '' used when decoding encounters invalid
// bytes
#define UTF8_REPLACEMENT_CODEPOINT 0xFFFD

/* ==========================================================================
 * TYPES & STRUCTURES
 * ========================================================================== */

typedef enum {
  UTF8_OK = 0,
  UTF8_ERR_INVALID_BYTE, // Byte does not follow UTF-8 bit patterns
  UTF8_ERR_TRUNCATED,    // String ended before the codepoint finished
  UTF8_ERR_OVERLONG,  // Codepoint was encoded using more bytes than necessary
  UTF8_ERR_SURROGATE, // Codepoint falls in the invalid UTF-16 surrogate range
  UTF8_ERR_OUT_OF_RANGE,    // Codepoint is > 0x10FFFF
  UTF8_ERR_BUFFER_TOO_SMALL // Output buffer is too small to hold encoded bytes
} Utf8Error;

// A decoded Unicode character and the number of bytes it took up in the string
typedef struct {
  uint32_t codepoint;
  size_t bytes_consumed;
} Utf8Decoded;

// Tagged Union for Decode Result
typedef struct {
  union {
    Utf8Decoded value;
    Utf8Error error;
  } as;
  uint8_t is_error;
} Utf8DecodeResult;

// Tagged Union for Encode Result
typedef struct {
  union {
    size_t bytes_written; // Will be between 1 and 4
    Utf8Error error;
  } as;
  uint8_t is_error;
} Utf8EncodeResult;

/* ==========================================================================
 * DECODING & ENCODING
 * ========================================================================== */

/**
 * Reads the next Unicode codepoint from a UTF-8 encoded string.
 * @param str const char* Pointer to the start of the sequence.
 * @param byte_len size_t Maximum number of bytes left to read in the buffer.
 * @return Utf8DecodeResult Contains the codepoint and how many bytes to advance
 * your pointer by.
 */
Utf8DecodeResult utf8_decode(const char *str, size_t byte_len);

/**
 * Encodes a raw Unicode codepoint into UTF-8 bytes.
 * @param codepoint uint32_t The Unicode character to encode.
 * @param out_buffer char* Destination memory (must have up to 4 bytes
 * available).
 * @param buffer_len size_t The size of the destination memory.
 * @return Utf8EncodeResult The number of bytes written to out_buffer, or an
 * error.
 */
Utf8EncodeResult utf8_encode(uint32_t codepoint, char *out_buffer,
                             size_t buffer_len);

/* ==========================================================================
 * UTILITIES
 * ========================================================================== */

/**
 * Counts the number of actual Unicode characters (runes) in a UTF-8 string.
 * @param str const char* Pointer to the UTF-8 string.
 * @param byte_len size_t The length of the string in bytes.
 * @return size_t The number of valid codepoints.
 */
size_t utf8_codepoint_count(const char *str, size_t byte_len);

/**
 * Checks if a byte array is a strictly valid UTF-8 sequence.
 * Rejects overlong encodings, surrogates, and out-of-range codepoints.
 * @param str const char* Pointer to the sequence.
 * @param byte_len size_t Length of the sequence in bytes.
 * @return bool True if completely valid, false if any byte is invalid.
 */
bool utf8_is_valid(const char *str, size_t byte_len);

#endif // UTF8_H
