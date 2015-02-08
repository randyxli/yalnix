// reclaim test for CS58 Checkpoint 6
#include <stdio.h>
#include <yalnix.h>

#define MAX_LOCKS 100 // Set this equal to MAX_LOCKS in kernel.h
#define MAX_CVARS 100 // Set this equal to MAX_CVARS in kernel.h

int main()
{    
    int i;
    int lock_id;
    int cvar_id;
    int status;
    int locks[MAX_LOCKS];
    int cvars[MAX_CVARS];
    
//=============
// LOCK TEST
//============
    for(i = 0; i < MAX_LOCKS; i++)
    {
	if (-1 == LockInit(&lock_id))
	{
	    TracePrintf(100, "Error: Lock Init failed!\n");
	    return -1;
	}
	TracePrintf(100, "Lock %d created!\n", lock_id);
	locks[i] = lock_id;
    }
    
    int reclaim_me = locks[MAX_LOCKS/2];
    TracePrintf(100, "Reclaiming lock %d...\n", reclaim_me);
    Reclaim(reclaim_me);
    TracePrintf(100, "Lock %d reclaimed!\n", reclaim_me);

    TracePrintf(100, "Initializing a new lock...\n");
    if (ERROR == LockInit(&lock_id))
    {
	TracePrintf(100, "Error: A new lock could not be created. This probably means that lock number allocation is not working. Halting...\n");
	return -1;
    }
    TracePrintf(100, "Success: Lock %d created!\n", lock_id);


//============
// CVAR TEST
//============
    for(i = 0; i < MAX_CVARS; i++)
    {
	if (-1 == CvarInit(&cvar_id))
	{
	    TracePrintf(100, "Error: Cvar Init failed!\n");
	    return -1;
	}
	TracePrintf(100, "Cvar %d created!\n", cvar_id);
	cvars[i] = cvar_id;
    }
    
    reclaim_me = cvars[MAX_CVARS/2];
    TracePrintf(100, "Reclaiming cvar %d...\n", reclaim_me);
    Reclaim(reclaim_me);
    TracePrintf(100, "Cvar %d reclaimed!\n", reclaim_me);

    TracePrintf(100, "Initializing a new cvar...\n");
    if (ERROR == CvarInit(&cvar_id))
    {
	TracePrintf(100, "Error: A new cvar could not be created. This probably means that cvar number allocation is not working. Halting...\n");
	return -1;
    }
    TracePrintf(100, "Success: Cvar %d created!\n", cvar_id);

    TracePrintf(100, "All tests successful!\n");
    return 0;
}
