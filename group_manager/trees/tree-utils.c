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
  free(data->tree_node_data);
  free(data);
  if (node->children != NULL)
    free(node->children);
  free(node);
}

//destroys all data too
void free_tree(struct Node *root) {
  if (root->children != NULL) {
    int i;
    for (i = 0; i < root->num_children; i++)
      free_tree(*(root->children + i));
  }
  free_node(root);
}

void free_skeleton(struct SkeletonNode *root, int is_root, int crypto) {
  int i;  
  if (root->children != NULL) {
    for (i = 0; i < root->node->num_children; i++) {
      if (*(root->children + i) != NULL)
	free_skeleton(*(root->children + i), is_root, crypto);
    }
    free(root->children);
  }
  if (root->children_color != NULL)
    free(root->children_color);
  if (crypto) {
    if (is_root) {
      free((*(root->ciphertexts))->ct);
      free(*root->ciphertexts);
      free(root->ciphertexts);
    } else if (root->ciphertexts != NULL) {
      for (i = 0; i < root->node->num_children; i++) {
	if (*(root->ciphertexts + i) != NULL) {
	  free((*(root->ciphertexts + i))->ct);
	  free(*(root->ciphertexts + i));
	}
      }
      free(root->ciphertexts);    
    }
  }
  free(root);
}

void pretty_traverse_tree(void *tree, struct Node *root, int space, void (*f)(void *)) {
  struct BTree *btree = (struct BTree *) tree;
  
  int i;
  space += COUNT;
  if (root == NULL) {
    for (i = COUNT; i < space; i++)
      printf(" ");    
    printf("Empty Tree!\n");
  } else if (root->children == NULL) {
    printf("\n");
    for (i = COUNT; i < space; i++)
      printf(" ");
    f(root);
    if (root->parent != NULL) {
      printf(" parent: %d", ((struct NodeData *) root->parent->data)->id);
    }
    /*struct LBBTNodeData *lbbt_node_data = (struct LBBTNodeData *) ((struct NodeData *) root->data)->tree_node_data;
    if (lbbt_node_data->rightmost_blank != NULL) {
      printf(" rightmost blank: %d", ((struct NodeData *) lbbt_node_data->rightmost_blank->data)->id);
    } else {
      printf(" no rightmost blank");
      }*/
    printf("\n");    
  } else {
    for (i = 0; i < btree->order; i++) {
      pretty_traverse_tree(tree, *(root->children + i), space, f); // TODO: fix
      printf("\n");
      if (i == 0) {
	int j;
	for (j = COUNT; j < space; j++)
	  printf(" ");
	f(root);
	if (root->parent != NULL) {
	  printf(" parent: %d", ((struct NodeData *) root->parent->data)->id);
	}
      }
    }
    /*for (i = 0; i < root->num_children; i++) {
      pretty_traverse_tree(*(root->children + i), space, f); // TODO: fix
      printf("\n");
      if (i == 0) {
	int j;
	for (j = COUNT; j < space; j++)
	  printf(" ");
	f(root);
	if (root->parent != NULL) {
	  printf(" parent: %d", ((struct NodeData *) root->parent->data)->id);
	}
      }
      }*/
    /*struct LBBTNodeData *lbbt_node_data = (struct LBBTNodeData *) ((struct NodeData *) root->data)->tree_node_data;
    if (lbbt_node_data->rightmost_blank != NULL) {
      printf(" rightmost blank: %d", ((struct NodeData *) lbbt_node_data->rightmost_blank->data)->id);
    } else {
      printf(" no rightmost blank");
      }*/
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
    for (i = 0; i < root->node->num_children; i++) {
      if (*(root->children + i) != NULL) {
	pretty_traverse_skeleton(*(root->children + i), space, f);
      }
      printf("\n");
      if (i == 0) {
	int j;
	for (j = COUNT; j < space; j++)
	  printf(" ");
	f(root);
      }
    }
  }
}
