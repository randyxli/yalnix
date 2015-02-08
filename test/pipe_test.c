// init process for CS58 Checkpoint 3
#include <stdio.h>
#include <yalnix.h>
#include <string.h>

int main()
{
    int i = 1;
    int pipe_cnt = 50;
    TracePrintf(100, "Creating %d pipes to test PipeInit and Reclaim\n", pipe_cnt);
    int pipe_id[50];
    /*
    while(1)
    {
	TracePrintf(1, "Creating pipe %d\n", i);
	PipeInit(&pipe_id[0]);
	Reclaim(pipe_id[0]);
	TracePrintf(1, "Reclaiming pipe %d\n", i);
	i++;
    }
    */
    for(i=0; i < pipe_cnt; i++)
    {
	PipeInit(&pipe_id[i]);
	TracePrintf(1, "Created pipe %d\n", pipe_id[i]);
    }

    TracePrintf(1, "Now deleting pipes\n");
    for(i=0; i < pipe_cnt; i++)
    {
	Reclaim(pipe_id[i]);
	TracePrintf(1, "Deleted pipe %d\n", pipe_id[i]);
    }

    PipeInit(pipe_id);
    TracePrintf(1, "Created pipe %d to test IPC\n", pipe_id[0]);

    TracePrintf(100, "Running Init Process, about to fork...\n");
    int rc = Fork();
    TracePrintf(100, "Init: Back from fork with rc %d!\n", rc);
    int status_ptr;
    int charsread;
    if(0 == rc)
    {
        
	TracePrintf(100, "Init: I'M the reader, Process %d!!!!\n", GetPid());
	Delay(1);	    
	    
	char str1[2048];
	TracePrintf(1, "Reader is about to read repeatedly from the pipe until it is empty, causing him to block");
	for(i=0; i<4; i++)
	{
	    charsread = PipeRead(pipe_id[0], str1, 10);
	    str1[charsread] = 0;
	    TracePrintf(1, "Child read %d bytes from pipe %d:\n%s\n", charsread, pipe_id[0], str1);
	}
	TracePrintf(1, "Reader now attempting to write to the buffer to test one-directionality\n");
	PipeWrite(pipe_id[0], str1, 1);
	
	//TracePrintf(1, "Reader now attempting to read from the pipe into a string literal\n");
	
	//PipeRead(pipe_id[0], "evil!", 2);

	TracePrintf(1, "Reader about to delay to allow the writer to fill the pipe\n");
	Delay(10);

	for(i=0; i<15; i++)
	{
	    charsread = PipeRead(pipe_id[0], str1, 200);
	    str1[charsread] = 0;
	    TracePrintf(1, "Child read %d bytes from pipe %d:\n%s\n", charsread, pipe_id[0], str1);
	}
	
	TracePrintf(100, "Init: Child about to exit...\n");
	Exit(100);
	
    }
    else
	while(1)
	{
	    TracePrintf(100, "Init: I'M the writer, process %d!!!!\n", GetPid());
	    char str[2048];
	    for(i=0; i<100; i++)
		memcpy(str+i*20, "12345678900987654321", 20);

	    str[2047] = 0;

	    TracePrintf(1, "Writing the following to the pipe:\n%s\n", str);
	    charsread = PipeWrite(pipe_id[0], str, 20);

	    TracePrintf(1, "Writer about to attempt to read from the buffer\n");
	    PipeRead(pipe_id[0], str, 1);

	    Delay(10);
	    
	    TracePrintf(1, "Writer for pipe now attempting to fill buffer to test blocking\n");
	    for(i=0; i<15; i++)
	    {
		TracePrintf(1, "Iteration %d, About to write to pipe %d:\n", i, pipe_id[0]);
		charsread = PipeWrite(pipe_id[0], str, 200);
	    }
	    TracePrintf(1, "Init: Writer, Process %d about to exit\n", GetPid());
	    Delay(10);
	    Exit(100);
	}
    
}

void recursion(void)
{
    int i = 1;
    TracePrintf(100, "RECURSION!!!!!\n");
    recursion();
}
