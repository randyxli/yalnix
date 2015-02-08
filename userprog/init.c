// init process for CS58 Checkpoint 3
#include <stdio.h>
#include <yalnix.h>

int main()
{
    while(1)
    {
	TracePrintf(1, "Init: running\n");
	TracePrintf(1, "Init: about to delay\n");
	Delay(1);
    }
}
