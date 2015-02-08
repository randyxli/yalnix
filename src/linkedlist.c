#include <stdio.h>
#include "datastructures.h"
#include <yalnix.h>
#include <hardware.h>

/*
Description: Prints all of the nodes of a linked list in order.

Input: NodeLL *head -- head of a linked list

Output: IDs of all of the nodes in the linked list sequentially. Example output:
(10) (3) (5) (2) (4)
 */
void printLL(NodeLL *head)
{
    NodeLL *node = head;
    while(node != 0)
    {
	TracePrintf(100, "(%d)", node->id);
	node = node->next;
    }
}

/*
Description: Removes the first item in a queue and returns it.

Input: Queue *q -- a pointer to a queue

Output: The first element in the queue.
 */
NodeLL *dequeue(Queue *q)
{
    if(q == NULL)
	return NULL;
    if(q->head == NULL)
	return NULL;
    if(q->head->next == NULL)
	q->tail = NULL;
    NodeLL *temp = q->head;
    q->head = q->head->next;
    temp->next = NULL;
    temp->prev = NULL;
    return temp;
}


/*
Description: Puts an item to the end of a queue.

Input: Queue *q       -- a pointer to a queue.
       NodeLL *mynode -- the node to put at the end of queue q.

Output: 0 if success, -1 if failure
 */
int enqueue(Queue *q, NodeLL *mynode)
{
    if (q == NULL)
    {
	return -1;
    }
    if (q->head == NULL)
    {
	q->head = mynode;
	q->tail = mynode;
	mynode->next = NULL;
	mynode->prev = NULL;
    }
    else
    {
	q->tail->next = mynode;
	mynode->prev = q->tail;
	mynode->next = NULL;
	q->tail = mynode;
    }

    return 0;
}

/*
Description: Searches a linked list for a node.

Input: NodeLL *head -- the head of a linked list.
       int id       -- the ID of the node you want to find in the linked list

Output: If the desired node is found, returns a pointer to the node. Otherwise, returns NULL.
 */
NodeLL *searchLL(NodeLL *head, int id)
{
    NodeLL *node = head;
    while(node != 0)
    {
	if(id == node->id)
	    return node;
	node = node->next;
    }
    return NULL;
}

/*
Description: Removes a node from a linked list.

Input: NodeLL *head -- the head of a linked list
       int id       -- the id of the node to be removed from the linked list

Output: 0 if the node is successfully worked, and -1 if the node cannot be found.
 */
int removeLL(NodeLL *head, int id)
{
    NodeLL *remove_me = searchLL(head, id);
    if(remove_me != NULL)
    {
	if(remove_me->prev != NULL)
	    remove_me->prev->next = remove_me->next;
	if(remove_me->next != NULL)
	    remove_me->next->prev = remove_me->prev;
	if(remove_me->prev == NULL &&
	   remove_me->next == NULL)
	    return 1;
	return 0;
    }
    return -1;
}

/*
Description: Removes a Node from a queue structure used as a list

Input: Queue *q    -- the queue to remove from
       int id      -- id of the node to remove

Output: Returns pointer to the NodeLL removed if removal was successful,
        NULL otherwise
 */
NodeLL* removeFromList(Queue *q, int id)
{
    if (q == NULL)
	return NULL;
    if (q->head == NULL)
    {
	return NULL;
    }
    if (q->head->id == id)
	return dequeue(q);

    if (q->tail->id == id)
    {
	
	NodeLL* temp = q->tail;
	q->tail = q->tail->prev;
	q->tail->next = NULL;
	temp->prev = NULL;
	return temp;
    }

     NodeLL* remove_me = searchLL(q->head, id);
    remove_me->prev->next = remove_me->next;
    remove_me->next->prev = remove_me->prev;
    remove_me->next = NULL;
    remove_me->prev = NULL;
    return remove_me;
}

/*
Description: Adds a node to the head of a linked list.

Input: NodeLL *head    -- the head of the linked list.
       NodeLL *newnode -- the node to be added to the linked list.

Output: The new head of the linked list.
 */
NodeLL *addLL(NodeLL *head, NodeLL *newnode)
{
    newnode->next = head;
    head->prev = newnode;
    return newnode;
}

/*
Description: Checks if queue is empty.

Input: Queue *q -- the queue to check

Output: 1 if queue is empty, 0 otherwise
 */
int isQueueEmpty(Queue *q)
{
    if(q->head == NULL)
	return 1;
    return 0;
}
