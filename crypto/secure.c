#include <botan/ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int crypto_split(uint8_t *out, uint8_t *seed, uint8_t *key, uint8_t *nonce, uint8_t *next_seed, size_t seed_size, size_t key_size, size_t nonce_size) {
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *nonce_bytes = (uint8_t *) nonce;
  uint8_t *next_seed_bytes = (uint8_t *) next_seed;
  memcpy(seed_bytes, out_bytes, seed_size);
  memcpy(key_bytes, out_bytes + seed_size, key_size);
  memcpy(nonce_bytes, out_bytes + seed_size + key_size, nonce_size);
  memcpy(next_seed_bytes, out_bytes + seed_size + key_size + nonce_size, seed_size);
  return 0;
}

int sampler_init(void **sampler) {
  *sampler = malloc(sizeof(botan_rng_t));
  botan_rng_t *rng = *((botan_rng_t **) sampler);
  return botan_rng_init(rng, "user");
}

int prg_init(void **prg) {
  *prg = malloc(sizeof(botan_mac_t));
  botan_mac_t *prf = *((botan_mac_t **) prg);
  return botan_mac_init(prf, "HMAC(SHA-512)", 0);
}

int cipher_init(void **cipher, int mode) { //0 for enc mode, 1 for dec mode
  *cipher = malloc(sizeof(botan_cipher_t));
  botan_cipher_t *botan_cipher = *((botan_cipher_t **) cipher);
  if (mode == 0)
    return botan_cipher_init(botan_cipher, "ChaCha(20)", BOTAN_CIPHER_INIT_FLAG_ENCRYPT);
  else
    return botan_cipher_init(botan_cipher, "ChaCha(20)", BOTAN_CIPHER_INIT_FLAG_DECRYPT);
}

int sample(void *sampler, void *seed) {
  return botan_rng_get(*((botan_rng_t *) sampler), (uint8_t *) seed, 64); //TODO: fix hardcode
}

int prg(void *generator, void *seed, void *out) {
  botan_mac_t *prf = (botan_mac_t *) generator;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t in_bytes = 0;
  botan_mac_set_key(*prf, seed_bytes, 64); //TODO: fix hardcode
  botan_mac_update(*prf, &in_bytes, 1);
  botan_mac_final(*prf, out_bytes);
  in_bytes++;
  botan_mac_update(*prf, &in_bytes, 1);
  botan_mac_final(*prf, out_bytes + 64);
  in_bytes++;
  botan_mac_update(*prf, &in_bytes, 1);
  return botan_mac_final(*prf, out_bytes + 128);  
}

int get_prg_out_size(void *generator, size_t *size) {
  botan_mac_t *prf = (botan_mac_t *) generator;
  botan_mac_output_length(*prf, size);
  printf("got prg out size\n");
  *size *= 3;
  return 0; //TODO Fix
}

int get_key_size(void *cipher, size_t *size) {
  *size = 32; //TODO: Fix hardocde
  return 0;
}

int get_nonce_size(void *cipher, size_t *size) {
  *size = 8;
  return 0;
}

int get_seed_size(void *generator, size_t *size) {
  *size = 64;
  return 0;
}

int get_ct_size(void *cipher, size_t in_size, size_t *ct_size) {
  botan_cipher_t *botan_cipher = (botan_cipher_t *) cipher;
  return botan_cipher_output_length(*botan_cipher, in_size, ct_size);
}

int enc(void *cipher, void *generator, void *key, void *seed, void *nonce, void *pltxt, void *ctxt, size_t pltxt_size, size_t ctxt_size) {
  botan_cipher_t *botan_cipher = (botan_cipher_t *) cipher;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *nonce_bytes = (uint8_t *) nonce;
  uint8_t *pltxt_bytes = (uint8_t *) pltxt;
  uint8_t *ctxt_bytes = (uint8_t *) ctxt;
  size_t num_written;
  size_t num_consumed;

  botan_cipher_set_key(*botan_cipher, key_bytes, 32);
  botan_cipher_start(*botan_cipher, nonce_bytes, 8);
  botan_cipher_update(*botan_cipher, 0, ctxt_bytes, ctxt_size, &num_written, pltxt_bytes, pltxt_size, &num_consumed);

  printf("consumed: %zu, written: %zu\n", num_consumed, num_written);

  size_t key_size, seed_size, nonce_size, out_size;
  get_prg_out_size(generator, &out_size);
  get_key_size(cipher, &key_size);
  get_nonce_size(cipher, &nonce_size);
  get_seed_size(generator, &seed_size);
  printf("got sizes\n");
  uint8_t *next_seed = malloc(seed_size);
  if (next_seed == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  uint8_t *out = malloc(out_size);
  if (out == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  prg(generator, seed, out);
  crypto_split(out, seed, key, nonce, next_seed, seed_size, key_size, nonce_size);

  return num_written;
}

int dec(void *cipher, void *generator, void *key, void *seed, void *nonce, void *ctxt, void *pltxt, size_t ctxt_size, size_t pltxt_size) {
  botan_cipher_t *botan_cipher = (botan_cipher_t *) cipher;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *nonce_bytes = (uint8_t *) nonce;
  uint8_t *pltxt_bytes = (uint8_t *) pltxt;
  uint8_t *ctxt_bytes = (uint8_t *) ctxt;
  size_t num_written;
  size_t num_consumed;

  botan_cipher_set_key(*botan_cipher, key_bytes, 32);
  botan_cipher_start(*botan_cipher, nonce_bytes, 8);
  botan_cipher_update(*botan_cipher, 0, pltxt_bytes, pltxt_size, &num_written, ctxt_bytes, ctxt_size, &num_consumed);

  printf("consumed: %zu, written: %zu\n", num_consumed, num_written);

  size_t key_size, seed_size, nonce_size, out_size;
  get_prg_out_size(generator, &out_size);
  get_key_size(cipher, &key_size);
  get_nonce_size(cipher, &nonce_size);
  get_seed_size(generator, &seed_size);
  printf("got sizes\n");
  uint8_t *next_seed = malloc(seed_size);
  if (next_seed == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  uint8_t *out = malloc(out_size);
  if (out == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  prg(generator, seed, out);
  crypto_split(out, seed, key, nonce, next_seed, seed_size, key_size, nonce_size);  
  
  return num_written;
}
