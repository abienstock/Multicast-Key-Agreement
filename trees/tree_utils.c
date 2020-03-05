#include <stdlib.h>
#include "trees.h"

void traverse_tree(struct Node *root, void (*f)(void *)) {
  if (root->children == NULL)
    f(root->data);
  else {
    traverse_tree(*(root->children), f);
    f(root->data);
    traverse_tree(*(root->children+1), f);
  }
}
