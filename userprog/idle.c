// init process for CS58 Checkpoint 3
#include <stdio.h>


int main()
{
    TracePrintf(100, "Starting idle Process...\n");
    while(1)
    {
	Pause();
	TracePrintf(100, "Idle: Running idle process\n");
    }
}
