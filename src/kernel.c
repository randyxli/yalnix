#include "syscalls.h"
#include "datastructures.h"
#include <yalnix.h>
#include "kernel.h"
#include "header.h"

// INCLUDES FOR LOAD PROGRAM
#include <fcntl.h>
#include <unistd.h>
#include <hardware.h>
#include <load_info.h>

//===================
// UTILITY FUNCTIONS
//===================

// Prints the user stack
void printUserStack()
{
    int i;
    pcb * curr_process = (pcb *)currentprocess->data;
    TracePrintf(100, "************PRINTING USER STACK**************");
    TracePrintf(100, "Address\t Data\n");
    i = VMEM_1_LIMIT-4;
    while(curr_process->pagetable1[(i >> PAGESHIFT) - MAX_PT_LEN].valid == 1)
    {
        TracePrintf(1, "%08x\t %08x\n", i, *((unsigned int *)i));
	i = i-4;
    }
    TracePrintf(100, "******************************************");
}

// Prints kernel stack
void printKernelStack()
{
    int i;
    pcb * curr_process = (pcb *)currentprocess->data;
    TracePrintf(100, "************PRINTING KERNEL STACK**************");
    TracePrintf(100, "Address\t Data\n");
    for(i=KERNEL_STACK_BASE; i<KERNEL_STACK_LIMIT; i = i+4)
    {
        TracePrintf(1, "%08x\t %08x\n", i, *((unsigned int *)i));
    }
    TracePrintf(100, "******************************************");
}

// Prints current process and queues
void printAllProcesses()
{
    pcb * curr_process;
    NodeLL* curr_node;
    
    TracePrintf(51, "===============================================================================\n");
    TracePrintf(51, "CURRENT PROCESS\n");
    TracePrintf(51, "----------------\n");
    curr_process = (pcb*)(currentprocess->data);
    TracePrintf(51, "Name: %s\tPID: %d\n", curr_process->name, curr_process->pid);
    TracePrintf(51, "----------------\n");
    
    TracePrintf(51, "READY QUEUE\n");
    TracePrintf(51, "----------------\n");
    curr_node = readyqueue->head;
    while (NULL != curr_node)
    {
	curr_process = (pcb *)(curr_node->data);
	TracePrintf(1, "Name: %s\tPID: %d\n", curr_process->name, curr_process->pid);
	curr_node = curr_node->next;
    }
    TracePrintf(51, "----------------\n");

    TracePrintf(51, "DELAY QUEUE\n");
    TracePrintf(51, "----------------\n");
    curr_node = delayqueue->head;
    while (NULL != curr_node)
    {
	curr_process = (pcb*)(((NodeLL*)(curr_node->data))->data);
	TracePrintf(51, "Name: %s\tPID: %d\n", curr_process->name, curr_process->pid);
	curr_node = curr_node->next;
    }
    TracePrintf(51, "----------------\n");

    TracePrintf(51, "WAITING QUEUE\n");
    TracePrintf(51, "----------------\n");
    curr_node = waitingqueue->head;
    while (NULL != curr_node)
    {
	curr_process = (pcb*)(((NodeLL*)(curr_node->data))->data);
	TracePrintf(51, "Name: %s\tPID: %d\n", curr_process->name, curr_process->pid);
	curr_node = curr_node->next;
    }
    TracePrintf(51, "----------------\n");

    TracePrintf(51, "WRITE QUEUE 1\n");
    TracePrintf(51, "----------------\n");
    curr_node = writequeue[1]->head;
    while (NULL != curr_node)
    {
	curr_process = (pcb*)(((NodeLL*)(curr_node->data))->data);
	TracePrintf(51, "Name: %s\tPID: %d\n", curr_process->name, curr_process->pid);
	curr_node = curr_node->next;
    }
    TracePrintf(51, "----------------\n");

    TracePrintf(51, "GRAVEYARD (CURRENT PROCESS)\n");
    TracePrintf(51, "--------------------------------\n");
    curr_process = (pcb*)(currentprocess->data);
    curr_node = curr_process->graveyard->head;
    while (NULL != curr_node)
    {
	TracePrintf(51, "PID: %d\tExit Status: %d\n", curr_node->id, (int)(curr_node->data));
	curr_node = curr_node->next;
    }
    TracePrintf(51, "-------------------------------\n");

    TracePrintf(51, "===============================================================================\n\n");
}

// Prints valid entries in currently used page tables
void printPageTables(pcb *cp)
{
    int i;
    //pcb* cp = (pcb*)(currentprocess->data);
    
    // Print the region 0 page table
    TracePrintf(1, "===============================================================================\n");
    TracePrintf(1, "REGION 0\n");
    TracePrintf(1, "---------\n");
    for (i = 0; i < MAX_PT_LEN; i++)
    {
	if (pagetable0[i].valid == 1)
	    TracePrintf(1, "Virtual Page: %d\tValid: %d\tProt: %04x\tPFN: %d\n", i, pagetable0[i].valid, pagetable0[i].prot, pagetable0[i].pfn);
    }
    // Print the region 1 page table
    TracePrintf(1, "\n");
    TracePrintf(1, "REGION 1\n");
    TracePrintf(1, "---------\n");
    for (i = 0; i < MAX_PT_LEN; i++)
    {
	if (cp->pagetable1[i].valid == 1)
	    TracePrintf(1, "Virtual Page: %d\tValid: %d\tProt: %04x\tPFN: %d\n", (i+MAX_PT_LEN), cp->pagetable1[i].valid, cp->pagetable1[i].prot, cp->pagetable1[i].pfn);
    }
    TracePrintf(1, "===============================================================================\n\n");
}



//===================================
//  Kernel Context Switch Functions
//===================================

// Copies contents of current kernel stack frames
// into the frames allocated to this pcb's kernel stack
KernelContext *KCBootstrap(KernelContext *kc_in,
			   void *pcb_p,     // pcb to allocate kernel stack for
			   void *not_used)
{
    pcb* this_pcb = (pcb *) pcb_p;
    memcpy(&(this_pcb->kernelcontext), kc_in, sizeof(KernelContext));

    // Copy current current stack frame contents into this pcb's stack frames
    int i;
    for (i = 0; i < 2; i++)
    {
	unsigned int *dst, *src;
	pagetable0[RESERVED_KERNEL_PAGE].valid = 1;
	pagetable0[RESERVED_KERNEL_PAGE].prot = PROT_READ | PROT_WRITE;
	pagetable0[RESERVED_KERNEL_PAGE].pfn = this_pcb->kernelstack[i].pfn;
	dst = (void *)(RESERVED_KERNEL_PAGE << PAGESHIFT);
	src = (void *)( (126 + i) << PAGESHIFT);
	WriteRegister(REG_TLB_FLUSH, (unsigned int) dst);
	memcpy(dst, src, PAGESIZE);
    }

    // Unmap page RESERVED_KERNEL_PAGE after we are finished using it
    pagetable0[RESERVED_KERNEL_PAGE].valid = 0;
    pagetable0[RESERVED_KERNEL_PAGE].prot = 0;
    pagetable0[RESERVED_KERNEL_PAGE].pfn = 0;

    return kc_in;
}

KernelContext *MyKCS(KernelContext *kc_in, /* kernel context of kernel process calling KernelContextSwitch */
		     void *curr_pcb_p,     /* pointer to current process's PCB */
		     void *next_pcb_p)     /* pointer to next process's PCB */
{
    pcb *next_pcb = (pcb *) next_pcb_p;
    pcb *curr_pcb = (pcb *) curr_pcb_p;
    TracePrintf(50, "MyKCS: Switching between process %d and process %d\n", curr_pcb->pid, next_pcb->pid);
    int i;

    // Save temp copy of current kernel context into current PCB
    memcpy(&(curr_pcb->kernelcontext), kc_in, sizeof(KernelContext));

    TracePrintf(100, "REG_PTLR0 contains %d\n", ReadRegister(REG_PTLR0));
    

    // Remap the kernel stack
    for(i=KERNEL_STACK_BASE; i<KERNEL_STACK_LIMIT; i+=PAGESIZE)
    {
	int page_no = i >> PAGESHIFT;
	int stackpage  = (i-KERNEL_STACK_BASE)/PAGESIZE;
	TracePrintf(100, "Page_no = %d, Stackpage = %d\n", page_no, stackpage);

	// Check if kernel stacks of both current and next process are properly mapped in page table
	if(pagetable0[page_no].valid != next_pcb->kernelstack[stackpage].valid ||
	   pagetable0[page_no].prot != next_pcb->kernelstack[stackpage].prot)
	{
	    TracePrintf(1, "Error: inappropriate values for kernel stack page table entries in either pagetable0 or pcb bookkeeping\n");
	    //printPageTables(next_pcb);
	}
	pagetable0[page_no].pfn = next_pcb->kernelstack[stackpage].pfn;
	pagetable0[page_no].prot = next_pcb->kernelstack[stackpage].prot;
	pagetable0[page_no].valid = next_pcb->kernelstack[stackpage].valid;
    }

    // If global flag is set to deallocate current pcb's stack frames (i.e. it died),
    // do so
    if (remove_current_process == 1)
    {
	// Free the current pcb 
	free(curr_pcb);

	remove_current_process = 0;
    }


    TracePrintf(100, "REG_PTLR0 contains %d\n", ReadRegister(REG_PTLR0));
    
    // Write region 0 page table registers again in case the current process
    // hasn't already have them mapped in the register (guards against irregular error)
    WriteRegister(REG_PTLR0, MAX_PT_LEN);

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0); // Flush TLB after remapping
    
    // Remap Region 1 page table and flush entire region 1 TLB
    WriteRegister(REG_PTBR1, (unsigned int) next_pcb->pagetable1);
    WriteRegister(REG_PTLR1, MAX_PT_LEN);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    TracePrintf(50, "MyKCS: Returning from switching between process %d and process %d\n", curr_pcb->pid, next_pcb->pid);

    return &(next_pcb->kernelcontext); 
}

//=================
//  SCHEDULER
//=================

// Our scheduler assumes that the current process pcb node has already
// been enqueued in the queue it will be waiting in, and proceeds to run
// the next available process on the ready queue (or idle) via KernelContextSwitch
void scheduler(void)
{
    TracePrintf(50, "scheduler: Entering\n");
    unsigned int i;
    int rc;
    pcb *next_pcb;
    pcb *curr_pcb = (pcb*)(currentprocess->data);

    
    
    // Free the currently running process's PCB node if it exited
    if (remove_current_process == 1)
	free(currentprocess);

    // Ready queue is empty, so we run idle process next
    if (isQueueEmpty(readyqueue))
    {
	next_pcb = (pcb*)(idleprocess->data);
	currentprocess = idleprocess;
	TracePrintf(100, "Queue is empty! Switching to idle process, pid = %d\n", next_pcb->pid);
    }

    // If not, take next process off the ready queue to run
    else
    {
	NodeLL* next_pcb_node = dequeue(readyqueue);
	next_pcb = (pcb*)(next_pcb_node->data);
	currentprocess = next_pcb_node;
	TracePrintf(100, "Queue not empty! Switching to dequeued process, pid = %d\n", next_pcb->pid);
    }

    TracePrintf(100, "scheduler: Current process is %d, next process is %d. Going to call KernelContextSwitch...\n", curr_pcb->pid, next_pcb->pid);
    
    // Switch contexts to the next process we have selected
    rc = KernelContextSwitch(MyKCS, (void *) curr_pcb, (void *) next_pcb);
    
    if (rc != 0)
    {
	TracePrintf(1, "Error: Context Switch failed in scheduler. Exiting proces...\n");
	SysExit(ERROR_KCS_FAILED);
    }

}


int SetKernelBrk (void *addr)
{
    TracePrintf(100, "SetKernelBrk: addr is in page %d. UP_TO_PAGE(addr) is %08x  Entering...\n", (unsigned int)addr >> PAGESHIFT, UP_TO_PAGE(addr));
    unsigned int i;

    // Virutal memory is not enabled
    if(0 == virtual_memory_enabled)
    {
	// Check if pagetable0 is already bootstrapped
	if(1 == is_pagetable0_bootstrapped)
	{
	    TracePrintf(1, "Error: malloc attempt after bootstrap of kernel heat memory but before virtual memory is enabled\n");
	    return ERROR;
	}
	
	// Check for attempt to shrink kernel heap during bootstrap
	if(kernelheaptop > addr)
	{
	    TracePrintf(1, "Error: attempt to shrink heap during bootstrap\n");
	    return ERROR;
	}		
    }

    // Virtual memory is enabled
    else
    {   
	
	// Check that addr is not in kernel stack region or higher. If so, return error
	if ( ((unsigned int)addr>>PAGESHIFT) >= (DOWN_TO_PAGE(KERNEL_STACK_BASE)>>PAGESHIFT) )
	{
	    TracePrintf(1, "Error: Attempt to grow kernel heap into kernel stack or higher\n");
	    return ERROR;
	}

	// Check all pages between VMEM_0_BASE and kernelheaptop are valid
	for (i = (VMEM_0_BASE >> PAGESHIFT); i <= (DOWN_TO_PAGE(kernelheaptop)>>PAGESHIFT); i++)
	{
	    // Return error if we find an invalid page
	    if (pagetable0[i].valid == 0)
	    {
		TracePrintf(1, "Error: Region 0 page %d, below the top of kernel heap, is not valid. Region 0 data has been corrupted!\n", i);
		return ERROR;
	    }
	}

	// If addr is in a valid page, we shrink the heap
	if (1 == pagetable0[(unsigned int)addr>>PAGESHIFT].valid)
	{	
	    for(i = NEXT_PAGE(addr); i <= DOWN_TO_PAGE(kernelheaptop)>>PAGESHIFT; i++)
	    {
		if(pagetable0[i].valid == 1)
		{
		    pagetable0[i].valid = 0;
		    deallocateFrame(pagetable0[i].pfn);
		    WriteRegister(REG_TLB_FLUSH, i << PAGESHIFT);
		}	    
	    }

	}

	// Else, we grow the heap
	else
	{
	    if ((DOWN_TO_PAGE(addr)>>PAGESHIFT) == RESERVED_KERNEL_PAGE)
	    {
		TracePrintf(1, "Error: Trying to grow kernel heap into reserved kernel page %d!\n", RESERVED_KERNEL_PAGE);
		return ERROR;
	    }
	    for(i = NEXT_PAGE(kernelheaptop); i <= DOWN_TO_PAGE(addr)>>PAGESHIFT; i++)
	    {		
		if(pagetable0[i].valid == 0)
		{		    
		    int allocated_frame = allocateFreeFrame();
		    if(allocated_frame == ERROR)
			return ERROR;
		    else
		    {
			pagetable0[i].valid = 1;
			pagetable0[i].prot = PROT_READ | PROT_WRITE;
			pagetable0[i].pfn = allocated_frame;
			TracePrintf(100, "SetKernelBrk: Page %d of Region 0 allocated frame %d\n", i, allocated_frame);
		    }
		}
		
		// If we encounter a valid page between kernelheaptop and addr, return ERROR
		else
		{
		    TracePrintf(1, "Error: Page %d, encountered while growing kernel heap top from %08x to %08x, is valid. Region 0 data corrupted!\n", i, kernelheaptop, addr);
		    return ERROR;
		}
	    }
	}

	// Check that all pages between addr and KERNEL_STACK_BASE are not valid
	for (i = NEXT_PAGE(addr); i < (DOWN_TO_PAGE(KERNEL_STACK_BASE)>>PAGESHIFT); i++)
	{
	    // Return ERROR if we find an valid page
	    if (pagetable0[i].valid == 1)
	    {
		TracePrintf(1, "Error: Region 0 page %d, between the heap top and the stack, is valid. Region 0 data has been corrupted!\n", i);
		return ERROR;
	    }
	}


    }

    kernelheaptop = addr;
    return 0;
}

//========================
// BOOKKEEPING FUNCTIONS
//========================

// Allocates a free frame from the queue of free frame numbers
// Returns the free frame number if we can find a free frame to allocate.
// Returns ERROR otherwise (no free frame available)
int allocateFreeFrame(void)
{
    // We have no more free frames
    if (isQueueEmpty(freeframes) != 0)
    {
	TracePrintf(1, "allocateFreeFrame: No free frames, allocation FAILED!\n");
	return ERROR;
    }

    // We have free frames, so find one
    else
    {
	NodeLL* free_frame = dequeue(freeframes);
	int free_frame_no = free_frame->id;
	free(free_frame);
	TracePrintf(50, "allocateFreeFrame: allocated frame %d!\n", free_frame_no);
	return free_frame_no;
    }
}

// Dealloacates a user page by unmapping the page table entry, and adding the 
// frame number it is mapped to back to the free frame number queue.
// Returns 0 if we successfully deallocate the user page
// Returns ERROR otherwise (page is already invalid)
int deallocateUserPage(pcb *mypcb, unsigned int page_number)
{
    TracePrintf(50, "Deallocating page %d in Region 1\n", page_number);

    // Page is already invalid
    if(0 == mypcb->pagetable1[page_number].valid)
    {
	TracePrintf(1, "deallocateUserPage: Error: Tried to deallocated invalid page\n");
	return ERROR;
    }

    // Page is currently valid, so we deallocate it
    else
    {
	mypcb->pagetable1[page_number].valid = 0;
	int physical_frame = mypcb->pagetable1[page_number].pfn;
	
	// Create new free frame node and add it to free frames queue
	NodeLL* curr_frame_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(curr_frame_node);
	BZERO(curr_frame_node, sizeof(NodeLL));
	curr_frame_node->id = physical_frame;
	curr_frame_node->data = NULL;
	enqueue(freeframes, curr_frame_node);

	//flush TLB
	WriteRegister(REG_TLB_FLUSH, ((MAX_PT_LEN + page_number) << PAGESHIFT) );
    }
    return 0;
}

// Dealloacates a used frame adding its number back to the free frame number queue
// Returns 0 if we successfully deallocate the page
// This function will onyl fail if malloc fails, in which case the current process will Exit
// with error ERROR_MALLOC_FAILED
int deallocateFrame(unsigned int frame_number)
{
    TracePrintf(50, "Deallocating frame %d\n", frame_number);
    
    // Create new free frame node and add it to free frames queue
    NodeLL* curr_frame_node = (NodeLL*) malloc(sizeof(NodeLL));
    MALLOC_CHECK(curr_frame_node);
    BZERO(curr_frame_node, sizeof(NodeLL));
    curr_frame_node->id = frame_number;
    curr_frame_node->data = NULL;
    enqueue(freeframes, curr_frame_node);

    return 0;
}

// Allocate a given frame number by removing it from the free frames queue
// Returns 0 if the allocation was successful
// Exits the current process with ERROR_ALLOCATION_FAILED otherwise
int allocateFrame(unsigned int frame_number)
{
    // Cannot allocate the frame number if the queue is empty
    if (isQueueEmpty(freeframes) != 0)
    {
	TracePrintf(1, "allocateFrame: No free frames. Allocation FAILED! Exiting current process...\n");
	SysExit(ERROR_EXIT_FAILED);
    }
    NodeLL* removed = removeFromList(freeframes, frame_number);
    if (removed == NULL)
    {
	TracePrintf(1, "allocateFrame: Failed to remove frame %d's node from freeframes. Exiting current rpocess...\n", frame_number);
	SysExit(ERROR_EXIT_FAILED);
    }
    TracePrintf(100, "allocateFrame: allocated frame %d!\n", frame_number);
    free(removed);

    return 0;
}

// Find the next free number for a lock, cvar, pipe or sem
// Returns ERROR if flag is invalid or if no free number is available.
// Returns the allocated number otherwise
int allocateLCP(int flag) // flag to indicate lock, cvar, pipe or sem
{
    Queue* queue;
    
    // Set the queue to allocate to depending on flag
    switch(flag) {
    
    case LOCK:
	queue = freelocks;
	break;
    case CVAR:
	queue = freecvars;
	break;
    case PIPE:
	queue = freepipes;
	break;
    case SEM:
	queue = freesems;
	break;
    default:
	TracePrintf(1, "allocateLCP: Invalid flag %d", flag);
	return ERROR;
	break;
    }
    
    // Queue is empty, so we cannot allocate
    if (isQueueEmpty(queue) != 0)
    {
	TracePrintf(1, "allocateLCP: No free numbers, allocation FAILED!\n");
	return ERROR;
    }

    // Queue is not empty. Allocate by removing free number from the queue
    else
    {
	NodeLL* free_number_node = dequeue(queue);
	int free_number = free_number_node->id;
	free(free_number_node);
	TracePrintf(100, "allocateLCP: allocated number is %d!\n", free_number);
	return free_number;
    }
}

// Deallocate a used number for lock, cvar, pipe or sem
// Returns ERROR if flag is invalid
// Returns 0 otherwise
int deallocateLCP(int number, // number to deallocate
		  int flag)   // flag to indicate lock, cvar, pipe or sem
{
    Queue* queue;
    
    TracePrintf(50, "Deallocating number %d\n", number);

    // Set the queue to allocate to depending on flag
    switch(flag) {
    
    case LOCK:
	queue = freelocks;
	break;
    case CVAR:
	queue = freecvars;
	break;
    case PIPE:
	queue = freepipes;
	break;
    case SEM:
	queue = freesems;
	break;
    default:
	TracePrintf(1, "allocateLCP: Invalid flag %d", flag);
	return ERROR;
	break;
    }


    // Create new free number node and add it to appropriate queue
    NodeLL* number_node = (NodeLL*) malloc(sizeof(NodeLL));
    MALLOC_CHECK(number_node);
    BZERO(number_node, sizeof(NodeLL));
    number_node->id = number;
    number_node->data = NULL;
    enqueue(queue, number_node);

    return 0;
}

//=================================
// KERNEL BOOTSTRAPPING FUNCTIONS
//=================================

// Initialize the init process
// Returns pointer to the init process PCB node if successful
// Function is only not successful if malloc fails, in which case the
// current process exits with ERROR_MALLOC_FAILED
NodeLL* initInitProcess(char* cmd_args[],
			UserContext *uctxt, 
			unsigned int pmem_size)
{
    int i;
    
    // Create a PCB for the idle process
    pcb *init_process_pcb = (pcb *) malloc(sizeof(pcb));
    MALLOC_CHECK(init_process_pcb);
    BZERO(init_process_pcb, sizeof(pcb));
    init_process_pcb->pid = pidcount;
    TracePrintf(1, "IDLE PROCESS PCB ID IS %d\n", init_process_pcb->pid);
    pidcount++;

    // Create pipes and children queues for this process
    init_process_pcb->children = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(init_process_pcb->children);
    BZERO(init_process_pcb->children, sizeof(Queue));
    init_process_pcb->pipes = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(init_process_pcb->pipes);
    BZERO(init_process_pcb->pipes, sizeof(Queue));
    init_process_pcb->graveyard = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(init_process_pcb->graveyard);
    BZERO(init_process_pcb->pipes, sizeof(Queue));

    // Copy the first user context into this PCB
    memcpy(&(init_process_pcb->usercontext), uctxt, sizeof(UserContext));

    // Initialize Region 1 pagetable in this PCB
    for(i = 0; i < MAX_PT_LEN; i++)
    {
	if(i < MAX_PT_LEN-1)
	{
	    init_process_pcb->pagetable1[i].valid = 0;
	    init_process_pcb->pagetable1[i].prot = 0;
	    init_process_pcb->pagetable1[i].pfn = 0;
	}
	// For the last frame, map it to valid for the user stack
	else
	{
	    init_process_pcb->pagetable1[i].valid = 1;
	    init_process_pcb->pagetable1[i].prot = PROT_READ | PROT_WRITE;
	    init_process_pcb->pagetable1[i].pfn = (pmem_size/PAGESIZE)-1;
	    allocateFrame((pmem_size/PAGESIZE)-1);
	}
    }

    // Allocate the init process its kernel stack
    for(i=KERNEL_STACK_BASE; i<KERNEL_STACK_LIMIT; i+=PAGESIZE)
    {
	// Allocate memory for the stack
	allocateFrame((i >> PAGESHIFT));
	int index = (i >> PAGESHIFT) / 32;
	int offset = (i >> PAGESHIFT) % 32;

	int stackpage  = (i-KERNEL_STACK_BASE)/PAGESIZE;
	int physical_frame_no = i >> PAGESHIFT;
      
	// Copy these mappings into PCB's saved kernel stack mappings
	init_process_pcb->kernelstack[stackpage].valid = 1;
	init_process_pcb->kernelstack[stackpage].prot = PROT_READ | PROT_WRITE;
	init_process_pcb->kernelstack[stackpage].pfn = physical_frame_no;

	// Copy these mappings into PCB's saved kernel stack mappings
	// Note: mappings are 1-for-1
	pagetable0[physical_frame_no].valid = init_process_pcb->kernelstack[stackpage].valid;
	pagetable0[physical_frame_no].prot = init_process_pcb->kernelstack[stackpage].prot;
	pagetable0[physical_frame_no].pfn = init_process_pcb->kernelstack[stackpage].pfn;
	

	TracePrintf(100, "Frame %d allocated to first process's kernel stack page %d\n", physical_frame_no, physical_frame_no);
    }
    
    // Create a node for init process
    NodeLL *init_pcb_node = (NodeLL *) malloc(sizeof(NodeLL));    
    init_pcb_node->id = init_process_pcb->pid;
    init_pcb_node->next = NULL;
    init_pcb_node->prev = NULL;
    init_pcb_node->data = (void *)init_process_pcb;

    return init_pcb_node;
}

// Initialize the idle process
// Returns pointer to the idle process PCB node if successful
// Function is only not successful if malloc fails, in which case the
// current process exits with ERROR_MALLOC_FAILED
NodeLL* initIdleProcess(char* cmd_args[],     // pointer to array of arguments passed to KernelStart
			UserContext *uctxt)   // pointer to user context passed to KernelStart
{
    unsigned int i;

    // Create idle program's PCB
    pcb *idle_process_pcb = (pcb *) malloc(sizeof(pcb));
    MALLOC_CHECK(idle_process_pcb);
    BZERO(idle_process_pcb, sizeof(pcb));
    idle_process_pcb->pid = pidcount;
    pidcount++;
    
    // Idle process does not actually need these structures, but we
    // malloc them to be safe
    // Create pipes and children queues for this process
    idle_process_pcb->children = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(idle_process_pcb->children);
    BZERO(idle_process_pcb->children, sizeof(Queue));
    idle_process_pcb->pipes = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(idle_process_pcb->pipes);
    BZERO(idle_process_pcb->pipes, sizeof(Queue));
    idle_process_pcb->graveyard = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(idle_process_pcb->graveyard);
    BZERO(idle_process_pcb->graveyard, sizeof(Queue));

    // Copy first user context into this PCB and set its pc to
    // point to kernel idle process function
    memcpy(&(idle_process_pcb->usercontext), uctxt, sizeof(UserContext));    

    // Allocate the first process its kernel stack
    for(i=KERNEL_STACK_BASE; i<KERNEL_STACK_LIMIT; i+=PAGESIZE)
    {
	TracePrintf(100, "Setting up page %d for idle process\n", i>>PAGESHIFT);
	

	// Allocate memory for the stack
	int stackpage  = (i-KERNEL_STACK_BASE)/PAGESIZE;
	int physical_frame_no = allocateFreeFrame();
	if (ERROR == physical_frame_no)
	{
	    TracePrintf(1, "initIdleProcess: allocateFreeFrame() failed during bootstrap! Halting system...");
	    Halt();
	}

	// Copy these mappings into PCB's saved kernel stack mappings
	idle_process_pcb->kernelstack[stackpage].valid = 1;
	idle_process_pcb->kernelstack[stackpage].prot = PROT_READ | PROT_WRITE;
	idle_process_pcb->kernelstack[stackpage].pfn = physical_frame_no;

	TracePrintf(100, "Idle proces's Stack Page %d given frame %d\n", (i>>PAGESHIFT), physical_frame_no);
    }

    // Initialize Region 1 pagetable in this PCB
    for(i = 0; i < MAX_PT_LEN; i++)
    {
	    idle_process_pcb->pagetable1[i].valid = 0;
	    idle_process_pcb->pagetable1[i].prot = 0;
	    idle_process_pcb->pagetable1[i].pfn = 0;
    }

    // Make a node for the idle PCB
    NodeLL *idle_pcb_node = (NodeLL *) malloc(sizeof(NodeLL));    
    idle_pcb_node->id = idle_process_pcb->pid;
    idle_pcb_node->next = NULL;
    idle_pcb_node->prev = NULL;
    idle_pcb_node->data = (void *)idle_process_pcb;
    
    // Set global pointer to point to this PCB node
    idleprocess = idle_pcb_node;

    return idle_pcb_node;
}

// This is the primary entry point into the kernel
// Creates init and idle process, sets up page table and virtual memory,
// enables virtual memory before going to user mode
void KernelStart (char* cmd_args[],
		  unsigned int pmem_size,
		  UserContext *uctxt)
{
    int i; // loop incrementor
    int rc;
    
    remove_current_process = 0; // set a global flag
    
    virtual_memory_enabled = 0;
    kernelheaptop = (void *) KernelDataEnd;

    // Initialize interrupt vector    
    trapvector[0] = TrapKernelHandler;
    trapvector[1] = TrapClockHandler;
    trapvector[2] = TrapIllegalHandler;
    trapvector[3] = TrapMemoryHandler;
    trapvector[4] = TrapMathHandler;
    trapvector[5] = TrapReceiveHandler;
    trapvector[6] = TrapTransmitHandler;
    for (i = 7; i < TRAP_VECTOR_SIZE; i++)
    {
	trapvector[i] = TrapBad;
    }

    // Initialize REG_VECTOR_BASE to point to trap vector
    WriteRegister(REG_VECTOR_BASE, (unsigned int) trapvector);

    // Initialize page tables and first (current) process
    initDataStructures(pmem_size, uctxt, cmd_args);

    // Set registers to point to Region 0 page tables
    WriteRegister(REG_PTBR0, (unsigned int) pagetable0);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);

    // Enable virtual memory
    TracePrintf(100, "About to enable virtual memory...\n");
    WriteRegister(REG_VM_ENABLE, (unsigned int) 1);
    virtual_memory_enabled = 1;
    TracePrintf(100, "Virtual Memory Enabled!\n");

    // Init queues
    readyqueue = (Queue *) malloc(sizeof(Queue));
    delayqueue = (Queue *) malloc(sizeof(Queue));
    waitingqueue = (Queue *) malloc(sizeof(Queue));
    for(i=0; i<NUM_TERMINALS; i++)
    {
	writequeue[i] = (Queue *) malloc(sizeof(Queue));
	readqueue[i] = (Queue *) malloc(sizeof(Queue));
	readbuffer[i] = (char *) malloc(TERMINAL_MAX_LINE*sizeof(char));
	writebuffer[i] = (char *) malloc(TERMINAL_MAX_LINE*sizeof(char));
	MALLOC_CHECK(writequeue[i]);
	MALLOC_CHECK(readqueue[i]);
	MALLOC_CHECK(writebuffer[i]);
	MALLOC_CHECK(readbuffer[i]);
	BZERO(readqueue[i], sizeof(Queue));
	BZERO(writequeue[i], sizeof(Queue));
	BZERO(readbuffer[i], TERMINAL_MAX_LINE*sizeof(char));
	BZERO(writebuffer[i], TERMINAL_MAX_LINE*sizeof(char));
    }
    MALLOC_CHECK(readyqueue);
    MALLOC_CHECK(delayqueue);
    MALLOC_CHECK(waitingqueue);
    
    BZERO(readyqueue, sizeof(Queue));
    BZERO(delayqueue, sizeof(Queue));
    BZERO(waitingqueue, sizeof(Queue));
    
    
    // Make the idle process
    NodeLL* idle_pcb_node = initIdleProcess(cmd_args, uctxt); 

    // Load idle process program
    pcb *idle_process_pcb = (pcb *)(idleprocess->data);
    TracePrintf(100, "KernelStart: About to call LoadProgram on idle process...\n");
    rc = LoadProgram(IDLE_PROCESS_PATH, cmd_args, idle_process_pcb);
    if (0 != rc)
    {
	TracePrintf(100, "KernelStart: Error: LoadProgram failed! Halting machine...\n");
	Halt();
    }
    else
	TracePrintf(100, "KernelStart: Finished LoadProgram on idle process!\n");

    // Load init process program
    pcb *init_process_pcb = (pcb *)(currentprocess->data);
    TracePrintf(100, "KernelStart: About to call LoadProgram on init process...\n");
    
    // Load default if no argument is passed to run yalnix
    if (cmd_args[0] == NULL)
	rc = LoadProgram(DEFAULT_INIT_PROCESS_PATH, cmd_args, init_process_pcb);
    
    // Otherwise, use user-provided program
    else
	rc = LoadProgram(cmd_args[0], cmd_args, init_process_pcb);
    if (0 != rc)
    {
	TracePrintf(100, "KernelStart: Error: LoadProgram failed! Halting machine...\n");
	Halt();
    }
    else
	TracePrintf(100, "KernelStart: Finished LoadProgram on init process!\n");

    // Bootstrap idle process and init process with kernel contexts
    TracePrintf(100, "About to KCBootstrap idleprocess\n");
    KernelContextSwitch(KCBootstrap, (pcb*)(idleprocess->data), NULL);
    TracePrintf(100, "Done KCBootstrap with idleprocess\n");

    TracePrintf(100, "Current process pid is: %d\n", ((pcb*)(currentprocess->data))->pid);

    pcb *curr_process = (pcb *)(currentprocess->data);

    // Set PC and SP to make userland process returns to run
    // the init process    
    uctxt->pc = curr_process->usercontext.pc; 
    uctxt->sp = curr_process->usercontext.sp;

    TracePrintf(100, "Finished KernelStart %d!\n", RESERVED_KERNEL_PAGE);
}

// Initialize the freeframes queue, global queues, region 0 page table,
// init and idle process pcbs
void initDataStructures(unsigned int pmem_size, UserContext *uctxt, char* cmd_args[])
{
    int i;
    NodeLL* curr_frame_node;
    NodeLL* curr_node;
    
    // Instantiate queue for free frames
    pframeslength = (pmem_size/PAGESIZE);
    TracePrintf(100, "Number of physical frames: %d\n", pframeslength);
    freeframes = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(freeframes);
    BZERO(freeframes, sizeof(Queue));
    for (i = 0; i < pframeslength; i++)
    {
	curr_frame_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(curr_frame_node);
	BZERO(curr_frame_node, sizeof(NodeLL));
	curr_frame_node->id = i;
	curr_frame_node->data = NULL;
	curr_frame_node->prev = NULL;
	curr_frame_node->next = NULL;
	enqueue(freeframes, curr_frame_node);
    }

    // Instantiate queue for free lock numbers
    freelocks = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(freelocks);
    BZERO(freelocks, sizeof(Queue));
    
    for (i = 0; i < MAX_LOCKS; i++)
    {
	curr_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(curr_node);
	BZERO(curr_node, sizeof(NodeLL));
	curr_node->id = i;
	curr_node->data = NULL;
	curr_node->prev = NULL;
	curr_node->next = NULL;
	enqueue(freelocks, curr_node);
    }

    // Instantiate queue for free cvar numbers
    freecvars = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(freecvars);
    BZERO(freecvars, sizeof(Queue));
    for (i = 0; i < MAX_CVARS; i++)
    {
	curr_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(curr_node);
	BZERO(curr_node, sizeof(NodeLL));
	curr_node->id = i;
	curr_node->data = NULL;
	curr_node->prev = NULL;
	curr_node->next = NULL;
	enqueue(freecvars, curr_node);
    }

    // Instantiate queue for free sem numbers
    freesems = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(freesems);
    BZERO(freesems, sizeof(Queue));
    for (i = 0; i < MAX_CVARS; i++)
    {
	curr_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(curr_node);
	BZERO(curr_node, sizeof(NodeLL));
	curr_node->id = i;
	curr_node->data = NULL;
	curr_node->prev = NULL;
	curr_node->next = NULL;
	enqueue(freesems, curr_node);
    }

    // Instantiate queue for free pipe numbers
    freepipes = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(freepipes);
    BZERO(freepipes, sizeof(Queue));
    for (i = 0; i < MAX_PIPES; i++)
    {
	curr_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(curr_node);
	BZERO(curr_node, sizeof(NodeLL));
	curr_node->id = i;
	curr_node->data = NULL;
	curr_node->prev = NULL;
	curr_node->next = NULL;
	enqueue(freepipes, curr_node);
    }

    NodeLL *init_pcb_node = initInitProcess(cmd_args, uctxt, pmem_size);
    currentprocess = init_pcb_node;


    // Set up region 0 page table 

    unsigned int data_start_page = (KernelDataStart >> PAGESHIFT);
    unsigned int data_end_page = (KernelDataEnd >> PAGESHIFT);
    unsigned int kernel_heap_top_page = ((unsigned int) kernelheaptop >> PAGESHIFT);
    
    // Set mappings and bit vectors for Kernel text
    for (i = 0; i < data_start_page; i++)
    {
	allocateFrame(i);
	pagetable0[i].valid = 1;
	pagetable0[i].prot = PROT_READ | PROT_EXEC;
	pagetable0[i].pfn = i;
	TracePrintf(100, "Kernel Text Page %d set to valid rx\n", i);
    }
    // Set mappings and bit vectors for Kernel data
    for (i = data_start_page; i < data_end_page; i++)
    {
	allocateFrame(i);
	pagetable0[i].valid = 1;
	pagetable0[i].prot = PROT_READ | PROT_WRITE;
	pagetable0[i].pfn = i;
	TracePrintf(100, "Kernel Data Page %d set to valid rw\n", i);
    }
    // Set mappings and bit vectors for Kernel heap (up to this point)
    for (i = data_end_page; i <= kernel_heap_top_page; i++)
    {
	allocateFrame(i);
	pagetable0[i].valid = 1;
	pagetable0[i].prot = PROT_READ | PROT_WRITE;
	pagetable0[i].pfn = i;
	TracePrintf(100, "Kernel Heap Page %d set to valid rw\n", i);
    }
    TracePrintf(100, "Done with bootstrapping Region 0 Page Table!\n");

    // Set flag to ensure that no malloc's are called after the region 0
    // page table bootstrap and enabling of virtual memory
    is_pagetable0_bootstrapped = 1;
}

void SetKernelData (void * _KernelDataStart, void *_KernelDataEnd)
{
    
    KernelDataStart = (unsigned int) _KernelDataStart;
    KernelDataEnd = (unsigned int) _KernelDataEnd;     
}


//=======================
// LOAD PROGRAM FUNCTION
//=======================

/*
 *  Load a program into an existing address space.  The program comes from
 *  the Linux file named "name", and its arguments come from the array at
 *  "args", which is in standard argv format.  The argument "proc" points
 *  to the process or PCB structure for the process into which the program
 *  is to be loaded. 
 */
int
LoadProgram(char *name, char *args[], pcb *proc) 
/*
  ==>> declare the argument "proc" to be a pointer to your pcb or
  ==>> process descriptor data structure.  we assume you have a member
  ==>> of this structure used to hold the cpu context 
  ==>> for the process holding the new program.  
*/
{
    int fd;
    int (*entry)();
    struct load_info li;
    int i;
    char c;
    char *cp;
    char **cpp;
    char *cp2;
    int argcount;
    int size;
    int text_pg1;
    int data_pg1;
    int data_npg;
    int stack_npg;
    long segment_size;
    char *argbuf;
    char saved_name[PROC_NAME_LEN];
    
    // Save the userland name into this function stack
    BZERO(saved_name, PROC_NAME_LEN);
    strncpy(saved_name, name, PROC_NAME_LEN);
    saved_name[PROC_NAME_LEN-1] = 0;

    /*
     * Open the executable file 
     */
    if ((fd = open(name, O_RDONLY)) < 0) {
	TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
	return ERROR;
    }

    if (LoadInfo(fd, &li) != LI_NO_ERROR) {
	TracePrintf(0, "LoadProgram: '%s' not in Yalnix format\n", name);
	close(fd);
	return (-1);
    }

    if (li.entry < VMEM_1_BASE) {
	TracePrintf(0, "LoadProgram: '%s' not linked for Yalnix\n", name);
	close(fd);
	return ERROR;
    }

    /*
     * Figure out in what region 1 page the different program sections
     * start and end
     */
    text_pg1 = (li.t_vaddr - VMEM_1_BASE) >> PAGESHIFT;
    data_pg1 = (li.id_vaddr - VMEM_1_BASE) >> PAGESHIFT;
    data_npg = li.id_npg + li.ud_npg;
  
    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
  
    size = 0;
    for (i = 0; args[i] != NULL; i++) {
	TracePrintf(3, "counting arg %d = '%s'\n", i, args[i]);
	size += strlen(args[i]) + 1;
    }
    argcount = i;
  

    TracePrintf(2, "LoadProgram: argsize %d, argcount %d\n", size, argcount);
  
    /*
     *  The arguments will get copied starting at "cp", and the argv
     *  pointers to the arguments (and the argc value) will get built
     *  starting at "cpp".  The value for "cpp" is computed by subtracting
     *  off space for the number of arguments (plus 3, for the argc value,
     *  a NULL pointer terminating the argv pointers, and a NULL pointer
     *  terminating the envp pointers) times the size of each,
     *  and then rounding the value *down* to a double-word boundary.
     */
    cp = ((char *)VMEM_1_LIMIT) - size;

    cpp = (char **)
	(((int)cp - 
	  ((argcount + 3 + POST_ARGV_NULL_SPACE) *sizeof (void *))) 
	 & ~7);

    /*
     * Compute the new stack pointer, leaving INITIAL_STACK_FRAME_SIZE bytes
     * reserved above the stack pointer, before the arguments.
     */
    cp2 = (caddr_t)cpp - INITIAL_STACK_FRAME_SIZE;



    TracePrintf(1, "prog_size %d, text %d data %d bss %d pages\n",
		li.t_npg + data_npg, li.t_npg, li.id_npg, li.ud_npg);


    /* 
     * Compute how many pages we need for the stack */
    stack_npg = (VMEM_1_LIMIT - DOWN_TO_PAGE(cp2)) >> PAGESHIFT;

    TracePrintf(1, "LoadProgram: heap_size %d, stack_size %d\n",
		li.t_npg + data_npg, stack_npg);


    /* leave at least one page between heap and stack */
    if (stack_npg + data_pg1 + data_npg >= MAX_PT_LEN) {
	close(fd);
	return ERROR;
    }

    /*
     * This completes all the checks before we proceed to actually load
     * the new program.  From this point on, we are committed to either
     * loading succesfully or killing the process.
     */

    /*
     * Set the new stack pointer value in the process's exception frame.
     */

    //Here you replace your data structure proc
    // proc->context.sp = cp2;

    proc->usercontext.sp = cp2;

    /*
     * Now save the arguments in a separate buffer in region 0, since
     * we are about to blow away all of region 1.
     */
    cp2 = argbuf = (char *)malloc(size);
    if(cp2 == 0)
    {
	TracePrintf(1,"malloc failed\n");
	return ERROR;
    }
    for (i = 0; args[i] != NULL; i++) {
	TracePrintf(3, "saving arg %d = '%s'\n", i, args[i]);
	strcpy(cp2, args[i]);
	cp2 += strlen(cp2) + 1;
    }

   

    /*
     * Set up the page tables for the process so that we can read the
     * program into memory.  Get the right number of physical pages
     * allocated, and set them all to writable.
     */
/*
  ==>> Throw away the old region 1 virtual address space of the
  ==>> curent process by freeing
  ==>> all physical pages currently mapped to region 1, and setting all 
  ==>> region 1 PTEs to invalid.
  ==>> Since the currently active address space will be overwritten
  ==>> by the new program, it is just as easy to free all the physical
  ==>> pages currently mapped to region 1 and allocate afresh all the
  ==>> pages the new program needs, than to keep track of
  ==>> how many pages the new process needs and allocate or
  ==>> deallocate a few pages to fit the size of memory to the requirements
  ==>> of the new process.
*/
    for(i=0; i<MAX_PT_LEN; i++)
    {
	if (proc->pagetable1[i].valid == 1)
	    deallocateUserPage(proc, i);
    }
/*
  ==>> Allocate "li.t_npg" physical pages and map them starting at
  ==>> the "text_pg1" page in region 1 address space.  
  ==>> These pages should be marked valid, with a protection of 
  ==>> (PROT_READ | PROT_WRITE).
*/
    for(i=text_pg1; i< (text_pg1 + li.t_npg); i++)
    {
	
	proc->pagetable1[i].pfn = allocateFreeFrame();
	if (proc->pagetable1[i].pfn == ERROR)
	{
	    return ERROR;
	}
	TracePrintf(100, "Text: Allocating frame %d to page %d for init process text\n", proc->pagetable1[i].pfn, i);
	proc->pagetable1[i].valid = 1;
	proc->pagetable1[i].prot = PROT_READ | PROT_WRITE;
    }
  
/*
  ==>> Allocate "data_npg" physical pages and map them starting at
  ==>> the  "data_pg1" in region 1 address space.  
  ==>> These pages should be marked valid, with a protection of 
  ==>> (PROT_READ | PROT_WRITE).
*/
    for(i=data_pg1; i < (data_pg1 + data_npg); i++)
    {
	TracePrintf(100, "Data: Allocating page %d for init process data\n", i);
	proc->pagetable1[i].pfn = allocateFreeFrame();
	if (proc->pagetable1[i].pfn == ERROR)
	{
	    return ERROR;
	}
	proc->pagetable1[i].valid = 1;
	proc->pagetable1[i].prot = PROT_READ | PROT_WRITE;
    }
  
    // Set user heap top pointer to highest boundary address of the highest
    // page we allocated for user data
    proc->userheaptop = (void *) UP_TO_PAGE(((data_pg1 + data_npg)-1+MAX_PT_LEN)<<PAGESHIFT);
  
    /*
     * Allocate memory for the user stack too.
     */
/*
  ==>> Allocate "stack_npg" physical pages and map them to the top
  ==>> of the region 1 virtual address space.
  ==>> These pages should be marked valid, with a
  ==>> protection of (PROT_READ | PROT_WRITE).
*/

    for(i = (MAX_PT_LEN - 1); i >= (MAX_PT_LEN - stack_npg); i--)
    {
	TracePrintf(100, "Stack: Allocating page %d for init process stack\n", i);
	proc->pagetable1[i].pfn = allocateFreeFrame();
	if (proc->pagetable1[i].pfn == ERROR)
	{
	    return ERROR;
	}
	proc->pagetable1[i].valid = 1;
	proc->pagetable1[i].prot = PROT_READ | PROT_WRITE;
    }

    /*
     * All pages for the new address space are now in the page table.  
     * But they are not yet in the TLB, remember!
     */
    /*
     * Read the text from the file into memory.
     */
    lseek(fd, li.t_faddr, SEEK_SET);
    segment_size = li.t_npg << PAGESHIFT;
    
    
    // Set region 1 page table registers
    WriteRegister(REG_PTBR1, (unsigned int) proc->pagetable1);
    WriteRegister(REG_PTLR1, (unsigned int) MAX_PT_LEN);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
   
    if (read(fd, (void *) li.t_vaddr, segment_size) != segment_size) {
	close(fd);
/*
  ==>> KILL is not defined anywhere: it is an error code distinct
  ==>> from ERROR because it requires different action in the caller.
  ==>> Since this error code is internal to your kernel, you get to define it.
*/
	return KILL;
    }
    /*
     * Read the data from the file into memory.
     */
    
    lseek(fd, li.id_faddr, 0);
    segment_size = li.id_npg << PAGESHIFT;
    
    if (read(fd, (void *) li.id_vaddr, segment_size) != segment_size) {
	close(fd);
	return KILL;
    }

    /*
     * Now set the page table entries for the program text to be readable
     * and executable, but not writable.
     */
/*
  ==>> Change the protection on the "li.t_npg" pages starting at
  ==>> virtual address VMEM_1_BASE + (text_pg1 << PAGESHIFT).  Note
  ==>> that these pages will have indices starting at text_pg1 in 
  ==>> the page table for region 1.
  ==>> The new protection should be (PROT_READ | PROT_EXEC).
  ==>> If any of these page table entries is also in the TLB, either
  ==>> invalidate their entries in the TLB or write the updated entries
  ==>> into the TLB.  Its nice for the TLB and the page tables to remain
  ==>> consistent.
*/
    for(i=text_pg1; i < (text_pg1 + li.t_npg); i++)
    {
	TracePrintf(100, "Changing protection on page %d\n", i);
	proc->pagetable1[i].prot = PROT_READ | PROT_EXEC;
	WriteRegister(REG_TLB_FLUSH, (MAX_PT_LEN + i) << PAGESHIFT);
    }
    

    close(fd);			/* we've read it all now */

    /*
     * Zero out the uninitialized data area
     */
    bzero(li.id_end, li.ud_end - li.id_end);

    /*
     * Set the entry point in the exception frame.
     */
/*
  ==>> Here you should put your data structure (PCB or process)
  ==>>  proc->context.pc = (caddr_t) li.entry;
*/
    proc->usercontext.pc = (caddr_t) li.entry;

    /*
     * Now, finally, build the argument list on the new stack.
     */

#ifdef LINUX
    memset(cpp, 0x00, VMEM_1_LIMIT - ((int) cpp));
#endif



    *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
	*cpp++ = cp;
	strcpy(cp, cp2);
	cp += strlen(cp) + 1;
	cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL;			/* the last argv is a NULL pointer */
    *cpp++ = NULL;			/* a NULL pointer for an empty envp */

    // Set the file name for this process to the program name, excluding
    // the file path

    char *name_without_path = (char *) strrchr(saved_name, '/');
    if (NULL != name_without_path)
    {
	name_without_path++;
	strncpy(proc->name, name_without_path, PROC_NAME_LEN);
    }
    else
    {
	strncpy(proc->name, saved_name, PROC_NAME_LEN);
    }

    return 0;
}


// Checks if a pointer to a memory address to read from points to
// a readable memory address.
//  Kills current process if the pointer is not in such a location.
/*
Checks whether memory block at ptr with size len bytes has protections equal 
to check_prot. If ptr points to a string, then the size of the string may not 
be known, so boolean flag string keeps track of it.
 */
void pointerCheck(int check_prot, void *ptr, int string, int len)
{
    pcb* curr_process = (pcb*)(currentprocess->data);
    int page_no;
    int start_page_no = DOWN_TO_PAGE(ptr) >> PAGESHIFT;
    int end_page_no;
    unsigned int c;

    void *end_addr = ptr + len;
    
    //If memory to be checked is a string, we want to look for the null terminator instead of stopping at a certain page
    if(string == 0)
	end_page_no = DOWN_TO_PAGE(end_addr) >> PAGESHIFT;
    else
	end_page_no = 2*MAX_PT_LEN;

    for(page_no = start_page_no; page_no <= end_page_no; page_no++)
    {
    
	/*
	// If address is in kernel space. Check region 0 page table
	if (page_no < MAX_PT_LEN &&
	    page_no >= 0)
	{
	    if (0 == (pagetable0[page_no].prot & check_prot))
		break;	    
	}
	

	// If address is in user space. Check region 1 page table
	else*/ if(page_no >= MAX_PT_LEN &&
		page_no < 2*MAX_PT_LEN)
	{
	    if (0 == ((curr_process->pagetable1[page_no - MAX_PT_LEN].prot) & check_prot))
		break;
	    	// If code reaches here, then page page_no is readable. If ptr is a string, then look for the null-terminator, and if you found it, then the whole string must be readable
	    if(string == 1)
		for(c = (unsigned int)ptr; c < (unsigned int)ptr + PAGESIZE; c++)
		    if(*(char *)c == 0)
			return;

	}
	// If address is outside virtual memory space, break
	else break;
    }

    //If this check passes, then the loop did not finish, which means a page is not readable
    if(page_no <= end_page_no)
    {
	TracePrintf(1, "pointerCheck: pointer %08x points to memory that does not have required protection(s) %d. Exiting process %d...\n", ptr, check_prot, curr_process->pid);
	SysExit(ERROR_PTR_READ_PERM);
    }
}


