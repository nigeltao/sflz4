// Copyright 2022 Nigel Tao.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SFLZ4_INCLUDE_GUARD
#define SFLZ4_INCLUDE_GUARD

// SFLZ4 is a single file C library for the LZ4 block compression format.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ================================ +Public Interface

// SFLZ4 ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define SFLZ4_IMPLEMENTATION before #include'ing or
// compiling it.

// -------- Compile-time Configuration

// The compile-time configuration macros are:
//  - SFLZ4_CONFIG__STATIC_FUNCTIONS

// ----

// Define SFLZ4_CONFIG__STATIC_FUNCTIONS (combined with SFLZ4_IMPLEMENTATION) to
// make all of SFLZ4's functions have static storage.
//
// This can help the compiler ignore or discard unused code, which can produce
// faster compiles and smaller binaries. Other motivations are discussed in the
// "ALLOW STATIC IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#if defined(SFLZ4_CONFIG__STATIC_FUNCTIONS)
#define SFLZ4_MAYBE_STATIC static
#else
#define SFLZ4_MAYBE_STATIC
#endif  // defined(SFLZ4_CONFIG__STATIC_FUNCTIONS)

// -------- Basics

// Clang also #define's "__GNUC__".
#if defined(__GNUC__) || defined(_MSC_VER)
#define SFLZ4_RESTRICT __restrict
#else
#define SFLZ4_RESTRICT
#endif

typedef struct sflz4_size_result_struct {
  const char* status_message;
  size_t value;
} sflz4_size_result;

// -------- Status Messages

extern const char sflz4_status_message__error_dst_is_too_short[];
extern const char sflz4_status_message__error_invalid_data[];
extern const char sflz4_status_message__error_src_is_too_long[];

// -------- LZ4 Decode

// SFLZ4_LZ4_BLOCK_DECODE_MAX_INCL_SRC_LEN is the maximum (inclusive) supported
// input length for this file's LZ4 decode functions. The LZ4 block format can
// generally support longer inputs, but this implementation specifically is
// more limited, to simplify overflow checking.
//
// With sufficiently large input, sflz4_block_encode (note that that's
// encode, not decode) may very well produce output that is longer than this.
// That output is valid (in terms of the LZ4 file format) but isn't decodable
// by sflz4_block_decode.
//
// 0x00FFFFFF = 16777215, which is over 16 million bytes.
#define SFLZ4_LZ4_BLOCK_DECODE_MAX_INCL_SRC_LEN 0x00FFFFFF

// sflz4_block_decode writes to dst the LZ4 block decompressed form of src,
// returning the number of bytes written.
//
// It fails with sflz4_status_message__error_dst_is_too_short if dst_len is
// not long enough to hold the decompressed form.
SFLZ4_MAYBE_STATIC sflz4_size_result        //
sflz4_block_decode(                         //
    uint8_t* SFLZ4_RESTRICT dst_ptr,        //
    size_t dst_len,                         //
    const uint8_t* SFLZ4_RESTRICT src_ptr,  //
    size_t src_len);

// -------- LZ4 Encode

// SFLZ4_LZ4_BLOCK_ENCODE_MAX_INCL_SRC_LEN is the maximum (inclusive) supported
// input length for this file's LZ4 encode functions. The LZ4 block format can
// generally support longer inputs, but this implementation specifically is
// more limited, to simplify overflow checking.
//
// 0x7E000000 = 2113929216, which is over 2 billion bytes.
#define SFLZ4_LZ4_BLOCK_ENCODE_MAX_INCL_SRC_LEN 0x7E000000

// sflz4_block_encode_worst_case_dst_len returns the maximum (inclusive)
// number of bytes required to LZ4 block compress src_len input bytes.
SFLZ4_MAYBE_STATIC sflz4_size_result    //
sflz4_block_encode_worst_case_dst_len(  //
    size_t src_len);

// sflz4_block_encode writes to dst the LZ4 block compressed form of src,
// returning the number of bytes written.
//
// Unlike the LZ4_compress_default function from the official implementation
// (https://github.com/lz4/lz4), it fails immediately with
// sflz4_status_message__error_dst_is_too_short if dst_len is less than
// sflz4_block_encode_worst_case_dst_len(src_len), even if the worst case is
// unrealized and the compressed form would actually fit.
SFLZ4_MAYBE_STATIC sflz4_size_result        //
sflz4_block_encode(                         //
    uint8_t* SFLZ4_RESTRICT dst_ptr,        //
    size_t dst_len,                         //
    const uint8_t* SFLZ4_RESTRICT src_ptr,  //
    size_t src_len);

// ================================ -Public Interface

#ifdef SFLZ4_IMPLEMENTATION

// ================================ +Private Implementation

// -------- Basics

// This implementation assumes that:
//  - converting a uint32_t to a size_t will never overflow.
//  - converting a size_t to a uint64_t will never overflow.
#if defined(__WORDSIZE)
#if (__WORDSIZE != 32) && (__WORDSIZE != 64)
#error "sflz4.h requires a word size of either 32 or 64 bits"
#endif
#endif

// Normally, the sflz4_private_peek_u32le implementation is both (1) correct
// regardless of CPU endianness and (2) very fast (e.g. an inlined
// sflz4_private_peek_u32le call, in an optimized clang or gcc build, is a
// single MOV instruction on x86_64).
//
// However, the endian-agnostic implementations are slow on Microsoft's C
// compiler (MSC). Alternative memcpy-based implementations restore speed, but
// they are only correct on little-endian CPU architectures. Defining
// SFLZ4_USE_MEMCPY_LE_PEEK_POKE opts in to these implementations.
//
// https://godbolt.org/z/q4MfjzTPh
#if defined(_MSC_VER) && !defined(__clang__) && \
    (defined(_M_ARM64) || defined(_M_X64))
#define SFLZ4_USE_MEMCPY_LE_PEEK_POKE
#endif

static inline uint32_t     //
sflz4_private_peek_u32le(  //
    const uint8_t* p) {
#if defined(SFLZ4_USE_MEMCPY_LE_PEEK_POKE)
  uint32_t x;
  memcpy(&x, p, 4);
  return x;
#else
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
#endif
}

// -------- Status Messages

const char sflz4_status_message__error_dst_is_too_short[] =  //
    "#sflz4: dst is too short";
const char sflz4_status_message__error_invalid_data[] =  //
    "#sflz4: invalid data";
const char sflz4_status_message__error_src_is_too_long[] =  //
    "#sflz4: src is too long";

// -------- LZ4 Decode

SFLZ4_MAYBE_STATIC sflz4_size_result        //
sflz4_block_decode(                         //
    uint8_t* SFLZ4_RESTRICT dst_ptr,        //
    size_t dst_len,                         //
    const uint8_t* SFLZ4_RESTRICT src_ptr,  //
    size_t src_len) {
  sflz4_size_result result = {0};

  if (src_len > SFLZ4_LZ4_BLOCK_DECODE_MAX_INCL_SRC_LEN) {
    result.status_message = sflz4_status_message__error_src_is_too_long;
    return result;
  }

  uint8_t* const original_dst_ptr = dst_ptr;

  // See https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md for file
  // format details, such as the LZ4 token's bit patterns.
  while (src_len > 0) {
    uint32_t token = *src_ptr++;
    src_len--;

    uint32_t literal_len = token >> 4;
    if (literal_len > 0) {
      if (literal_len == 15) {
        while (1) {
          if (src_len == 0) {
            goto fail_invalid_data;
          }
          uint32_t s = *src_ptr++;
          src_len--;
          literal_len += s;
          if (s != 255) {
            break;
          }
        }
      }

      if (literal_len > src_len) {
        goto fail_invalid_data;
      } else if (literal_len > dst_len) {
        result.status_message = sflz4_status_message__error_dst_is_too_short;
        return result;
      }
      memcpy(dst_ptr, src_ptr, literal_len);
      dst_ptr += literal_len;
      dst_len -= literal_len;
      src_ptr += literal_len;
      src_len -= literal_len;
      if (src_len == 0) {
        result.value = ((size_t)(dst_ptr - original_dst_ptr));
        return result;
      }
    }

    if (src_len < 2) {
      goto fail_invalid_data;
    }
    uint32_t copy_off = ((uint32_t)src_ptr[0]) | (((uint32_t)src_ptr[1]) << 8);
    src_ptr += 2;
    src_len -= 2;
    if ((copy_off == 0) ||  //
        (copy_off > ((size_t)(dst_ptr - original_dst_ptr)))) {
      goto fail_invalid_data;
    }

    uint32_t copy_len = (token & 15) + 4;
    if (copy_len == 19) {
      while (1) {
        if (src_len == 0) {
          goto fail_invalid_data;
        }
        uint32_t s = *src_ptr++;
        src_len--;
        copy_len += s;
        if (s != 255) {
          break;
        }
      }
    }

    if (dst_len < copy_len) {
      result.status_message = sflz4_status_message__error_dst_is_too_short;
      return result;
    }
    dst_len -= copy_len;
    for (const uint8_t* from = dst_ptr - copy_off; copy_len > 0; copy_len--) {
      *dst_ptr++ = *from++;
    }
  }

fail_invalid_data:
  result.status_message = sflz4_status_message__error_invalid_data;
  return result;
}

// -------- LZ4 Encode

#define SFLZ4_HASH_TABLE_SHIFT 12

static inline uint32_t  //
sflz4_private_hash(     //
    uint32_t x) {
  // 2654435761u is Knuth's magic constant.
  return (x * 2654435761u) >> (32 - SFLZ4_HASH_TABLE_SHIFT);
}

static inline size_t                  //
sflz4_private_longest_common_prefix(  //
    const uint8_t* p,                 //
    const uint8_t* q,                 //
    const uint8_t* p_limit) {
  const uint8_t* const original_p = p;
  size_t n = p_limit - p;
  while ((n >= 4) &&
         (sflz4_private_peek_u32le(p) == sflz4_private_peek_u32le(q))) {
    p += 4;
    q += 4;
    n -= 4;
  }
  while ((n > 0) && (*p == *q)) {
    p += 1;
    q += 1;
    n -= 1;
  }
  return (size_t)(p - original_p);
}

SFLZ4_MAYBE_STATIC sflz4_size_result    //
sflz4_block_encode_worst_case_dst_len(  //
    size_t src_len) {
  sflz4_size_result result = {0};

  if (src_len > SFLZ4_LZ4_BLOCK_ENCODE_MAX_INCL_SRC_LEN) {
    result.status_message = sflz4_status_message__error_src_is_too_long;
    return result;
  }
  // For the curious, if (src_len <= 0x7E000000) then (value <= 0x7E7E7E8E).
  result.value = src_len + (src_len / 255) + 16;
  return result;
}

SFLZ4_MAYBE_STATIC sflz4_size_result        //
sflz4_block_encode(                         //
    uint8_t* SFLZ4_RESTRICT dst_ptr,        //
    size_t dst_len,                         //
    const uint8_t* SFLZ4_RESTRICT src_ptr,  //
    size_t src_len) {
  sflz4_size_result result = sflz4_block_encode_worst_case_dst_len(src_len);
  if (result.status_message) {
    return result;
  } else if (result.value > dst_len) {
    result.status_message = sflz4_status_message__error_dst_is_too_short;
    result.value = 0;
    return result;
  }
  result.value = 0;

  uint8_t* dp = dst_ptr;
  const uint8_t* sp = src_ptr;
  const uint8_t* literal_start = src_ptr;

  // See https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md for "The
  // last match must start at least 12 bytes before the end of block" and other
  // file format details, such as the LZ4 token's bit patterns.
  if (src_len > 12) {
    const uint8_t* const match_limit = src_ptr + src_len - 5;
    const size_t final_literals_limit = src_len - 11;

    // hash_table maps from SFLZ4_HASH_TABLE_SHIFT-bit keys to 32-bit values.
    // Each value is an offset o, relative to src_ptr, initialized to zero.
    // Each key, when set, is a hash of 4 bytes src_ptr[o .. o+4].
    uint32_t hash_table[1 << SFLZ4_HASH_TABLE_SHIFT] = {0};

    while (1) {
      // Start with 1-byte steps, accelerating when not finding any matches
      // (e.g. when compressing binary data, not text data).
      size_t step = 1;
      size_t step_counter = 1 << 6;

      // Start with a non-empty literal.
      const uint8_t* next_sp = sp + 1;
      uint32_t next_hash =
          sflz4_private_hash(sflz4_private_peek_u32le(next_sp));

      // Find a match or goto final_literals.
      const uint8_t* match = NULL;
      do {
        sp = next_sp;
        next_sp += step;
        step = step_counter++ >> 6;
        if (((size_t)(next_sp - src_ptr)) > final_literals_limit) {
          goto final_literals;
        }
        uint32_t* hash_table_entry = &hash_table[next_hash];
        match = src_ptr + *hash_table_entry;
        next_hash = sflz4_private_hash(sflz4_private_peek_u32le(next_sp));
        *hash_table_entry = (uint32_t)(sp - src_ptr);
      } while (((sp - match) > 0xFFFF) || (sflz4_private_peek_u32le(sp) !=
                                           sflz4_private_peek_u32le(match)));

      // Extend the match backwards.
      while ((sp > literal_start) && (match > src_ptr) &&
             (sp[-1] == match[-1])) {
        sp--;
        match--;
      }

      // Emit half of the LZ4 token, encoding the literal length. We'll fix up
      // the other half later.
      uint8_t* token = dp;
      size_t literal_len = (size_t)(sp - literal_start);
      if (literal_len < 15) {
        *dp++ = (uint8_t)(literal_len << 4);
      } else {
        size_t n = literal_len - 15;
        *dp++ = 0xF0;
        for (; n >= 255; n -= 255) {
          *dp++ = 0xFF;
        }
        *dp++ = (uint8_t)n;
      }
      memcpy(dp, literal_start, literal_len);
      dp += literal_len;

      while (1) {
        // At this point:
        //  - sp    points to the start of the match's later   copy.
        //  - match points to the start of the match's earlier copy.
        //  - token points to the LZ4 token.

        // Calculate the match length and update the token's other half.
        size_t copy_off = (size_t)(sp - match);
        *dp++ = (uint8_t)(copy_off >> 0);
        *dp++ = (uint8_t)(copy_off >> 8);
        size_t adj_copy_len =
            sflz4_private_longest_common_prefix(4 + sp, 4 + match, match_limit);
        if (adj_copy_len < 15) {
          *token |= (uint8_t)adj_copy_len;
        } else {
          size_t n = adj_copy_len - 15;
          *token |= 0x0F;
          for (; n >= 255; n -= 255) {
            *dp++ = 0xFF;
          }
          *dp++ = (uint8_t)n;
        }
        sp += 4 + adj_copy_len;

        // Update the literal_start and check the final_literals_limit.
        literal_start = sp;
        if (((size_t)(sp - src_ptr)) >= final_literals_limit) {
          goto final_literals;
        }

        // We've skipped over hashing everything within the match. Also, the
        // minimum match length is 4. Update the hash table for one of those
        // skipped positions.
        hash_table[sflz4_private_hash(sflz4_private_peek_u32le(sp - 2))] =
            (uint32_t)(sp - 2 - src_ptr);

        // Check if this match can be followed immediately by another match.
        // If so, continue the loop. Otherwise, break.
        uint32_t* hash_table_entry =
            &hash_table[sflz4_private_hash(sflz4_private_peek_u32le(sp))];
        uint32_t old_offset = *hash_table_entry;
        uint32_t new_offset = (uint32_t)(sp - src_ptr);
        *hash_table_entry = new_offset;
        match = src_ptr + old_offset;
        if (((new_offset - old_offset) > 0xFFFF) ||
            (sflz4_private_peek_u32le(sp) != sflz4_private_peek_u32le(match))) {
          break;
        }
        token = dp++;
        *token = 0;
      }
    }
  }

final_literals:
  do {
    size_t final_literal_len = src_len - (size_t)(literal_start - src_ptr);
    if (final_literal_len < 15) {
      *dp++ = (uint8_t)(final_literal_len << 4);
    } else {
      size_t n = final_literal_len - 15;
      *dp++ = 0xF0;
      for (; n >= 255; n -= 255) {
        *dp++ = 0xFF;
      }
      *dp++ = (uint8_t)n;
    }
    memcpy(dp, literal_start, final_literal_len);
    dp += final_literal_len;
  } while (0);

  result.value = (size_t)(dp - dst_ptr);
  return result;
}

// -------- Private Macros

#undef SFLZ4_HASH_TABLE_SHIFT
#undef SFLZ4_USE_MEMCPY_LE_PEEK_POKE

// ================================ -Private Implementation

#endif  // SFLZ4_IMPLEMENTATION

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SFLZ4_INCLUDE_GUARD
