#include "../trees/ll.h"
#include "../trees/trees.h"

struct Multicast {
  int epoch;
  struct List *users; // ll for now... should be hashmap maybe??? users in group at certain time (most recently added in front)
  void *tree;
  int *counts; //0th index is PRGs (from children in skeleton), 1st index is encs (that the grp mgr performs)
  int tree_type; //0 for lbbt, 1 for btree
};

struct Multicast *mult_init(int n, int *tree_flags, int tree_type);

struct Node *mult_add(struct Multicast *multicast, int id); //TODO: id is for new user

int mult_update(struct Multicast *multicast, struct Node *user);

void *mult_rem(struct Multicast *multicast, struct Node *user);

void mult_destroy(struct Multicast *multicast);
