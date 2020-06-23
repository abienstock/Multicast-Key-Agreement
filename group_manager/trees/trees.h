#ifndef _TREES_H_
#define _TREES_H_

#include "../../ll/ll.h"
#include "../../skeleton.h"

struct NodeData {
  int id;
  void *key;
  void *seed;
  int blank; // 0 = notblank, 1 = blank
};

struct Node {
  struct Node **children;
  struct Node *parent;
  struct ListNode *rightmost_blank;
  void *data;
  int num_leaves; // including blanks
  //int h;
};

struct LBBT {
  struct Node *root;
  int add_strat;
  int trunc_strat;
  struct List *blanks;
  struct Node *rightmost_leaf;
};

struct BTree {
  struct Node *root;
  int order;
  int add_strat;
  struct List *leaves; // TODO: should this be ll?
};

struct InitRet {
  void *tree;
  struct SkeletonNode *skeleton;
};

struct AddRet {
  struct Node *added;
  struct SkeletonNode *skeleton;
};

struct RemRet {
  void *data;
  struct SkeletonNode *skeleton;
};

struct Node **get_path(struct Node *node);

struct Node **get_copath(struct Node *node);

void traverse_tree(struct Node *root, void (*f)(void *));

void pretty_traverse_tree(struct Node *root, int space, void (*f)(void *));
void pretty_traverse_skeleton(struct SkeletonNode *root, int space, void (*f)(void *));

//DESTROYS ALL DATA TOO
void destroy_tree(struct Node *root);

void destroy_skeleton(struct SkeletonNode *root);

struct InitRet lbbt_init(int *ids, int n, int add_strat, int trunc_strat, struct List *users);

struct AddRet lbbt_add(void *tree, int id);

struct RemRet lbbt_rem(void *tree, struct Node *node);

struct InitRet btree_init(int n);

struct AddRet btree_add(void *tree, int id);

struct RemRet btree_rem(void *tree, struct Node *node);

#endif /* #ifndef _TREES_H_ */
