#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <hardware.h>

#define PIPE_SIZE 1024 /* size of pipe in bytes */
#define PROC_NAME_LEN 50


//================
//    STRUCTS
//===============

// Prototypes
typedef struct _pcb pcb;
typedef struct _Node Node;
typedef struct _Pipe Pipe;
typedef struct _Lock Lock;
typedef struct _Cvar Cvar;

typedef struct _NodeLL
{
    struct _NodeLL *next;
    struct _NodeLL *prev;
    void *data;
    int id;
} NodeLL;

typedef struct _Queue
{
    NodeLL *head;
    NodeLL *tail;
} Queue;


/* Node for trees and queues */

/* Pipe structure */
typedef struct _Pipe
{
    char buffer[PIPE_SIZE]; /* buffer to read from and write to  */
    int pipeid; /* id of the pipe */
    int numbytes;
    NodeLL *readblocking;
    NodeLL *writeblocking;
    pcb *writer;
    pcb *reader;
} Pipe;

/* Lock structure */
typedef struct _Lock
{
    int owner; /* pid of owner of the lock */
    int id; /* id of the lock */
    Queue *blocking; /* queue of processes waiting for this lock */
} Lock;

typedef struct _Cvar
{
    int owner; /* pid of owner of the cvar */
    int id; /* id of the cvar */
    Queue *blocking; /* queue of processes waiting for this cvar */
} Cvar;

typedef struct _Sem
{
    int id; /* id of the semaphore */
    int val; /* current value of the semaphore */
    Queue *blocking; /* queue of processes waiting for this semaphore */
} Sem;

/* Process Control Block */
typedef struct _pcb
{
    
    int pid; // process id 
    char name[PROC_NAME_LEN]; // name of this process
    Queue *pipes; // linked list of pipes
    Queue *graveyard; // linked list of dead children
    UserContext usercontext; // user context
    KernelContext kernelcontext; // kernel context
    pte pagetable1[MAX_PT_LEN]; // process's page table (room for optimization)
    pte kernelstack[KERNEL_STACK_MAXSIZE/PAGESIZE];
    int lockwaitedon; // id of lock this process is currently waiting on
    int cvarwaitedon; // id of the cvar this process is currently waiting on
    int exitstatus; // exit status of this proces
    int clockticksleft; // number of clock ticks left in the delay if process is in delay queue
    Queue *children; // pointer to the head of the linked list containing the children of this process
    pcb *parent; // pointer to parent of this process
    void *userheaptop; // address of the lowest location not used by the program
    
} pcb;

void printLL(NodeLL *head);
NodeLL *dequeue(Queue *q);
int enqueue(Queue *q, NodeLL *mynode);
NodeLL *searchLL(NodeLL *head, int id);
int removeLL(NodeLL *head, int id);
NodeLL* removeFromList(Queue *q, int id);
NodeLL *addLL(NodeLL *head, NodeLL *newnode);
int isQueueEmpty(Queue *q);

#endif
