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

// ----

// Expected output:
//
// $ gcc example.c && ./a.out
// Encoded 158 bytes as 114 bytes:
//     0xF1, 0x01, 0x53, 0x68, 0x65, 0x20, 0x73, 0x65,
//     0x6C, 0x6C, 0x73, 0x20, 0x73, 0x65, 0x61, 0x20,
//     0x73, 0x68, 0x0B, 0x00, 0x41, 0x62, 0x79, 0x20,
//     0x74, 0x18, 0x00, 0x00, 0x12, 0x00, 0x60, 0x6F,
//     0x72, 0x65, 0x2E, 0x0A, 0x54, 0x0F, 0x00, 0x02,
//     0x1D, 0x00, 0x10, 0x73, 0x0B, 0x00, 0x01, 0x27,
//     0x00, 0xA0, 0x61, 0x72, 0x65, 0x20, 0x73, 0x75,
//     0x72, 0x65, 0x6C, 0x79, 0x3D, 0x00, 0x02, 0x3C,
//     0x00, 0x70, 0x2E, 0x0A, 0x53, 0x6F, 0x20, 0x69,
//     0x66, 0x2D, 0x00, 0x03, 0x26, 0x00, 0x02, 0x18,
//     0x00, 0x34, 0x20, 0x6F, 0x6E, 0x54, 0x00, 0x01,
//     0x53, 0x00, 0x51, 0x2C, 0x0A, 0x49, 0x27, 0x6D,
//     0x3E, 0x00, 0x08, 0x2B, 0x00, 0x03, 0x1D, 0x00,
//     0x90, 0x20, 0x73, 0x68, 0x65, 0x6C, 0x6C, 0x73,
//     0x2E, 0x0A,
//
// Decoded 114 bytes as 158 bytes:
// She sells sea shells by the sea shore.
// The shells she sells are surely seashells.
// So if she sells shells on the seashore,
// I'm sure she sells seashore shells.

#include <stdio.h>
#include <string.h>

#define SFLZ4_IMPLEMENTATION
#include "src/sflz4.h"

const char* ssss_ptr =
    "She sells sea shells by the sea shore.\n"
    "The shells she sells are surely seashells.\n"
    "So if she sells shells on the seashore,\n"
    "I'm sure she sells seashore shells.\n";

#define ENC_BUFFER_SIZE 1024
uint8_t enc_buffer[ENC_BUFFER_SIZE];

#define DEC_BUFFER_SIZE 1024
uint8_t dec_buffer[DEC_BUFFER_SIZE];

int            //
main(          //
    int argc,  //
    char** argv) {
  const size_t ssss_len = strlen(ssss_ptr);
  sflz4_size_result res;

  res = sflz4_block_encode_worst_case_dst_len(ssss_len);
  if (res.status_message) {
    fprintf(stderr, "sflz4_block_encode_worst_case_dst_len failed: %s\n",
            res.status_message);
    return 1;
  } else if (res.value > ENC_BUFFER_SIZE) {
    fprintf(stderr, "input is too long\n");
    return 1;
  }

  res = sflz4_block_encode(enc_buffer, ENC_BUFFER_SIZE,
                           (const uint8_t*)ssss_ptr, ssss_len);
  if (res.status_message) {
    fprintf(stderr, "sflz4_block_encode failed: %s\n", res.status_message);
    return 1;
  }
  const size_t enc_len = res.value;

  printf("Encoded %zu bytes as %zu bytes:\n", ssss_len, enc_len);
  for (size_t i = 0; i < enc_len; i++) {
    size_t column = i & 7;
    printf("%s0x%02X,%s",                 //
           (column == 0) ? "    " : " ",  //
           enc_buffer[i],                 //
           ((column == 7) || ((i + 1) == enc_len)) ? "\n" : "");
  }

  res = sflz4_block_decode(dec_buffer, DEC_BUFFER_SIZE, enc_buffer, enc_len);
  if (res.status_message) {
    fprintf(stderr, "sflz4_block_decode failed: %s\n", res.status_message);
    return 1;
  }
  const size_t dec_len = res.value;

  printf("\nDecoded %zu bytes as %zu bytes:\n", enc_len, dec_len);
  for (size_t i = 0; i < dec_len; i++) {
    putchar(dec_buffer[i]);
  }
  return 0;
}
