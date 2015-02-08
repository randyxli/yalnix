void grow_stack(int val) {
  int int_arr[100];
  
  TracePrintf(0, "grow_stack %d reached\n", val);
  grow_stack(val + 1);

}

int main()
{
  int i;
  int s_len = 2000;
  char *s = malloc(s_len * sizeof(char));
  int pid;
  int printed;
  int nonsense;
  int status;
  int *illegal_addr = 0x000010;

  /* testing Trap Math */
  TracePrintf(0, "Testing Trap Math. My Pid is %d\n", GetPid());
  pid = Fork();
  if (pid == 0) {
    nonsense = 4/0;
    while (1) {
      TracePrintf(0, "I shouldn't be here.\n");
      Pause();
    }
  } else {
    pid = Wait(&status);
    TracePrintf(0, "Child %d exited with exit status %d\n", pid, status);
  }

  /* testing Trap Memory */
  TracePrintf(0, "Testing Trap Memory - First time. My Pid is %d\n", GetPid());
  pid = Fork();
  if (pid == 0) {
    *illegal_addr = 3;
    while (1) {
      TracePrintf(0, "I shouldn't be here.\n");
      Pause();
    }
  } else {
    pid = Wait(&status);
    TracePrintf(0, "Child %d exited with exit status %d\n", pid, status);
  }

  TracePrintf(0, "Testing Trap Memory - Second time. My Pid is %d\n", GetPid());
  pid = Fork();
  if (pid == 0) {
    TracePrintf(0, "Growing stack...\n");
    grow_stack(0);
    while (1) {
      TracePrintf(0, "I shouldn't be here.\n");
      Pause();
    }
  } else {
    pid = Wait(&status);
    TracePrintf(0, "Child %d exited with exit status %d\n", pid, status);
  }

  /* test TtyWrite and TtyRead */
  for (i = 0; i < s_len; ++i) {
    s[i] = '0' + (i % 10);
    if (i % 100 == 0)
      s[i] = '\n';
  }

  TracePrintf(0, "Trying to print out -1 characters of s\n");
  printed = TtyWrite(0, s, -1);
  TracePrintf(0, "Printed %d chars.\n", printed);
  TracePrintf(0, "Trying to print out 0 characters of s\n");
  printed = TtyWrite(0, s, 0);
  TracePrintf(0, "Printed %d chars.\n", printed);
  TracePrintf(0, "Trying to print out 1 character of s\n");
  printed = TtyWrite(0, s, 1);
  TracePrintf(0, "Printed %d chars.\n", printed);
  TracePrintf(0, "Trying to print out 10 characters of s\n");
  printed = TtyWrite(0, s, 10);
  TracePrintf(0, "Printed %d chars.\n", printed);
  TracePrintf(0, "Trying to print out all of s\n");
  printed = TtyWrite(0, s, s_len);
  TracePrintf(0, "Printed %d chars.\n", printed);

  pid = Fork();
  if (pid) {
    while (1) {
      s_len = TtyRead(1, s, 10);
      TracePrintf(0, "Read %d chars.\n", s_len);

      printed = TtyWrite(0, s, s_len);
      TracePrintf(0, "Wrote %d chars.\n", printed);
    }
  } else {
    while (1) {
      s_len = TtyRead(2, s, 10);
      TracePrintf(0, "Read %d chars.\n", s_len);

      printed = TtyWrite(0, s, s_len);
      TracePrintf(0, "Wrote %d chars.\n", printed);
    }
  }
}
