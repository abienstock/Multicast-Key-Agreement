#include <stdlib.h>
#include <stdio.h>
#include "trees.h"
#define COUNT 20

void traverse_tree(struct Node *root, void (*f)(void *)) {
  if (root->children == NULL) {
    printf("leaf");
    f(root);
  } else {
    traverse_tree(*(root->children), f);
    printf("node");
    f(root);
    traverse_tree(*(root->children+1), f);
  }
}

//destroys all data too
void destroy_tree(struct Node *root) {
  if (root->children != NULL) {
    destroy_tree(*(root->children));
    destroy_tree(*(root->children +1));
    free(root->children);
  }
  if (root->data != NULL)
    free(root->data);
  free(root);
}

void destroy_skeleton(struct SkeletonNode *root) {
  if (root->children != NULL) {
    if (*(root->children) != NULL)
      destroy_skeleton(*(root->children));
    if (*(root->children + 1) != NULL)
      destroy_skeleton(*(root->children +1));
    free(root->children);
    free(root->children_color);
  }
  free(root);
}

void pretty_traverse_tree(struct Node *root, int space, void (*f)(void *)) {
  int i;
  space += COUNT;
  if (root->children == NULL) {
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    if (root->parent != NULL) {
      printf(" parent: ");
      f(root->parent);
    }
    if (root->rightmost_blank != NULL) {
      printf(" rightmost blank: ");
      f(root->rightmost_blank->data);
    } else {
      printf(" no rightmost blank");
    }
    printf("\n");    
  } else {
    pretty_traverse_tree(*(root->children+1), space, f);
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    if (root->parent != NULL) {
      printf(" parent: ");
      f(root->parent);
    }
    if (root->rightmost_blank != NULL) {
      printf(" rightmost blank: ");
      f(root->rightmost_blank->data);
    } else {
      printf(" no rightmost blank");
    }
    printf("\n");    
    pretty_traverse_tree(*(root->children), space, f);
  }
}

void pretty_traverse_skeleton(struct SkeletonNode *root, int space, void (*f)(void *)) {
  int i;
  space += COUNT;
  if (root->children == NULL) {
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    printf("\n");
  } else {
    pretty_traverse_skeleton(*(root->children+1), space, f);
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    printf("\n");
    pretty_traverse_skeleton(*(root->children), space, f);
  }
}

struct InitRet gen_tree_init(void **ids, int n, int add_strat, int trunc_strat, struct List *users, struct InitRet (*tree_init)(void **, int, int, int, struct List *)) {
  return tree_init(ids, n, add_strat, trunc_strat, users);
}

struct AddRet gen_tree_add(void *tree, void *data, struct AddRet (*tree_add)(void *, void *)) {
  return tree_add(tree, data);
}

struct RemRet gen_tree_rem(void *tree, struct Node *node, struct RemRet (*tree_rem)(void *, struct Node *)) {
  return tree_rem(tree, node);
}

struct UpdRet gen_tree_upd(void *tree, struct Node *node) {
  printf("Nothing yet\n");
  struct UpdRet ret = { NULL };
  return ret;
}
