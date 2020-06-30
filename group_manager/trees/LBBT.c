#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "trees.h"
#include "../../ll/ll.h"
#include "../../skeleton.h"
#include "../../utils.h"

struct TruncRet {
  struct SkeletonNode *skeleton;
  struct Node *node;
};

struct _InitRet {
  struct SkeletonNode *skeleton;
  struct Node *node;
};

/*
 * initialize the root of a perfect subtree in the lbbt with height h starting at id leftmost_id
 */
struct _InitRet init_perfect(int h, int leftmost_id, int *ids, struct List *users) {
  struct _InitRet ret = { NULL, NULL };

  struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
  struct Node *root = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));
  root->data = data;
  data->blank = 0;
  data->key = NULL;
  data->seed = NULL;
  
  root->num_leaves = 1 << h;	
  root->rightmost_blank = NULL;
  
  skeleton->node = root;
  skeleton->ciphertexts = NULL;
  
  if (h == 0) { // root is a leaf
    data->id = *(ids+leftmost_id);
    skeleton->node_id = data->id;
    root->children = NULL;
    addAfter(users, users->tail, (void *) root);
    
    skeleton->children_color = NULL;
    skeleton->children = NULL;
    
    ret.skeleton = skeleton;
    ret.node = root;
    return ret;
  }
  
  struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * 2);
  int *children_color = malloc_check(sizeof(int) * 2);
  
  skeleton->children = skel_children;
  *children_color++ = 0;
  *children_color-- = 1;
  skeleton->children_color = children_color;
  
  data->id = rand();
  skeleton->node_id = data->id;
  
  struct Node **children = malloc_check(sizeof(struct Node *) * 2);
  struct _InitRet left_ret = init_perfect(h-1, leftmost_id, ids, users);
  struct _InitRet right_ret = init_perfect(h-1, leftmost_id + (1 << (h-1)), ids, users);
  *skel_children++ = left_ret.skeleton;
  *skel_children-- = right_ret.skeleton;
  *children++ = left_ret.node;
  *children-- = right_ret.node;
  left_ret.node->parent = root;
  right_ret.node->parent = root;
  left_ret.skeleton->parent = skeleton;
  right_ret.skeleton->parent = skeleton;
  root->children = children;
  
  ret.node = root;
  ret.skeleton = skeleton;
  return ret;
}

/*
 * initialize the root of a subtree in the lbbt with n leaves starting at id leftmost_id
 */
struct _InitRet root_init(int n, int leftmost_id, int *ids, struct List *users){
  struct _InitRet ret = { NULL, NULL };

  struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
  struct Node *root = malloc_check(sizeof(struct Node));
  struct NodeData *data = malloc_check(sizeof(struct NodeData));

  root->data = data;
  data->blank = 0;
  data->key = NULL;
  data->seed = NULL;
  root->num_leaves = n;
  root->rightmost_blank = NULL;
  skeleton->node = root;
  skeleton->ciphertexts = NULL;
  if (n == 1) { // root is a leaf
    data->id = *(ids+leftmost_id);
    skeleton->node_id = data->id;
    root->children = NULL;
    addAfter(users, users->tail, (void *) root);
    skeleton->children_color = NULL;
    skeleton->children = NULL;
    ret.skeleton = skeleton;
    ret.node = root;
    return ret;
  }

  struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * 2);
  int *children_color = malloc_check(sizeof(int) * 2);
  skeleton->children = skel_children;
  *children_color++ = 0;
  *children_color-- = 1;
  skeleton->children_color = children_color;
  
  data->id = rand();
  skeleton->node_id = data->id;

  // n-1???
  struct Node **children = malloc_check(sizeof(struct Node *) * 2);
  double h = log2(n);
  double h_flr = floor(h);
  
  struct _InitRet left_ret, right_ret;
  if (h-h_flr == 0) {
    left_ret = init_perfect((int) h-1, leftmost_id, ids, users);
    right_ret = init_perfect((int) h-1, leftmost_id + (1 << (int) (h-1)), ids, users);
  } else { // always left skel child; n- (1 << h_flr) = 1: no right skel child
    left_ret = init_perfect((int) h_flr, leftmost_id, ids, users);
    right_ret = root_init(n - (1 << (int) h_flr), leftmost_id  + (1 << (int) h_flr), ids, users);
  }
  *skel_children++ = left_ret.skeleton;
  *skel_children-- = right_ret.skeleton;
  skeleton->children = skel_children;
  *children++ = left_ret.node;
  *children-- = right_ret.node;
  left_ret.node->parent = root;
  right_ret.node->parent = root;
  left_ret.skeleton->parent = skeleton;
  right_ret.skeleton->parent = skeleton;
  root->children = children;

  ret.node = root;
  ret.skeleton = skeleton;
  return ret;
}

/*
 * initialize lbbt with n users with ids
 */
struct InitRet lbbt_init(int *ids, int n, int add_strat, int trunc_strat, struct List *users) {
  if (n < 1)
    die_with_error("n has to be at least 1");

  struct LBBT *tree = malloc_check(sizeof(struct LBBT));
  struct SkeletonNode *skeleton = root_init(n, 0, ids, users).skeleton;
  skeleton->parent = NULL;
  struct Node *root = skeleton->node;
  root->parent = NULL;
  tree->root = root;
  tree->rightmost_leaf = (struct Node *) users->tail->data;
  
  struct List *blanks = malloc_check(sizeof(struct List));
  initList(blanks);
  tree->blanks = blanks;
  tree->add_strat = add_strat;
  tree->trunc_strat = trunc_strat;
  
  struct InitRet ret = { (void *) tree, skeleton };
  return ret;
}

int is_perfect(struct Node *root) {
  if (root->children == NULL) {
    return 1;
  }
  int l_leaves = (*(root->children))->num_leaves;
  int r_leaves = (*(root->children+1))->num_leaves;
  return (l_leaves == r_leaves);
}

/*
 * append new_leaf with id to the subtree rooted at node.
 */
struct SkeletonNode *lbbt_append(struct LBBT *lbbt, struct Node *node, int id, struct Node **new_leaf) {
  struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
  int *children_color = malloc_check(sizeof(int) * 2);
  struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * 2);

  // if subtree rooted at node is perfect, create new root with left child as node and right child as new_leaf
  if (is_perfect(node)) {
    struct SkeletonNode *root_skeleton = malloc_check(sizeof(struct SkeletonNode));
    skeleton->parent = root_skeleton;
    
    struct Node *leaf = malloc_check(sizeof(struct Node));
    struct NodeData *leaf_data = malloc_check(sizeof(struct NodeData));

    leaf->data = leaf_data;
    leaf_data->blank = 0;
    leaf_data->key = NULL;
    leaf_data->seed = NULL;
    leaf_data->id = id;
    leaf->children = NULL;
    leaf->num_leaves = 1;
    leaf->rightmost_blank = NULL;

    skeleton->node_id = leaf_data->id;
    skeleton->node = leaf;
    skeleton->children = NULL;
    skeleton->children_color = NULL;
    skeleton->ciphertexts = NULL;

    struct Node *root = malloc_check(sizeof(struct Node));
    struct NodeData *root_data = malloc_check(sizeof(struct NodeData));

    root->data = root_data;
    root_data->blank = 0;
    root_data->key = NULL;
    root_data->seed = NULL;
    root_data->id = rand();

    root_skeleton->node_id = root_data->id;
    root_skeleton->node = root;
    root_skeleton->ciphertexts = NULL;
    
    struct Node **root_children = malloc_check(sizeof(struct Node *) * 2);
    *root_children++ = node;
    *root_children-- = leaf;
    root->children = root_children;
    root->num_leaves = node->num_leaves + leaf->num_leaves;
    root->rightmost_blank = NULL;
    if (node == lbbt->root) {
      root->parent = NULL;
      lbbt->root = root;
    }
    
    leaf->parent = root;
    node->parent = root;

    *new_leaf = leaf;

    *skel_children++ = NULL;
    *skel_children-- = skeleton;
    *children_color++ = 1;
    *children_color-- = 0;
    root_skeleton->children_color = children_color;
    root_skeleton->children = skel_children;

    return root_skeleton;
  }

  // recursively append new_leaf to right subtree
  skeleton->node_id = ((struct NodeData *) node->data)->id;
  skeleton->node = node;
  skeleton->ciphertexts = NULL;
  skeleton->children_color = children_color;
  skeleton->children = skel_children;

  struct SkeletonNode *right_skel = lbbt_append(lbbt, *(node->children+1), id, new_leaf);
  right_skel->parent = skeleton;
  *skel_children++ = NULL;
  *skel_children-- = right_skel;
  *children_color++ = 1;
  *children_color-- = 0;
  
  struct Node *new_right = right_skel->node;
  *(node->children+1) = new_right;
  node->num_leaves = (*(node->children))->num_leaves + new_right->num_leaves;
  new_right->parent = node;
  return skeleton;
}

/*
 * recursively resets the rightmost blanks in the subtrees rooted at the nodes along the direct path of node.
 * also recursively builds a skeleton consisting of the nodes along the direct path of node.
 */
struct SkeletonNode *augment_blanks_build_skel(struct Node *node, struct Node *child, struct SkeletonNode *child_skel, int build_skel) {
  if (node != NULL) {
    struct SkeletonNode *skeleton = NULL;
    if (build_skel) {
      skeleton = malloc_check(sizeof(struct SkeletonNode));
      skeleton->node_id = ((struct NodeData *) node->data)->id;
      skeleton->node = node;
      skeleton->ciphertexts = NULL;
      
      int child_pos = 1;
      if (child != NULL) {
	int *children_color = malloc_check(sizeof(int) * 2);
	if (child == *(node->children))
	  child_pos = 0;
	if (((struct NodeData *)child->data)->blank == 1)
	  *(children_color + child_pos) = -1; // -1 means do nothing
	else
	  *(children_color + child_pos) = 0;
	*(children_color + (1 - child_pos)) = 1;
	skeleton->children_color = children_color;
	
      } else
	skeleton->children_color = NULL;
      
      if (child_skel != NULL) {
	struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * 2);
	skeleton->children = skel_children;
	*(skel_children + child_pos) = child_skel;
	*(skel_children + (1 - child_pos)) = NULL;
	child_skel->parent = skeleton;      
      } else {
	skeleton->children = NULL;
      }
    }
    if (child != NULL) {
	// augmenting rightmost_blanks
	if ((*(node->children+1))->rightmost_blank == NULL)
	  node->rightmost_blank = (*(node->children))->rightmost_blank;
	else
	  node->rightmost_blank = (*(node->children+1))->rightmost_blank;
    }
    return augment_blanks_build_skel(node->parent, node, skeleton, build_skel);
  }
  return child_skel;
}

/*
 * add a leaf with id to the tree
 */
struct AddRet lbbt_add(void *tree, int id) {
  struct AddRet ret = { NULL, NULL };
  
  struct LBBT *lbbt = (struct LBBT *) tree;
  struct Node *new_leaf = NULL; // // pointer to leaf newly occupied by id
  struct SkeletonNode *skeleton = NULL;
  switch (lbbt->add_strat) {
  case 0: //greedy
    if (!isEmptyList(lbbt->blanks)) {
      new_leaf = (struct Node *) popFront(lbbt->blanks); // get leftmost blank node
	struct NodeData *data = (struct NodeData *)new_leaf->data;
	data->id = id;
	data->blank = 0;
	new_leaf->rightmost_blank = NULL;
	skeleton = augment_blanks_build_skel(new_leaf, NULL, NULL, 1);
	skeleton->parent = NULL;
      }
    break;
  case 1: //random
    break;
  case 2: //append
    break;
  }
  if (new_leaf == NULL) {
    skeleton = lbbt_append(lbbt, lbbt->root, id, &new_leaf);
    skeleton->parent = NULL;
    lbbt->rightmost_leaf = new_leaf;
  }

  ret.added = new_leaf;
  ret.skeleton = skeleton;
  return ret;
}

/*
 * recursively truncate the subtree rooted at node until the rightmost leaf is not blank.
 * on_dir_path = 1 means node is on the direct path of the removed leaf
 */
struct TruncRet truncate(struct LBBT *lbbt, struct Node *node, int on_dir_path) {
  struct TruncRet ret = { NULL, NULL };
  struct NodeData *data = (struct NodeData *) node->data;
  if (data->blank == 1) {
    popBack(lbbt->blanks);
    free_node(node);
    return ret;
  }
  if (node->children == NULL) { //rightmost non_blank node
    lbbt->rightmost_leaf = node;
    ret.node = node;
    return ret;
  }

  // recursively truncate the right subtree
  struct TruncRet trunc_ret = truncate(lbbt, *(node->children+1), on_dir_path);
  if (trunc_ret.node != NULL) { // the right subtree is not completely removed
    struct Node *trunc_child = trunc_ret.node;
    *(node->children+1) = trunc_child;
    node->num_leaves = (*(node->children))->num_leaves + trunc_child->num_leaves;
    trunc_child->parent = node;
    if (!on_dir_path) { // so not part of skeleton
      ret.node = node;
      return ret;
    }
    
    struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
    int *children_color = malloc_check(sizeof(int) * 2);
    skeleton->node_id = ((struct NodeData *) node->data)->id;
    skeleton->node = node;
    skeleton->children_color = children_color;
    skeleton->ciphertexts = NULL;
    
    if (trunc_ret.skeleton == NULL) {
      *children_color++ = 1;
      *children_color-- = 1;
      skeleton->children = NULL;
    } else {
      struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * 2);
      trunc_ret.skeleton->parent = skeleton;
      skeleton->children = skel_children;
      *skel_children++ = NULL;
      *skel_children-- = trunc_ret.skeleton;
      *children_color++ = 1;
      *children_color-- = 0;
    }
    ret.skeleton = skeleton;
    ret.node = node;
    return ret;
  }

  trunc_ret = truncate(lbbt, *(node->children), 0); // recursively truncate left subtree
  struct Node *replacement = trunc_ret.node; // new right child of parent (or root)
  if (node == lbbt->root) {
    lbbt->root = replacement;
    replacement->parent = NULL;

    // skeleton is just the root in this case
    struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
    if (lbbt->root->num_leaves > 1) {
      int *children_color = malloc_check(sizeof(int) * 2);
      skeleton->children_color = children_color;
      *children_color++ = 1;
      *children_color-- = 1;
    } else {
      skeleton->children_color = NULL;
    }
    skeleton->node_id = ((struct NodeData *) lbbt->root->data)->id;
    skeleton->node = lbbt->root;
    skeleton->children = NULL;
    skeleton->ciphertexts = NULL;
    ret.skeleton = skeleton;
  }
  free_node(node);
  ret.node = replacement;
  return ret;
}

/*
 * recursively find the first blank leaf to the left of node
 */
struct ListNode *find_prev_blank(struct Node *node, struct Node *prev_node) {
  if (node == NULL)
    return NULL;
  struct ListNode *candidate = (*(node->children))->rightmost_blank;
  if (candidate == NULL || *(node->children) == prev_node)
    return find_prev_blank(node->parent, node);
  return candidate;
}

/*
 * remove leaf node from tree.
 */
struct RemRet lbbt_rem(void *tree, struct Node *node) {
  struct LBBT *lbbt = (struct LBBT *) tree;
  struct RemRet ret = { -1, NULL };
  
  switch (lbbt->trunc_strat) {
  case 0: //truncate
    if (node == lbbt->root)
      die_with_error("Cannot delete root");
    struct NodeData *data = (struct NodeData *) node->data;
    data->blank = 1;
    ret.id = data->id;
    struct ListNode *prev_blank = find_prev_blank(node->parent, node);
    struct ListNode *new_list_node = addAfter(lbbt->blanks, prev_blank, node); //insert new blank node into list
    node->rightmost_blank = new_list_node;

    // NOTE need augment after truncate too!
    if (node != lbbt->rightmost_leaf) {
      /*free(data->key);
      data->key = NULL;
      free(data->seed);
      data->seed = NULL;*/
      ret.skeleton = augment_blanks_build_skel(node->parent, node, NULL, 1);
      ret.skeleton->parent = NULL;
    } else {
      struct TruncRet trunc_ret = truncate(lbbt, lbbt->root, 1);
      struct SkeletonNode *trunc_skel = trunc_ret.skeleton;
      trunc_skel->parent = NULL;
      ret.skeleton = trunc_skel;
      augment_blanks_build_skel(lbbt->rightmost_leaf->parent, lbbt->rightmost_leaf, NULL, 0);
    }
    break;
  case 1: //keep
    break;
  }

  return ret;
}
