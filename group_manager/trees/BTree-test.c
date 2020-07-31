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

static void printIntLine(void *p) {
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  struct BTreeNodeData *btree_data = (struct BTreeNodeData *) data->tree_node_data;
  printf("id: %d, height: %d, opt add child: %d, lowest nonfull: %d, num leaves: %d", data->id, btree_data->height, btree_data->opt_add_child, btree_data->lowest_nonfull, ((struct Node *) p)->num_leaves);
}

int main() {
  int n = 1;
  int a1 = 9;
  int a2 = 3;
  int r = 6;
  int order = 3;

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
  pretty_traverse_tree(btree, btree->root, 0, &printIntLine);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);

  printf("\n\n\n==================================\n");
  printf("testing add: \n");

  for (i = n; i < n+a1; i++) {
    printf("\nadd:\n");
    struct Node *added = btree_add(btree, i).added;
    addFront(users, (void *) added);
    printf("added: %d\n", *(int *)added->data);
    pretty_traverse_tree(btree, btree->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
  }

  printf("\n\n\n==================================\n");
  printf("testing rem \n");
  printf("node to be removed (in ascending order of time in tree): %d\n", n);
  struct Node *rem = (struct Node *) findAndRemoveNode(users, n);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);  
  id = btree_rem((void *) btree, rem).id;
  printf("\nremoved node data: %d\n", id);
  pretty_traverse_tree(btree, btree->root, 0, &printIntLine);

  printf("\n\n\n==================================\n");
  printf("testing add in blank \n");
  struct Node *added = btree_add(btree, n+a1).added;
  addFront(users, (void *) added);
  printf("traverse users list:\n");
  traverseList(users, &printIntLine);    

  pretty_traverse_tree(btree, btree->root, 0, &printIntLine);

  printf("\n\n\n==================================\n");
  printf("testing truncate\n");
  for (i = n+a1+1; i < n+a1+a2+1; i++) {
    printf("\nadd:\n");
    struct Node *added = btree_add(btree, i).added;
    printf("added: %d\n", *(int *)added->data);
    addFront(users, (void *) added);    
    pretty_traverse_tree(btree, btree->root, 0, &printIntLine);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);    
  }

  //int remove_nodes[3] = {1, 1, 0};
  for (i=0; i<r; i++) {
    //rem = (struct Node *) findAndRemoveNode(users, remove_nodes[i]);
    rem = (struct Node *) findAndRemoveNode(users, 0);
    printf("traverse users list:\n");
    traverseList(users, &printIntLine);
    id = btree_rem((void *) btree, rem).id;
    printf("\nremoved node id: %d\n", id);
    pretty_traverse_tree(btree, btree->root, 0, &printIntLine);    
  }

  //free_tree(btree->root);
  free(btree);

  removeAllNodes(users);
  free(users);
  return 0;
}
