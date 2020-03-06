#include "../trees/ll.h"

struct Multicast {
  int epoch;
  struct List *users; // ll for now... should be hashmap maybe??? users in group at certain time (most recently added in front)
  void *tree;
  int *counts;
  int tree_type; //0 for lbbt, 1 for btree
};

struct Node *mult_add(struct Multicast *multicast, void *data); //TODO: data is for new user

void mult_update(struct Multicast *mluticast, struct Node *user);

void mult_rem(struct Muticast *multicast, struct Node *user);
