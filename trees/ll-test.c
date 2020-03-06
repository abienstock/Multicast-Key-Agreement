#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ll.h"

int compareDouble(const void *data1, const void *data2) {
  if (*(double *)data1 == *(double *)data2)
    return 0;
  return 1;
}

static void printDouble(void *p)
{
  printf("%.1f ", *(double *)p);
}

static void die(const char *message)
{
  perror(message);
  exit(1); 
}

int main() 
{
  double a[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
  int n = sizeof(a) / sizeof(a[0]);
  
  int i;
  void *data;
  struct ListNode *node;
  
  // initialize list
  struct List list;
  initList(&list);
  printf("list len: %d\n", list.len);
  
  // test addFront()
  printf("testing addFront(): ");
  for (i = 0; i < n; i++) {
    if (addFront(&list, a+i) == NULL)
      die("addFront() failed");
    printf("list len: %d\n", list.len);
  }
  
  
  traverseList(&list, &printDouble);
  printf("\n");
  
  // test popFront()
  while ((data = popFront(&list)) != NULL) {
    printf("popped %.1f, the rest is: [ ", *(double *)data);
    traverseList(&list, &printDouble);
    printf("]\n");
    printf("list len: %d\n", list.len);
  }
  
  // test addAfter()
  printf("testing addAfter(): ");
  node = NULL;
  for (i = 0; i < n; i++) {
    // We keep adding after the previously added node,
    // so we are in effect 'appending' to the list.
    node = addAfter(&list, node, a+i);
    if (node == NULL) 
      die("addAfter() failed");
    printf("list len: %d\n", list.len);
  }
  
  printf("List backwards:\n");
  traverseListBackwards(&list, &printDouble);
  printf("\n");
  
  // test popBack()
  while ((data = popBack(&list)) != NULL) {
    printf("popped (from back) %.1f, the rest is (backwards): [ ", *(double *)data);
    traverseListBackwards(&list, &printDouble);
    printf("]\n");
    printf("list len: %d\n", list.len);
  }
  
  printf("More addAfter() testing: ");
  struct ListNode *test;
  for (i = 0; i < n; i++) {
    if ((test = addFront(&list, a+i)) == NULL)
      die("addFront() failed");
    if (i == 3)
      node = test;
    printf("list len: %d\n", list.len);
  }
  
  node = addAfter(&list, node, a+7);
  if (node == NULL) 
    die("addAfter() failed");
  printf("List backwards:\n");
  traverseListBackwards(&list, &printDouble);
  printf("\n");
  
  // test popBack()
  while ((data = popBack(&list)) != NULL) {
    printf("popped (from back) %.1f, the rest is (backwards): [ ", *(double *)data);
    traverseListBackwards(&list, &printDouble);
    printf("]\n");
    printf("list len: %d\n", list.len);
  }

  printf("testing findNode(): ");
  for (i = 0; i < n; i++) {
    if (addFront(&list, a+i) == NULL)
      die("addFront() failed");
    printf("list len: %d\n", list.len);
  }

  traverseList(&list, &printDouble);

  double x = 1.0;
  struct ListNode *found = findNode(&list, &x, &compareDouble);
  printf("\nfound: %.1f\n", *(double *)found->data);
  printf("list len: %d\n", list.len);

  x = 1.0;
  double y = 9.0;
  double z = 5.0;
  printf("testing findAndRemoveNode(): ");
  void *ret = findAndRemoveNode(&list, &x, &compareDouble);
  printf("\nfound: %.1f\n", *(double *)ret);
  printf("list len: %d\n", list.len);
  traverseList(&list, &printDouble);
  traverseListBackwards(&list, &printDouble);  
  ret = findAndRemoveNode(&list, &y, &compareDouble);
  printf("\nfound: %.1f\n", *(double *)ret);
  printf("list len: %d\n", list.len);
  traverseList(&list, &printDouble);
  traverseListBackwards(&list, &printDouble);  
  ret = findAndRemoveNode(&list, &z, &compareDouble);
  printf("\nfound: %.1f\n", *(double *)ret);
  printf("list len: %d\n", list.len);
  traverseList(&list, &printDouble);
  traverseListBackwards(&list, &printDouble);  
  

  removeAllNodes(&list);
  
  return 0;
}
