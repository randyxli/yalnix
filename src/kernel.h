#ifndef KERNEL_H
#define KERNEL_H

#include <hardware.h>
#include <unistd.h>

//=========
// MACROS
//=========

// Macro to go to next page
#define NEXT_PAGE(s) (((unsigned int)(s) >> PAGESHIFT)+1)


//==========
// DEFINES
//==========

#define NO_OWNER -100 /* value to indicate no owner */
#define MAX_LOCKS 100
#define MAX_CVARS 100
#define MAX_PIPES 100
#define MAX_SEMS 100
#define IDLE_PROCESS_PATH "./userprog/idle"
#define DEFAULT_INIT_PROCESS_PATH "./userprog/init"
#define STACK_BASE 0xFFFFFFFF
#define PIPE_SIZE 1024 /* size of pipe in bytes */
#define IDSCALE 4
#define PIPE 0
#define LOCK 1
#define CVAR 2
#define SEM 3
#define PAGE_TABLE_ENTRIES VMEM_REGION_SIZE/PAGESIZE
#define REGION_1_NUM_PAGES VMEM_1_SIZE/PAGESIZE
#define REGION_0_NUM_PAGES VMEM_0_SIZE/PAGESIZE
#define MAX_IO_BUFFER 100
#define RESERVED_KERNEL_PAGE ((VMEM_0_LIMIT - KERNEL_STACK_MAXSIZE - PAGESIZE)>>PAGESHIFT)

// EXIT CODE DEFINES
#define KILL                          -9
#define ERROR_BAD_TRAP                -23
#define ERROR_STACK_INTO_HEAP         -24
#define ERROR_ILLEGAL_MEMORY          -25
#define ERROR_OUT_OF_VMEM             -26
#define ERROR_RED_ZONE                -27
#define ERROR_KERNEL_PTR              -28
#define ERROR_USER_WRITE_PTR          -29
#define LOADPROGRAM_FAILED            -30
#define ERROR_MALLOC_FAILED           -31
#define ERROR_EXIT_FAILED             -32
#define ERROR_PTR_READ_PERM           -33
#define ERROR_KCS_FAILED              -34
#define ERROR_NO_FREE_FRAME           -35
#define ERROR_REMOVE_LIST_FAILED      -36
#define ERROR_SEM_VAL                 -37


//=============
//  GLOBALS 
//=============

Queue* freelocks; // Queue of free lock numbers
Queue* freecvars; // Queue of free cvar numbers
Queue* freepipes; // Queue of free pipe numbers
Queue* freesems;  // Queue of free semaphore numbers
Queue* freeframes; // Queue of free frames;
int idle_process_kc_set; // 1 if we have set the idle process's kernel context
int virtual_memory_enabled; //0 if VM is not enabled
int is_pagetable0_bootstrapped; // 0 if kernel page table is not bootstrapped
int remove_current_process; // flag for scheduler and myKCS to remove current rpocess
pte pagetable0[MAX_PT_LEN]; /* kernel page table (room for optimization) */
unsigned int *pframes; /* array of bit vectors keeping track of physical memory */
int pframeslength; //number of the integers used in pframes to keep track of free frames
void *kernelheaptop; //lowest memory location not used by the kernel
/*
  Given an ID number n for a lock, cvar, or pipe, n%IDSCALE = 0 means n refers to a pipe. n%IDSCALE = 1 means lock, and n%IDSCALE = 2 means cvar
*/
int pidcount; /* PID that will be allocated to next process */
unsigned int KernelDataStart; //Lowest address used by kernel global data
unsigned int KernelDataEnd; //Lowest address not used by the kernel
Queue *readyqueue; /* ready queue of processes */
Queue *waitingqueue; /* waiting queue of processes that have called wait */
Queue *delayqueue; /* waiting queue of processes that have called delay */
NodeLL *currentprocess; /* currently executing process */
NodeLL *idleprocess; /* idle process that runs when no other process is running */
Node *cvarblocklist; /* queue of processes waiting for cvar signal/broadcast */
Node *lockblocklist; /*  queue of processes waiting on locks */
//Node *allprocesses; /* list of all the processes that are not dead */
Queue *readqueue[NUM_TERMINALS]; /* Array of queues of processes waiting to read */
Queue *writequeue[NUM_TERMINALS]; /* Array of queues of processes waiting to write */
char *readbuffer[NUM_TERMINALS]; /* array of buffers for input from terminals */
char *writebuffer[NUM_TERMINALS]; /* array of buffers for output to terminals */
void (*trapvector[TRAP_VECTOR_SIZE])(UserContext *); /* interrupt vector table */
Lock *locks[MAX_LOCKS]; /* array of pointers to locks */
Cvar *cvars[MAX_CVARS]; /* array of pointers to cvars */
Pipe *pipes[MAX_PIPES]; /* array of pointers to pipes */
Sem *sems[MAX_SEMS];     /* array of pointers to semaphores */

//============
// PROTOTYPES
//============

void printAllProcesses();
void printPageTables(pcb *cp);
KernelContext *KCBootstrap(KernelContext *kc_in, void *pcb_p, void *not_used);
KernelContext *MyKCS(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
void scheduler(void);
int SetKernelBrk (void *addr);
int allocateFreeFrame(void);
int deallocateUserPage(pcb *mypcb, unsigned int page_number);
int deallocateFrame(unsigned int frame_number);
int allocateFrame(unsigned int frame_number);
int allocateLCP(int flag);
int deallocateLCP(int number, int flag);
NodeLL* initInitProcess(char* cmd_args[], UserContext *uctxt, unsigned int pmem_size);
NodeLL* initIdleProcess(char* cmd_args[], UserContext *uctxt);  
void KernelStart (char* cmd_args[], unsigned int pmem_size, UserContext *uctxt);
void initDataStructures(unsigned int pmem_size, UserContext *uctxt, char* cmd_args[]);
void SetKernelData (void * _KernelDataStart, void *_KernelDataEnd);
int LoadProgram(char *name, char *args[], pcb *proc);
void pointerCheck(int check_prot, void *ptr, int string, int len);

#endif
