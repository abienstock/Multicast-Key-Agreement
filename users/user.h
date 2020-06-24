#include "../ll/ll.h"
#include "../skeleton.h"

struct User {
  int id;
  struct List *secrets;
  int in_group;
  size_t prg_out_size;
  size_t seed_size;
};

struct PathData {
  int node_id;
  void *key;
  void *seed;
};

//id -- 0: create, otherwise the id of person being updated/removed/added
void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, void *oob_seed, void *generator); //returns group secret

int proc_broadcast(struct User *user, void **buf, void *generator);

struct User *init_user(int id, size_t prg_out_size, size_t seed_size);

void destroy_users(struct List *users);

void destroy_user(struct User *user);
