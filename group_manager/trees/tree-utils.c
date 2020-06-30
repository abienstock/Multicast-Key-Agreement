#include <stdlib.h>
#include <stdio.h>
#include "trees.h"
#include "../../skeleton.h"

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

void free_node(struct Node *node) {
  struct NodeData *data = (struct NodeData *) node->data;
  if (data->key != NULL) {
    free(data->key);
    free(data->seed);
  }
  free(data);
  if (node->children != NULL)
    free(node->children);
  free(node);
}

//destroys all data too
void free_tree(struct Node *root) {
  if (root->children != NULL) {
    free_tree(*(root->children));
    free_tree(*(root->children +1));
  }
  free_node(root);
}

void free_skeleton(struct SkeletonNode *root) {
  if (root->children != NULL) {
    if (*(root->children) != NULL)
      free_skeleton(*(root->children));
    if (*(root->children + 1) != NULL)
      free_skeleton(*(root->children +1));
    free(root->children);
  }
  if (root->children_color != NULL)
    free(root->children_color);
  if (root->ciphertexts != NULL) {
    if (*(root->ciphertexts) != NULL) {
      free((*(root->ciphertexts))->ct);
      free(*(root->ciphertexts));
    }
    if (*(root->ciphertexts + 1) != NULL) {
      free((*(root->ciphertexts + 1))->ct);
      free(*(root->ciphertexts + 1));
    }
    free(root->ciphertexts);
  }
  free(root);
}

void pretty_traverse_tree(struct Node *root, int space, void (*f)(void *)) {
  int i;
  space += COUNT;
  if (root == NULL) {
    printf("Empty skeleton!\n");
  } else if (root->children == NULL) {
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    if (root->parent != NULL) {
      printf(" parent: %d", ((struct NodeData *) root->parent->data)->id);
    }
    if (root->rightmost_blank != NULL) {
      printf(" rightmost blank: %d", ((struct NodeData *) root->rightmost_blank->data)->id);
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
      printf(" parent: %d", ((struct NodeData *) root->parent->data)->id);
    }
    if (root->rightmost_blank != NULL) {
      printf(" rightmost blank: %d", ((struct NodeData *) root->rightmost_blank->data)->id);
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
    if (*(root->children+1) != NULL) {
      pretty_traverse_skeleton(*(root->children+1), space, f);
    }
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    printf("\n");
    if (*(root->children) != NULL) {
      pretty_traverse_skeleton(*(root->children), space, f);
    }
  }
}
