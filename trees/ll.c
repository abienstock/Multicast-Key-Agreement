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
  //new if
  if (list->head !=0) 
    list->head->prev = head; //new line
  else
    list->tail = head; //new line
  head->prev = 0; //new line
  head->next = list->head;
  
  list->head = head;
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

void *popFront(struct List *list) {
  if (list->head == 0)
    return NULL;
  struct ListNode *head = list->head;
  list->head = head->next;
  //new if
  if (list->head !=0)
    list->head->prev = 0; // new line
  else
    list->tail = 0; //new line
  void *data = head->data;
  free(head);
  return data;
}

//new func
void *popBack(struct List *list) {
  if (list->tail == 0)
    return NULL;
  struct ListNode *tail = list->tail;
  list->tail = tail->prev;
  //new if
  if (list->tail !=0)
    list->tail->next = 0; // new line
  else
    list->head = 0; //new line
  void *data = tail->data;
  free(tail);
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
  new_node->prev = prevNode; //new line
  if (prevNode->next != NULL)
    prevNode->next->prev = new_node; //new line
  else 
    list->tail = new_node;
  prevNode->next = new_node;
  return new_node;
}
