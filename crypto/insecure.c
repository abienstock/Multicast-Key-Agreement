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
  int *seed_bytes = (int *) seed;
  *seed_bytes = rand();
  return 0;
}

int prg(void *generator, void *seed, void *out) {
  int *seed_bytes = (int *) seed;
  int *out_bytes = (int *) out;
  memcpy(out_bytes, seed_bytes, sizeof(int));
  *((int *) seed)+=1;
  memcpy(out_bytes + 1, seed_bytes, sizeof(int));
  *((int *) seed)+=1;  
  memcpy(out_bytes + 2, seed_bytes, sizeof(int));
  *((int *) seed)-=2;
  return 0;
}

void split(void *out, void *seed, void *key, void *next_seed, size_t seed_size) {
  int *out_bytes = (int *) out;
  int *seed_bytes = (int *) seed;
  int *key_bytes = (int *) key;
  int *next_seed_bytes = (int *) next_seed;
  memcpy(seed_bytes, out_bytes, seed_size);
  memcpy(key_bytes, out_bytes + 1, seed_size);
  memcpy(next_seed_bytes, out_bytes + 2, seed_size);
}

void get_prg_out_size(void *generator, size_t *size) { // in bytes
  *size = sizeof(int) * 3;
}

void get_seed_size(void *generator, size_t *size) { // in bytes
  *size = sizeof(int);
}

int enc(void *generator, void *key, void *seed, void *pltxt, void *ctxt, size_t pltxt_size) {
  memcpy(ctxt, pltxt, sizeof(int));
  return 0;
}

int dec(void *generator, void *key, void *seed, void *ctxt, void *pltxt, size_t ctxt_size) {
  memcpy(pltxt, ctxt, sizeof(int));
  return 0;
}
