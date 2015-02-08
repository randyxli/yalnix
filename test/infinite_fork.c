// init process for CS58 Checkpoint 3
#include <stdio.h>
#include <yalnix.h>

int main()
{
/*
    char* test[] = {"lock_test", 0};
    if (-1 == Exec("lock_test", test))
    {
	TracePrintf(100, "FAILED!\n");
	return 0;
    }
*/

    int rc, status;
    int count = 1;
    while(1)
    {
	rc = Fork();
	if (0 == rc)
	{
	    TracePrintf(1, "Child: Hi I'm Child! Exiting...\n");
	    Exit(-1);
	}
	Wait(&status);
	TracePrintf(1, "Parent: Child died! Completed round %d\n", count);
	count++;
    }
    

    int i;
    TracePrintf(100, "Running Init Process, about to fork...\n");
    rc = Fork();
    TracePrintf(100, "Init: Back from fork with rc %d!\n", rc);
    int status_ptr;
    int charsread;
    if(0 == rc)
	while(1)
	{
	    TracePrintf(100, "Init: I'M CHILD, Process %d!!!!\n", GetPid());
	    Delay(10);
	    TracePrintf(100, "Init: Child about to exit...\n");
	    //Pause();
	    
	    char str1[2048];
	    charsread = TtyRead(2, str1, 1000);
	    str1[charsread] = 0;
	    Wait(&status_ptr);
	    TracePrintf(1, "Child read from terminal %d:\n%s\n", 2, str1);
	    Exit(100);
	    //Delay(5);
	    //TracePrintf(100, "Init: Child back from delay!!\n");
	}
    else
	while(1)
	{
	    TracePrintf(100, "Init: I'M process %d!!!!\n", GetPid());
	    //TracePrintf(100, "Init: Parent about to delay...\n");
	    //Delay(5);
	    //TracePrintf(100, "Init: Parent back from delay!!\n");
	    char str[2048];
	    /*
	    for(i=0; i<2048; i++)
		str[i] = 'a';
	    str[2047] = 0;
	    
	    
	    TtyWrite(1, str, 2047);
	    */
	    charsread = TtyRead(1, str, 1000);
	    str[charsread] = 0;
	    TracePrintf(1, "Read the following from the terminal:\n%s\n", str);
	    int return_v = Wait(&status_ptr);
	    TracePrintf(100, "Init: Child returned with exit code %d. Process %d about to exit\n", status_ptr, GetPid());
	    Exit(100);
	}
    
}

void recursion(void)
{
    int i = 1;
    TracePrintf(100, "RECURSION!!!!!\n");
    recursion();
}
