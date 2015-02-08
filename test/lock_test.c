// lock test for CS58 Checkpoint 6
#include <stdio.h>
#include <yalnix.h>

int main()
{    
    TracePrintf(100, "Executing Lock Test!\n");
    int rc;
    int lock_id;
    int status;
    LockInit(&lock_id);
    
    rc = Fork();
    if (0 == rc)
    {
	TracePrintf(100, "Child 1: I'm child 1. Acquiring lock...!\n");
	Acquire(lock_id);
	TracePrintf(100, "Child 1: I have the lock! Delaying then releasing....\n");
	Delay(3);
	Release(lock_id);
	TracePrintf(100, "Child 1: Lock released! Exiting....\n");
	Exit(1);

    }
    else
    {
	TracePrintf(100, "Parent: I'm parent!\n");
	Delay(5);
	int count = 2;
	int i;
	for (i = 0; i < 5; i++)
	{
	    rc = Fork();
	    if (0 == rc)
	    {
		int child_no = count;
		TracePrintf(100, "Child %d: I'm child %d! Trying to acquire lock...\n", child_no, child_no);
		Acquire(lock_id);
		TracePrintf(100, "Child %d: I have the lock! Releasing...\n", child_no);
		Delay(2);
		Release(lock_id);
		Exit(child_no);
	    }
	    count++;
	    Pause();
	}
	
	TracePrintf(100, "Parent: All children I have spawned have acquired and released locks and exited! Collecting exit status of children...\n");
	for (i = 0; i < 6; i++)
	{
	    Wait(&status);
	    TracePrintf(100, "Parent: Collected dead child with exit status %d\n", status);
	}
	TracePrintf(100, "Parent: Reclaiming lock...\n");
	Reclaim(lock_id);
	TracePrintf(100, "Parent: Success!\n");
    }
    return 0;
}
