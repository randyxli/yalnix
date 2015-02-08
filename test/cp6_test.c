#include "/net/class/cs58/yalnix/include/yalnix.h"

#define DO_PIPE_TEST 1
#define DO_LOCK_TEST 1
#define DO_RECLAIM 1

int main()
{
  int pid;
  int pipe_id;
  int lock_id;
  int cvar_id;  
  int i;
  int r;
  char *str;
  int *status;

  if (DO_PIPE_TEST) {

    TracePrintf(0, "%d: Attempting to create pipe.\n", GetPid());

    r = PipeInit(&pipe_id);
    if (r == ERROR) {
      TracePrintf(0, "What Happened?!\n");
      Exit(1);
    }

    TracePrintf(0, "%d: Successfully created pipe with pipe id %d\n", GetPid(), pipe_id);

    r = PipeWrite(pipe_id, "Hi! ", 4);
    if (r == ERROR) {
      TracePrintf(0, "What Happened?!\n");
      Exit(1);
    }
    pid = Fork();

    TracePrintf(0, "%d: Writing to pipe\n", GetPid());
    if (pid) {
      r = PipeWrite(pipe_id, "How ", 4);
      if (r == ERROR) {
        TracePrintf(0, "What Happened?!\n");
        Exit(1);
      }
      r = PipeWrite(pipe_id, "are ", 4);
      if (r == ERROR) {
        TracePrintf(0, "What Happened?!\n");
        Exit(1);
      }
      r = PipeWrite(pipe_id, "you?\n", 5);
      if (r == ERROR) {
        TracePrintf(0, "What Happened?!\n");
        Exit(1);
      }

      Delay(4);

    }
    else {
      Delay(2);
      str = (char *) malloc(20 * sizeof(char));
      
      TracePrintf(0, "%d: Reading from pipe\n", GetPid());
      r = PipeRead(pipe_id, str, 17);
      if (r == ERROR) {
        TracePrintf(0, "What Happened?!\n");
        Exit(1);
      }      
      str[17] = 0;

      TracePrintf(0, str);
      TracePrintf(0, "%d: The string above should say Hi! How are you?\n", GetPid());

      Exit(0);
    }

    if (DO_RECLAIM) {
      TracePrintf(0, "%d: Reclaiming pipe.\n", GetPid());
      r = Reclaim(pipe_id);
      if (r == ERROR) {
	TracePrintf(0, "What Happened?!\n");
	Exit(1);
      }
    }

    TracePrintf(0, "%d: Pipe test has ended.\n", GetPid());

  }

  if (DO_LOCK_TEST) {

    TracePrintf(0, "%d: Attempting to create lock.\n", GetPid());

    LockInit(&lock_id);

    TracePrintf(0, "%d: Successfully created lock with lock id %d\n", GetPid(), lock_id);

    i = 3;
    pid = Fork();
    if (pid)
      pid = Fork();

    while (--i) {
      TracePrintf(0, "%d: Attempting to acquire lock id %d..\n", GetPid(), lock_id);
      r = Acquire(lock_id);
      if (r == ERROR) {
	TracePrintf(0, "What Happened?!\n");
	Exit(1);
      }
      TracePrintf(0, "%d: Acquired. Now, I will delay for 2 ticks. Then, I'll release the lock.\n", GetPid());
      Delay(2);
      TracePrintf(0, "%d: Back from delay.\n", GetPid());
      r = Release(lock_id);
      if (r == ERROR) {
	TracePrintf(0, "What Happened?!\n");
	Exit(1);
      }
      TracePrintf(0, "%d: Return value for previous attempt was %d\n", GetPid(), r);
      TracePrintf(0, "%d: Lock successfully released. Now, I will delay for 2 ticks again.\n", GetPid());
      Delay(2);
    } 
  
    if (!pid)
      Exit(0);
    
    Delay(16);
    
    if (DO_RECLAIM) {
      TracePrintf(0, "%d: Attempting to reclaim lock.\n", GetPid());
      r = Reclaim(lock_id);
      if (r == ERROR) {
	TracePrintf(0,"What Happened?!\n");
	Exit(1);
      }
    
      TracePrintf(0, "%d: Successfully reclaimed lock.\n", GetPid());
    }

    TracePrintf(0, "%d: Lock test has ended.\n", GetPid());
  }

  TracePrintf(0, "%d: Exiting!\n", GetPid());
  Exit(0);
}
