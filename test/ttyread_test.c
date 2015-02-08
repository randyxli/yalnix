// init process for CS58 Checkpoint 3
#include <stdio.h>
#include <yalnix.h>

int main()
{    
    int i;
    TracePrintf(100, "Running Init Process, about to fork...\n");
    int rc = Fork();
    TracePrintf(100, "Init: Back from fork with rc %d!\n", rc);
    int status_ptr;
    int charsread;
    if(0 == rc)
	while(1)
	{
	    TracePrintf(100, "Init: I'M CHILD, Process %d. Blocking until something is entered in terminal 1. To test reading stored input in the internal buffer that is not immediately read by a process, enter a string in terminal 2 for the parent to read.\n", GetPid());
	    
	    
	    char str1[2048];
	    for(i=0; i<20; i++)
		charsread = TtyRead(1, str1+i, 1);
	    
	    str1[20] = 0;
	    TracePrintf(1, "Child read from terminal %d:\n%s\n", 1, str1);
	    TracePrintf(100, "Init: Child about to exit...\n");
	    Exit(100);
	    //Delay(5);
	    //TracePrintf(100, "Init: Child back from delay!!\n");
	}
    else
	while(1)
	{
	    TracePrintf(100, "Init: I'M process %d, the parent. Waiting for the child to return, and then reading from terminal 2 upon returning from Wait.\n", GetPid());
	    Wait(&status_ptr);
	    char str[2048];
	    charsread = TtyRead(2, str, 1000);
	    str[charsread] = 0;
	    TracePrintf(1, "Init: Parent read the following from terminal 2:\n%s\n", str);
	    TracePrintf(1, "Process %d about to exit\n", status_ptr, GetPid());
	    Exit(0);
	}
    
}

void recursion(void)
{
    int i = 1;
    //TracePrintf(100, "RECURSION!!!!!\n");
    recursion();
}
