#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "crypto.h"

int sampler_init(void *sampler) {
  sampler = NULL;
  return 0;
}

int prg_init(void *prg) {
  prg = NULL;
  return 0;
}

int sample(void *sampler, void *seed) {
  uint8_t *seed_bytes = (uint8_t *) seed;
  *seed_bytes = rand();
  *(seed_bytes + 1) = rand();
  *(seed_bytes + 2) = rand();
  *(seed_bytes + 3) = rand();
  return 0;
}

int prg(void *generator, void *seed, void *out) {
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *out_bytes = (uint8_t *) out;
  memcpy(out_bytes, seed_bytes, 4);
  *((int *) seed)+=1;
  memcpy(out_bytes + 4, seed_bytes, 4);
  *((int *) seed)+=1;  
  memcpy(out_bytes + 8, seed_bytes, 4);
  *((int *) seed)+=1;  
  return 0;
}

int split(void *out, void *seed, void *key, void *next_seed, size_t seed_size) {
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *next_seed_bytes = (uint8_t *) next_seed;
  memcpy(seed_bytes, out_bytes, seed_size);
  memcpy(key_bytes, out_bytes + seed_size, seed_size);
  memcpy(next_seed_bytes, out_bytes + 2 * seed_size, seed_size);
  return 0;
}

int get_prg_out_size(void *generator, size_t *size) { // in bytes
  *size = 4 * 3;
  return 0;
}

int get_seed_size(void *generator, size_t *size) { // in bytes
  *size = 4;
  return 0;
}

int enc(void *generator, void *key, void *seed, void *pltxt, void *ctxt, size_t pltxt_size) {
  //*((int *) ctxt) = *((int *) pltxt);
  memcpy(ctxt, pltxt, 4);
  return 0;
}

int dec(void *generator, void *key, void *seed, void *ctxt, void *pltxt, size_t ctxt_size) {
  //*((int *) pltxt) = *((int *) ctxt);
  memcpy(pltxt, ctxt, 4);
  return 0;
}
