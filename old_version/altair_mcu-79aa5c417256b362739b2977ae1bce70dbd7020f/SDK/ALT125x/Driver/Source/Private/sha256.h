/*
 * sha256.h
 */

#ifndef DRV_INC_SHA256_H_
#define DRV_INC_SHA256_H_

#include <string.h>
#include <stdio.h>

#define SHA256_SUM_LEN 32

typedef struct {
  uint32_t total[2];
  uint32_t state[8];
  uint8_t buffer[64];
} sha256_context;

void sha256_starts(sha256_context *ctx);
void sha256_update(sha256_context *ctx, uint8_t *input, uint32_t length);
void sha256_finish(sha256_context *ctx, uint8_t digest[SHA256_SUM_LEN]);
void sha256_memory(uint8_t *start_addr, int data_len, uint8_t *out_addr);
#endif /* DRV_INC_SHA256_H_ */
