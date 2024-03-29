#ifndef _LL_H_
#define _LL_H_

/*
 * A node in a linked list.
 */
struct ListNode {
  void *data;
  struct ListNode *next;
  struct ListNode *prev;
};

/*
 * A linked list.  
 * 'head' points to the first node in the list.
 * 'tail' points to the last node in the list.
 */
struct List {
  struct ListNode *head;
  struct ListNode *tail;
  int len;
};

/*
 * Initialize an empty list.  
 */
static inline void initList(struct List *list)
{
  list->head = 0;
  list->tail = 0;
  list->len = 0;
}

/*
 * In all functions below, the 'list' parameter is assumed to point to
 * a valid List structure.
 */

/*
 * Create a node that holds the given data pointer,
 * and add the node to the front of the list.
 *
 * Note that this function does not manage the lifetime of the object
 * pointed to by 'data'.
 * 
 * It returns the newly created node on success and NULL on failure.
 */
struct ListNode *addFront(struct List *list, void *data);

/*
 * Traverse the list, calling f() with each data item.
 */
void traverseList(struct List *list, void (*f)(void *));
/*
 * Traverse the list backwards, calling f() with each data item.
 */
void traverseListBackwards(struct List *list, void (*f)(void *));
/*
 * Traverse the list until the ith node is found.
 *
 * Returns the ith node,
 * NULL if less than i nodes.
 */
struct ListNode *findNode(struct List *list, int i);
/*
 * Traverse the list until the ith node is found.
 *
 * Removes and deallocates the memory for the ith node,
 * does nothing if not found.
 *
 * Returns the data of the removed node if found,
 * returns NULL otherwise.
 */
void *findAndRemoveNode(struct List *list, int i);

/*
 * Returns 1 if the list is empty, 0 otherwise.
 */
static inline int isEmptyList(struct List *list)
{
  return (list->head == 0);
}

/*
 * Remove the first node from the list, deallocate the memory for the
 * ndoe, and return the 'data' pointer that was stored in the node.
 * Returns NULL is the list is empty.
 */
void *popFront(struct List *list);
/*
 * Remove the last node from the list, deallocate the memory for the
 * node, and return the 'data' pointer that was stored in the node.
 * Returns NULL if the list is empty.
 */
void *popBack(struct List *list);
/*
 * Remove all nodes from the list, deallocating the memory for the
 * nodes.  You can implement this function using popFront().
 */
void removeAllNodes(struct List *list);

/*
 * Create a node that holds the given data pointer,
 * and add the node right after the node passed in as the 'prevNode'
 * parameter.  If 'prevNode' is NULL, this function is equivalent to
 * addFront().
 *
 * Note that prevNode, if not NULL, is assumed to be one of the nodes
 * in the given list.  The behavior of this function is undefined if
 * prevNode does not belong in the given list.
 *
 * Note that this function does not manage the lifetime of the object
 * pointed to by 'data'.
 * 
 * It returns the newly created node on success and NULL on failure.
 */
struct ListNode *addAfter(struct List *list, 
			  struct ListNode *prevNode, void *data);

#endif /* #ifndef _LL_H_ */
