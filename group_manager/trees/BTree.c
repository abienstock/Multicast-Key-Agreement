#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
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
  struct BTreeNodeData *btree_data = malloc_check(sizeof(struct BTreeNodeData));
  data->tree_node_data = btree_data;
  btree_data->height = h;  
  root->num_leaves = n;
  if (n == 1) { // root is a leaf
    data->id = *(ids+leftmost_id);
    btree_data->lowest_nonfull = INT_MAX; // TODO: no btree_data at all?
    btree_data->opt_add_child = -1;
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
  int lowest_nonfull = INT_MAX;
  int opt_add_child = 0;
  int i;
  for (i = 0; i < num_children; i++) {
    int extra = 0;
    if (i < r)
      extra = 1;
    struct _InitRet init_ret = root_init(order, h-1, m + extra, leftmost_id + m * i + extras, ids, users);
    struct Node *child = init_ret.node;
    *children++ = child;
    child->parent = root;
    int child_lowest = ((struct BTreeNodeData *) ((struct NodeData *) child->data)->tree_node_data)->lowest_nonfull;
    if (child_lowest < lowest_nonfull) {
      lowest_nonfull = child_lowest;
      opt_add_child = i;
    }
    extras += extra;
  }
  if (lowest_nonfull == INT_MAX && num_children < order)
    lowest_nonfull = h;
  btree_data->lowest_nonfull = lowest_nonfull;
  btree_data->opt_add_child = opt_add_child;

  ret.node = root;
  // TODO: skeleton??
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

/*
 * recursively update lowest_nonfull and opt_add_child along the direct path of node.
 */
void update_add_hints(struct Node *node, int order, int recurse) {
  struct Node **children = node->children;
  struct BTreeNodeData *btree_data = (struct BTreeNodeData *) ((struct NodeData *) node->data)->tree_node_data;
  int i;
  int curr_lowest_nonfull = ((struct BTreeNodeData *) ((struct NodeData *) (*(children + btree_data->opt_add_child))->data)->tree_node_data)->lowest_nonfull;
  // optimum add child subtree has been at least partially filled -- also if lowest_nonfull is height of node and node now has order-many children
  if (btree_data->lowest_nonfull < curr_lowest_nonfull) {
    btree_data->lowest_nonfull = INT_MAX;
    btree_data->opt_add_child = 0;
  }
  for (i = 0; i < node->num_children; i++) {
    int child_lowest_nonfull = ((struct BTreeNodeData *) ((struct NodeData *) (*(children + i))->data)->tree_node_data)->lowest_nonfull;
    if (btree_data->lowest_nonfull > child_lowest_nonfull) {
      btree_data->lowest_nonfull = child_lowest_nonfull;
      btree_data->opt_add_child = i;
    }
  }
  // TODO: set opt_add_child to 0?
  if (btree_data->lowest_nonfull == INT_MAX && node->num_children < order)
    btree_data->lowest_nonfull = btree_data->height;

  if (recurse && node->parent != NULL)
    update_add_hints(node->parent, order, recurse);
}

/*
 * recursively add child to parent. If n=1, parent is the only node in the tree and we create a new root.
 */
void add_node(struct Node *parent, struct Node *child, struct BTree *btree) {
  int order = btree->order;
  struct BTreeNodeData *parent_data = (struct BTreeNodeData *) ((struct NodeData *) parent->data)->tree_node_data;
  // second case is for group of size 1
  if (parent->num_children < order && parent->children != NULL) { 
    *(parent->children + parent->num_children) = child;
    parent->num_children++;
    child->parent = parent;
    parent->num_leaves += child->num_leaves;
    update_add_hints(parent, order, 1);
    return;
  } else if (parent->children != NULL) {
    int half = order / 2;
    struct Node *split = malloc_check(sizeof(struct Node));
    struct NodeData *data = malloc_check(sizeof(struct NodeData));
    split->data = data;
    data->key = NULL;
    data->seed = NULL;
    data->id = rand();
    struct BTreeNodeData *split_btree_data = malloc_check(sizeof(struct BTreeNodeData));
    split_btree_data->lowest_nonfull = INT_MAX;
    split_btree_data->opt_add_child = 0;
    split_btree_data->height = parent_data->height;
    data->tree_node_data = split_btree_data;

    struct Node **split_children = malloc_check(sizeof(struct Node *) * order);
    split->children = split_children;    
    *split_children++ = child;
    child->parent = split;        
    split->num_leaves = child->num_leaves;
    split->num_children = half + 1;
    parent->num_children = ceil(order / 2.0);
    int i;
    for (i = 1; i < half + 1; i++) {
      struct Node *moved_child = *(parent->children + order - i);
      *split_children++ = moved_child;
      *(parent->children + order - i) = NULL;
      moved_child->parent = split;
      split->num_leaves += moved_child->num_leaves;
      parent->num_leaves -= moved_child->num_leaves;
    }
    for (; i < order; i++)
      *split_children++ = NULL;

    if (parent_data->opt_add_child >= parent->num_children) {
      parent_data->lowest_nonfull = INT_MAX;
      parent_data->opt_add_child = 0;
    }
    update_add_hints(parent, order, 0);
    update_add_hints(split, order, 0);
    
    if (parent->parent != NULL) {
      add_node(parent->parent, split, btree);
      return;
    } else {
      child = split; // for simplification of joining next part with add to n=1 case
    }
  }
  struct Node *new_root = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));
  new_root->data = data;
  data->key = NULL;
  data->seed = NULL;
  data->id = rand();
  struct BTreeNodeData *new_root_btree_data = malloc_check(sizeof(struct BTreeNodeData));
  new_root_btree_data->lowest_nonfull = INT_MAX;
  new_root_btree_data->opt_add_child = 0;
  new_root_btree_data->height = parent_data->height + 1;
  data->tree_node_data = new_root_btree_data;
  
  new_root->num_children = 2;
  struct Node **new_root_children = malloc_check(sizeof(struct Node *) * order);
  new_root->children = new_root_children;
  *new_root_children++ = parent;
  *new_root_children++ = child;
  child->parent = new_root;
  parent->parent = new_root;
  new_root->num_leaves = child->num_leaves + parent->num_leaves;
  int i;
  for (i = 2; i < order; i++)
    *new_root_children++ = NULL;  
  new_root->parent = NULL;
  btree->root = new_root;

  if (parent->children != NULL)
    update_add_hints(parent, order, 1); // recurse on new root too
  else
    update_add_hints(new_root, order, 0); // only update root
}

/*
 * Find a height-1 node in the subtree of the lowest non-full node.
 * If n=1, the parent is the only node in the tree.
 */
struct Node *find_add_parent(struct Node *root, int order) {
  if (root->num_leaves <= order)
    return root;
  int opt_add_child = ((struct BTreeNodeData *) ((struct NodeData *) root->data)->tree_node_data)->opt_add_child;
  return find_add_parent(*(root->children + opt_add_child), order);
}

/*
 * add a leaf with id to the tree
 */
struct AddRet btree_add(void *tree, int id) {
  struct AddRet ret = { NULL, NULL };

  struct BTree *btree = (struct BTree *) tree;
  struct Node *new_node = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));
  new_node->data = data;
  data->key = NULL;
  data->seed = NULL;
  data->id = id;
  struct BTreeNodeData *btree_data = malloc_check(sizeof(struct BTreeNodeData));
  btree_data->lowest_nonfull = INT_MAX; // TODO: no btree_data at all?
  btree_data->opt_add_child = -1;
  btree_data->height = 0;
  data->tree_node_data = btree_data;
  new_node->num_leaves = 1;
  new_node->children = NULL;
  new_node->num_children = 0;  
  switch (btree->add_strat) {
  case 0: { //greedy
    struct Node *add_parent = find_add_parent(btree->root, btree->order);
    add_node(add_parent, new_node, btree);
    ret.added = new_node;
    break;
  }
  case 1: //random
    break;
  }
  return ret;
}
