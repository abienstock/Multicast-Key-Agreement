#ifndef _MULTICAST_H_
#define _MULTICAST_H_

#include "../../ll/ll.h"
#include "../trees/trees.h"

struct Multicast {
  int epoch;
  struct List *users; // ll for now... should be hashmap maybe??? users in group at certain time (most recently added in front)
  void *tree;
  int *counts; //0th index is PRGs (from children in skeleton), 1st index is encs (that the grp mgr performs)
  int tree_type; //0 for lbbt, 1 for btree
};

struct MultInitRet {
  struct Multicast *multicast;
  struct SkeletonNode *skeleton;
  struct List *oob_seeds;
};

struct MultAddRet {
  struct Node *added;
  struct SkeletonNode *skeleton;
  void *oob_seed;
};

struct MultUpdRet {
  struct SkeletonNode *skeleton;
  void *oob_seed;
};

struct MultInitRet mult_init(int n, int *tree_flags, int tree_type, void *sampler, void *prg, void *cipher);

struct MultAddRet mult_add(struct Multicast *multicast, int id, void *sampler, void *prg, void *cipher); //TODO: id is for new user

struct MultUpdRet mult_update(struct Multicast *multicast, struct Node *user, void *sampler, void *prg, void *cipher);

struct RemRet mult_rem(struct Multicast *multicast, struct Node *user, void *sampler, void *prg, void *cipher);

void mult_destroy(struct Multicast *multicast);

#endif /* #ifndef _MULTICAST_H_ */
