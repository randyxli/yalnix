#include "syscalls.h"
#include "yalnix.h"
#include "hardware.h"
#include "datastructures.h"
#include "kernel.h"
#include "header.h"
#include <unistd.h>

//===========
// SYSCALLS
//===========

int SysFork (UserContext *context) // void input
{
    TracePrintf(50, "SysFork: Entering Fork\n");

    int i, rc;
    unsigned int page_i;
    pte *pte_i;
    pcb *curr_proc = (pcb *)(currentprocess->data);
    
    pcb *new_proc = (pcb *) malloc(sizeof(pcb));
    MALLOC_CHECK(new_proc);
    new_proc->pid = pidcount;
    pidcount++;
    strncpy(new_proc->name, curr_proc->name, PROC_NAME_LEN);

    // Copy the passed user context into both parent and child
    curr_proc->usercontext = *context;
    new_proc->usercontext = *context;
    
    // page to temporarily hold data
    void *dummy_page = (void *) malloc(PAGESIZE);
    MALLOC_CHECK(dummy_page);

    // Walk down from top of region 0 pages to find a free dummy page
    int free_page;
    for(i = ((VMEM_0_LIMIT-1) >> PAGESHIFT); i >= 0 ; i--)    
	if(pagetable0[i].valid == 0)
	{
	    free_page = i;
	    pagetable0[free_page].valid = 1;
	    pagetable0[free_page].prot = PROT_READ | PROT_WRITE;
	    pagetable0[free_page].pfn = allocateFreeFrame();
	    if(pagetable0[free_page].pfn == ERROR)
	    {
		TracePrintf(1, "SysFork: New frame not found via allocateFreeFrame(). Exiting current process...\n");
		SysExit(ERROR_NO_FREE_FRAME);
	    }
	    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
	    break;
	}
/*
    // Copy frame contents of region 0 via the dummy page
    for(i=0; i < MAX_PT_LEN; i++)
    {
	pte_i = &(new_proc->pagetable1[i]);

	if(curr_proc->pagetable1[i].valid == 1)
	{
	    memcpy(free_page << PAGESHIFT, (i+MAX_PT_LEN) << PAGESHIFT, PAGESIZE);

	    (*pte_i).valid = 1;
	    (*pte_i).prot = PROT_READ | PROT_WRITE;
	    (*pte_i).pfn = pagetable0[free_page].pfn;
	    pagetable0[free_page].pfn = allocateFreeFrame();
	    if(pagetable0[free_page].pfn == ERROR)
	    {
		TracePrintf(1, "SysFork: New frame not found via allocateFreeFrame(). Exiting current process...\n");
		SysExit(ERROR_NO_FREE_FRAME);
	    }
	    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
	}
    }
*/   
    
    // initialize pagetable1 for the new process
    
    memcpy(&(new_proc->pagetable1), &(curr_proc->pagetable1), MAX_PT_LEN*sizeof(pte));    
    for(i=0; i<MAX_PT_LEN; i++)
    {
	pte_i = &(curr_proc->pagetable1[i]); // current proccess's pagetable1
	page_i = (i + MAX_PT_LEN); // page in Region 1 to copy
	if((*pte_i).valid == 1)
	{
	    //copy contents of a frame into the dummy page, and then copy those contents to a new frame
	    memcpy(dummy_page, page_i << PAGESHIFT, PAGESIZE);
	    (*pte_i).pfn = allocateFreeFrame();
	    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	    if(((*pte_i).prot & PROT_WRITE) == 0) // if this frame isn't writable make it writable temporarily to copy contents in
	    {
		
	        curr_proc->pagetable1[i].prot = (*pte_i).prot | PROT_WRITE;
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
		//printPageTables(curr_proc);
		memcpy(page_i << PAGESHIFT, dummy_page, PAGESIZE);
		(*pte_i).prot = (*pte_i).prot ^ PROT_WRITE;
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	    }
	    else
	    {
		//printPageTables(curr_proc);
		memcpy(page_i << PAGESHIFT, dummy_page, PAGESIZE);
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
	    }
	}
    }

    if(new_proc->kernelstack[1].pfn == ERROR)
    {
	TracePrintf(1, "SysFork: New frame not found via allocateFreeFrame(). Exiting current process...\n");
	SysExit(ERROR_NO_FREE_FRAME);
    }

    //copy the kernel stack
    for(i=0; i < KERNEL_STACK_MAXSIZE/PAGESIZE; i++)
    {
	pte_i = &new_proc->kernelstack[i];
	page_i = (KERNEL_STACK_BASE >> PAGESHIFT) + i;
	    // copy kernel stack contents into the new frame
	    memcpy(free_page << PAGESHIFT, page_i << PAGESHIFT, PAGESIZE);

	    (*pte_i).valid = 1;
	    (*pte_i).prot = PROT_READ | PROT_WRITE;
	    (*pte_i).pfn = pagetable0[free_page].pfn; // point to the frame containing kernel stack content
	    pagetable0[free_page].pfn = allocateFreeFrame();
	    if(pagetable0[free_page].pfn == ERROR)
	    {
		TracePrintf(1, "SysFork: New frame not found via allocateFreeFrame(). Exiting current process...\n");
		SysExit(ERROR_NO_FREE_FRAME);
	    }	
    }

    // deallocate free_page and it's current (empty) frame
    pagetable0[free_page].valid = 0;
    deallocateFrame(pagetable0[free_page].pfn);
    WriteRegister(REG_TLB_FLUSH, free_page << PAGESHIFT);
    free(dummy_page);
    
    // Create pipes queue and populate it with the pipes owned by
    // parent process
    new_proc->pipes = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(new_proc->pipes);
    BZERO(new_proc->pipes, sizeof(Queue));
    NodeLL *node = curr_proc->pipes->head;
    while(node != NULL)
    {
	NodeLL *new_node = (NodeLL *) malloc(sizeof(NodeLL));
	MALLOC_CHECK(new_node);
	memcpy(new_node, node, sizeof(NodeLL));
	enqueue(new_proc->pipes, new_node);
	node = node->next;
    }

    new_proc->lockwaitedon = curr_proc->lockwaitedon;
    new_proc->cvarwaitedon = curr_proc->cvarwaitedon;
    new_proc->exitstatus; // leave uninitialized since exit status can be anything
    new_proc->clockticksleft = curr_proc->clockticksleft;

    new_proc->children = (Queue *) malloc(sizeof(Queue));
    MALLOC_CHECK(new_proc->children);
    BZERO(new_proc->children, sizeof(Queue));
    new_proc->children->head = NULL;
    new_proc->children->tail = NULL;

    new_proc->graveyard = (Queue *) malloc(sizeof(Queue));
    new_proc->graveyard->head = NULL;
    new_proc->graveyard->tail = NULL;
    

    new_proc->parent = curr_proc;
    new_proc->userheaptop = curr_proc->userheaptop;

    NodeLL *new_child_node = (NodeLL *) malloc(sizeof(NodeLL));
    MALLOC_CHECK(new_child_node);
    new_child_node->data = new_proc;
    new_child_node->next = NULL;
    new_child_node->prev = NULL;
    new_child_node->id = new_proc->pid;
    enqueue(curr_proc->children, new_child_node);

    NodeLL *new_pcb_node = (NodeLL *) malloc(sizeof(NodeLL));
    MALLOC_CHECK(new_pcb_node);
    new_pcb_node->data = new_proc;
    new_pcb_node->next = NULL;
    new_pcb_node->prev = NULL;
    new_pcb_node->id = new_proc->pid;
    enqueue(readyqueue, new_pcb_node);
    KernelContextSwitch(KCBootstrap, new_proc, NULL);
   
    pcb *current_process = (pcb*)(currentprocess->data);
    
    // Make sure right region 1 page table is set if we are the child
    WriteRegister(REG_PTBR1, (unsigned int) current_process->pagetable1);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

    // Return 0 if we are the child (i.e. no children)
    if(isQueueEmpty(current_process->children) != 0)
    {
	return 0;
    }
    return new_proc->pid; // if not, return child id
}

int SysExec (char *filename, /* path of program to execute */
	     char **argvec) /* pointer to vector of arguments to pass to new program */
{
    TracePrintf(100, "SysExec: Entering with filename: %s, argvec[0]: %08x\n", filename, argvec[0]);
    pcb* curr_process = (pcb*)(currentprocess->data);
    TracePrintf(100, "SysExec: About to Load Program...\n");
    int rc = LoadProgram(filename, argvec, curr_process);
    TracePrintf(100, "SysExec: Finished Load Program...\n");
    if (KILL == rc)
    {
	SysExit(LOADPROGRAM_FAILED);
    }
    else if (ERROR == rc)
    {
	TracePrintf(100, "SysExec: Exiting with ERROR...\n");
	return ERROR;
    }
}

void SysExit(int status) /* status of exiting process */
{
    TracePrintf(100, "SysExit: Entering...\n");
    pcb* curr_process = (pcb*)(currentprocess->data);
    int curr_process_pid = curr_process->pid;
    NodeLL* removed;
    NodeLL* temp;
    NodeLL* current_node;
    int i;

    // Add a graveyard entry if this process's parent is not dead
    if (NULL != curr_process->parent)
    {
	// Create a new graveyard node for this process and
	// put it in graveyard
	NodeLL* graveyard_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(graveyard_node);
	BZERO(graveyard_node, sizeof(NodeLL));
	graveyard_node->id = curr_process->pid;
	graveyard_node->data = (void *)status;
	enqueue(curr_process->parent->graveyard, graveyard_node);
	
	// Remove this process from parent's children queue
	removed = removeFromList(curr_process->parent->children, curr_process_pid);
	if (NULL == removed)
	{
	    TracePrintf(1, "SysExit: Failed to remove child from parent's children list. Exiting current process...\n");
	    SysExit(ERROR_REMOVE_LIST_FAILED);
	}
	else
	    free(removed);

	// If parent is in waiting queue, take it off and put it
	// back on ready queue
	pcb* parent = curr_process->parent;
	int parent_pid = parent->pid;
	NodeLL* parent_wait_node = removeFromList(waitingqueue, parent_pid);
	if (NULL != parent_wait_node)
	{
	    TracePrintf(100, "Removing process %d from waitingqueue\n", parent_pid);
	    NodeLL* parent_pcb_node = (NodeLL*)(parent_wait_node->data);
	    enqueue(readyqueue, parent_pcb_node);
	    free(parent_wait_node); 
	}
	else
	{
	    TracePrintf(100, "SysExit: Parent not in waiting queue right now!\n");
	}    
    }

    // If this process owns any lock, release it
    for (i = 0; i < MAX_LOCKS; i++)
    {
	if(locks[i] != NULL)
	{
	    if(locks[i]->owner == curr_process_pid)
	    {
		SysRelease(locks[i]->id);
	    }
	}
    }

    // If this process is blocking on any cvar, take it off that queue
    // and free the blocking node
    for (i = 0; i < MAX_CVARS; i++)
    {
	if (cvars[i] != NULL)
	{
	    if (isQueueEmpty(cvars[i]->blocking) != 1)
	    {
		NodeLL* proc_node = removeFromList(cvars[i]->blocking, curr_process_pid);
		if (proc_node != NULL)
		    free(proc_node);
	    }
	}
    }

    // Deallocate stack frames
    for(i=KERNEL_STACK_BASE; i<KERNEL_STACK_LIMIT; i+=PAGESIZE)
    {
	int stackpage  = (i-KERNEL_STACK_BASE)/PAGESIZE;
	int stackframe = curr_process->kernelstack[stackpage].pfn;
	deallocateFrame(stackframe);
    }
	
    // Deallocate user frames
    for(i=0; i<MAX_PT_LEN; i++)
    {
	if (curr_process->pagetable1[i].valid == 1)
	    deallocateUserPage(curr_process, i);
    }
    
    // Free children queue and make sure all children now have no parent and
    current_node = curr_process->children->head;
    while (current_node != NULL)
    {
	temp = current_node;
	pcb* current_child = (pcb *)(current_node->data);
	current_child->parent = NULL;
	current_node = current_node->next;
	free(temp);
    }
    free(curr_process->children);

    // Free graveyard
    current_node = curr_process->graveyard->head;
    while (current_node != NULL)
    {
	temp = current_node;
	current_node = current_node->next;
	free(temp);
    }
    free(curr_process->graveyard);

    // Free pipes queue
    current_node = curr_process->pipes->head;
    while (current_node != NULL)
    {
	temp = current_node;
	current_node = current_node->next;
	free(temp);
    }
    free(curr_process->pipes);

    // Set flag for scheduler() to free the current process's pcb node
    // and myKCS() to free the current process's pcb
    remove_current_process = 1;

    // If the init process (process 0) just exited, halt system)
    if (curr_process->pid == 0)
    {
	TracePrintf(100, "SysExit: Init process exited. Halting system...\n");
	Halt();
    }
    // Otherwise, Call scheduler to run next available process, and to
    // clean up later on the next kernel context switch
    else
    {
    TracePrintf(100, "SysExit: Scheduling...\n");
    scheduler();
    }
    TracePrintf(100, "SysExit: ERROR: Exiting (SHOULD NOT PRINT)...\n");
}

int SysWait (int *status_ptr) /* integer to child exit status in*/
{
    TracePrintf(50, "SysWait: Entering...\n");

    pcb* curr_process = (pcb *)(currentprocess->data);    
    int rc, child_pid;
    
    // If current process has no children, there is nothing to wait on
    if (isQueueEmpty(curr_process->children) != 0)
    {
	TracePrintf(100, "SysWait: Process has no children!\n");
	return ERROR;
    }

    // If current process has nothing in its graveyard, we need
    // to block in waiting queue
    if(isQueueEmpty(curr_process->graveyard) != 0)
    {
	TracePrintf(100, "SysWait: Nothing in my graveyard! Blocking...\n");
	NodeLL* waitNode = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(waitNode);
	BZERO(waitNode, sizeof(NodeLL));
	waitNode->id = curr_process->pid;
	waitNode->data = currentprocess;
	enqueue(waitingqueue, waitNode);
	scheduler();
    }

    // There is now at least one child in the graveyard   
    NodeLL* dead_child = dequeue(curr_process->graveyard);
    

    // Free graveyard node
    TracePrintf(100, "Removing process %d from graveyard\n", dead_child->id);
    rc = (int)(dead_child->data); // extract the return status at exit
    int dead_child_id = dead_child->id;
    free(dead_child);

	    
    // Write the saved return status using pointer given to us
    // and return dead child's id
    *status_ptr = rc;
    TracePrintf(50, "SysWait: Exiting...\n");
    return dead_child_id;
}

int SysGetPid (void)
{  
    return ((pcb *)(currentprocess->data))->pid;
}

int SysBrk(void *addr)
{
    TracePrintf(50, "SysBrk: Entering SysBrk. addr is %08x\n", (unsigned int) addr);
    unsigned int i;
    pcb *curr_process = (pcb *)(currentprocess->data);
  
    // Check if we are trying to grow user heap into user stack
    // Check that addr is not in user stack region or higher. If so, return error
    if ( ((unsigned int)addr >> PAGESHIFT) >= (DOWN_TO_PAGE(curr_process->usercontext.sp)>>PAGESHIFT) )
    {
	TracePrintf(1, "Error: Attempt to grow user heap into user stack or higher\n");
	return ERROR;
    }


    // if the page addr is on isn't allocated yet, then allocate memory for all pages between that page and kernelheaptop
    if (0 == curr_process->pagetable1[((unsigned int)addr>>PAGESHIFT) - MAX_PT_LEN].valid)
	for(i = NEXT_PAGE(curr_process->userheaptop); i <= DOWN_TO_PAGE(addr)>>PAGESHIFT; i++)
	{	
	    int pt_index = i - MAX_PT_LEN;
	    
	    if(curr_process->pagetable1[pt_index].valid == 0)
	    {		    
		int allocated_frame = allocateFreeFrame();
		if(allocated_frame == ERROR)
		    return ERROR;
		else
		{
		    TracePrintf(100, "Sysbrk: Allocating page %d\n", pt_index);
		    curr_process->pagetable1[pt_index].valid = 1;
		    curr_process->pagetable1[pt_index].prot = PROT_READ | PROT_WRITE;
		    curr_process->pagetable1[pt_index].pfn = allocated_frame;
		}
	    }
	}	
    // if the page addr is on is allocated. *****UNTESTED*****
    else	
	for(i = NEXT_PAGE(addr); i <= DOWN_TO_PAGE(curr_process->userheaptop)>>PAGESHIFT; i++)
	{
	    int pt_index = i - MAX_PT_LEN;

	    if(curr_process->pagetable1[pt_index].valid == 1)
	    {
		pagetable0[pt_index].valid = 0;
		deallocateFrame(pagetable0[pt_index].pfn);
		WriteRegister(REG_TLB_FLUSH, i << PAGESHIFT);
	    }	    
	}
    curr_process->userheaptop = addr;
    TracePrintf(1, "SysBrk: New heap top for process %d is %d\n", curr_process->pid, addr);

    TracePrintf(50, "Sysbrk: Leaving Sysbrk\n");
    return 0;
}

int SysDelay (int clock_ticks) /* number of ticks to block for */
{
    TracePrintf(100, "SysDelay: Entering....\n");
    if (clock_ticks == 0)
	return 0;
    else if (clock_ticks < 0)
    {
	TracePrintf(1, "SysDelay: Error: clock_ticks %d is less than 0!\n", clock_ticks);
	return ERROR;
    }
    else
    {
	NodeLL* delayNode = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(delayNode);
	BZERO(delayNode, sizeof(NodeLL));
	delayNode->next = NULL;
	delayNode->prev = NULL;
	delayNode->id = clock_ticks;
	delayNode->data = currentprocess;
	enqueue(delayqueue, delayNode);
	scheduler();
	return 0;
    }
    return ERROR;
}

/*
 * Note: Only reads the last line entered by the user. If more than one line is entered
 * by the user before a process calls TtyRead, only the last line is saved in the buffer
 * and read by the process on that call to TtyRead.  We chose this approach we think it 
 * best emulates the behavior of the Linux shell (i.e. if no process associated with the
 * shell is blocking for input, all but the last line entered by user is dropped). Next,
 * we felt that implementing unbounded buffering of inputs received before TtyRead is called 
 * is too complicated and costly to justify the benefits
 */
int SysTtyRead (int tty_id, /* terminal to read from  */
		void *buf, /* address of the buffer to write to */
		int len) /* number of characters to read into buffer */
{
    TracePrintf(50, "SysTtyRead: Entering\n");
    char *buffer = readbuffer[tty_id];
    int i;
    int bytes_read;
    if(len < 0)
    {
	TracePrintf(1, "SysTtyRead: Error: trying to read %d bytes, but must read a non-negative number of bytes\n", len);
	return ERROR;
    }

    if(len > TERMINAL_MAX_LINE)
    {
	TracePrintf(1, "SysTtyRead: Error: trying to read, %d, but must read fewer than %d bytes\n", len, TERMINAL_MAX_LINE);
	return ERROR;
    }

    if(len == 0)
	return 0;

    //put current process into the readqueue for the terminal tty_id. If it's not first in line to read, block self by switching to another process
    NodeLL *read_node = (NodeLL *) malloc(sizeof(NodeLL));
    read_node->data = currentprocess;
    read_node->prev = NULL;
    read_node->next = NULL;
    read_node->id = currentprocess->id;
    enqueue(readqueue[tty_id], read_node);
    while(readqueue[tty_id]->head != read_node)
    {
	scheduler();
    }
    

    //if the code is executing here, then the current process must be first in line to read from the terminal
    //first check to see if there is a newline character in the kernel buffer. If there isn't, we must block
    for(i=0; i<TERMINAL_MAX_LINE; i++)
    {
	if(readbuffer[tty_id][i] == '\n')
	    break;
    }

    if(i == TERMINAL_MAX_LINE)
    {
	TracePrintf(75, "SysTtyRead: Nothing to read from terminal %d buffer, waiting for input\n", tty_id);
	scheduler();
    }
    
    //if code is executing here, then there is something to read from the buffer
    if(i+1 < len)	
	bytes_read = i+1;
    else    
	bytes_read = len;
    
    //copy as many bytes as possible up to len bytes into buf, move up all the remaining bytes to the beginning, and then clear the moved bytes
    memcpy(buf, readbuffer[tty_id], bytes_read);
    char tmp[TERMINAL_MAX_LINE];
    memcpy(tmp, &readbuffer[tty_id][bytes_read], TERMINAL_MAX_LINE - bytes_read);
    memcpy(readbuffer[tty_id], tmp, TERMINAL_MAX_LINE - bytes_read);
    //BZERO(&readbuffer[tty_id][bytes_read], TERMINAL_MAX_LINE - bytes_read);

    //remove self from readqueue
    dequeue(readqueue[tty_id]);
    if(isQueueEmpty(readqueue[tty_id]) == 0)
	enqueue(readyqueue, readqueue[tty_id]->head);

    TracePrintf(50, "SysTtyRead: Exiting\n");
    return bytes_read;
}

int SysTtyWrite (int tty_id, /* id to terminal to write to */
		 void * buf, /* pointer to buffer to read from */
		 int len)    /* max length of bytes to write */
{
    int bytes_to_write;
    TracePrintf(50, "SysTtyWrite: Entering\n");
    char *buffer = writebuffer[tty_id];
    
    if(len < 0)
    {
	TracePrintf(1, "SysTtyWrite: Error: len must be greater than 0\n");
	return ERROR;
    }
    
    if(len == 0)
	return 0;

    //put current process into the writequeue for terminal tty_id. If it's not first in line to write, block self by switching to another process
    NodeLL *write_node = (NodeLL *) malloc(sizeof(NodeLL));
    write_node->data = currentprocess;
    write_node->prev = NULL;
    write_node->next = NULL;
    write_node->id = currentprocess->id;
    enqueue(writequeue[tty_id], write_node);
    while(writequeue[tty_id]->head != write_node)
    {
	scheduler();
    }

    //if code is executing here, this means current process is first in line to write. It will monopolize terminal tty_id until it is done writing.
    int tlen = len;
    while(len > 0)
    {
	
	//write as many bytes as possible to the terminal
	if(len >= TERMINAL_MAX_LINE)	
	    bytes_to_write = TERMINAL_MAX_LINE;	
	else
	    bytes_to_write = len;

	TracePrintf(75, "SysTtyWrite: Printing %d bytes.\n", bytes_to_write);
	BZERO(writebuffer[tty_id], TERMINAL_MAX_LINE);
	memcpy(writebuffer[tty_id], buf + (tlen-len), bytes_to_write);

	//write to the terminal, and as we're waiting for it to happen, change to another process
	TtyTransmit(tty_id, writebuffer[tty_id], bytes_to_write);
	scheduler();
	len -= bytes_to_write;	
    }

    //if code is executing here, the current process is done writing. Remove self from writequeue and unblock the next process in line to write to tty_id
    NodeLL *node_to_free = dequeue(writequeue[tty_id]);
    free(node_to_free);
    if(isQueueEmpty(writequeue[tty_id]) == 0)
    {
	if(writequeue[tty_id]->head->data == NULL)
	{
	    TracePrintf(1, "SysTtyWrite: Data inside write node is null!\n");
	    Pause();
	}
	enqueue(readyqueue, writequeue[tty_id]->head->data);
    }
    TracePrintf(50, "SysTtyWrite: Exiting with code %d\n", tlen);
    return tlen;
}

/*
  The pipes implemented here are unidirectional, so a pipe 
  can have only one reader and one writer, and the reader 
  must be distinct from the writer. These pipes can only be 
  used for IPC between two process that instantiates the pipe 
  and one of it's forked descendants. The reader of the pipe 
  and the writer of the pipe are decided on a first come, 
  first serve basis, meaning that the first process to write 
  will become the sole writer, and the first process to read 
  will become the sole reader of the pipe. When a pipe becomes 
  full while the writer is writing to it, the writer blocks 
  until buffer space become available. Similarly, the reader 
  blocks when the buffer is empty.
 */
int SysPipeInit (int *pipe_idp) /* pointer to int to store id of created pipe */
{
    TracePrintf(50, "SysPipeInit: Entering\n");

    pcb *curr_process = (pcb *)currentprocess->data;
    
    int pipe_no = allocateLCP(PIPE);
    if (ERROR == pipe_no)
    {
	TracePrintf(1, "SysPipeInit: Error: Already at max free pipes of %d!\n", MAX_PIPES);
	return ERROR;
    }

    Pipe *new_pipe = (Pipe*) malloc(sizeof(Pipe));
    MALLOC_CHECK(new_pipe);
    BZERO(new_pipe, sizeof(Pipe));
    new_pipe->numbytes = 0;
    new_pipe->pipeid = IDSCALE*pipe_no + PIPE;
    new_pipe->readblocking = NULL;
    new_pipe->writeblocking = NULL;
    new_pipe->writer = NULL;
    new_pipe->reader = NULL;

    pipes[pipe_no] = new_pipe;

    NodeLL *new_node = (NodeLL *) malloc(sizeof(NodeLL));
    BZERO(new_node, sizeof(NodeLL));
    new_node->id = new_pipe->pipeid;
    enqueue(curr_process->pipes, new_node);

    *pipe_idp = new_pipe->pipeid;

    TracePrintf(50, "SysPipeInit: Exiting\n");
    return 0;
}

int SysPipeRead (int pipe_id, /* pipe to read from */
		 void *buf,   /* buffer to write to */
		 int len)     /* number of bytes to read */
{
    TracePrintf(50, "SysPipeRead: Entering\n");
    int bytes_to_read = 0;
    pcb *curr_process = (pcb *)currentprocess->data;
    Pipe *my_pipe = pipes[pipe_id/IDSCALE];
    if(pipe_id % IDSCALE != PIPE)
    {
	TracePrintf(1, "SysPipeRead: Error: the id passed, %d, is not a pipe", pipe_id);
	return ERROR;
    }    

    if(my_pipe == NULL)
    {
	TracePrintf(1, "SysPipeRead: Error: pipe %d does not exist.\n", pipe_id);
	return ERROR;
    }

    if(len < 0)
    {
	TracePrintf(1, "SysPipeRead: Error: cannot read %d bytes from buffer. Use a positive integer.\n", len);
	return ERROR;
    }

    if(len == 0)
	return 0;

    //if the reader of the pipe has not been set and the current process owns the pipe and the current process is not the writer, set the reader of the pipe to the current process
    if(my_pipe->reader == NULL)
    {
	//don't let the writer become the reader as well
	if(curr_process == my_pipe->writer)
	{
	    TracePrintf(1, "SysPipeRead: Error: current process %d cannot read because it is the writer.\n", curr_process->pid);
	    return ERROR;
	}

	//if the current process owns the pipe and is not the reader, then he is eligible to become the reader
	if(searchLL(curr_process->pipes->head, pipe_id) != NULL)
		my_pipe->reader = curr_process;
	else
	{
	    TracePrintf(1, "SysPipeRead: Error: current process %d does not own pipe %d\n", curr_process->pid, pipe_id);
	    return ERROR;
	}
    }
    
    //if current process is not the reader of the pipe, deny access
    if(my_pipe->reader->pid != curr_process->pid)
    {
	TracePrintf(1, "SysPipeRead: Error: Current process is not the reader for this pipe.\n");
	return ERROR;
    }
    while(1)
    {
	//figure out how many bytes to read
	if(my_pipe->numbytes > len)
	    bytes_to_read = len;
	else if(my_pipe->numbytes <= len)
	    bytes_to_read = my_pipe->numbytes;

	//if there are no bytes to read, block until there are
	if(bytes_to_read == 0)
	{
	    TracePrintf(50, "SysPipeRead: No bytes to read in the buffer, so blocking process %d\n", curr_process->pid);
	    my_pipe->readblocking = currentprocess;
	    //if there is a writer waiting for the buffer to empty, unblock it
	    if(my_pipe->writeblocking != NULL)
	    {
		enqueue(readyqueue, my_pipe->writeblocking);
		my_pipe->writeblocking = NULL;
	    }
	    scheduler();
	}
	//as soon as there are bytes to read, read as many as desired/possible, and then return
	else
	{
	    memcpy(buf, my_pipe->buffer, bytes_to_read);
	    //TracePrintf(1, "buffer has: \n%s\n", (char *) my_pipe->buffer);
	    //TracePrintf(1, "buf has:\n%s\n", (char *) buf);
	    char tmp[TERMINAL_MAX_LINE];
	    memcpy(tmp, my_pipe->buffer + bytes_to_read, PIPE_SIZE - bytes_to_read);
	    //TracePrintf(1, "tmp has: \n%s\n", tmp);
	    //memcpy(my_pipe->buffer, my_pipe->buffer + bytes_to_read, bytes_to_read);
	    memcpy(my_pipe->buffer, tmp, PIPE_SIZE - bytes_to_read);
	    
	    my_pipe->numbytes -= bytes_to_read;
	    break;
	}
    }

    //if there is a writer waiting for the buffer to empty, unblock it
    if(my_pipe->writeblocking != NULL)
    {
	enqueue(readyqueue, my_pipe->writeblocking);
	my_pipe->writeblocking = NULL;
    }
    TracePrintf(50, "SysPipeRead: Exiting\n");
    return bytes_to_read;
}
int SysPipeWrite (int pipe_id, /* pipe to write to */
		  void *buf,   /* buffer to read write data from */
		  int len)     /* number of bytes to write */
{
    TracePrintf(50, "SysPipeWrite: Entering\n");
    int bytes_to_write = 0;
    int tlen = len; //save len before we stomp on it
    pcb *curr_process = (pcb *)currentprocess->data;
    Pipe *my_pipe = pipes[pipe_id/IDSCALE];

    if(pipe_id % IDSCALE != PIPE)
    {
	TracePrintf(1, "SysPipeWrite: Error: the id passed, %d, is not a pipe", pipe_id);
	return ERROR;
    }

    if(my_pipe == NULL)
    {
	TracePrintf(1, "SysPipeWrite: Error: pipe %d does not exist.\n", pipe_id);
	return ERROR;
    }

    if(len < 0)
    {
	TracePrintf(1, "SysPipeWrite: Error: cannot write %d bytes from buffer. Use a positive integer.\n", len);
	return ERROR;
    }

    if(len == 0)
	return 0;

    
    //if the writer of the pipe has not been set and the current process owns the pipe and the current process is not the writer, set the writer of the pipe to the current process
    if(my_pipe->writer == NULL)
    {
	//don't let the reader become the writer as well
	if(curr_process == my_pipe->reader)
	{
	    TracePrintf(1, "SysPipeWrite: Error: current process %d cannot write because it is the writer.\n", curr_process->pid);
	    return ERROR;
	}

	//if the current process owns the pipe and is not the writer, then he is eligible to become the writer
	if(searchLL(curr_process->pipes->head, pipe_id) != NULL)
		my_pipe->writer = curr_process;
	else
	{
	    TracePrintf(1, "SysPipeWrite: Error: current process %d does not own pipe %d\n", curr_process->pid, pipe_id);
	    return ERROR;
	}
    }
    
    //if current process is not the writer of the pipe, deny access
    if(my_pipe->writer->pid != curr_process->pid)
    {
	TracePrintf(1, "SysPipeWrite: Error: Current process is not the writer for this pipe.\n");
	return ERROR;
    }

    while(len > 0)
    {
	//figure out how many bytes to write
	if(PIPE_SIZE - my_pipe->numbytes > len)
	    bytes_to_write = len;
	else if(PIPE_SIZE - my_pipe->numbytes <= len)
	    bytes_to_write = my_pipe->numbytes;

	//if there no space to write, block until there are
	if(bytes_to_write + my_pipe->numbytes > PIPE_SIZE)
	{
	    TracePrintf(50, "SysPipeWrite: Buffer is full, so blocking process %d now\n", curr_process->pid);
	    my_pipe->writeblocking = currentprocess;
	    //if the reader is waiting, then let it back onto the ready queue
	    if(my_pipe->readblocking != NULL)
	    {
		enqueue(readyqueue, my_pipe->readblocking);
		my_pipe->readblocking = NULL;
	    }

	    scheduler();
	}
	//as soon as there is room to write, write as many bytes as desired/can
	else
	{
	    //WHAT IF BUFFER IS NOT EMPTY?
	    memcpy(my_pipe->buffer + my_pipe->numbytes, buf, bytes_to_write);
	    buf = buf + bytes_to_write;	    
	    my_pipe->numbytes = my_pipe->numbytes + bytes_to_write;
	    len -= bytes_to_write;
	}
    }

    //if there is a writer waiting for the buffer to empty, unblock it
    if(my_pipe->readblocking != NULL)
    {
	enqueue(readyqueue, my_pipe->readblocking);
	my_pipe->readblocking = NULL;
    }
    TracePrintf(50, "SysPipeWrite: Exiting\n");
    return tlen;
}

int SysLockInit(int *lock_idp) /* pointer to int to store lock id in */
{
    TracePrintf(100, "SysLockInit: Entering...\n");
    pcb* curr_process = (pcb*)(currentprocess->data);
    
    // Get a free lock number
    int lock_no = allocateLCP(LOCK);
    if (ERROR == lock_no)
    {
	TracePrintf(1, "SysLockInit: Error: Already at max free locks of %d!\n", MAX_LOCKS);
	return ERROR;
    }
    
    // Create new lock
    locks[lock_no] = (Lock*) malloc(sizeof(Lock));
    MALLOC_CHECK(locks[lock_no]);
    BZERO(locks[lock_no], sizeof(Lock));
    locks[lock_no]->owner = NO_OWNER;
    locks[lock_no]->id = lock_no*IDSCALE + LOCK;
    *lock_idp = lock_no*IDSCALE + LOCK;
    locks[lock_no]->blocking = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(locks[lock_no]->blocking);
    BZERO(locks[lock_no]->blocking, sizeof(Queue));


    TracePrintf(100, "SysLockInit: Exiting...\n");
    return 0;
}

int SysAcquire (int lock_id) /* id of lock to acquire */
{
    TracePrintf(100, "SysAcquire: Entering...\n");
    
    int lock_no = (lock_id - LOCK) / IDSCALE;
    int remainder = (lock_id - LOCK) % IDSCALE;

    // Invalid lock id
    if (remainder != 0)
    {
	TracePrintf(1, "SysAcquire: Error: lock_id %d is not a valid lock number!\n", lock_id);
	return ERROR;
    }

    // Lock not created yet
    if (NULL == locks[lock_no])
    {
	TracePrintf(1, "SysAcquire: Error: lock_id %d, corresponding to lock %d, does not exist!\n", lock_id, lock_no);
	return ERROR;
    }
    
    Lock* lock_to_acq = locks[lock_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    // Lock already owned by current process
    if (lock_to_acq->owner == curr_process->pid)
    {
	TracePrintf(1, "SysAcquire: Error: Current process (pid %d) already owns lock %d (lock_id = %d)\n", curr_process->pid, lock_no, lock_id);
	return ERROR;
    }

    // Add current process to blocking queue is lock is currently owned
    // by some other process
    if (lock_to_acq->owner != NO_OWNER)
    {
	TracePrintf(100, "SysAcquire: Lock %d is currently owned by process %d. Blocking...\n", lock_no, lock_to_acq->owner);

	// Create new blocking node
	NodeLL* blocking_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(blocking_node);
	BZERO(blocking_node, sizeof(NodeLL));
	blocking_node->next = NULL;
	blocking_node->prev = NULL;
	blocking_node->id = curr_process->pid;
	blocking_node->data = (void *) currentprocess;
	enqueue(lock_to_acq->blocking, blocking_node);
	scheduler();
    }
    
    // Acquire lock now that it has no owner
    lock_to_acq->owner = curr_process->pid;
    
    TracePrintf(100, "SysAcquire: Exiting...\n");
    return 0;
}

int SysRelease (int lock_id) /* id of lock to release */
{
    TracePrintf(100, "SysRelease: Entering...\n");
    
    int lock_no = (lock_id - LOCK) / IDSCALE;
    int remainder = (lock_id - LOCK) % IDSCALE;

    // Invalid lock id
    if (remainder != 0)
    {
	TracePrintf(1, "SysRelease: Error: lock_id %d is not a valid lock number!\n", lock_id);
	return ERROR;
    }
    
    // Lock not created yet
    if (NULL == locks[lock_no])
    {
	TracePrintf(1, "SysRelease: Error: lock_id %d, corresponding to lock %d, does not exist!\n", lock_id, lock_no);
	return ERROR;
    }

    Lock* lock_to_release = locks[lock_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    // Lock not owned by current process
    if (lock_to_release->owner != curr_process->pid)
    {
	TracePrintf(1, "SysRelease: Error: Current process (pid %d) does not own lock %d (lock_id = %d)\n", curr_process->pid, lock_no, lock_id);
	return ERROR;
    }

    // Release lock
    lock_to_release->owner = NO_OWNER;
    
    // Take next process waiting on this lock off queue
    // if there are processes waiting on it
    if (isQueueEmpty(lock_to_release->blocking) == 0)
    {
	NodeLL* waiting_proc = dequeue(lock_to_release->blocking);
	NodeLL* waiting_proc_pcb_node = (NodeLL*)(waiting_proc->data);
	free(waiting_proc);
	TracePrintf(100, "SysRelease: Taking process %d off the blocking queue for lock %d...\n", waiting_proc->id, lock_no);
	enqueue(readyqueue, waiting_proc_pcb_node);
    }

    TracePrintf(100, "SysRelease: Exiting...\n");
    return 0;
}

int SysCvarInit (int *cvar_idp)
{
    TracePrintf(100, "SysCvarInit: Entering...\n");
    pcb* curr_process = (pcb*)(currentprocess->data);
    
    // Get a free cvar number
    int cvar_no = allocateLCP(CVAR);
    if (ERROR == cvar_no)
    {
	TracePrintf(1, "SysCvarInit: Error: Already at max free cvars of %d!\n", MAX_CVARS);
	return ERROR;
    }
    
    // Create new cvar
    cvars[cvar_no] = (Cvar*) malloc(sizeof(Cvar));
    MALLOC_CHECK(cvars[cvar_no]);
    BZERO(cvars[cvar_no], sizeof(Cvar));
    cvars[cvar_no]->owner = NO_OWNER;
    cvars[cvar_no]->id = cvar_no*IDSCALE + CVAR;
    *cvar_idp = cvar_no*IDSCALE + CVAR;
    cvars[cvar_no]->blocking = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(cvars[cvar_no]->blocking);
    BZERO(cvars[cvar_no]->blocking, sizeof(Queue));

    TracePrintf(100, "SysCvarInit: Exiting...\n");
    return 0;
}

int SysCvarWait (int cvar_id, // ID of the cvar this process will wait on
		 int lock_id) // ID of the lock this process will release and acquire early
{
    TracePrintf(100, "SysCvarWait: Entering...\n");
    
    int cvar_no = (cvar_id - CVAR) / IDSCALE;
    int remainder = (cvar_id - CVAR) % IDSCALE;

    // Invalid cvar id
    if (remainder != 0)
    {
	TracePrintf(1, "SysCvarWait: Error: cvar_id %d is not a valid cvar number!\n", cvar_id);
	return ERROR;
    }

    // Cvar not created yet
    if (NULL == cvars[cvar_no])
    {
	TracePrintf(1, "SysCvarWait: Error: cvar_id %d, corresponding to cvar %d, does not exist!\n", cvar_id, cvar_no);
	return ERROR;
    }

    Cvar* cvar_to_wait = cvars[cvar_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    TracePrintf(100, "SysCvarWait: Calling SysRelease...\n");
    SysRelease(lock_id);
    TracePrintf(100, "SysCvarWait: Returned from SysRelease. Lock released! Blocking...\n");

    // Create new blocking node
    NodeLL* blocking_node = (NodeLL*) malloc(sizeof(NodeLL));
    MALLOC_CHECK(blocking_node);
    BZERO(blocking_node, sizeof(NodeLL));
    blocking_node->next = NULL;
    blocking_node->prev = NULL;
    blocking_node->id = curr_process->pid;
    blocking_node->data = (void *) currentprocess;
    enqueue(cvar_to_wait->blocking, blocking_node);
    scheduler();
    
    TracePrintf(100, "SysCvarWait: Awake! Calling SysAcquire...\n");
    SysAcquire(lock_id);
    TracePrintf(100, "SysCvarWait: Returned from SysAcquire. Lock acquired! Returning...\n");

    return 0;
}

int SysCvarSignal (int cvar_id)//cvar to signal
{
    TracePrintf(100, "SysCvarSignal: Entering...\n");
    
    int cvar_no = (cvar_id - CVAR) / IDSCALE;
    int remainder = (cvar_id - CVAR) % IDSCALE;

    // Invalid cvar id
    if (remainder != 0)
    {
	TracePrintf(1, "SysCvarSignal: Error: cvar_id %d is not a valid cvar number!\n", cvar_id);
	return ERROR;
    }

    // Cvar not created yet
    if (NULL == cvars[cvar_no])
    {
	TracePrintf(1, "SysCvarSignal: Error: cvar_id %d, corresponding to cvar %d, does not exist!\n", cvar_id, cvar_no);
	return ERROR;
    }

    Cvar* cvar_to_signal = cvars[cvar_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    // Take next process waiting on this cvar off queue
    // if there are processes waiting on it
    if (isQueueEmpty(cvar_to_signal->blocking) == 0)
    {
	NodeLL* waiting_proc = dequeue(cvar_to_signal->blocking);
	NodeLL* waiting_proc_pcb_node = (NodeLL*)(waiting_proc->data);
	free(waiting_proc);
	TracePrintf(100, "SysCvarSignal: Taking process %d off the blocking queue for cvar %d...\n", waiting_proc->id, cvar_no);
	enqueue(readyqueue, waiting_proc_pcb_node);
    }
    
    // We do nothing if there is nothing waiting on this Cvar

    TracePrintf(100, "SysCvarSignal: Exiting...\n");
    return 0;
}

int SysCvarBroadcast (int cvar_id)
{
    TracePrintf(100, "SysCvarBroadcast: Entering...\n");
    
    int cvar_no = (cvar_id - CVAR) / IDSCALE;
    int remainder = (cvar_id - CVAR) % IDSCALE;

    // Invalid cvar id
    if (remainder != 0)
    {
	TracePrintf(1, "SysCvarBroadcast: Error: cvar_id %d is not a valid cvar number!\n", cvar_id);
	return ERROR;
    }

    // Cvar not created yet
    if (NULL == cvars[cvar_no])
    {
	TracePrintf(1, "SysCvarBroadcast: Error: cvar_id %d, corresponding to cvar %d, does not exist!\n", cvar_id, cvar_no);
	return ERROR;
    }

    Cvar* cvar_to_broadcast = cvars[cvar_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    // Take all processes waiting on this cvar off queue
    // if there are any processes waiting on it
    while (isQueueEmpty(cvar_to_broadcast->blocking) == 0)
    {
	NodeLL* waiting_proc = dequeue(cvar_to_broadcast->blocking);
	NodeLL* waiting_proc_pcb_node = (NodeLL*)(waiting_proc->data);
	free(waiting_proc);
	TracePrintf(100, "SysCvarBroadcast: Taking process %d off the blocking queue for cvar %d...\n", waiting_proc->id, cvar_no);
	enqueue(readyqueue, waiting_proc_pcb_node);
    }
    
    // We do nothing if there is nothing waiting on this Cvar

    TracePrintf(100, "SysCvarBroadcast: Exiting...\n");
    return 0;
}

int SysReclaim (int id)
{
    int no = id % IDSCALE;

    switch(no)
    {

    case PIPE:
    {
	int pipe_no = (id - PIPE) / IDSCALE;
	NodeLL *pcb_pipe;
	
	// Pipe not created yet
	if (NULL == pipes[pipe_no])
	{
	    TracePrintf(1, "SysReclaim: Error: pipe_id %d, corresponding to pipe %d, does not exist!\n", id, pipe_no);
	    return ERROR;
	}

	//If something is waiting to read or write to the pipe, don't reclaim
	if(pipes[pipe_no]->readblocking != NULL || pipes[pipe_no]->writeblocking != NULL)
	{
	    TracePrintf(1, "SysReclaim: Error: A process is blocking for the pipe. Reclaim aborted.\n");
	    return ERROR;
	}

	// Delete pipe from the reader and writer's pcb
	if(pipes[pipe_no]->writer != NULL)
	{
	    pcb_pipe = removeFromList(pipes[pipe_no]->writer->pipes, id);
	    if(pcb_pipe == NULL)
	    {
		TracePrintf(1, "SysReclaim: Pipe %d not found in process %d pipes queue\n", id, pipes[pipe_no]->writer->pid);
		Pause();
	    }
	    else free(pcb_pipe);
	}

	if(pipes[pipe_no]->reader != NULL)
	{
	    pcb_pipe = removeFromList(pipes[pipe_no]->reader->pipes, id);
	    if(pcb_pipe == NULL)
	    {
		TracePrintf(1, "SysReclaim: Pipe %d not found in process %d pipes queue\n", id, pipes[pipe_no]->reader->pid);
		Pause();
	    }
	    else free(pcb_pipe);
	}

	Pipe *pipe_to_free = pipes[pipe_no];
	free(pipe_to_free);

	// Free up the pipe number
	deallocateLCP(pipe_no, PIPE);
    }
	break;

    case LOCK:
    {
	int lock_no = (id - LOCK) / IDSCALE;
	
	// Lock not created yet
	if (NULL == locks[lock_no])
	{
	    TracePrintf(1, "SysReclaim: Error: lock_id %d, corresponding to lock %d, does not exist!\n", id, lock_no);
	    return ERROR;
	}

	Lock *lock_to_free = locks[lock_no];

	// If there are processes blocking for this lock, return error
	// because we cannot reclaim it yet
	if (lock_to_free->blocking != NULL)
	{
	    if (isQueueEmpty(lock_to_free->blocking) != 1)
	    {
		TracePrintf(1, "SysReclaim: Error: lock %d has processes blocking on it. Cannot reclaim!\n", lock_no);
		return ERROR;
	    }
	    free(lock_to_free->blocking);
	}

	free(lock_to_free);
	locks[lock_no] = NULL;

	// Free up the lock number
	deallocateLCP(lock_no, LOCK);
    }
	break;

    case CVAR:
    {
	int cvar_no = (id - CVAR) / IDSCALE;
	
	// Cvar not created yet
	if (NULL == cvars[cvar_no])
	{
	    TracePrintf(1, "SysReclaim: Error: cvar_id %d, corresponding to cvar %d, does not exist!\n", id, cvar_no);
	    return ERROR;
	}

	Cvar *cvar_to_free = cvars[cvar_no];

	// If there are processes blocking for this cvar, return error
	// because we cannot reclaim it yet
	if (cvar_to_free->blocking != NULL)
	{
	    if (isQueueEmpty(cvar_to_free->blocking) != 1)
	    {
		TracePrintf(1, "SysReclaim: Error: cvar %d has processes blocking on it. Cannot reclaim!\n", cvar_no);
		return ERROR;
	    }
	    free(cvar_to_free->blocking);
	}
       
	free(cvar_to_free);

	// Free up the cvar number
	deallocateLCP(cvar_no, CVAR);
    }
	break;

    case SEM:
    {
	int sem_no = (id - SEM) / IDSCALE;
	
	// Sem not created yet
	if (NULL == sems[sem_no])
	{
	    TracePrintf(1, "SysReclaim: Error: sem_id %d, corresponding to sem %d, does not exist!\n", id, sem_no);
	    return ERROR;
	}

	Sem *sem_to_free = sems[sem_no];

	// If there are processes blocking for this sem, return error
	// because we cannot reclaim it yet
	if (sem_to_free->blocking != NULL)
	{
	    if (isQueueEmpty(sem_to_free->blocking) != 1)
	    {
		TracePrintf(1, "SysReclaim: Error: sem %d has processes blocking on it. Cannot reclaim!\n", sem_no);
		return ERROR;
	    }
	    free(sem_to_free->blocking);
	}
       
	free(sem_to_free);

	// Free up the sem number
	deallocateLCP(sem_no, SEM);
    }
	break;
 
    }

    return 0;
}

//============================
// EXTRA FUNCTION: SEMAPHORES
//============================

// Initializes a semaphore, storing the id of the semaphore created
// is stored in sem_idp, and the semaphore has an initial capacity of init_val
int SysSemInit (int *sem_idp,  // pointer to int to store initialized sem id 
		int init_val)  // initial value to assign to semaphore
{
    TracePrintf(100, "SysSemInit: Entering...%08x\n", sem_idp);
    pcb* curr_process = (pcb*)(currentprocess->data);
    
    // Get a free sem number
    int sem_no = allocateLCP(SEM);
    if (ERROR == sem_no)
    {
	TracePrintf(1, "SysSemInit: Error: Already at max free sems of %d!\n", MAX_SEMS);
	return ERROR;
    }
    
    // Create new sem
    sems[sem_no] = (Sem*) malloc(sizeof(Sem));
    MALLOC_CHECK(sems[sem_no]);
    BZERO(sems[sem_no], sizeof(Sem));
    sems[sem_no]->val = init_val;
    sems[sem_no]->id = sem_no*IDSCALE + SEM;

    *sem_idp = sem_no*IDSCALE + SEM;
    sems[sem_no]->blocking = (Queue*) malloc(sizeof(Queue));
    MALLOC_CHECK(sems[sem_no]->blocking);
    BZERO(sems[sem_no]->blocking, sizeof(Queue));

    TracePrintf(100, "SysSemInit: Exiting...\n");
    return 0;
}

// Ups a semaphore (increment val) and take off a process
// blocking on the semaphore (previous tried to down when val was 0)
// off the blocking queue and put it into ready queue
int SysSemUp (int sem_id) // ID of semaphore to up
{
    TracePrintf(100, "SysSemUp: Entering...\n");
    
    int sem_no = (sem_id - SEM) / IDSCALE;
    int remainder = (sem_id - SEM) % IDSCALE;

    // Invalid sem id
    if (remainder != 0)
    {
	TracePrintf(1, "SysSemUp: Error: sem_id %d is not a valid sem number!\n", sem_id);
	return ERROR;
    }

    // Sem not created yet
    if (NULL == sems[sem_no])
    {
	TracePrintf(1, "SysSemUp: Error: sem_id %d, corresponding to sem %d, does not exist!\n", sem_id, sem_no);
	return ERROR;
    }

    Sem* sem_to_up = sems[sem_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    sem_to_up->val++;

    // Take next process waiting on this sem off queue
    // if there are processes waiting on it
    if (isQueueEmpty(sem_to_up->blocking) == 0)
    {
	NodeLL* waiting_proc = dequeue(sem_to_up->blocking);
	NodeLL* waiting_proc_pcb_node = (NodeLL*)(waiting_proc->data);
	free(waiting_proc);
	TracePrintf(100, "SysSemUp: Taking process %d off the blocking queue for sem %d...\n", waiting_proc->id, sem_no);
	enqueue(readyqueue, waiting_proc_pcb_node);
    }

    TracePrintf(100, "SysSemUp: Exiting...\n");
    return 0;
}

// Downs a sem value, checking first if the sem value is currently
// more than 0. If the value is currently 0 or smaller, we block
// until another process ups the sem and takes this process wanting to
// down the sem off the blocking queue.
// NOTE: Sem val should not ever be less than 0 in this implementation
int SysSemDown (int sem_id) // semaphore to down
{
    TracePrintf(100, "SysSemDown: Entering...\n");
    
    int sem_no = (sem_id - SEM) / IDSCALE;
    int remainder = (sem_id - SEM) % IDSCALE;

    // Invalid sem id
    if (remainder != 0)
    {
	TracePrintf(1, "SysSemDown: Error: sem_id %d is not a valid sem number!\n", sem_id);
	return ERROR;
    }

    // Sem not created yet
    if (NULL == sems[sem_no])
    {
	TracePrintf(1, "SysSemDown: Error: sem_id %d, corresponding to sem %d, does not exist!\n", sem_id, sem_no);
	return ERROR;
    }

    Sem* sem_to_down = sems[sem_no];
    pcb* curr_process = (pcb*)(currentprocess->data);

    
    // Block if sem val is currently 0 (no more resources)
    while (sem_to_down->val <= 0)
    {
	NodeLL* blocking_node = (NodeLL*) malloc(sizeof(NodeLL));
	MALLOC_CHECK(blocking_node);
	BZERO(blocking_node, sizeof(NodeLL));
	blocking_node->next = NULL;
	blocking_node->prev = NULL;
	blocking_node->id = curr_process->pid;
	blocking_node->data = (void *) currentprocess;
	enqueue(sem_to_down->blocking, blocking_node);
	scheduler();
    }
    
    // Back from blocking. Sem val must be larger or equal to 1 now
    TracePrintf(100, "SysSemDown: Awake! Downing Sem...\n");
    if (sem_to_down->val <= 0)
    {
	TracePrintf(1, "SysSemDown: Error: Back from blocking but val is still non-positive (%d)! Exiting current process...\n", sem_to_down->val);
	SysExit(ERROR_SEM_VAL);
    }
    sem_to_down->val--;

    TracePrintf(100, "SysSemDown: Exiting...\n");
    return 0;
}
