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
void destroy_tree(struct Node *root){
  if (root->children != NULL) {
    destroy_tree(*(root->children));
    destroy_tree(*(root->children +1));
    free(root->children);
  }
  free(root->data);
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
  } else {
    pretty_traverse_tree(*(root->children+1), space, f);
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    pretty_traverse_tree(*(root->children), space, f);
  }
}

