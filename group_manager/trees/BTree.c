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
  skeleton->ciphertext_lists = NULL;  
  if (n == 1) { // root is a leaf
    data->blank = 0;    
    data->id = *(ids+leftmost_id);
    data->tk_unmerged = NULL;
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

  data->blank = 1;
  data->tk_unmerged = malloc_check(sizeof(struct List));
  initList(data->tk_unmerged);

  struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * num_children);
  int *children_color = malloc_check(sizeof(int) * num_children);
  skeleton->children = skel_children;
  skeleton->children_color = children_color;  
  
  data->id = rand();
  skeleton->node_id = data->id;
  struct Node **children = malloc_check(sizeof(struct Node *) * order);
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
  for (; i < order; i++)
    *children++ = NULL;
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
 * rearrange children so that they are the first num_children of the node's children array
 *
 */
void reorder_children(struct Node *node, int order) {
  struct Node **children = node->children;
  int i, first_empty = 0;
  for (i = 0; i < order; i++) {
    if (*(children + i) != NULL) {
      if (i == first_empty) {
	first_empty++;
	continue;
      }
      int j;
      for (j = first_empty; j < order; j++) {
	if (*(children + j) == NULL)
	  break;
      }
      *(children + j) = *(children + i);
      *(children + i) = NULL;
      first_empty = j + 1;
    }
  }
}

/*
 * recursively update lowest_nonfull and opt_add_child along the direct path of node
 * and build the skeleton consisting of PRGs on the direct path and encryptions on the copath.
 * child and child_skel are always non-NULL.
 * PARENT IS NEVER A LEAF UNLESS IT IS ALSO THE ROOT!!
 */
struct SkeletonNode *update_add_hints_build_skel(struct Node *parent, struct Node *child, struct Node *sibling, struct List *unmerged, struct SkeletonNode *child_skel, struct SkeletonNode *sibling_skel, int order, int recurse, int special) {
  struct Node **children = parent->children;
  struct NodeData *parent_data = (struct NodeData *) parent->data;  
  struct BTreeNodeData *parent_btree_data = (struct BTreeNodeData *) parent_data->tree_node_data;
  struct SkeletonNode *parent_skel = malloc_check(sizeof(struct SkeletonNode));
  parent_skel->node_id = parent_data->id;
  parent_skel->node = parent;
  parent_skel->ciphertext_lists = NULL;
  // if parent is root
  if (children == NULL) {
    parent_skel->parent = NULL;
    parent_skel->children_color = NULL;
    parent_skel->children = NULL;
    parent_skel->special = 0; //TODO: correct??
    return parent_skel;
  }
  parent_skel->special = special;
  if (special && !parent_data->blank) {
    struct ListNode *curr = unmerged->head;
    while (curr != NULL) {
      struct Node *unmerged_node = (struct Node *) curr->data;
      addFront(parent_data->tk_unmerged, unmerged_node);
      curr = curr->next;
    }
  }
  
  int *children_color = malloc_check(sizeof(int) * parent->num_children);
  struct SkeletonNode **skel_children = malloc_check(sizeof(struct SkeletonNode *) * parent->num_children);
  
  int i;
  // optimum add child has been removed or its subtree has been at least partially filled; also if lowest_nonfull is height of node and node now has order-many children        
  if (*(children + parent_btree_data->opt_add_child) == NULL || parent_btree_data->lowest_nonfull < ((struct BTreeNodeData *) ((struct NodeData *) (*(children + parent_btree_data->opt_add_child))->data)->tree_node_data)->lowest_nonfull) {
    parent_btree_data->lowest_nonfull = INT_MAX;
    // find first non-NULL child and set opt_add to it
    for (i = 0; i < order; i++) {
      if (*(children + i) != NULL) {
	parent_btree_data->opt_add_child = i;
	break;
      }
    }
  }
  // find new candidate lowest_nonfull and opt_add_child and also update num_leaves
  parent->num_leaves = 0;
  int j = 0;
  for (i = 0; i < order; i++) {
    if (*(children + i) != NULL) {
      parent->num_leaves += (*(children + i))->num_leaves;
      int child_lowest_nonfull = ((struct BTreeNodeData *) ((struct NodeData *) (*(children + i))->data)->tree_node_data)->lowest_nonfull;
      if (parent_btree_data->lowest_nonfull > child_lowest_nonfull) {
	parent_btree_data->lowest_nonfull = child_lowest_nonfull;
	parent_btree_data->opt_add_child = i;
      }

      if (*(children + i) == child) {
	*(children_color + j) = 0;
	*(skel_children + j) = child_skel;
	child_skel->parent = parent_skel;
      } else if (*(children + i) == sibling) {
	*(children_color + j) = 1;
	*(skel_children + j) = sibling_skel;
	if (sibling_skel != NULL)
	  sibling_skel->parent = parent_skel;
      } else {
	*(children_color + j) = 1;
	*(skel_children + j) = NULL;
      }
      j++;
    }
  }
  parent_skel->children_color = children_color;
  parent_skel->children = skel_children;
  
  // TODO: set opt_add_child to first non-NULL?
  if (parent_btree_data->lowest_nonfull == INT_MAX && parent->num_children < order)
    parent_btree_data->lowest_nonfull = parent_btree_data->height;

  if (recurse && parent->parent != NULL)
    return update_add_hints_build_skel(parent->parent, parent, NULL, unmerged, parent_skel, NULL, order, recurse, special);
  else if (recurse)
    parent_skel->parent = NULL;
  return parent_skel;
}

/*
 * recursively add child to parent. If n=1, parent is the only node in the tree and we create a new root.
 * child_skel is always non-NULL, sibling_skel is non-NULL if in the previous layer of recursion, the parent node was split.
 * if adding child to parent splits parent, sibling and child are both assigned to the new node.
 */
struct SkeletonNode *add_node(struct Node *parent, struct Node *child, struct Node *sibling, struct Node *leaf, struct SkeletonNode *child_skel, struct SkeletonNode *sibling_skel, struct BTree *btree) {
  int order = btree->order;
  struct NodeData *parent_data = (struct NodeData *) parent->data;
  struct BTreeNodeData *parent_btree_data = (struct BTreeNodeData *) parent_data->tree_node_data;
  struct SkeletonNode *parent_skel = NULL;
  
  // second case is for group of size 1
  if (parent->num_children < order && parent->children != NULL) {    
    // find first NULL child add put new child node there, also adjust skeleton
    int i;
    for (i = 0; i < order; i++) {
      if (*(parent->children + i) == NULL) {
	*(parent->children + i) = child;
	break;
      }
    }
    parent->num_children++;
    child->parent = parent;
    struct List unmerged;
    initList(&unmerged);
    addFront(&unmerged, leaf);
    return update_add_hints_build_skel(parent, child, sibling, &unmerged, child_skel, sibling_skel, order, 1, 1);
  } else if (parent->children != NULL) {
    int half = order / 2;
    struct Node *split = malloc_check(sizeof(struct Node));
    struct NodeData *data = malloc_check(sizeof(struct NodeData));
    split->data = data;
    data->key = NULL;
    data->seed = NULL;
    data->id = rand();
    data->blank = 1;
    data->tk_unmerged = malloc_check(sizeof(struct List));
    initList(data->tk_unmerged);
    struct BTreeNodeData *split_btree_data = malloc_check(sizeof(struct BTreeNodeData));
    split_btree_data->lowest_nonfull = INT_MAX;
    split_btree_data->opt_add_child = 0;
    split_btree_data->height = parent_btree_data->height;
    data->tree_node_data = split_btree_data;

    struct Node **split_children = malloc_check(sizeof(struct Node *) * order);
    split->children = split_children;    
    *split_children++ = child;
    child->parent = split;      
    int j = 1;
    if (sibling != NULL) {
      *split_children++ = sibling;
      sibling->parent = split;
      j++;
    }
    split->num_children = half + 1;
    parent->num_children = ceil(order / 2.0);
    int i;
    for (i = 0; i < order; i++) {
      if (j < half + 1 && *(parent->children + i) != sibling) {
	struct Node *moved_child = *(parent->children + i);
	*split_children++ = moved_child;
	*(parent->children + i) = NULL;
	moved_child->parent = split;
	j++;
      } else if (*(parent->children + i) == sibling && sibling != NULL) {
	*(parent->children + i) = NULL;
      }
    }
    for (; j < order; j++)
      *split_children++ = NULL;
    reorder_children(parent, order); //TODO: only do this in remove??

    struct SkeletonNode *split_skel = update_add_hints_build_skel(split, child, sibling, NULL, child_skel, sibling_skel, order, 0, 0);    
    parent_skel = update_add_hints_build_skel(parent, NULL, NULL, NULL, NULL, NULL, order, 0, 0);
    
    if (parent->parent != NULL) {
      return add_node(parent->parent, split, parent, leaf, split_skel, parent_skel, btree);
    } else {
      child = split; // for simplification of joining next part with add to n=1 case
      child_skel = split_skel;
    }
  }
  struct Node *new_root = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));
  new_root->data = data;
  data->key = NULL;
  data->seed = NULL;
  data->id = rand();
  data->blank = 1;
  data->tk_unmerged = malloc_check(sizeof(struct List));
  initList(data->tk_unmerged);
  struct BTreeNodeData *new_root_btree_data = malloc_check(sizeof(struct BTreeNodeData));
  new_root_btree_data->lowest_nonfull = INT_MAX;
  new_root_btree_data->opt_add_child = 0;
  new_root_btree_data->height = parent_btree_data->height + 1;
  data->tree_node_data = new_root_btree_data;
  
  new_root->num_children = 2;
  struct Node **new_root_children = malloc_check(sizeof(struct Node *) * order);
  new_root->children = new_root_children;
  *new_root_children++ = parent;
  *new_root_children++ = child;
  child->parent = new_root;
  parent->parent = new_root;
  
  int i;
  for (i = 2; i < order; i++) {
    *new_root_children++ = NULL;
  }
  new_root->parent = NULL;
  btree->root = new_root;

  //if (parent->children != NULL)
  return update_add_hints_build_skel(new_root, child, parent, NULL, child_skel, parent_skel, order, 1, 0);
  //else
  //update_add_hints_build_skel(new_root, child, order, 0); // only update root
  // always return skeleton formed above
  //return root_skel;
}

/*
 * Find a height-1 node in the subtree of the lowest non-full node.
 * If n=1, the parent is the only node in the tree.
 */
struct Node *find_add_parent(struct Node *root, int order) {
  if (root->num_leaves == root->num_children || root->num_children == 0)
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
  data->blank = 0; //TODO: not needed?
  data->tk_unmerged = NULL;
  struct BTreeNodeData *btree_data = malloc_check(sizeof(struct BTreeNodeData));
  btree_data->lowest_nonfull = INT_MAX; // TODO: no btree_data at all?
  btree_data->opt_add_child = -1;
  btree_data->height = 0;
  data->tree_node_data = btree_data;
  new_node->num_leaves = 1;
  new_node->children = NULL;
  new_node->num_children = 0;

  struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
  skeleton->node_id = data->id;
  skeleton->node = new_node;
  skeleton->ciphertext_lists = NULL;
  skeleton->children_color = NULL;
  skeleton->children = NULL;
  skeleton->special = 1; //TODO: check if need this
  switch (btree->add_strat) {
  case 0: { //greedy
    struct Node *add_parent = find_add_parent(btree->root, btree->order);
    ret.skeleton = add_node(add_parent, new_node, NULL, new_node, skeleton, NULL, btree);
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
 * sibling and sibling_skel are the sibling and skeleton of the sibling of child if recursive removal has occured.
 * TODO: optimizing child borrowing further??
 */
struct SkeletonNode *remove_node(struct Node *parent, struct Node *child, struct Node *child_sibling, struct List *unmerged, struct SkeletonNode *child_sibling_skel, struct BTree *btree) {
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
	  reorder_children(parent, order);
	  reorder_children(sibling, order); //TODO: only do this on parent??

	  struct SkeletonNode *parent_skel = update_add_hints_build_skel(parent, child_sibling, NULL, NULL, child_sibling_skel, NULL, order, 0, 0);
	  struct SkeletonNode *sibling_skel = update_add_hints_build_skel(sibling, NULL, NULL, NULL, NULL, NULL, order, 0, 0);
	  return update_add_hints_build_skel(grandparent, parent, sibling, NULL, parent_skel, sibling_skel, order, 1, 0);
	}
      }
      // every sibling has minimum number of children -- give children of parent to first non-NULL sibling
      // TODO: try to balance children moves??
      struct Node **parent_children = parent->children;
      if (unmerged == NULL) {
	unmerged = malloc_check(sizeof(struct List));
	initList(unmerged);
      }
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
		  addFront(unmerged, moved_child);
		  break;
		}
	      }
	    }
	  }
	  break;
	}
      }
      struct SkeletonNode *sibling_skel = update_add_hints_build_skel(sibling, child_sibling, NULL, unmerged, child_sibling_skel, NULL, order, 0, 1);
      return remove_node(grandparent, parent, sibling, unmerged, sibling_skel, btree);
    } else if (parent->num_children == 1) { // delete root
      struct Node *new_root = NULL;
      for (i = 0; i < order; i++) {
	if ((new_root = *(parent->children + i)) != NULL)
	  break;
      }
      new_root->parent = NULL;
      btree->root = new_root;
      if (child_sibling_skel != NULL) {
	child_sibling_skel->parent = NULL;
	return child_sibling_skel;
      } else {
	return update_add_hints_build_skel(new_root, NULL, NULL, NULL, NULL, NULL, order, 1, 0);
      }
    } else {
      reorder_children(parent, order);
      return update_add_hints_build_skel(parent, child_sibling, NULL, NULL, child_sibling_skel, NULL, order, 1, 0);
    }
  } else {
    reorder_children(parent, order);
    return update_add_hints_build_skel(parent, child_sibling, NULL, NULL, child_sibling_skel, NULL, order, 1, 0);
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
  ret.skeleton = remove_node(parent, node, NULL, NULL, NULL, btree);
  return ret;
}
