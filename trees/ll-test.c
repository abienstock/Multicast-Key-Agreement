#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ll.h"

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

    // test addFront()
    printf("testing addFront(): ");
    for (i = 0; i < n; i++) {
	if (addFront(&list, a+i) == NULL)
	  die("addFront() failed");
    }

    
    traverseList(&list, &printDouble);
    printf("\n");
    
    // test popFront()
    while ((data = popFront(&list)) != NULL) {
	printf("popped %.1f, the rest is: [ ", *(double *)data);
	traverseList(&list, &printDouble);
	printf("]\n");
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
    }
    
    traverseList(&list, &printDouble);
    printf("\n");
    
    return 0;
}
