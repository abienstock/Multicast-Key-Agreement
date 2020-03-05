#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "trees.h"
#include "ll.h"
 
// TODO: documentation
 
struct Node *init_perfect(int h, int leftmost_id, void **ids, struct List *nonblanks) {
   struct Node *root = malloc(sizeof(struct Node));
   if (root == NULL) {
     perror("malloc returned NULL");
     return NULL;
   }
   
   root->num_leaves = 1 << h;	
   root->rightmost_blank = NULL;
   
   if (h == 0) {
     root->data = *(ids+leftmost_id);     
     root->children = NULL;
     //addFront(nonblanks, (void *) root);
     return root;
   }

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
   struct Node *left = init_perfect(h-1, leftmost_id, ids, NULL);
   struct Node *right = init_perfect(h-1, leftmost_id + (1 << (h-1)), ids, NULL);
   *children++ = left;
   *children-- = right;
   left->parent = root;
   right->parent = root;
   
   root->children = children;
   
   return root;
 }

struct Node *root_init(int n, int leftmost_id, void **ids, struct List *nonblanks){
  struct Node *root = malloc(sizeof(struct Node));
  if (root == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  
  root->num_leaves = n;
  root->rightmost_blank = NULL;
  
  if (n > 1) {
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

    struct Node *left, *right;
    if (h-h_flr == 0) {
      left = init_perfect((int) h-1, leftmost_id, ids, NULL);
      right = init_perfect((int) h-1, leftmost_id + (1 << (int) (h-1)), ids, NULL);
    } else {
      left = init_perfect((int) h_flr, leftmost_id, ids, NULL);
      right = root_init(n - (1 << (int) h_flr), leftmost_id  + (1 << (int) h_flr), ids, NULL);
    }
    *children++ = left;
    *children-- = right;
    left->parent = root;
    right->parent = root;
    
    root->children = children;
  } else {
    root->data = *(ids+leftmost_id);
    root->children = NULL;
    //addFront(nonblanks, (void *) root); // TODO: make sure keeping tack of nonblanks correctly
  }
  
  return root;
}

void *lbbt_init(void **ids, int n, int add_strat, int trunc_strat) {
  if (n < 1) {
    perror("n has to be at least 1");
    return NULL;
  }
  struct LBBT *tree = malloc(sizeof(struct LBBT));
  if (tree == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  
  //struct List *nonblanks = malloc(sizeof(struct List));
  //if (nonblanks == NULL) {
  //	perror("malloc returned NULL");
  //	return NULL;
  //}
  //initList(nonblanks);
  //tree->nonblanks = nonblanks;
  
  struct Node *root = root_init(n, 0, ids, NULL);
  root->parent = NULL;
  
  tree->root = root;
  
  struct List *blanks = malloc(sizeof(struct List));
  if (blanks == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  initList(blanks);
  tree->blanks = blanks;
  tree->add_strat = add_strat;
  tree->trunc_strat = trunc_strat;
  return (void *) tree;
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
    //addFront(lbbt->nonblanks, (void *) leaf);
    
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
    if (node == lbbt->root) { //TODO: CHECK THIS COMPARISON IS CORRECT (should be good)
      root->parent = NULL;
      lbbt->root = root;
    }
    
    leaf->parent = root;
    node->parent = root;

    *new_leaf = leaf;
    return root;
  }
  struct Node *new_right = lbbt_append(lbbt, *(node->children+1), data, new_leaf); 
  *(node->children+1) = new_right; // TODO: check dereference correct (should be good)
  node->num_leaves = (*(node->children))->num_leaves + new_right->num_leaves;
  new_right->parent = node;
  return node;
}

void augment_blanks(struct Node *node) {
  if (node != NULL) {
    if ((*(node->children+1))->rightmost_blank == NULL) // TODO: check this works (should be good)
      node->rightmost_blank = (*(node->children))->rightmost_blank;
    else
      node->rightmost_blank = (*(node->children+1))->rightmost_blank;
    augment_blanks(node->parent);
  }
}

struct Node *lbbt_add(void *tree, void *data) {
  struct LBBT *lbbt = (struct LBBT *) tree;
  struct Node *new_leaf = NULL; // pointer to new leaf
  switch (lbbt->add_strat) {
  case 0: //greedy
    if (!isEmptyList(lbbt->blanks)) {
	new_leaf = (struct Node *) popFront(lbbt->blanks);
	new_leaf->data = data;
	new_leaf->rightmost_blank = NULL;
	augment_blanks(new_leaf->parent);
	//addFront(lbbt->nonblanks, (void *) blank);
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
  }
  return new_leaf;
}

// TODO: optimize if right child subtree is all blanks??? -- prob wouldn't do much since O(log n) anyway
struct Node *truncate(struct LBBT *lbbt, struct Node *node, struct Node *new_rightmost) {
  if (node->data == NULL) {
    popBack(lbbt->blanks);
    free(node);
    return NULL;
  }
  if (node->children == NULL) {
    new_rightmost = node;
    return node;
  }

  struct Node *trunc_child = truncate(lbbt, *(node->children+1), new_rightmost); //TODO: check this dereferencing is correct (should be good)
  if (trunc_child != NULL) {
    *(node->children+1) = trunc_child; // This is probably faster than having an if statement??
    node->num_leaves = (*(node->children))->num_leaves + trunc_child->num_leaves;
    trunc_child->parent = node;
    //augment_blanks(node); // TODO: check this correct (I think it is)
    return node;
  }
  
  struct Node *replacement = truncate(lbbt, *(node->children), new_rightmost);
  if (node == lbbt->root) {
    lbbt->root = replacement;
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
  if (candidate == NULL || *(node->children) == prev_node) // TODO: check this works (think its ok)
    return find_prev_blank(node->parent, node);
  return candidate;
}

void *lbbt_rem(void *tree, struct Node *node) {
  struct LBBT *lbbt = (struct LBBT *) tree;
  void *data;
  
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
    augment_blanks(node->parent);
    
    struct Node *new_rightmost = NULL; // rightmost leaf in tree (even blank)
    truncate(lbbt, lbbt->root, &new_rightmost);
    if(new_rightmost != NULL) { //TODO: more efficient way to do this??
      printf("new rightmost: %d\n", *(int *)new_rightmost->data);
      augment_blanks(new_rightmost);
    }
    break;
  case 1: //keep
    data = NULL;
    break;
  }
  return data;
}
