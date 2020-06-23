#include "../ll/ll.h"
#include "../skeleton.h"

struct User {
  int id;
  struct List *secrets;
  int in_group;
};

struct PathData {
  int node_id;
  uint8_t *key;
  uint8_t *seed;
};

//id -- 0: create, otherwise the id of person being updated/removed/added
void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, uint8_t *oob_seed, void *generator, size_t seed_size); //returns group secret

void proc_broadcast(struct User *user, uint8_t **buf, void *generator, size_t seed_size);

struct User *init_user(int id);

void destroy_users(struct List *users);

void destroy_user(struct User *user);