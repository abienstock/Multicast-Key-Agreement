#ifndef _TREES_H_
#define _TREES_H_

#include "../../ll/ll.h"
#include "../../skeleton.h"

struct LBBTNodeData {
  struct ListNode *rightmost_blank;
};


struct BTreeNodeData {
  int lowest_nonfull; // lowest level in subtree in which there is a nonfull node
  int opt_add_child; // 0-indexed child which has lowest nonfull node
  int height; // height of node in tree
};
  
enum LLRBTreeColor {BLACK, RED};
struct LLRBTreeNodeData {
  enum LLRBTreeColor colorL, colorR;
  int num_optimal; // for greedy strategy
  int heightBlack; // only for correctness check
};

struct NodeData {
  int id;
  int blank; // 0 = nonblank, 1 = blank. only leaves for MKA, all nodes for TK
  struct List *tk_unmerged;  
  void *key;
  void *seed;
  void *tree_node_data;
};

struct Node { 
  struct Node **children;
  int num_children;
  struct Node *parent;
  void *data;
  int num_leaves; // including blanks for LBBT
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
  int order; // max # of children
  int add_strat;
};

struct LLRBTree {
  struct Node *root;
  int add_strat;
  int mode_order;
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
  int id;
  struct SkeletonNode *skeleton;
};

void traverse_tree(struct Node *root, void (*f)(void *));
void pretty_traverse_tree(void *tree, struct Node *root, int space, void (*f)(void *));
void pretty_traverse_skeleton(struct SkeletonNode *root, int space, void (*f)(void *));

//DESTROYS DATA TOO
void free_node(struct Node *node);
void free_tree(struct Node *root);
void free_skeleton(struct SkeletonNode *root, int is_root, int crypto);

// needs to add created leaf nodes to users (from left to right?)
struct InitRet lbbt_init(int *ids, int n, int add_strat, int trunc_strat, struct List *users);

struct AddRet lbbt_add(void *tree, int id);

struct RemRet lbbt_rem(void *tree, struct Node *node);

// needs to add created leaf nodes to users (from left to right?)
struct InitRet btree_init(int *ids, int n, int add_strat, int order, struct List *users);

struct AddRet btree_add(void *tree, int id);

struct RemRet btree_rem(void *tree, struct Node *node);

// LLRBTree

static const int LLRBTree_STRAT_GREEDY = 0;
static const int LLRBTree_STRAT_RANDOM = 1;

static const int LLRBTree_MODE_23 = 3;
static const int LLRBTree_MODE_234 = 4;

struct InitRet LLRBTree_init(int *ids, int n, int add_strat, int mode_order, struct List *users);

struct AddRet LLRBTree_add(void *tree, int id);

struct RemRet LLRBTree_rem(void *tree, struct Node *node);

#endif /* #ifndef _TREES_H_ */
