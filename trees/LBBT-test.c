#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "trees.h"
#include "ll.h"

static void printIntLine(void *p)
{
  if (p == NULL)
    printf("blank \n");
  else
    printf("%d \n", *(int *)p);
}

int main() {
  int n = 11;
  int i;
  void **ids = malloc(sizeof(void *) * n);
  for (i = 0; i < n; i++) {
    int *dub = malloc(sizeof(int));
    *dub = (int) i;
    *(ids + i) = (void *) dub;
  }

  void *data;
  struct Node *node;

  struct LBBT *lbbt = lbbt_init(ids, n, 0, 0);
  pretty_traverse_tree(lbbt->root, 0, &printIntLine);
  destroy_tree(lbbt->root);
  removeAllNodes(lbbt->blanks);
  free(lbbt->blanks);
  free(lbbt);

  free(ids);

  return 0;
}
