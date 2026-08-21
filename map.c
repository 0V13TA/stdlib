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


#include "map.h"

/* ==========================================================================
 * INTERNAL MACROS & CONSTANTS
 * ========================================================================== */

// Special hash values to indicate empty or deleted slots in the open-addressing table
#define MAP_EMPTY 0ULL
#define MAP_TOMBSTONE 1ULL

// Forces the Most Significant Bit (MSB) to 1. 
// This ensures a valid hash never accidentally collides with MAP_EMPTY or MAP_TOMBSTONE.
#define MAP_MAKE_VALID_HASH(h) ((h) | (1ULL << 63))

/* ==========================================================================
 * MEMORY ALIGNMENT & OFFSETS
 * ========================================================================== */

// Aligns a size to the nearest 8-byte boundary to prevent unaligned access penalties
static inline size_t align8(size_t s) {
  return (s + 7) & ~7;
}

static inline size_t keys_offset(size_t capacity) {
  // Hashes come first in the FAM, taking 8 bytes each
  return capacity * sizeof(uint64_t);
}

static inline size_t values_offset(size_t capacity, size_t key_size) {
  // Values come after keys, aligned to 8 bytes
  return align8(keys_offset(capacity) + (capacity * key_size));
}

static inline size_t total_map_size(size_t capacity, size_t key_size, size_t value_size) {
  // Total size = sizeof(Map) + aligned hashes + aligned keys + values
  return sizeof(Map) + align8(values_offset(capacity, key_size) + (capacity * value_size));
}

/* Helper macros to grab typed pointers straight out of the FAM block */
#define GET_HASHES(m)  ((uint64_t *)(m)->data)
#define GET_KEYS(m)    ((uint8_t *)(m)->data + keys_offset((m)->_capacity))
#define GET_VALUES(m)  ((uint8_t *)(m)->data + values_offset((m)->_capacity, (m)->key_size))

/* ==========================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

static inline size_t next_pow2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}

static inline uint64_t get_valid_hash(const Map *m, const void *key) {
  uint64_t h;
  if (m->hash_func) {
    h = m->hash_func(key, m->ctx);
  } else {
    h = map_hash_fnv1a(key, m->key_size);
  }
  return MAP_MAKE_VALID_HASH(h);
}

static inline bool keys_equal(const Map *m, const void *k1, const void *k2) {
  if (m->eq_func) {
    return m->eq_func(k1, k2, m->ctx);
  }
  return memcmp(k1, k2, m->key_size) == 0;
}

/* ==========================================================================
 * LIFECYCLE
 * ========================================================================== */

MapResult map_create(size_t capacity, size_t key_size, size_t value_size,
                     uint64_t (*hash_func)(const void *key, void *ctx),
                     bool (*eq_func)(const void *key_a, const void *key_b, void *ctx),
                     void *ctx, Allocator *alloc) {
  MapResult res = {0};

  if (capacity < 4) capacity = 4;
  capacity = next_pow2(capacity);

  size_t total = total_map_size(capacity, key_size, value_size);
  Map *m = map_internal_malloc(alloc, total);
  
  if (!m) {
    res.as.error = MAP_ERR_ALLOC;
    res.is_error = 1;
    return res;
  }

  m->_length = 0;
  m->_capacity = capacity;
  m->key_size = key_size;
  m->value_size = value_size;
  m->allocator = alloc;
  m->hash_func = hash_func;
  m->eq_func = eq_func;
  m->ctx = ctx;

  // Zero-initialize the entire data block to ensure all hashes start as MAP_EMPTY (0)
  memset(m->data, 0, total - sizeof(Map));

  res.as.value = m;
  res.is_error = 0;
  return res;
}

void map_free(Map **m) {
  if (!m || !*m) return;
  map_internal_free((*m)->allocator, *m);
  *m = NULL;
}

void map_array_free(MapArray **arr) {
  if (!arr || !*arr) return;
  map_internal_free((*arr)->allocator, *arr);
  *arr = NULL;
}

/* ==========================================================================
 * MUTATIONS & REHASHING
 * ========================================================================== */

MapError map_set(Map **m_ptr, const void *key, const void *value) {
  if (!m_ptr || !*m_ptr || !key || !value) return MAP_ERR_NULL_PTR;
  Map *m = *m_ptr;

  // Hysteresis: Resize if we hit 75% load factor to maintain fast probing
  if (m->_length * 4 >= m->_capacity * 3) {
    MapResult resize_res = map_create(m->_capacity * 2, m->key_size, m->value_size, 
                                      m->hash_func, m->eq_func, m->ctx, m->allocator);
    if (resize_res.is_error) return resize_res.as.error;
    
    Map *new_m = resize_res.as.value;
    uint64_t *old_hashes = GET_HASHES(m);
    uint8_t *old_keys = GET_KEYS(m);
    uint8_t *old_values = GET_VALUES(m);
    
    uint64_t *new_hashes = GET_HASHES(new_m);
    uint8_t *new_keys = GET_KEYS(new_m);
    uint8_t *new_values = GET_VALUES(new_m);
    size_t new_mask = new_m->_capacity - 1;

    // Rehash all valid entries
    for (size_t i = 0; i < m->_capacity; i++) {
      uint64_t h = old_hashes[i];
      if (h != MAP_EMPTY && h != MAP_TOMBSTONE) {
        size_t idx = h & new_mask;
        // Probe until empty (no tombstones in a fresh map)
        while (new_hashes[idx] != MAP_EMPTY) {
          idx = (idx + 1) & new_mask;
        }
        new_hashes[idx] = h;
        memcpy(new_keys + (idx * m->key_size), old_keys + (i * m->key_size), m->key_size);
        memcpy(new_values + (idx * m->value_size), old_values + (i * m->value_size), m->value_size);
      }
    }
    
    new_m->_length = m->_length;
    map_free(m_ptr);
    *m_ptr = new_m;
    m = new_m;
  }

  uint64_t hash = get_valid_hash(m, key);
  size_t mask = m->_capacity - 1;
  size_t idx = hash & mask;
  
  uint64_t *hashes = GET_HASHES(m);
  uint8_t *keys = GET_KEYS(m);
  uint8_t *values = GET_VALUES(m);
  
  size_t tombstone_idx = MAP_NPOS;
  
  for (size_t i = 0; i < m->_capacity; i++) {
    uint64_t curr_h = hashes[idx];
    
    if (curr_h == MAP_EMPTY) {
      break; // Reached the end of the probe chain
    } else if (curr_h == MAP_TOMBSTONE) {
      if (tombstone_idx == MAP_NPOS) tombstone_idx = idx; // Remember the first tombstone
    } else if (curr_h == hash) {
      if (keys_equal(m, key, keys + (idx * m->key_size))) {
        // Update existing value
        memcpy(values + (idx * m->value_size), value, m->value_size);
        return MAP_OK;
      }
    }
    idx = (idx + 1) & mask;
  }
  
  // Insert at the optimal slot (reuse tombstone if found, else use the empty slot)
  size_t insert_idx = (tombstone_idx != MAP_NPOS) ? tombstone_idx : idx;
  
  hashes[insert_idx] = hash;
  memcpy(keys + (insert_idx * m->key_size), key, m->key_size);
  memcpy(values + (insert_idx * m->value_size), value, m->value_size);
  m->_length++;
  
  return MAP_OK;
}

MapError map_remove(Map **m_ptr, const void *key, void *out_value) {
  if (!m_ptr || !*m_ptr || !key) return MAP_ERR_NULL_PTR;
  Map *m = *m_ptr;

  uint64_t hash = get_valid_hash(m, key);
  size_t mask = m->_capacity - 1;
  size_t idx = hash & mask;
  
  uint64_t *hashes = GET_HASHES(m);
  uint8_t *keys = GET_KEYS(m);
  uint8_t *values = GET_VALUES(m);
  
  for (size_t i = 0; i < m->_capacity; i++) {
    uint64_t curr_h = hashes[idx];
    
    if (curr_h == MAP_EMPTY) {
      return MAP_ERR_NOT_FOUND;
    }
    
    if (curr_h == hash && keys_equal(m, key, keys + (idx * m->key_size))) {
      hashes[idx] = MAP_TOMBSTONE; // Mark as deleted
      m->_length--;
      
      if (out_value) {
        memcpy(out_value, values + (idx * m->value_size), m->value_size);
      }
      return MAP_OK;
    }
    idx = (idx + 1) & mask;
  }
  
  return MAP_ERR_NOT_FOUND;
}

/* ==========================================================================
 * ACCESS & SEARCH
 * ========================================================================== */

MapValueResult map_get(const Map *m, const void *key) {
  MapValueResult res = {0};
  if (!m || !key) {
    res.as.error = MAP_ERR_NULL_PTR;
    res.is_error = 1;
    return res;
  }

  uint64_t hash = get_valid_hash(m, key);
  size_t mask = m->_capacity - 1;
  size_t idx = hash & mask;
  
  uint64_t *hashes = GET_HASHES(m);
  uint8_t *keys = GET_KEYS(m);
  uint8_t *values = GET_VALUES(m);
  
  for (size_t i = 0; i < m->_capacity; i++) {
    uint64_t curr_h = hashes[idx];
    
    if (curr_h == MAP_EMPTY) {
      break;
    }
    if (curr_h == hash && keys_equal(m, key, keys + (idx * m->key_size))) {
      res.as.value = (void *)(values + (idx * m->value_size));
      res.is_error = 0;
      return res;
    }
    idx = (idx + 1) & mask;
  }
  
  res.as.error = MAP_ERR_NOT_FOUND;
  res.is_error = 1;
  return res;
}

bool map_contains(const Map *m, const void *key) {
  MapValueResult res = map_get(m, key);
  return !res.is_error;
}

/* ==========================================================================
 * ITERATION & EXTRACTION
 * ========================================================================== */

MapError map_for_each(Map *m, void (*func)(const void *key, void *value, void *ctx), void *ctx) {
  if (!m || !func) return MAP_ERR_NULL_PTR;

  uint64_t *hashes = GET_HASHES(m);
  uint8_t *keys = GET_KEYS(m);
  uint8_t *values = GET_VALUES(m);

  for (size_t i = 0; i < m->_capacity; i++) {
    uint64_t h = hashes[i];
    if (h != MAP_EMPTY && h != MAP_TOMBSTONE) {
      void *k_ptr = keys + (i * m->key_size);
      void *v_ptr = values + (i * m->value_size);
      func(k_ptr, v_ptr, ctx);
    }
  }

  return MAP_OK;
}

static MapArrayResult allocate_map_array(size_t len, size_t el_size, Allocator *alloc) {
  MapArrayResult res = {0};
  size_t total = sizeof(MapArray) + (len * el_size);
  
  MapArray *arr = map_internal_malloc(alloc, total);
  if (!arr) {
    res.as.error = MAP_ERR_ALLOC;
    res.is_error = 1;
    return res;
  }
  
  arr->length = len;
  arr->element_size = el_size;
  arr->allocator = alloc;
  
  res.as.value = arr;
  res.is_error = 0;
  return res;
}

MapArrayResult map_keys(const Map *m, Allocator *alloc) {
  MapArrayResult res = allocate_map_array(m ? m->_length : 0, m ? m->key_size : 0, alloc);
  if (res.is_error || !m || m->_length == 0) return res;

  uint64_t *hashes = GET_HASHES(m);
  uint8_t *keys = GET_KEYS(m);
  uint8_t *dest = res.as.value->data;
  size_t count = 0;

  for (size_t i = 0; i < m->_capacity; i++) {
    if (hashes[i] != MAP_EMPTY && hashes[i] != MAP_TOMBSTONE) {
      memcpy(dest + (count * m->key_size), keys + (i * m->key_size), m->key_size);
      count++;
    }
  }
  
  return res;
}

MapArrayResult map_values(const Map *m, Allocator *alloc) {
  MapArrayResult res = allocate_map_array(m ? m->_length : 0, m ? m->value_size : 0, alloc);
  if (res.is_error || !m || m->_length == 0) return res;

  uint64_t *hashes = GET_HASHES(m);
  uint8_t *values = GET_VALUES(m);
  uint8_t *dest = res.as.value->data;
  size_t count = 0;

  for (size_t i = 0; i < m->_capacity; i++) {
    if (hashes[i] != MAP_EMPTY && hashes[i] != MAP_TOMBSTONE) {
      memcpy(dest + (count * m->value_size), values + (i * m->value_size), m->value_size);
      count++;
    }
  }
  
  return res;
}
