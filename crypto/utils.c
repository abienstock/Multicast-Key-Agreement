#include "crypto.h"
#include "../utils.h"

void alloc_prg_out(void *out, void *seed, void *key, void *next_seed, size_t prg_out_size, size_t seed_size) {
  out = malloc_check(prg_out_size);
  seed = malloc_check(seed_size);
  key = malloc_check(seed_size);
  next_seed = malloc_check(seed_size);
}
