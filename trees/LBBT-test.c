#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "trees.h"
#include "ll.h"

int compareIds(const void *data1, const void *data2) { //TODO: data1 id, data2 user node
  if (*(int *) data1 == *(int *) (((struct Node *) data2)->data))
    return 0;
  return 1;
}

static void printIntLine(void *p)
{
  struct Node *node = (struct Node *) p;
  if (node->data == NULL)
    printf("blank");
  else
    printf("%d", *(int *)node->data);
}

int main() {
  int n = 7;
  int a1 = 5;
  int a2 = 3;
  int i;
  void **ids = malloc(sizeof(void *) * n);
  if (ids == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  for (i = 0; i < n; i++) {
    int *data = malloc(sizeof(int));
    if (data == NULL) {
      perror("malloc returned NULL");
      return -1;
    }
    *data = i;
    *(ids + i) = (void *) data;
  }

  void *data;

  printf("testing init: \n");

  struct List *users = malloc(sizeof(struct List));
  if (users == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  initList(users);

  struct LBBT *lbbt = lbbt_init(ids, n, 0, 0, users);
  pretty_traverse_tree(lbbt->root, 0, &printIntLine);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);  

  printf("\n\n\n==================================\n");
  printf("testing add: \n");

  for (i = n; i < n+a1; i++) {
    printf("add:\n");
    int *add_data = malloc(sizeof(int));
    if (add_data == NULL) {
      perror("malloc returned NULL");
      return -1;
    }
    *add_data = i;
    struct Node *added = lbbt_add(lbbt, (void *) add_data);
    addAfter(users, users->tail, (void *) added);
    printf("added: %d\n", *(int *)added->data);
    pretty_traverse_tree(lbbt->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
    printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);
  }

  printf("\n\n\n==================================\n");
  printf("testing rem \n");
  printf("node to be removed: %d\n", n);
  struct Node *rem = (struct Node *) findAndRemoveNode(users, n);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);  
  data = lbbt_rem((void *) lbbt, rem);
  printf("removed node data: %d\n", *(int *) data);
  free(data);
  pretty_traverse_tree(lbbt->root, 0, &printIntLine);
  printf("\n traverse blank list \n");
  traverseList(lbbt->blanks, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);

  printf("\n\n\n==================================\n");
  printf("testing add in blank \n");
  int *add_data = malloc(sizeof(int));
  if (add_data == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  *add_data = n+a1;
  struct Node *added = lbbt_add(lbbt, (void *) add_data);
  addAfter(users, users->tail, (void *) added);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);    

  printf("\n traverse blank list \n");
  traverseList(lbbt->blanks, &printIntLine);

  pretty_traverse_tree(lbbt->root, 0, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);

  printf("\n\n\n==================================\n");
  printf("testing truncate\n");
  for (i = n+a1+1; i < n+a1+a2+1; i++) {
    printf("add:\n");
    int *add_data = malloc(sizeof(int));
    if (add_data == NULL) {
      perror("malloc returned NULL");
      return -1;
    }
    *add_data = i;
    struct Node *added = lbbt_add(lbbt, (void *) add_data);
    printf("added: %d\n", *(int *)added->data);
    addAfter(users, users->tail, (void *) added);    
    pretty_traverse_tree(lbbt->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
    printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);
  }

  int remove_nodes[3] = {2, 2, 1};
  for (i=0; i<a2; i++) {
    rem = (struct Node *) findAndRemoveNode(users, users->len-remove_nodes[i]);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);      
    data = lbbt_rem((void *) lbbt, rem);
    printf("removed node data: %d\n", *(int *) data);
    pretty_traverse_tree(lbbt->root, 0, &printIntLine);    
    printf("\n traverse blank list \n");
    traverseList(lbbt->blanks, &printIntLine);
    printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);
    free(data);
  }

  destroy_tree(lbbt->root);
  removeAllNodes(lbbt->blanks);
  free(lbbt->blanks);
  free(lbbt);

  removeAllNodes(users);
  free(users);

  free(ids);

  return 0;
}
