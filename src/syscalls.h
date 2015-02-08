#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include <hardware.h>
#include "datastructures.h"



int SysFork (UserContext *context);
int SysExec (char *filename, char **argvec);
void SysExit(int status);
int SysWait (int *status_ptr);
int SysGetPid ();
int SysBrk(void *addr);
int SysDelay (int clock_ticks);
int SysTtyRead (int tty_id, void *buf, int len);
int SysPipeInit (int *pipe_idp);
int SysPipeRead (int pipe_id, void *buf, int len);
int SysPipeWrite (int pipe_id, void *buf, int len);
int SysLockInit(int *lock_idp);
int SysAcquire (int lock_id);
int SysRelease (int lock_id);
int SysCvarInit (int *cvar_idp);
int SysCvarWait (int cvar_id, int lock_id);
int SysCvarBroadcast (int cvar_id);
int SysReclaim (int id);

void TrapKernelHandler(UserContext *context);
void TrapClockHandler(UserContext *context);
void TrapIllegalHandler(UserContext *context);
void TrapMemoryHandler(UserContext *context);
void TrapMathHandler(UserContext *context);
void TrapReceiveHandler(UserContext *context);
void TrapTransmitHandler(UserContext *context);
void TrapBad(UserContext *context);

#endif
