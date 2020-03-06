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
    printf("blank \n");
  else
    printf("%d \n", *(int *)node->data);
}

int main() {
  int n = 7;
  int a1 = 5;
  int a2 = 3;
  int i;
  void **ids = malloc(sizeof(void *) * n);
  if (ids == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  for (i = 0; i < n; i++) {
    int *data = malloc(sizeof(int));
    if (data == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    *data = i;
    *(ids + i) = (void *) data;
  }

  void *data;

  printf("testing init: \n");

  struct List *users = malloc(sizeof(struct List));
  if (users == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  initList(users);

  struct LBBT *lbbt = lbbt_init(ids, n, 0, 0, users);
  pretty_traverse_tree(lbbt->root, 0, &printIntLine);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);
  

  printf("\n\n\n==================================\n");
  printf("testing add: \n");

  for (i = n; i < n+a1; i++) {
    printf("add:\n");
    int *add_data = malloc(sizeof(int));
    if (add_data == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    *add_data = i;
    struct Node *added = lbbt_add(lbbt, (void *) add_data);
    addFront(users, (void *) added);
    printf("added: %d\n", *(int *)added->data);
    pretty_traverse_tree(lbbt->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
  }

  printf("\n\n\n==================================\n");
  printf("testing rem \n");
  printf("node to be removed: %d\n", n);
  struct Node *rem = (struct Node *) findAndRemoveNode(users, (void *)&n, compareIds);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);  
  data = lbbt_rem((void *) lbbt, rem);
  printf("removed node data: %d\n", *(int *) data);
  free(data);
  pretty_traverse_tree(lbbt->root, 0, &printIntLine);
  printf("\n traverse blank list \n");
  traverseList(lbbt->blanks, &printIntLine);

  printf("\n\n\n==================================\n");
  printf("testing add in blank \n");
  int *add_data = malloc(sizeof(int));
  if (add_data == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  *add_data = n+a1;
  struct Node *added = lbbt_add(lbbt, (void *) add_data);
  addFront(users, (void *) added);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);    

  printf("\n traverse blank list \n");
  traverseList(lbbt->blanks, &printIntLine);

  pretty_traverse_tree(lbbt->root, 0, &printIntLine);

  printf("\n\n\n==================================\n");
  printf("testing truncate\n");
  int removed_nodes[a2];
  for (i = n+a1+1; i < n+a1+a2+1; i++) {
    printf("add:\n");
    int *add_data = malloc(sizeof(int));
    if (add_data == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    *add_data = i;
    struct Node *added = lbbt_add(lbbt, (void *) add_data);
    printf("added: %d\n", *(int *)added->data);
    addFront(users, (void *) added);    
    removed_nodes[i-n-a1-1] = i;
    pretty_traverse_tree(lbbt->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
  }

  int temp = removed_nodes[1];
  removed_nodes[1] = removed_nodes[0];
  removed_nodes[0] = temp;

  for (i=0; i<a2; i++) {
    rem = (struct Node *) findAndRemoveNode(users, (void *)(removed_nodes + i), compareIds);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);      
    data = lbbt_rem((void *) lbbt, rem);
    printf("removed node data: %d\n", *(int *) data);
    printf("\n traverse blank list \n");
    traverseList(lbbt->blanks, &printIntLine);
    free(data);
  }

  pretty_traverse_tree(lbbt->root, 0, &printIntLine);

  destroy_tree(lbbt->root);
  removeAllNodes(lbbt->blanks);
  free(lbbt->blanks);
  free(lbbt);

  free(ids);

  return 0;
}
