#include <botan/ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  botan_rng_t auto_rng;
  botan_rng_init(&auto_rng, "user");
  uint8_t *seed = malloc(sizeof(uint8_t) * 32);
  botan_rng_get(auto_rng, seed, 32);
  printf("%s\n\n", (char *) seed);
  uint8_t *iv = malloc(sizeof(uint8_t) * 32);
  botan_rng_get(auto_rng, iv, 32);
  printf("%s\n\n", (char *) iv);  

  uint8_t out[100];
  botan_mac_t prf;
  botan_mac_init(&prf, "HMAC(SHA-512)", 0);
  botan_mac_set_key(prf, seed, 64); //(512 bits)
  size_t out_size;
  botan_mac_output_length(prf, &out_size);
  printf("size: %zu\n", out_size);
  uint8_t test_bytes = 0;
  botan_mac_update(prf, &test_bytes, sizeof(test_bytes));
  botan_mac_final(prf, out);
  printf("OUT\n");
  printf("%s\n\n", (char *) out);
  test_bytes++;
  botan_mac_update(prf, &test_bytes, sizeof(test_bytes));
  botan_mac_final(prf, out);
  printf("OUT\n");
  printf("%s\n\n", (char *) out);
  test_bytes++;
  botan_mac_update(prf, &test_bytes, sizeof(test_bytes));
  botan_mac_final(prf, out);
  printf("OUT\n");
  printf("%s\n\n", (char *) out);  
  
  botan_cipher_t enc_cipher;
  botan_cipher_init(&enc_cipher, "ChaCha(20)", BOTAN_CIPHER_INIT_FLAG_ENCRYPT);
  botan_cipher_set_key(enc_cipher, seed, 32);
  botan_cipher_start(enc_cipher, iv, 32);

  uint8_t enc[100];
  size_t *written = malloc(sizeof(size_t));
  size_t *consumed = malloc(sizeof(size_t));
  botan_cipher_update(enc_cipher, 0, enc, 100, written, out, sizeof(out), consumed);

  printf("%s\n\n", (char *) enc);

  botan_cipher_t dec_cipher;
  botan_cipher_init(&dec_cipher, "ChaCha(20)", BOTAN_CIPHER_INIT_FLAG_DECRYPT);
  botan_cipher_set_key(dec_cipher, seed, 32);
  botan_cipher_start(dec_cipher, iv, 32);  

  //uint8_t *bytes = (uint8_t *) pltxt;
  uint8_t dec[100];
  //size_t *written = malloc(sizeof(size_t));
  //size_t *consumed = malloc(sizeof(size_t));
  botan_cipher_update(dec_cipher, 0, dec, 100, written, enc, sizeof(enc), consumed);

  printf("%s\n\n", (char *) dec);
  
  return 0;
}
