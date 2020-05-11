#include "../ll/ll.h"
#include "../skeleton.h"

struct User {
  int id;
  struct List *secrets;
};

struct PathData {
  int node_id;
  uint8_t *key;
  uint8_t *nonce;
  uint8_t *seed;
};

//id -- 0: create, otherwise the id of person being updated/removed/added
void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, uint8_t *oob_seed, void *generator, void *cipher, size_t seed_size); //returns group secret

struct User *init_user(int id);

void destroy_users(struct List *users);

void destroy_user(struct User *user);
