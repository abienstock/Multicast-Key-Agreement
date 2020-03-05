#include <assert.h>
#include "trees.h"

static void printDouble(void *p)
{
  printf("%.1f ", *(double *)p);
}

int main() {
  double a[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
  int n = sizeof(a)/sizeof(a[0]);

  int i;
  void *data;
  struct Node *node;

  struct LBBT *lbbt = lbbt_init((void **)a, 0, 0);
  traverse_tree(lbbt->root, &printDouble);
  
}
