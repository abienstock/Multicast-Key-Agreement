#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "trees.h"
#include "../../ll/ll.h"
#include "../../utils.h"

int compareIds(const void *data1, const void *data2) { //TODO: data1 id, data2 user node
  if (*(int *) data1 == *(int *) (((struct Node *) data2)->data))
    return 0;
  return 1;
}

static void printIntLine(void *p)
{
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  if (((struct LBBTNodeData *) data->tree_node_data)->blank == 1)
    printf("BLANK, id: %d", data->id);
  else
    printf("%d", data->id);
}

int main() {
  int n = 7;
  int a1 = 5;
  int a2 = 3;

  int ids[n];
  int i;
  for (i = 0; i < n; i++) {
    ids[i] = i;
  }

  int id;

  printf("testing init: \n");

  struct List *users = malloc_check(sizeof(struct List));
  initList(users);

  struct LBBT *lbbt = (struct LBBT *) lbbt_init(ids, n, 0, 0, users).tree;
  pretty_traverse_tree(lbbt, lbbt->root, 0, &printIntLine);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);  

  printf("\n\n\n==================================\n");
  printf("testing add: \n");

  for (i = n; i < n+a1; i++) {
    printf("add:\n");
    struct Node *added = lbbt_add(lbbt, i).added;
    addFront(users, (void *) added);
    printf("added: %d\n", *(int *)added->data);
    pretty_traverse_tree(lbbt, lbbt->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
    printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);
  }

  printf("\n\n\n==================================\n");
  printf("testing rem \n");
  printf("node to be removed (in ascending order of time in tree): %d\n", n);
  struct Node *rem = (struct Node *) findAndRemoveNode(users, n);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);  
  id = lbbt_rem((void *) lbbt, rem).id;
  printf("removed node data: %d\n", id);
  pretty_traverse_tree(lbbt, lbbt->root, 0, &printIntLine);
  printf("\n traverse blank list \n");
  traverseList(lbbt->blanks, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);

  printf("\n\n\n==================================\n");
  printf("testing add in blank \n");
  struct Node *added = lbbt_add(lbbt, n+a1).added;
  addFront(users, (void *) added);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);    

  printf("\n traverse blank list \n");
  traverseList(lbbt->blanks, &printIntLine);

  pretty_traverse_tree(lbbt, lbbt->root, 0, &printIntLine);
  printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);

  printf("\n\n\n==================================\n");
  printf("testing truncate\n");
  for (i = n+a1+1; i < n+a1+a2+1; i++) {
    printf("add:\n");
    struct Node *added = lbbt_add(lbbt, i).added;
    printf("added: %d\n", *(int *)added->data);
    addFront(users, (void *) added);    
    pretty_traverse_tree(lbbt, lbbt->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
    printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);
  }

  int remove_nodes[3] = {1, 1, 0};
  for (i=0; i<a2; i++) {
    rem = (struct Node *) findAndRemoveNode(users, remove_nodes[i]);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);
    id = lbbt_rem((void *) lbbt, rem).id;
    printf("removed node data: %d\n", id);
    pretty_traverse_tree(lbbt, lbbt->root, 0, &printIntLine);    
    printf("\n traverse blank list \n");
    traverseList(lbbt->blanks, &printIntLine);
    printf("rightmost leaf: %d\n", *(int *) lbbt->rightmost_leaf->data);
  }

  free_tree(lbbt->root);
  removeAllNodes(lbbt->blanks);
  free(lbbt->blanks);
  free(lbbt);

  removeAllNodes(users);
  free(users);

  return 0;
}
