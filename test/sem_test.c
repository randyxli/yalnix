// sem test for yalnix
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <yalnix.h>

int main()
{    
    TracePrintf(10, "Executing Sem Test!\n");
    int rc;
    int sem_id;
    int status;
    
//========================
// TEST BINARY SEMAPHORE
//========================
    TracePrintf(10, "=================================================================\n");
    TracePrintf(10, "TEST 1: SEM VAL STARTS AT 1, 7 PROCESSES WANTING TO DOWN IT\n");
    TracePrintf(10, "=================================================================\n");
    SemInit(&sem_id, 1);    
    rc = Fork();
    if (0 == rc)
    {
	TracePrintf(10, "Child 1: I'm child 1. Trying to down sem...!\n");
	SemDown(sem_id);
	TracePrintf(10, "Child 1: I have downed the sem! Delaying then releasing....\n");
	Delay((rand() % 5));
	SemUp(sem_id);
	TracePrintf(10, "Child 1: Sem released! Exiting....\n");
	Exit(1);

    }
    else
    {
	TracePrintf(10, "Parent: I'm parent!\n");
	Delay((rand() % 5));
	int count = 2;
	int i;
	for (i = 0; i < 5; i++)
	{
	    rc = Fork();
	    if (0 == rc)
	    {
		int child_no = count;
		TracePrintf(10, "Child %d: I'm child %d! Trying to down sem...\n", child_no, child_no);
		SemDown(sem_id);
		TracePrintf(10, "Child %d: I have downed the sem! Releasing...\n", child_no);
		Delay((rand() % 5));
		SemUp(sem_id);
		Exit(child_no);
	    }
	    count++;
	    Pause();
	}
	
	TracePrintf(10, "Parent: Waiting to children to down sem so I can collecting exit status...\n");
	for (i = 0; i < 6; i++)
	{
	    Wait(&status);
	    TracePrintf(10, "Parent: Collected dead child with exit status %d\n", status);
	}
	TracePrintf(10, "Parent: Reclaiming sem...\n");
	Reclaim(sem_id);
	TracePrintf(10, "Parent: Success!\n");
    }

//====================================
// TEST FOR LARGE AMOUNT OF RESOURCE
//====================================
    TracePrintf(10, "=================================================================\n");
    TracePrintf(10, "TEST 2: SEM VAL STARTS AT 20, 7 PROCESSES WANTING TO DOWN IT\n");
    TracePrintf(10, "=================================================================\n");
    SemInit(&sem_id, 20);
    rc = Fork();
    if (0 == rc)
    {
	TracePrintf(10, "Child 1: I'm child 1. Trying to down sem...!\n");
	SemDown(sem_id);
	TracePrintf(10, "Child 1: I have downed the sem! Delaying then releasing....\n");
	Delay((rand() % 5));
	SemUp(sem_id);
	TracePrintf(10, "Child 1: Sem released! Exiting....\n");
	Exit(1);

    }
    else
    {
	TracePrintf(10, "Parent: I'm parent!\n");
	Delay(5);
	int count = 2;
	int i;
	for (i = 0; i < 5; i++)
	{
	    rc = Fork();
	    if (0 == rc)
	    {
		int child_no = count;
		TracePrintf(10, "Child %d: I'm child %d! Trying to down sem...\n", child_no, child_no);
		SemDown(sem_id);
		TracePrintf(10, "Child %d: I have downed the sem! Releasing...\n", child_no);
		Delay((rand() % 5));
		SemUp(sem_id);
		Exit(child_no);
	    }
	    count++;
	    Pause();
	}
	
	TracePrintf(10, "Parent: Waiting to children to down sem so I can collecting exit status...\n");
	for (i = 0; i < 6; i++)
	{
	    Wait(&status);
	    TracePrintf(10, "Parent: Collected dead child with exit status %d\n", status);
	}
	TracePrintf(10, "Parent: Reclaiming sem...\n");
	Reclaim(sem_id);
	TracePrintf(10, "Parent: Success!\n");
    }

//===========================================================
// TEST SEM VAL STARTS AT 3, 4 PROCESSES WANTING TO DOWN IT
//===========================================================
    TracePrintf(10, "=================================================================\n");
    TracePrintf(10, "TEST 3: SEM VAL STARTS AT 3, 7 PROCESSES WANTING TO DOWN IT\n");
    TracePrintf(10, "=================================================================\n");
    SemInit(&sem_id, 3);    
    rc = Fork();
    if (0 == rc)
    {
	TracePrintf(10, "Child 1: I'm child 1. Trying to down sem...!\n");
	SemDown(sem_id);
	TracePrintf(10, "Child 1: I have downed the sem! Delaying then releasing....\n");
	Delay((rand() % 5));
	SemUp(sem_id);
	TracePrintf(10, "Child 1: Sem released! Exiting....\n");
	Exit(1);

    }
    else
    {
	TracePrintf(10, "Parent: I'm parent!\n");
	Delay(5);
	int count = 2;
	int i;
	for (i = 0; i < 5; i++)
	{
	    rc = Fork();
	    if (0 == rc)
	    {
		int child_no = count;
		TracePrintf(10, "Child %d: I'm child %d! Trying to down sem...\n", child_no, child_no);
		SemDown(sem_id);
		TracePrintf(10, "Child %d: I have downed the sem! Releasing...\n", child_no);
		Delay((rand() % 5));
		SemUp(sem_id);
		Exit(child_no);
	    }
	    count++;
	    Pause();
	}
	
	TracePrintf(10, "Parent: Waiting to children to down sem so I can collecting exit status...\n");
	for (i = 0; i < 100; i++)
	{
	    if (ERROR == Wait(&status))
		break;
	    TracePrintf(10, "Parent: Collected dead child with exit status %d\n", status);
	}
	TracePrintf(10, "Parent: Reclaiming sem...\n");
	Reclaim(sem_id);
	TracePrintf(10, "Parent: Success!\n");
    }

    
    return 0;
}
