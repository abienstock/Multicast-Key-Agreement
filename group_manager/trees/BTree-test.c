#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "trees.h"
#include "../../utils.h"

int compareIds(const void *data1, const void *data2) { //TODO: data1 id, data2 user node
  if (*(int *) data1 == *(int *) (((struct Node *) data2)->data))
    return 0;
  return 1;
}

static void printIntLine(void *p)
{
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  printf("%d", data->id);
}

int main() {
  int n = 17;
  int a1 = 5;
  int a2 = 3;
  int order = 4;

  int ids[n];
  int i;
  for (i = 0; i < n; i++) {
    ids[i] = i;
  }

  int id;

  printf("testing init: \n");

  struct List *users = malloc_check(sizeof(struct List));
  initList(users);

  struct BTree *btree = (struct BTree *) btree_init(ids, n, 0, order, users).tree;
  pretty_traverse_tree(btree->root, 0, &printIntLine);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);

  /*printf("\n\n\n==================================\n");
  printf("testing add: \n");

  for (i = n; i < n+a1; i++) {
    printf("add:\n");
    struct Node *added = btree_add(btree, i).added;
    addFront(users, (void *) added);
    printf("added: %d\n", *(int *)added->data);
    pretty_traverse_tree(btree->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
    printf("rightmost leaf: %d\n", *(int *) btree->rightmost_leaf->data);
  }

  printf("\n\n\n==================================\n");
  printf("testing rem \n");
  printf("node to be removed (in ascending order of time in tree): %d\n", n);
  struct Node *rem = (struct Node *) findAndRemoveNode(users, n);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);  
  id = btree_rem((void *) btree, rem).id;
  printf("removed node data: %d\n", id);
  pretty_traverse_tree(btree->root, 0, &printIntLine);
  printf("\n traverse blank list \n");
  traverseList(btree->blanks, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) btree->rightmost_leaf->data);

  printf("\n\n\n==================================\n");
  printf("testing add in blank \n");
  struct Node *added = btree_add(btree, n+a1).added;
  addFront(users, (void *) added);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);    

  printf("\n traverse blank list \n");
  traverseList(btree->blanks, &printIntLine);

  pretty_traverse_tree(btree->root, 0, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) btree->rightmost_leaf->data);

  printf("\n\n\n==================================\n");
  printf("testing truncate\n");
  for (i = n+a1+1; i < n+a1+a2+1; i++) {
    printf("add:\n");
    struct Node *added = btree_add(btree, i).added;
    printf("added: %d\n", *(int *)added->data);
    addFront(users, (void *) added);    
    pretty_traverse_tree(btree->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
    printf("rightmost leaf: %d\n", *(int *) btree->rightmost_leaf->data);
  }

  int remove_nodes[3] = {1, 1, 0};
  for (i=0; i<a2; i++) {
    rem = (struct Node *) findAndRemoveNode(users, remove_nodes[i]);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);
    id = btree_rem((void *) btree, rem).id;
    printf("removed node data: %d\n", id);
    pretty_traverse_tree(btree->root, 0, &printIntLine);    
    printf("\n traverse blank list \n");
    traverseList(btree->blanks, &printIntLine);
    printf("rightmost leaf: %d\n", *(int *) btree->rightmost_leaf->data);
  }

  free_tree(btree->root);
  removeAllNodes(btree->blanks);
  free(btree->blanks);
  free(btree);

  removeAllNodes(users);
  free(users);
  */
  return 0;
}
