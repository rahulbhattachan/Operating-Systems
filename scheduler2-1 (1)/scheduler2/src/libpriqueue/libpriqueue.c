/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

//courtesy of Job @ http://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c-code
#define UNUSED(x) (void)(x)

/**
  Initializes the priqueue_t data structure.

  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
  q->root = 0;
  q->size = 0;
  q->comp = comparer;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
  Node* node = malloc(sizeof(Node));
  node->pointer = ptr;
  node->next = 0;
  if(q->size == 0)
  {
    q->size = 1;
    q->root = node;

    return 0;
  }
  Node* temp = q->root;
  Node* parent = 0;

  int num = 0;
  while(temp != 0 && q->comp(temp->pointer,ptr) < 0)
  {
    parent = temp;
    temp = temp->next;
    num++;
  }
  if(num == 0)
  {
    q->size++;
    node->next = q->root;
    q->root = node;
    return 0;
  }

  parent->next = node;
  node->next = temp;

  q->size++;
	return num;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
  if(q->size == 0) {
    return NULL;
  } else {
    return q->root->pointer;
  }
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
  if(q->size == 0) {
    return NULL;
  }
  Node *temp = q->root;
  void* ptr = 0;
  if(temp != 0)
  {
    q->root = temp->next;
  }
  else
  {
    q->root = 0;
  }
  ptr = temp->pointer;
  free(temp);
  q->size--;
  return ptr;
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
  if(index >= q->size) {
    return 0;
  } else {
    Node* temp = q->root;

    int i = 1;
    while(i <= index) {
      temp = temp->next;
      i++;
    }

    return temp->pointer;
  }
}


/**
  Removes all instances of ptr from the queue.

  This function should not use the comparer function,
  but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
  if(q->size < 1)
  {
    return 0;
  }


  if(q->comp(ptr,q->root->pointer) == 0)
  {
    q->size--;
    Node* temp = q->root;
    q->root = q->root->next;
    free(temp);
    return 1 + priqueue_remove(q,ptr);
  }

  Node* current = q->root->next;
  Node* parent = q->root;

  int num = 0;
  while(current != 0) {
    if(q->comp(current->pointer,ptr) == 0) {
      Node* temp = current->next;
      parent->next = temp;
      num++;
      free(current);
      current = temp;
      q->size--;
    } else {
      parent = current;
      current = current->next;
    }
  }
	return num;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
  if(index > q->size - 1)
    return 0;

  q->size--;

  Node* temp = q->root;

  if(index == 0)
  {
    q->root = q->root->next;
    return temp;
  }
  Node* parent = temp;
  temp = temp->next;
  index--;
  while(index > 0)
  {
    parent = temp;
    temp = temp->next;
    index--;
  }
  parent->next = temp->next;
	return temp;
}


/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
  return q->size;
}


/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
  while(q->size > 0)
  {
    void* temp = priqueue_remove_at(q,0);
    free(temp);
  }
}
