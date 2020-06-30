#include <botan/ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils.h"

int sampler_init(void **sampler) {
  *sampler = malloc_check(sizeof(botan_rng_t));
  botan_rng_t *rng = *((botan_rng_t **) sampler);
  return botan_rng_init(rng, "user");
}

int prg_init(void **prg) {
  *prg = malloc_check(sizeof(botan_mac_t));
  botan_mac_t *prf = *((botan_mac_t **) prg);
  return botan_mac_init(prf, "HMAC(SHA-512)", 0); //512 bit (64 byte) seeds
}

int sample(void *sampler, void *seed) {
  return botan_rng_get(*((botan_rng_t *) sampler), (uint8_t *) seed, 64); //TODO: fix hardcode
}

//TODO: make sure this is correct
int prg(void *generator, void *seed, void *out) {
  botan_mac_t *prf = (botan_mac_t *) generator;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t in_bytes = 0;
  int success = 0;
  success += botan_mac_set_key(*prf, seed_bytes, 64); //TODO: fix hardcode ... 512 bit output
  success += botan_mac_update(*prf, &in_bytes, 1);
  success += botan_mac_final(*prf, out_bytes);
  in_bytes++;
  success += botan_mac_update(*prf, &in_bytes, 1);
  success += botan_mac_final(*prf, out_bytes + 64);
  in_bytes++;
  success += botan_mac_update(*prf, &in_bytes, 1);
  return success += botan_mac_final(*prf, out_bytes + 128);  
}

void split(void *out, void *seed, void *key, void *next_seed, size_t seed_size) {
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *next_seed_bytes = (uint8_t *) next_seed;
  memcpy(seed_bytes, out_bytes, seed_size);
  memcpy(key_bytes, out_bytes + seed_size, seed_size);
  memcpy(next_seed_bytes, out_bytes + 2 * seed_size, seed_size);
}

void get_prg_out_size(void *generator, size_t *size) {
  botan_mac_t *prf = (botan_mac_t *) generator;
  botan_mac_output_length(*prf, size);
  *size *= 3;
}

void get_seed_size(void *generator, size_t *size) {
  *size = 64;
}

int enc(void *generator, void *key, void *seed, void *pltxt, void *ctxt, size_t pltxt_size) {
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *pltxt_bytes = (uint8_t *) pltxt;
  uint8_t *ctxt_bytes = (uint8_t *) ctxt;
  int success = 0;
  int i;
  for (i = 0; i < pltxt_size; i ++)
    *(ctxt_bytes + i) = *(pltxt_bytes + i) ^ *(key_bytes + i);

  //one-time pad refresh
  size_t out_size, seed_size;
  get_prg_out_size(generator, &out_size);
  get_seed_size(generator, &seed_size);  
  uint8_t next_seed[seed_size];
  uint8_t *out = malloc_check(out_size);
  success += prg(generator, seed, out);
  split(out, seed, key, next_seed, seed_size);
  free(out);
  return success;
}

int dec(void *generator, void *key, void *seed, void *ctxt, void *pltxt, size_t ctxt_size) {
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *pltxt_bytes = (uint8_t *) pltxt;
  uint8_t *ctxt_bytes = (uint8_t *) ctxt;
  int success = 0;
  int i;
  for (i = 0; i < ctxt_size; i ++)
    *(pltxt_bytes + i) = *(ctxt_bytes + i) ^ *(key_bytes + i);

  //one-time pad refresh  
  size_t out_size, seed_size;
  get_prg_out_size(generator, &out_size);
  get_seed_size(generator, &seed_size);  
  uint8_t next_seed[seed_size];
  uint8_t *out = malloc_check(out_size);
  success += prg(generator, seed, out);
  split(out, seed, key, next_seed, seed_size);
  free(out);
  return success;
}

int free_sampler(void *sampler) {
  botan_rng_t *rng = (botan_rng_t *) sampler;
  int ret = botan_rng_destroy(*rng);
  free(sampler);
  return ret;
}

int free_prg(void *prg) {
  botan_mac_t *prf = (botan_mac_t *) prg;
  int ret = botan_mac_destroy(*prf);
  free(prg);
  return ret;
}
