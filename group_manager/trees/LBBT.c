#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "trees.h"
#include "../../ll/ll.h"
#include "../../skeleton.h"

struct TruncRet {
  struct SkeletonNode *skeleton;
  struct Node *node;
};

struct _InitRet {
  struct SkeletonNode *skeleton;
  struct Node *node;
};

// TODO: documentation
struct _InitRet init_perfect(int h, int leftmost_id, int *ids, struct List *users) {
  struct _InitRet ret = { NULL, NULL };

  struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
   if (skeleton == NULL) {
     perror("malloc returned NULL");
     return ret;
   }
   struct Node *root = malloc(sizeof(struct Node));
   if (root == NULL) {
     perror("malloc returned NULL");
     return ret;
   }
   struct NodeData *data = malloc(sizeof(struct NodeData));
   if (data == NULL) {
     perror("malloc returned NULL");
     return ret;
   }

   root->data = data;
   data->blank = 0;
   data->key = NULL;
   data->seed = NULL;
   
   root->num_leaves = 1 << h;	
   root->rightmost_blank = NULL;

   skeleton->node = root;
   skeleton->ciphertexts = NULL;
   
   if (h == 0) {
     data->id = *(ids+leftmost_id);
     skeleton->node_id = data->id;
     root->children = NULL;
     addFront(users, (void *) root);

     skeleton->children_color = NULL;
     skeleton->children = NULL;

     ret.skeleton = skeleton;
     ret.node = root;
     return ret;
   }

   struct SkeletonNode **skel_children = malloc(sizeof(struct skeletonNode *) * 2);
   if (skel_children == NULL) {
     perror("malloc returned NULL");
     return ret;
   }   
   int *children_color = malloc(sizeof(int) * 2);
   if (children_color == NULL) {
     perror("malloc returned NULL");
     return ret;
   }

   skeleton->children = skel_children;
   *children_color++ = 0;
   *children_color-- = 1;
   skeleton->children_color = children_color;

   data->id = rand();
   
   struct Node **children = malloc(sizeof(struct Node *) * 2);
   if (children == NULL) {
     perror("malloc returned NULL");
     return ret;
   }
   
   struct _InitRet left_ret = init_perfect(h-1, leftmost_id, ids, users);
   struct _InitRet right_ret = init_perfect(h-1, leftmost_id + (1 << (h-1)), ids, users);
   
   //TODO: get rid of node children??
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

struct _InitRet root_init(int n, int leftmost_id, int *ids, struct List *users){
  struct _InitRet ret = { NULL, NULL };

  struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
  if (skeleton == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  struct Node *root = malloc(sizeof(struct Node));
  if (root == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  struct NodeData *data = malloc(sizeof(struct NodeData));
  if (data == NULL) {
    perror("malloc returned NULL");
    return ret;
  }

  root->data = data;
  data->blank = 0;
  data->key = NULL;
  data->seed = NULL;
  
  root->num_leaves = n;
  root->rightmost_blank = NULL;

  skeleton->node = root;
  skeleton->ciphertexts = NULL;

  if (n == 1) {
    data->id = *(ids+leftmost_id);
    skeleton->node_id = data->id;
    root->children = NULL;
    addFront(users, (void *) root);

    skeleton->children_color = NULL;
    skeleton->children = NULL;

    ret.skeleton = skeleton;
    ret.node = root;
    return ret;
  }

  struct SkeletonNode **skel_children = malloc(sizeof(struct skeletonNode *) * 2);
  if (skel_children == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  int *children_color = malloc(sizeof(int) * 2);
  if (children_color == NULL) {
    perror("malloc returned NULL");
    return ret;
  }

  skeleton->children = skel_children;
  *children_color++ = 0;
  *children_color-- = 1;
  skeleton->children_color = children_color;
  
  data->id = rand();
  // n-1???
  struct Node **children = malloc(sizeof(struct Node *) * 2);
  if (children == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  double h = log2(n);
  double h_flr = floor(h);
  
  struct _InitRet left_ret, right_ret;
  if (h-h_flr == 0) {
    left_ret = init_perfect((int) h-1, leftmost_id, ids, users);
    right_ret = init_perfect((int) h-1, leftmost_id + (1 << (int) (h-1)), ids, users);
  } else { // always left skel child, n- (1 << h_flr) = 1: no right skel child
    left_ret = init_perfect((int) h_flr, leftmost_id, ids, users);
    right_ret = root_init(n - (1 << (int) h_flr), leftmost_id  + (1 << (int) h_flr), ids, users);
  }

  //TODO: get rid of node children??
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

struct InitRet lbbt_init(int *ids, int n, int add_strat, int trunc_strat, struct List *users) {
  if (n < 1) {
    perror("n has to be at least 1");
    struct InitRet ret = { NULL, NULL };
    return ret;
  }

  struct LBBT *tree = malloc(sizeof(struct LBBT));
  if (tree == NULL) {
    perror("malloc returned NULL");
    struct InitRet ret = { NULL, NULL };
    return ret;
  }

  struct SkeletonNode *skeleton = root_init(n, 0, ids, users).skeleton;
  skeleton->parent = NULL;

  struct InitRet ret = { (void *) tree, skeleton };

  struct Node *root = skeleton->node;
  root->parent = NULL;
  tree->root = root;
  tree->rightmost_leaf = (struct Node *) users->head->data; // TODO: FIX!! (Fine for now)
  
  struct List *blanks = malloc(sizeof(struct List));
  if (blanks == NULL) {
    perror("malloc returned NULL");
    struct InitRet ret = { NULL, NULL };
    return ret;
  }
  initList(blanks);
  tree->blanks = blanks;
  tree->add_strat = add_strat;
  tree->trunc_strat = trunc_strat;

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

struct SkeletonNode *lbbt_append(struct LBBT *lbbt, struct Node *node, int id, struct Node **new_leaf) {
  struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
  if (skeleton == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  int *children_color = malloc(sizeof(int) * 2);
  if (children_color == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  struct SkeletonNode **skel_children = malloc(sizeof(struct skeletonNode *) * 2);
  if (skel_children == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  
  if (is_perfect(node)) {
    struct SkeletonNode *root_skeleton = malloc(sizeof(struct SkeletonNode));
    if (root_skeleton == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }

    skeleton->parent = root_skeleton;
    
    struct Node *leaf = malloc(sizeof(struct Node));
    if (leaf == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    struct NodeData *leaf_data = malloc(sizeof(struct NodeData));
    if (leaf_data == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }

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

    struct Node *root = malloc(sizeof(struct Node));
    if (root == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    struct NodeData *root_data = malloc(sizeof(struct NodeData));
    if (root_data == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }

    root->data = root_data;
    root_data->blank = 0;
    root_data->key = NULL;
    root_data->seed = NULL;
    root_data->id = rand();

    root_skeleton->node_id = root_data->id;
    root_skeleton->node = root;
    root_skeleton->ciphertexts = NULL;
    
    struct Node **root_children = malloc(sizeof(struct Node *) * 2);
    if (root_children == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
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

  skeleton->node_id = ((struct NodeData *) node)->id;
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

struct SkeletonNode *augment_blanks_build_skel(struct Node *node, struct Node *child, struct SkeletonNode *child_skel) {
  if (node != NULL) {
    struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
    if (skeleton == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    int *children_color = malloc(sizeof(int) * 2);
    if (children_color == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    skeleton->children_color = children_color;
    skeleton->node_id = ((struct NodeData *) node->data)->id;
    skeleton->node = node;
    skeleton->ciphertexts = NULL;

    int child_pos;
    if (child == *(node->children)) {
      child_pos = 0;
    } else {
      child_pos = 1;
    }
    if (((struct NodeData *)child->data)->blank == 1)
      *(children_color + child_pos) = -1; // -1 means do nothing
    else
      *(children_color + child_pos) = 0;
    *(children_color + (1 - child_pos)) = 1;
    
    if (child_skel != NULL) {
      child_skel->parent = skeleton;
      
      struct SkeletonNode **skel_children = malloc(sizeof(struct skeletonNode *) * 2);
      if (skel_children == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      skeleton->children = skel_children;
      
      *(skel_children + child_pos) = child_skel;
      *(skel_children + (1 - child_pos)) = NULL;
    } else {
      skeleton->children = NULL;
    }
      
    if ((*(node->children+1))->rightmost_blank == NULL)
      node->rightmost_blank = (*(node->children))->rightmost_blank;
    else
      node->rightmost_blank = (*(node->children+1))->rightmost_blank;
    return augment_blanks_build_skel(node->parent, node, skeleton);
  }
  return child_skel;
}

struct AddRet lbbt_add(void *tree, int id) {
  struct AddRet ret = { NULL, NULL };
  
  struct LBBT *lbbt = (struct LBBT *) tree;
  struct Node *new_leaf = NULL; // pointer to new leaf
  struct SkeletonNode *skeleton = NULL;
  switch (lbbt->add_strat) {
  case 0: //greedy
    if (!isEmptyList(lbbt->blanks)) {
	new_leaf = (struct Node *) popFront(lbbt->blanks);
	struct NodeData *data = (struct NodeData *)new_leaf->data;
	data->id = id;
	data->blank = 0;
	new_leaf->rightmost_blank = NULL;

	struct SkeletonNode *leaf_skeleton = malloc(sizeof(struct SkeletonNode));
	if (leaf_skeleton == NULL) {
	  perror("malloc returned NULL");
	  return ret;
	}
	leaf_skeleton->node_id = data->id;
	leaf_skeleton->node = new_leaf;
	leaf_skeleton->children = NULL;
	leaf_skeleton->children_color = NULL;
	leaf_skeleton->ciphertexts = NULL;
	skeleton = augment_blanks_build_skel(new_leaf->parent, new_leaf, leaf_skeleton);
	skeleton->parent = NULL;
      }
    break;
  case 1: //random
    break;
  case 2: //append
    break;
  }
  if (new_leaf == NULL) {
    skeleton = lbbt_append(lbbt, lbbt->root, id, &new_leaf); // returns new root
    skeleton->parent = NULL;
    lbbt->rightmost_leaf = new_leaf;
  }

  ret.added = new_leaf;
  ret.skeleton = skeleton;
  return ret;
}

// TODO: optimize if right child subtree is all blanks??? -- prob wouldn't do much since O(log n) anyway
// on_dir_path = 1 means node is on the direct path of the removed leaf (rightmost leaf by construction)
struct TruncRet truncate(struct LBBT *lbbt, struct Node *node, int on_dir_path) {
  struct TruncRet ret = { NULL, NULL };
  struct NodeData *data = (struct NodeData *) node->data;
  if (data->blank == 1) {
    popBack(lbbt->blanks);
    /*free(data->key);    
    free(data->seed);
    free(data);
    free(node);*/
    return ret;
  }
  if (node->children == NULL) {
    lbbt->rightmost_leaf = node;
    ret.node = node;
    return ret;
  }

  struct TruncRet trunc_ret = truncate(lbbt, *(node->children+1), on_dir_path);
  if (trunc_ret.node != NULL) {
    struct Node *trunc_child = trunc_ret.node;
    *(node->children+1) = trunc_child;
    node->num_leaves = (*(node->children))->num_leaves + trunc_child->num_leaves;
    trunc_child->parent = node;
    if (!on_dir_path) {
      ret.node = node;
      return ret;
    }
    
    struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
    if (skeleton == NULL) {
      perror("malloc returned NULL");
      return ret;
    }
    int *children_color = malloc(sizeof(int) * 2);
    if (children_color == NULL) {
      perror("malloc returned NULL");
      return ret;
    }
    skeleton->node_id = ((struct NodeData *) node->data)->id;
    skeleton->node = node;
    skeleton->children_color = children_color;
    skeleton->ciphertexts = NULL;
    
    if (trunc_ret.skeleton == NULL) {
      *children_color++ = 1;
      *children_color-- = 1;
      skeleton->children = NULL;
    } else {
      struct SkeletonNode **skel_children = malloc(sizeof(struct skeletonNode *) * 2);
      trunc_ret.skeleton->parent = skeleton;
      if (skel_children == NULL) {
	perror("malloc returned NULL");
	return ret;
      }
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

  trunc_ret = truncate(lbbt, *(node->children), 0);
  struct Node *replacement = trunc_ret.node;
  if (node == lbbt->root) {
    lbbt->root = replacement;
    replacement->parent = NULL;
  }
  /*free(node->children);
  free(data->key);
  free(data->seed);
  free(data);
  free(node);*/
  ret.node = replacement;
  return ret;
}

struct ListNode *find_prev_blank(struct Node *node, struct Node *prev_node) {
  if (node == NULL)
    return NULL;
  struct ListNode *candidate = (*(node->children))->rightmost_blank;
  if (candidate == NULL || *(node->children) == prev_node) // TODO: check this works (think its ok but check when have identifiers for leaves)
    return find_prev_blank(node->parent, node);
  return candidate;
}

struct RemRet lbbt_rem(void *tree, struct Node *node) {
  struct LBBT *lbbt = (struct LBBT *) tree;
  struct RemRet ret = { NULL, NULL };
  
  switch (lbbt->trunc_strat) {
  case 0: //truncate
    if (node == lbbt->root) {
      perror("Cannot delete root");
    }
    ret.data = node->data;
    struct NodeData *data = (struct NodeData *) node->data;
    data->blank = 1;
    struct ListNode *prev_blank = find_prev_blank(node->parent, node);
    struct ListNode *new_list_node = addAfter(lbbt->blanks, prev_blank, node);
    node->rightmost_blank = new_list_node;

    // TODO: check aug_blanks works once have identifiers for leaves; more efficient?? (NOTE need augment after truncate too!)
    if (node != lbbt->rightmost_leaf) {
      /*free(data->key);
      data->key = NULL;
      free(data->seed);
      data->seed = NULL;*/
      ret.skeleton = augment_blanks_build_skel(node->parent, node, NULL);
      ret.skeleton->parent = NULL;
    } else {
      struct TruncRet trunc_ret = truncate(lbbt, lbbt->root, 1);
      struct SkeletonNode *trunc_skel = trunc_ret.skeleton;
      if (trunc_skel == NULL) {
	trunc_skel = malloc(sizeof(struct SkeletonNode));
	if (trunc_skel == NULL) {
	  perror("malloc returned NULL");
	  return ret;
	}
	if (lbbt->root->num_leaves > 1) {
	  int *children_color = malloc(sizeof(int) * 2);
	  if (children_color == NULL) {
	    perror("malloc returned NULL");
	    return ret;
	  }
	  trunc_skel->children_color = children_color;
	  *children_color++ = 1;
	  *children_color-- = 1;
	} else {
	  trunc_skel->children_color = NULL;
	}
	trunc_skel->node_id = ((struct NodeData *) lbbt->root->data)->id;
	trunc_skel->node = lbbt->root;
	trunc_skel->children = NULL;
	trunc_skel->ciphertexts = NULL;
      }
      trunc_skel->parent = NULL;
      ret.skeleton = trunc_skel;
      struct SkeletonNode *skeleton = augment_blanks_build_skel(lbbt->rightmost_leaf->parent, lbbt->rightmost_leaf, NULL);
      if (skeleton != NULL)
	destroy_skeleton(skeleton);
    }
    break;
  case 1: //keep
    break;
  }

  return ret;
}
