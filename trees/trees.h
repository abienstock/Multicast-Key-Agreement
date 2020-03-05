#include "ll.h"

struct Node {
  struct Node **children;
  struct Node *parent;
  struct ListNode *rightmost_blank;
  void *data;
  int num_leaves; //TODO: including blanks??
  //int h;
};

struct LBBT {
  struct Node *root;
  int add_strat;
  int trunc_strat;
  struct List *blanks;
  //struct List *nonblanks; // TODO: should this be ll? -- for clients to choose removal??
};

struct BTree {
  struct Node *root;
  int order;
  int add_strat;
  struct List *leaves; // TODO: should this be ll?
};

struct Node **get_path(struct Node *node);

struct Node **get_copath(struct Node *node);

struct Node *init(void **ids, int n, void *(*treeInit)(int));

struct Node *add(void *tree, void *data, struct Node * (*tree_add)(void *, void *));

void *rem(void *tree, struct Node *node, void *(*tree_rem)(void *, struct Node *));

struct Node *update(void *tree, struct Node *node, void (*tree_upd)(void *, struct Node *));

void traverse_tree(struct Node *root, void (*f)(void *));

void pretty_traverse_tree(struct Node *root, int space, void (*f)(void *));

//DESTROYS ALL DATA TOO
void destroy_tree(struct Node *root);

void *lbbt_init(void **ids, int n, int add_strat, int trunc_strat);

struct Node *lbbt_add(void *tree, void *data);

void *lbbt_rem(void *tree, struct Node *node);

void lbbt_upd(void *tree, struct Node *node);

void *btree_init(int n);

struct Node *btree_add(void *tree, void *data);

void *btree_rem(void *tree, struct Node *node);

void btree_upd(void *tree, struct Node *node);
