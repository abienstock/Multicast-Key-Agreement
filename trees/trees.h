#include "ll.h"

struct Node {
  struct Node **children;
  struct Node *parent;
  struct ListNode *rightmost_blank;
  void *data;
  int num_leaves;
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

struct SkeletonNode {
  struct Node *node;
  int *children_color; // the color of the edge between the node and its children -- 0 = red, 1 = blue
  struct SkeletonNode **children; // children in the skeleton (possibly one or both NULL)
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

struct UpdRet {
  struct SkeletonNode *skeleton;
};

struct Node **get_path(struct Node *node);

struct Node **get_copath(struct Node *node);

struct InitRet gen_tree_init(void **ids, int n, int add_strat, int trunc_strat, struct List *users, struct InitRet (*tree_init)(void **, int, int, int, struct List *));

struct AddRet gen_tree_add(void *tree, void *data, struct AddRet (*tree_add)(void *, void *));

struct RemRet gen_tree_rem(void *tree, struct Node *node, struct RemRet (*tree_rem)(void *, struct Node *));

struct UpdRet gen_tree_upd(void *tree, struct Node *node); //, void (*tree_upd)(void *, struct Node *));

void traverse_tree(struct Node *root, void (*f)(void *));

void pretty_traverse_tree(struct Node *root, int space, void (*f)(void *));
void pretty_traverse_skeleton(struct SkeletonNode *root, int space, void (*f)(void *));

//DESTROYS ALL DATA TOO
void destroy_tree(struct Node *root);

void destroy_skeleton(struct SkeletonNode *root);

struct InitRet lbbt_init(void **ids, int n, int add_strat, int trunc_strat, struct List *users);

struct AddRet lbbt_add(void *tree, void *data);

struct RemRet lbbt_rem(void *tree, struct Node *node);

//void lbbt_upd(void *tree, struct Node *node);

struct InitRet btree_init(int n);

struct AddRet btree_add(void *tree, void *data);

struct RemRet btree_rem(void *tree, struct Node *node);

//void btree_upd(void *tree, struct Node *node);
