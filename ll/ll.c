#include "ll.h"
#include <stdlib.h>
#include <stdio.h>

struct ListNode *addFront(struct List *list, void *data) {
  if (data == NULL)
    return NULL;
  struct ListNode *head = malloc(sizeof(struct ListNode));
  if (head == NULL)
    {
      perror("malloc returned NULL");
      return NULL;
    }
  head->data = data;
  
  if (list->head !=0) 
    list->head->prev = head;
  else
    list->tail = head;
  head->prev = 0;
  head->next = list->head;
  
  list->head = head;
  list->len++;
  return head;
}


void traverseList(struct List *list, void (*f)(void *)) {
  struct ListNode *curr = list->head;
  while(curr != 0)
    {
      f(curr->data);
      curr = curr->next;
    }
}

void traverseListBackwards(struct List *list, void (*f)(void *)) {
  struct ListNode *curr = list->tail;
  while(curr != 0)
    {
      f(curr->data);
      curr = curr->prev;
    }
}

struct ListNode *findNode(struct List *list, int i) {
  struct ListNode *curr = list->head;
  int j = 0;
  while(curr != 0)
    {
      if (j == i)
	return curr;
      curr = curr->next;
      j++;
    }
  return NULL;
}

void *findAndRemoveNode(struct List *list, int i) {
  struct ListNode *rem = findNode(list, i);
  if (rem != NULL) {
    void *data = rem->data;
    if (list->head == rem) {
      return popFront(list);
    } else if (list->tail == rem) {
      return popBack(list);
    } else {
      rem->prev->next = rem->next;
      rem->next->prev = rem->prev;
      free(rem);
      list->len--;
      return data;
    }
  }
  return NULL;
}

void *popFront(struct List *list) {
  if (list->head == 0)
    return NULL;
  struct ListNode *head = list->head;
  list->head = head->next;
  if (list->head !=0)
    list->head->prev = 0;
  else
    list->tail = 0;
  void *data = head->data;
  free(head);
  list->len--;
  return data;
}

void *popBack(struct List *list) {
  if (list->tail == 0)
    return NULL;
  struct ListNode *tail = list->tail;
  list->tail = tail->prev;
  if (list->tail !=0)
    list->tail->next = 0;
  else
    list->head = 0;
  void *data = tail->data;
  free(tail);
  list->len--;
  return data;
}

void removeAllNodes(struct List *list) {
  while (popFront(list))
    ;
}

struct ListNode *addAfter(struct List *list,
			  struct ListNode *prevNode, void *data) {
  if (data == NULL)
    return NULL;
  if (prevNode == NULL)
    return addFront(list, data);
  struct ListNode *new_node = malloc(sizeof(struct ListNode));
  if (new_node == NULL)
    {
      perror("malloc returned NULL");
      return NULL;
    }
  new_node->data = data;
  new_node->next = prevNode->next;
  new_node->prev = prevNode;
  if (prevNode->next != NULL)
    prevNode->next->prev = new_node;
  else 
    list->tail = new_node;
  prevNode->next = new_node;
  list->len++;
  return new_node;
}
