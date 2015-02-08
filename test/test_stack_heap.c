// Test for memory allocation and stack management with recursion
#include <stdio.h>
#include <yalnix.h>


void recursionTest()
{
    int i = 1;
    TracePrintf(100, "RECURSION!!!!!\n");
    char *random_buffer = (char *)malloc(0x2000);
    recursionTest();
}


int main()
{

    TracePrintf(10, "Running stack + heap memory test!");
    
    // This infinite recursion should cause init process to
    // exit by TrapMemoryHandler or malloc
    recursionTest();
    return 0;
}
