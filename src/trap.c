#include "syscalls.h"
#include "kernel.h"
#include "datastructures.h"
#include <yalnix.h>
#include <hardware.h>
#include "header.h"

//=================
//  TRAP HANDLERS
//=================
// Update PCB's user context each with the fields of the user context passed
// to the trap handler

void TrapKernelHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50, "TrapKernelHandler: Entering...\n");
    int child_id;
    int i;

    int code = context->code;
    u_long *regs = context->regs;
    int rc;
    char* curr_ptr;
    pcb* curr_process = (pcb *)(currentprocess->data);

    switch(code)
    {
    case YALNIX_GETPID:
        rc = SysGetPid();
	break;
    case YALNIX_DELAY:
	rc = SysDelay(regs[0]);
	break;
    case YALNIX_BRK:
    {
	// Check if the pointer given is in user space
	int page_no = DOWN_TO_PAGE((void *) regs[0]) >> PAGESHIFT;
	if (page_no < MAX_PT_LEN)
	{
	    TracePrintf(1, "TrapKernelHandler: pointer %08x points to kernel space! Exiting process %d...\n", ((void *) regs[0]), curr_process->pid);
	    SysExit(ERROR_KERNEL_PTR);
	}
	rc = SysBrk((void *) regs[0]);
    }
	break;
    case YALNIX_EXEC:
	pointerCheck(PROT_READ, (void*)regs[0], 1, 0);
	pointerCheck(PROT_READ, (void*)regs[1], 0, sizeof(char *));
	curr_ptr = *((char **) regs[1]);
	while (curr_ptr != NULL)
	{
	    pointerCheck(PROT_READ, curr_ptr, 1, 0);
	    curr_ptr += sizeof(char *);
	}
	rc = SysExec((char *) regs[0], (char **) regs[1]);
	if (ERROR != rc)
	{
	    *context = (curr_process->usercontext);
	    return; // return here becase we do not want a return value
	}
	break;
    case YALNIX_FORK:
	rc = SysFork(context);
	break;
    case YALNIX_EXIT:
	SysExit(regs[0]);
	break;
    case YALNIX_WAIT:
	pointerCheck(PROT_WRITE, (void*)regs[0], 0, sizeof(int));
	rc = SysWait((int*)regs[0]);
	break;    
    case YALNIX_TTY_WRITE:
	pointerCheck(PROT_READ, (void *)regs[1], 0, regs[2]);
	rc = SysTtyWrite(regs[0],(void *)regs[1],regs[2]);
	break;
    case YALNIX_TTY_READ:
	pointerCheck(PROT_WRITE, (void *)regs[1], 0, regs[2]);
	rc = SysTtyRead(regs[0],(void *)regs[1],regs[2]);
	break;
    case YALNIX_LOCK_INIT:
	pointerCheck(PROT_WRITE, (void *)regs[0], 0, sizeof(int));
	rc = SysLockInit((int*)regs[0]);
	break;
    case YALNIX_LOCK_ACQUIRE:
	rc = SysAcquire(regs[0]);
	break;
    case YALNIX_LOCK_RELEASE:
	rc = SysRelease(regs[0]);
	break;
    case YALNIX_CVAR_INIT:
	pointerCheck(PROT_WRITE, (void *)regs[0], 0, sizeof(int));
	rc = SysCvarInit((int*)regs[0]);
	break;
    case YALNIX_CVAR_WAIT:
	rc = SysCvarWait(regs[0],regs[1]);
	break;
    case YALNIX_CVAR_SIGNAL:
	rc = SysCvarSignal(regs[0]);
	break;
    case YALNIX_CVAR_BROADCAST:
	rc = SysCvarBroadcast(regs[0]);
	break;
    case YALNIX_RECLAIM:
	rc = SysReclaim(regs[0]);
	break;
    case YALNIX_PIPE_INIT:
	pointerCheck(PROT_WRITE, (void *)regs[0], 0, sizeof(int));
	rc = SysPipeInit((int*)regs[0]);
	break;
    case YALNIX_PIPE_READ:
	pointerCheck(PROT_WRITE, (void *)regs[1], 0, sizeof(int));
	rc = SysPipeRead(regs[0],(void *)regs[1],regs[2]);
	break;
    case YALNIX_PIPE_WRITE:
	pointerCheck(PROT_READ, (void *)regs[1], 0, (unsigned int) regs[2]);
	rc = SysPipeWrite(regs[0],(void *)regs[1],regs[2]);
	break;
    case YALNIX_SEM_INIT:
	pointerCheck(PROT_WRITE, (void *)regs[0], 0, sizeof(int));
	rc = SysSemInit((int*)regs[0], (int)regs[1]);
	break;
    case YALNIX_SEM_UP:
	rc = SysSemUp((int)regs[0]);
	break;
    case YALNIX_SEM_DOWN:
	rc = SysSemDown((int)regs[0]);
	break;
    default:
	TracePrintf(1,"Error: kernel call with code %d not found\n", code);
	break;
    }

    // Put return value into register before returning
    // to userland
    context->regs[0] = rc;
    curr_process->usercontext.sp = context->sp;
    curr_process->usercontext.pc = context->pc;
    
    for(i=0; i<GREGS; i++)
    {
	curr_process->usercontext.regs[i] = context->regs[i];
    }

    TracePrintf(50, "TrapKernelHandler: Exiting...\n");
}

void TrapClockHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50, "TrapClockHandler: Entering...\n");

    printAllProcesses();

    // Check delayqueue
    NodeLL* current_delay_node = delayqueue->head;
    NodeLL* temp;
    NodeLL* removed;
    int rc;

    

    memcpy(&(((pcb *)(currentprocess->data))->usercontext), context, sizeof(UserContext));

    while (current_delay_node != NULL)
    {
	current_delay_node->id--;
	TracePrintf(100, "TrapClockHandler: current wait time is %d\n", current_delay_node->id);
	if (current_delay_node->id < 0)
	{
	    TracePrintf(100, "TrapClockHandler: Delay node wait time decremented past 0. We have a problem here!\n");
	    Halt();
	}

	if (current_delay_node->id == 0)
	{
	    removed = removeFromList(delayqueue, 0);
	    if (NULL == removed)
	    {
		TracePrintf(1, "TrapClockHandler: Failed to remove node from delayqueue\n");
		Halt();
	    }
	    
	    TracePrintf(100, "Removing process %d from delay queue\n", ((pcb *)(current_delay_node->data))->pid);
	    enqueue(readyqueue, current_delay_node->data);
	    temp = current_delay_node;
	    current_delay_node = current_delay_node->next;
	    free(temp);
	}
	else
	    current_delay_node = current_delay_node->next;
    }

    // If the ready queue is not empty and we are not running currently
    // running the idle process, put the current process into the ready
    // queue and run the scheduler
    if(!(isQueueEmpty(readyqueue)) && 
       currentprocess->id != 1)
    {
	enqueue(readyqueue, currentprocess);
	scheduler();
    }

    // Else if ready queue is not empty but we are currently running idle process
    // Do not enqueue idle process and simply run scheduler
    else if(!(isQueueEmpty(readyqueue)))
	scheduler();

    pcb* curr_process = (pcb*)(currentprocess->data);
    TracePrintf(100, "TrapClockHandler: Current process is pid %d\n", curr_process->pid);
    //printAllProcesses();

    TracePrintf(50, "TrapClockHandler: Exiting...\n");
}

void TrapIllegalHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50, "TrapIllegalHandler: Entering\n");
    
    TracePrintf(50, "TrapIllegalHandler: Exiting\n");
    SysExit(context->code);
}

void TrapMemoryHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50,"TrapMemoryHandler: Entering...\n");

    
    pcb *curr_process = (pcb *)(currentprocess->data);
    //printPageTables(curr_process);
    
    // Address is above highest adderss
    if ((unsigned int) context->addr > VMEM_1_LIMIT)
    {
	TracePrintf(1, "Error: User trying to access invalid address %08x. Exiting...\n", context->addr);
        SysExit(ERROR_OUT_OF_VMEM);
    }

    // Implicit request to grow user stack
    else if ( (context->code == SEGV_MAPERR) ||  (context->code == SEGV_MAPERR + 10) || ((unsigned int) context->addr) == (unsigned int)context->sp )
    {
	// Check if requested address is above the top of user heap
	if (((unsigned int) context->addr > (unsigned int) curr_process->userheaptop))
	{
	    TracePrintf(50, "TrapMemoryHandler: Implicit request to grow user stack to %08x. Servicing...\n", context->addr);
	    
	    // Check if a "red zone" 1 page buffer between top of user heap and bottom of user
	    // stack
	    if ((DOWN_TO_PAGE(context->addr)>>PAGESHIFT) <= (DOWN_TO_PAGE(curr_process->userheaptop) >> PAGESHIFT)+1)
	    {
		TracePrintf(1, "Error: User trying to grow stack into address %08x, which is in the red zone. Exiting...\n", context->addr);
		SysExit(ERROR_RED_ZONE);
	    }
	    // If we are not trying to grow into red zone, then grow stack from addr to top of existing stack
	    else
	    {
		unsigned int current_page = DOWN_TO_PAGE(context->addr) >> PAGESHIFT;
		int frame_allocated;
		while(0 == curr_process->pagetable1[current_page-MAX_PT_LEN].valid)
		{
		    frame_allocated = allocateFreeFrame();
		    curr_process->pagetable1[current_page-MAX_PT_LEN].valid = 1;
		    curr_process->pagetable1[current_page-MAX_PT_LEN].prot = PROT_READ | PROT_WRITE;
		    curr_process->pagetable1[current_page-MAX_PT_LEN].pfn = frame_allocated;
		    WriteRegister(REG_TLB_FLUSH, current_page << PAGESHIFT);
		    TracePrintf(50, "User Stack grown: frame %d allocated, page %d mapped\n", frame_allocated, current_page);
		    current_page++;
		}

	    }
	}
	// User trying to grow stack into heap. Exit.
	else
	{
	    TracePrintf(1, "Error: User trying to grow stack into address %08x, which is in user heap. Error code is %d. Exiting...\n", context->addr, context->code);
	    SysExit(ERROR_STACK_INTO_HEAP);
	}
	 
    }

    // Otherwise, we have an illegal memory access
    else
    {
	TracePrintf(1, "Error: Illegal memory access on address %08x, while SP is %08x\n", context->addr, context->sp);
	SysExit(ERROR_ILLEGAL_MEMORY);
    }

    TracePrintf(50,"TrapMemoryHandler: Exiting...\n");
}

void TrapMathHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50, "TrapMathHandler: Entering\n");
    TracePrintf(1, "Error: Illegal math operation. Killing process %d", ((pcb*)currentprocess->data)->pid);
    TracePrintf(50, "TrapMathHandler: Exiting\n");
    SysExit(context->code);
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
void TrapReceiveHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50, "TrapReceiveHandler: Entering\n");

    int tty_id = context->code;

    //clear the readbuffer for the new line, and copy the new line over
    BZERO(readbuffer[tty_id], TERMINAL_MAX_LINE);
    TtyReceive(tty_id, readbuffer[tty_id], TERMINAL_MAX_LINE);

    //if a reader is waiting on terminal input, put him into the readyqueue
    NodeLL *curr_reader_node = NULL;
    if(isQueueEmpty(readqueue[tty_id]) == 0)
	curr_reader_node  = ((NodeLL *)(readqueue[tty_id]->head))->data;
    if(curr_reader_node != NULL)
    {
	enqueue(readyqueue, curr_reader_node);
	enqueue(readyqueue, currentprocess);    
	scheduler();
    }

    TracePrintf(50, "TraceReceiveHandler: Exiting\n");
}

void TrapTransmitHandler(UserContext *context) /* User context passed by kernel on TRAP */
{
    TracePrintf(50, "TrapTransmitHandler: Entering\n");

    int tty_id = context->code;
    memset(writebuffer[tty_id], 0, TERMINAL_MAX_LINE);

    //switch to the process waiting on the write
    NodeLL *curr_writer_node = writequeue[tty_id]->head->data;
    enqueue(readyqueue, curr_writer_node);
    enqueue(readyqueue, currentprocess);
    TracePrintf(50, "TrapTransmitHandler: Switching to new process\n");
    scheduler();
    TracePrintf(50, "TrapTransmitHandler: Returning from interrupt\n");

}

void TrapBad(UserContext *context) /* User context passed by hardware on TRAP */
{
    TracePrintf(1, "TrapBad: BAD TRAP! Halting system...\n");
    Halt();
}
