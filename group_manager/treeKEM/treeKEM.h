#ifndef _MULTICAST_H_
#define _MULTICAST_H_

#include "../../ll/ll.h"
#include "../trees/trees.h"

struct treeKEM {
  struct List *users; // ll for now... should be hashmap maybe??? users in group at certain time (most recently added in front)
  void *tree;
  int *counts; //0th index is PRGs (from children in skeleton), 1st index is encs (that the grp mgr performs)
  int tree_type; //0 for lbbt, 1 for btree
  int crypto; //0 for no crypto, 1 for crypto
  size_t prg_out_size;
  size_t seed_size;
};

struct treeKEMInitRet {
  struct treeKEM *treeKEM;
  struct SkeletonNode *skeleton;
  struct List *user_seeds;
};

struct treeKEMAddRet {
  struct Node *added;
  void *seed;
};

struct treeKEMUpdRet {
  int id;
  void *seed;
};

struct treeKEMCommitRet {
  int committer_id;
  struct SkeletonNode *skeleton;
  void *seed;
};

struct treeKEMInitRet treeKEM_init(int n, int crypto, int *tree_flags, int tree_type, void *sampler, void *prg);

struct treeKEMAddRet treeKEM_add(struct treeKEM *treeKEM, int id, void *sampler, void *generator); // id is for new user

struct treeKEMUpdRet treeKEM_update(struct treeKEM *treeKEM, int user, void *sampler, void *generator);

int treeKEM_rem(struct treeKEM *treeKEM, int user);

struct treeKEMCommitRet treeKEM_commit(struct treeKEM *treeKEM, int committer, void *sampler, void *generator);

void free_treeKEM(struct treeKEM *treeKEM);

#endif /* #ifndef _MULTICAST_H_ */
