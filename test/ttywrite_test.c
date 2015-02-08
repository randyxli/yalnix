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
    int charswritten;
    if(0 == rc)
	while(1)
	{
	    TracePrintf(100, "Init: I'M CHILD, Process %d! I'm gonna write 2048 a's to terminal 1, followed by 10 childs.\n", GetPid());
	    
	    char str1[2049];
	    char child[10] = "child\n";
	    //for(i=0; i<2048; i++)
	    //	str1[i] = 'a';
	    for(i=0; i<2049; i++)
	    {
		str1[i] = (i%80)+32;
	    }
	    charswritten = TtyWrite(1, str1, 2049);
	    for(i=0; i<10; i++)
	    {
		TtyPrintf(1, "child\n");
		Pause();
		//TtyWrite(1, child, 6);
	    }
	    TracePrintf(100, "Init: Child about to exit...\n");
	    Exit(100);
	    //Delay(5);
	    //TracePrintf(100, "Init: Child back from delay!!\n");
	}
    else
	while(1)
	{
	    TracePrintf(100, "Init: I'M process %d! I'm going to write parent to terminal 1 30 times.\n", GetPid());
	    char str[2048] = "parent\n";

	    for(i=0; i<30; i++)		     
	    {
		//charswritten = TtyWrite(1, str, 7);
		TtyPrintf(1, "parent\n");
		Pause();
	    }

	    TracePrintf(100, "Init: Parent (Process %d) about to exit\n", status_ptr, GetPid());
	    Exit(100);
	}
    
}

void recursion(void)
{
    int i = 1;
    //TracePrintf(100, "RECURSION!!!!!\n");
    recursion();
}
