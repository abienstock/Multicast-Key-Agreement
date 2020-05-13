int sampler_init(void *sampler);

int prg_init(void *prg);

int cipher_init(void *cipher, int mode); // 0 for enc mode, 1 for dec mode

int sample(void *sampler, void *seed);

int prg(void *generator, void *seed, void *out);

int get_prg_out_size(void *generator, size_t *size); // in bytes

int get_key_size(void *cipher, size_t *size); // in bytes

int get_nonce_size(void *cipher, size_t *size); // in bytes

int get_seed_size(void *generator, size_t *size); // in bytes

int get_ct_size(void *cipher, size_t in_size, size_t *ct_size);

int enc(void *cipher, void *generator, void *key, void *seed, void *nonce, void *pltxt, void *ctxt, size_t pltxt_size, size_t ct_size);

int dec(void *cipher, void *generator, void *key, void *seed, void *nonce, void *ctxt, void *pltxt, size_t ctxt_size, size_t pltxt_size);
