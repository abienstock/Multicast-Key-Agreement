#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "trees.h"
#include "ll.h"
 
// TODO: documentation
 
struct SkeletonNode *init_perfect(int h, int leftmost_id, void **ids, struct List *users) {
   struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
   if (skeleton == NULL) {
     perror("malloc returned NULL");
     return NULL;
   }
  
   struct Node *root = malloc(sizeof(struct Node));
   if (root == NULL) {
     perror("malloc returned NULL");
     return NULL;
   }

   skeleton->node = root;   
   
   root->num_leaves = 1 << h;	
   root->rightmost_blank = NULL;
   
   if (h == 0) {
     skeleton->children_color = NULL;
     skeleton->children = NULL;
     
     root->data = *(ids+leftmost_id);     
     root->children = NULL;
     addFront(users, (void *) root);
     return skeleton;
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
   
   *children_color++ = 0;
   *children_color-- = 1;
   skeleton->children_color = children_color;   

   int *data = malloc(sizeof(int));
   if (data == NULL) {
     perror("malloc returned NULL");
     return NULL;		
   }
   *data = rand();
   root->data = data;
   
   struct Node **children = malloc(sizeof(struct Node *) * 2);
   if (children == NULL) {
     perror("malloc returned NULL");
     return NULL;
   }
   struct SkeletonNode *left = init_perfect(h-1, leftmost_id, ids, users);
   struct SkeletonNode *right = init_perfect(h-1, leftmost_id + (1 << (h-1)), ids, users);

   //TODO: get rid of node children??
   *skel_children++ = left;
   *skel_children-- = right;
   skeleton->children = skel_children;
   
   *children++ = left->node;
   *children-- = right->node;
   left->node->parent = root;
   right->node->parent = root;
   
   root->children = children;
   
   return skeleton;
 }

struct SkeletonNode *root_init(int n, int leftmost_id, void **ids, struct List *users){
  struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
  if (skeleton == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  
  struct Node *root = malloc(sizeof(struct Node));
  if (root == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }

  skeleton->node = root;
  
  root->num_leaves = n;
  root->rightmost_blank = NULL;
  
  if (n > 1) {
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

    *children_color++ = 0;
    *children_color-- = 1;
    skeleton->children_color = children_color;    
    
    int *data = malloc(sizeof(int));
    if (data == NULL) {
      perror("malloc returned NULL");
      return NULL;		
    }
    *data = rand();
    root->data = data;
    // n-1???
    struct Node **children = malloc(sizeof(struct Node *) * 2);
    if (children == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    double h = log2(n);
    double h_flr = floor(h);

    struct SkeletonNode *left, *right;
    if (h-h_flr == 0) {
      left = init_perfect((int) h-1, leftmost_id, ids, users);
      right = init_perfect((int) h-1, leftmost_id + (1 << (int) (h-1)), ids, users);
    } else {
      left = init_perfect((int) h_flr, leftmost_id, ids, users);
      right = root_init(n - (1 << (int) h_flr), leftmost_id  + (1 << (int) h_flr), ids, users);
    }
    //TODO: get rid of node children??
    *skel_children++ = left;
    *skel_children-- = right;
    skeleton->children = skel_children;
    
    *children++ = left->node;
    *children-- = right->node;
    left->node->parent = root;
    right->node->parent = root;
    
    root->children = children;
  } else {
    skeleton->children_color = NULL;
    skeleton->children = NULL;
    
    root->data = *(ids+leftmost_id);
    root->children = NULL;
    addFront(users, (void *) root);
  }
  
  return skeleton;
}

//TODO: takes in users???
struct InitRet lbbt_init(void **ids, int n, int add_strat, int trunc_strat, struct List *users) {
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

  struct SkeletonNode *skeleton = root_init(n, 0, ids, users);

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

struct Node *lbbt_append(struct LBBT *lbbt, struct Node *node, void *data, struct Node **new_leaf) {
  if (is_perfect(node)) {
    struct Node *leaf = malloc(sizeof(struct Node));
    if (leaf == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    
    leaf->data = data;
    leaf->children = NULL;
    leaf->num_leaves = 1;
    leaf->rightmost_blank = NULL;
    
    struct Node *root = malloc(sizeof(struct Node));
    if (root == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    
    int *root_data = malloc(sizeof(int));
    if (root_data == NULL) {
      perror("malloc returned NULL");
      return NULL;		
    }
    *root_data = rand();
    root->data = root_data;
    
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
    return root;
  }
  struct Node *new_right = lbbt_append(lbbt, *(node->children+1), data, new_leaf); 
  *(node->children+1) = new_right;
  node->num_leaves = (*(node->children))->num_leaves + new_right->num_leaves;
  new_right->parent = node;
  return node;
}

void augment_blanks(struct Node *node) {
  if (node != NULL) {
    if ((*(node->children+1))->rightmost_blank == NULL)
      node->rightmost_blank = (*(node->children))->rightmost_blank;
    else
      node->rightmost_blank = (*(node->children+1))->rightmost_blank;
    augment_blanks(node->parent);
  }
}

struct AddRet lbbt_add(void *tree, void *data) {
  struct LBBT *lbbt = (struct LBBT *) tree;
  struct Node *new_leaf = NULL; // pointer to new leaf
  switch (lbbt->add_strat) {
  case 0: //greedy
    if (!isEmptyList(lbbt->blanks)) {
	new_leaf = (struct Node *) popFront(lbbt->blanks);
	new_leaf->data = data;
	new_leaf->rightmost_blank = NULL;
	augment_blanks(new_leaf->parent);
	//printf("not implemented yet.");
      }
    break;
  case 1: //random
    break;
  case 2: //append
    break;
  }
  if (new_leaf == NULL) {
    lbbt_append(lbbt, lbbt->root, data, &new_leaf); // returns new root
    lbbt->rightmost_leaf = new_leaf;
  }

  struct AddRet ret = { new_leaf, NULL };
  return ret;
}

// TODO: optimize if right child subtree is all blanks??? -- prob wouldn't do much since O(log n) anyway
struct Node *truncate(struct LBBT *lbbt, struct Node *node) {
  if (node->data == NULL) {
    popBack(lbbt->blanks);
    free(node);
    return NULL;
  }
  if (node->children == NULL) {
    lbbt->rightmost_leaf = node;
    return node;
  }

  struct Node *trunc_child = truncate(lbbt, *(node->children+1));
  if (trunc_child != NULL) {
    *(node->children+1) = trunc_child; // This is probably faster than having an if statement??
    node->num_leaves = (*(node->children))->num_leaves + trunc_child->num_leaves;
    trunc_child->parent = node;
    //augment_blanks(node); // TODO: check this correct (I think it is)
    return node;
  }
  
  struct Node *replacement = truncate(lbbt, *(node->children));
  if (node == lbbt->root) {
    lbbt->root = replacement;
    replacement->parent = NULL;
  }
  free(node->children);
  free(node->data);
  free(node);
  return replacement;	
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
  void *data = NULL;
  
  switch (lbbt->trunc_strat) {
  case 0: //truncate
    if (node == lbbt->root) {
      perror("Cannot delete root");
    }
    //free(node->data); // TODO: depends on data alloc strat .. for now not (handled by user)
    data = node->data;
    node->data = NULL;
    struct ListNode *prev_blank = find_prev_blank(node->parent, node);
    struct ListNode *new_list_node = addAfter(lbbt->blanks, prev_blank, node);
    node->rightmost_blank = new_list_node;

    // TODO: check all of this works once have identifiers for leaves
    if (node != lbbt->rightmost_leaf)
      augment_blanks(node->parent);
    else {
      truncate(lbbt, lbbt->root);
      augment_blanks(lbbt->rightmost_leaf->parent);
    }
    break;
  case 1: //keep
    data = NULL;
    break;
  }

  struct RemRet ret = { data, NULL };
  return ret;
}
