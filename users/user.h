#include "../ll/ll.h"
#include "../skeleton.h"

struct User {
  int id;
  struct List *secrets;
};

struct PathData {
  int node_id;
  void *key;
  void *seed;
};

//id -- 0: create, otherwise the id of person being updated/removed/added
void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, void *oob_seed, void *(*prg)(void *), void **(*split)(void *), void *(*decrypt)(void *, void *)); //returns group secret

struct User *init_user(int id);

void destroy_users(struct List *users);

void destroy_user(struct User *user);
