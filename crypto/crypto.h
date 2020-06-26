#include <stdlib.h>

int sampler_init(void *sampler);

int prg_init(void *prg);

int sample(void *sampler, void *seed);

void alloc_prg_out(void **out, void **seed, void **key, void **next_seed, size_t prg_out_size, size_t seed_size);

int prg(void *generator, void *seed, void *out);

void split(void *out, void *seed, void *key, void *next_seed, size_t seed_size);

void get_prg_out_size(void *generator, size_t *size); // in bytes

void get_seed_size(void *generator, size_t *size); // in bytes

int enc(void *generator, void *key, void *seed, void *pltxt, void *ctxt, size_t pltxt_size);

int dec(void *generator, void *key, void *seed, void *ctxt, void *pltxt, size_t ctxt_size);
