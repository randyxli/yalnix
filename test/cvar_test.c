// cvar test for CS58 Checkpoint 6
#include <stdio.h>
#include <yalnix.h>

int main()
{    
    int rc;
    int cvar_id;
    int lock_id;
    LockInit(&lock_id);
    CvarInit(&cvar_id);
    
    rc = Fork();
    if (0 == rc)
    {
	TracePrintf(100, "Child: I'm parent! Delaying...\n");
	Delay(5);
	TracePrintf(100, "Child: Trying to get lock...\n");
	Acquire(lock_id);
	TracePrintf(100, "Child: I have lock! Calling CvarSignal\n");
	CvarSignal(cvar_id);
	TracePrintf(100, "Child: Called CvarSignal! Delaying for 5...\n");
	Delay(5);
	TracePrintf(100, "Child: Back from delay! Releasing lock....\n");
	Release(lock_id);
	TracePrintf(100, "Child: Lock released! Exiting....\n");
	Exit(2);
    }
    else
    {
	TracePrintf(100, "Parent: I'm parent. Acquiring lock...!\n");
	Acquire(lock_id);
	TracePrintf(100, "Parent: I have the lock! Calling CvarWait....\n");
	CvarWait(cvar_id, lock_id);
	TracePrintf(100, "Parent: Back from CvarWait! I have the lock too! Releasing....\n");
	Release(lock_id);
	TracePrintf(100, "Parent: Lock released!\n");
	TracePrintf(100, "Parent: Reclaiming lock...\n");
	Reclaim(lock_id);
	TracePrintf(100, "Parent: Lock reclaimed!\n");
	TracePrintf(100, "Parent: Reclaiming cvar...\n");
	Reclaim(cvar_id);
	TracePrintf(100, "Parent: Cvar Reclaimed!\n");

	TracePrintf(100, "Parent: Success!\n");
	Exit(0);
    }
    return 0;
}
