int main()
{
  int del_val;
  int del_min;
  int del_ret;
  int pid;
  int status;
  char *str;
  char *str2;
  int index;
  char *args_val[] = {"../userprog/idle", 0};
  char *args_inv[] = {"gibberish", 0};
  int exec_ret;

  TracePrintf(0, "Hi! This program comprises a simple series of tests. Your yalnix kernel should be able to handle way more!\n");

  // very simple exec; I did this as a separate test as it was implementation dependant
  /*

  //Exec("idle", args_val);
  
  TracePrintf(0, "Testing exec. First one should fail. Second should go through and put us in idle.\n");
  pid = Fork();
  if (pid) {
    Wait(&status);
  } else {
    TracePrintf(0, "Execing\n");
    exec_ret = Exec("gibberishhhhh", args_inv);
    TracePrintf(0, "Yay, it failed, as it should. Exec returned %d, which should be the ERROR code.\n", exec_ret);
    TracePrintf(0, "Execing again\n");
    //exec_ret = Exec("idle", args_val);
    //TracePrintf(0, "Something went wrong!\n");
    TracePrintf(0, "Exiting Child\n");
    Exit(0);
  }

  TracePrintf(0, "Exiting Parent\n");
  Exit(0);
  */

  /* simply isolation tests */

  // very simple GetPid
  // This is useful in determining which pid is associated with init!
  TracePrintf(0, "The init process has pid %d.\n", GetPid());

  // very simple brk
  str = malloc(20000);

  str2 = "Let's continue.\n";

  while (str2[index] != 0) {
    str[index] = str2[index];
    index++;
  }
  str[index] = 0;

  str[10000] = 'a';
  
  TracePrintf(0, str);

  // simple fork
  TracePrintf(0, "Forking!\n");
  pid = Fork();
  if (pid)
    TracePrintf(0, "Forked. I am the parent. My child's pid is %d.\n", pid);
  else
    TracePrintf(0, "Forked. I am the child.\n");

  // simple getpid
  if (pid)
    TracePrintf(0, "I am the parent. My pid is %d.\n", GetPid());
  else
    TracePrintf(0, "I am the child. My pid is %d.\n", GetPid());

  // simple exit
  if (0 == pid) {
    TracePrintf(0, "I am the child. I will exit.\n");
    Exit(0);
    TracePrintf(0, "What happened? I should have exited.\n");
  }

  // simple wait
  TracePrintf(0, "I am the parent. I will wait for my child to die.\n");
  pid = Wait(&status);
  TracePrintf(0, "My child with pid %d died.\n", pid);

  // simple delay after fork
  del_val = 3;
  del_min = -1;
  while (del_val >= del_min) {
    TracePrintf(0, "I am the parent and I am delaying for %d clock ticks.\n", del_val);
    del_ret = Delay(del_val--);
    if (del_ret < 0) {
      TracePrintf(0, "You should see this message once and ONLY once after delaying for -1 ticks.\n");
    }
  }

  if (del_ret >= 0) {
    TracePrintf(0, "It looks like you did not meet the API for delay! Error cases should behave logically.\n");
  }

  /* some very basic integration testing */

  // first integration test

  TracePrintf(0, "First Integration Test\n");
  pid = Fork();
  if (pid) {
    pid = Fork();
    if (pid) {
      TracePrintf(0, "I'm the parent. I have pid = %d\n", GetPid());
      TracePrintf(0, "I'm one twisted parent. I'm waiting for my children to die.\n");
      TracePrintf(0, "So, I will call wait twice. And then again, just to make sure they're both dead.\n");
      pid = Wait(&status);
      TracePrintf(0, "Child %d has died with status %d (should be 5). Yes, I give my children numbers, not names.\n", pid, status);
      pid = Wait(&status);
      TracePrintf(0, "Child %d has died with status %d (should be 10). Yes, I give my children numbers, not names.\n", pid, status);
      pid = Wait(&status);
      TracePrintf(0, "This should fail. %d should be an error code.\n", pid);
    } else {
      TracePrintf(0, "I'm one of two children. I have pid %d. Now, I shall exit.\n", GetPid());      
      return 5;
    }
  } else {
    TracePrintf(0, "I'm one of two children. I have pid %d. Now I shall delay for 4 ticks.\n", GetPid());
    Delay(4);
    TracePrintf(0, "I'm one of two children. I have pid %d. Now, I shall exit.\n", GetPid());    
    Exit(10);
  }

  // second integration test

  TracePrintf(0, "Second Integration Test\n");

  TracePrintf(0, "Forking a few times.\n");
  pid = Fork();
  if (pid)
    pid = Fork();
  if (pid)
    pid = Fork();

  if (pid) {
    TracePrintf(0, "Parent waiting.\n");
    Wait(&status);
    TracePrintf(0, "Parent waiting.\n");
    Wait(&status);
    TracePrintf(0, "Parent waiting.\n");
    Wait(&status);
  } else {
    TracePrintf(0, "Child exiting.\n");
    Exit(0);
  }

  TracePrintf(0, "Exiting. Yay! Hopefully we don't get a core dump and the machines halts, as it should. :)\n");

  // simple init exit
  Exit(0);
}
