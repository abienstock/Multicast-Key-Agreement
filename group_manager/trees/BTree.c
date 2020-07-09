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
struct _InitRet btree_root_init(int order, int h, int n, int leftmost_id, int *ids, struct List *users) {
  struct _InitRet ret = { NULL, NULL };

  int prop_children = ceil((double) n / pow(order, h - 1));
  int min_children = ceil((double) order / 2);
  int num_children = prop_children;
  if (prop_children < min_children)
    num_children = min_children;

  struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
  struct Node *root = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));
  root->data = data;
  data->key = NULL;
  data->seed = NULL;
  struct BTreeNodeData *btree_data = malloc_check(sizeof(struct BTreeNodeData));
  data->tree_node_data = btree_data;
  btree_data->height = h;  
  root->num_leaves = n;
  skeleton->node = root;
  skeleton->ciphertexts = NULL;  
  if (n == 1) { // root is a leaf
    data->id = *(ids+leftmost_id);
    skeleton->node_id = data->id;
    btree_data->lowest_nonfull = INT_MAX; // TODO: no btree_data at all?
    btree_data->opt_add_child = -1;
    root->children = NULL;
    root->num_children = 0;
    addAfter(users, users->tail, (void *) root);
    skeleton->children_color = NULL;
    skeleton->children = NULL;
    ret.skeleton = skeleton;
    ret.node = root;
    return ret;
  }

  struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * num_children);
  int *children_color = malloc_check(sizeof(int) * num_children);
  skeleton->children = skel_children;
  skeleton->children_color = children_color;  
  
  data->id = rand();
  skeleton->node_id = data->id;
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
    if (i == 0)
      *children_color++ = 0;
    else
      *children_color++ = 1;
    
    int extra = 0;
    if (i < r)
      extra = 1;
    struct _InitRet init_ret = btree_root_init(order, h-1, m + extra, leftmost_id + m * i + extras, ids, users);
    *skel_children++ = init_ret.skeleton;
    init_ret.skeleton->parent = skeleton;
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
  ret.skeleton = skeleton;
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
  struct SkeletonNode *skeleton = btree_root_init(order, (int) h_ceil, n, 0, ids, users).skeleton;
  skeleton->parent = NULL;
  struct Node *root = skeleton->node;
  root->parent = NULL;
  tree->root = root;
  tree->order = order;
  tree->add_strat = add_strat;

  struct InitRet ret = { (void *) tree, skeleton };
  return ret;
}

/*
 * recursively update lowest_nonfull and opt_add_child along the direct path of node.
 */
void update_add_hints(struct Node *node, int order, int recurse) {
  struct Node **children = node->children;
  struct BTreeNodeData *btree_data = (struct BTreeNodeData *) ((struct NodeData *) node->data)->tree_node_data;
  int i;
  // optimum add child has been removed or its subtree has been at least partially filled; also if lowest_nonfull is height of node and node now has order-many children        
  if (*(children + btree_data->opt_add_child) == NULL || btree_data->lowest_nonfull < ((struct BTreeNodeData *) ((struct NodeData *) (*(children + btree_data->opt_add_child))->data)->tree_node_data)->lowest_nonfull) {
    btree_data->lowest_nonfull = INT_MAX;
    // find first non-NULL child and set opt_add to it
    for (i = 0; i < order; i++) {
      if (*(children + i) != NULL) {
	btree_data->opt_add_child = i;
	break;
      }
    }
  }
  // find new candidate lowest_nonfull and opt_add_child and also update num_leaves
  node->num_leaves = 0;
  for (i = 0; i < order; i++) {
    if (*(children + i) != NULL) {
      node->num_leaves += (*(children + i))->num_leaves;
      int child_lowest_nonfull = ((struct BTreeNodeData *) ((struct NodeData *) (*(children + i))->data)->tree_node_data)->lowest_nonfull;
      if (btree_data->lowest_nonfull > child_lowest_nonfull) {
	btree_data->lowest_nonfull = child_lowest_nonfull;
	btree_data->opt_add_child = i;
      }
    }
  }
  // TODO: set opt_add_child to first non-NULL?
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
    // find first NULL child add put new child node there
    int i;
    for (i = 0; i < order; i++) {
      if (*(parent->children + i) == NULL) {
	*(parent->children + i) = child;
      }
    }
    parent->num_children++;
    child->parent = parent;
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
    split->num_children = half + 1;
    parent->num_children = ceil(order / 2.0);
    int i;
    for (i = 1; i < half + 1; i++) {
      struct Node *moved_child = *(parent->children + order - i);
      *split_children++ = moved_child;
      *(parent->children + order - i) = NULL;
      moved_child->parent = split;
    }
    for (; i < order; i++)
      *split_children++ = NULL;

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

/*
 * recursively add child to parent. If n=1, parent is the only node in the tree and we create a new root.
 * TODO: optimizing child borrowing further??
 */
void remove_node(struct Node *parent, struct Node *child, struct BTree *btree) {
  int order = btree->order;
  parent->num_children--;
  int i;
  for (i = 0; i < order; i++) {
    if (*(parent->children + i) == child)
      *(parent->children + i) = NULL;
  }
  if (parent->num_children < ceil(order / 2.0)) {
    if (parent->parent != NULL) {
      struct Node *grandparent = parent->parent;
      struct Node **siblings = grandparent->children;
      struct Node *borrowed = NULL, *sibling = NULL;
      for (i = 0; i < order; i++) {
	if ((sibling = *(siblings + i)) != NULL && sibling->num_children > ceil(order / 2.0)) {
	  struct Node **sibling_children = sibling->children;
	  // borrow first non-NULL child of sibling
	  int j;
	  for (j = 0; j < order; j++) {
	    if (*(sibling_children + j) != NULL) {
	      borrowed = *(sibling_children + j);
	      *(sibling_children + j) = NULL;	    
	      break;
	    }
	  }
	  sibling->num_children--;
	  for (j = 0; j < order; j++) {
	    if (*(parent->children + j) == NULL) {
	      *(parent->children + j) = borrowed;
	      break;
	    }
	  }
	  parent->num_children++;
	  borrowed->parent = parent;
	  
	  update_add_hints(sibling, order, 0);
	  update_add_hints(parent, order, 1);
	  return;
	}
      }
      // every sibling has minimum number of children -- give children of parent to first non-NULL sibling
      // TODO: try to balance children moves??
      struct Node **parent_children = parent->children;
      for (i = 0; i < order; i++) {
	if ((sibling = *(siblings + i)) != NULL && sibling != parent) {
	  struct Node **sibling_children = sibling->children;
	  struct Node *moved_child;
	  // replace sibling's NULL children with parent's children
	  int j, k = 0;
	  for (j = 0; j < order; j++) {
	    if ((moved_child = *(parent_children + j)) != NULL) {
	      for (; k < order; k++) {
		if (*(sibling_children + k) == NULL) {
		  *(sibling_children + k) = moved_child;
		  moved_child->parent = sibling;
		  sibling->num_children++;
		}
	      }
	    }
	  }
	  break;
	}
      }
      update_add_hints(sibling, order, 0);
      remove_node(grandparent, parent, btree);
    } else if (parent->num_children == 1) { // delete root
      struct Node *new_root = NULL;
      for (i = 0; i < order; i++) {
	if ((new_root = *(parent->children + i)) != NULL) {
	  new_root->parent = NULL;
	  btree->root = new_root;
	  break;
	}
      }
    }
  } else {
    update_add_hints(parent, order, 1);
  }
}

/*
 * remove leaf node from tree
 */
struct RemRet btree_rem(void *tree, struct Node *node) {
  struct BTree *btree = (struct BTree *) tree;
  struct RemRet ret = { -1, NULL };

  if (node == btree->root)
    die_with_error("Cannot delete root");
  struct NodeData *data = (struct NodeData *) node->data;
  ret.id = data->id;

  struct Node *parent = node->parent;
  remove_node(parent, node, btree);
  return ret;
}
