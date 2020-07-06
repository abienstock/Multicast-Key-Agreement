#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "trees.h"
#include "../../ll/ll.h"
#include "../../skeleton.h"
#include "../../utils.h"

// TODO: put in trees.h??
struct _InitRet {
  struct SkeletonNode *skeleton;
  struct Node *node;
};

/*
 * initialize the root of a subtree in the order b-tree with height h and n leaves starting at id leftmost_id
 */
struct _InitRet root_init(int order, int h, int n, int leftmost_id, int *ids, struct List *users) {
  struct _InitRet ret = { NULL, NULL };

  int prop_children = ceil((double) n / pow(order, h - 1));
  int min_children = ceil((double) order / 2);
  int num_children = prop_children;
  if (prop_children < min_children)
    num_children = min_children;
  
  struct Node *root = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));
  root->data = data;
  data->key = NULL;
  data->seed = NULL;
  data->tree_node_data = NULL; //TODO: anything??
  root->num_leaves = n;
  if (n == 1) { // root is a leaf
    data->id = *(ids+leftmost_id);
    root->children = NULL;
    root->num_children = 0;
    addAfter(users, users->tail, (void *) root);
    // TODO: skeleton??
    ret.skeleton = NULL;
    ret.node = root;
    return ret;
  }

  data->id = rand();
  struct Node **children = malloc_check(sizeof(struct Node *) * num_children);
  root->children = children;
  root->num_children = num_children;

  int m = floor(n/num_children);
  int r = n % num_children;
  int extras = 0;
  int i;
  for (i = 0; i < num_children; i++) {
    int extra = 0;
    if (i < r)
      extra = 1;
    struct _InitRet init_ret = root_init(order, h-1, m + extra, leftmost_id + m * i + extras, ids, users);
    *children++ = init_ret.node;
    init_ret.node->parent = root;
    extras += extra;
  }

  ret.node = root;
  // TODO: root??
  ret.skeleton = NULL;
  return ret;
}

/*
 * initialize order b-tree with n users with ids
 */
struct InitRet btree_init(int *ids, int n, int add_strat, int order, struct List *users) {
  if (n < 1)
    die_with_error("n has to be at least 1");

  struct BTree *tree = malloc_check(sizeof(struct BTree));
  double h = log(n) / log(order);
  double h_ceil = ceil(h);
  struct Node *root = root_init(order, (int) h_ceil, n, 0, ids, users).node;
  root->parent = NULL;
  tree->root = root;
  tree->order = order;
  tree->add_strat = add_strat;

  struct InitRet ret = { (void *) tree, NULL };
  return ret;
}
