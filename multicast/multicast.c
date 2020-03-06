#include <stdio.h>
#include <stdlib.h>
#include "../trees/trees.h"
#include "multicast.h"

//void *multicast_init(int tree_type, void *tree_flags, int n, int distrib, float add_wt, )

struct Node *mult_add(struct Multicast *multicast, void *data) {
  struct Node *added;
  if (multicast->tree_type == 0)
    added = gen_tree_add(multicast->tree, data, &lbbt_add);
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);
  return added;
}

void mult_update(struct Multicast *multicast, struct Node *user) { //TODO: user should be node???
  gen_tree_upd(multicast->tree, user);
}

void mult_rem(struct Multicast *multicast, struct Node *user) { //TODO: user should be node??
    if (multicast->tree_type == 0)
      gen_tree_rem(multicast->tree, user, &lbbt_rem);
    //    else
    //      gen_tree_rem(multicast->tree, user, &btree_rem);
}
