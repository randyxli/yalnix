#
#	Sample Makefile for Yalnix kernel and user programs.
#	
#	Prepared by Sean Smith and Adam Salem and various Yalnix developers
#	of years past...
#
#	You must modify the KERNEL_SRCS and KERNEL_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix kernel.
#
#	You must modify the USER_SRCS and USER_OBJS definition below to be your own
#	list of .c and .o files that should be linked to build your Yalnix user programs
#
#	The Yalnix kernel built will be named "yalnix".  *ALL* kernel
#	Makefiles for this lab must have a "yalnix" rule in them, and
#	must produce a kernel executable named "yalnix" -- we will run
#	your Makefile and will grade the resulting executable
#	named "yalnix".
#

#make all will make all the kernel objects and user objects
ALL = $(KERNEL_ALL) $(USER_APPS)
KERNEL_ALL = yalnix

#List all kernel source files here.  
KERNEL_SRCS = ./src/kernel.c ./src/linkedlist.c ./src/syscalls.c ./src/trap.c
#List the objects to be formed form the kernel source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
KERNEL_OBJS = ./src/kernel.o ./src/linkedlist.o ./src/syscalls.o ./src/trap.o
#List all of the header files necessary for your kernel
KERNEL_INCS = hardware.h yalnix.h ./src/datastructures.h ./src/kernel.h ./src/header.h


#List all user programs here.
USER_APPS = ./userprog/init ./userprog/idle ./test/ttyread_test ./test/ttywrite_test ./test/trapmath_test ./test/cp4_test ./test/lock_test ./test/sem_test ./test/cvar_test ./test/reclaim_test ./test/pipe_test ./test/torture ./test/test_stack_heap ./test/cp6_test ./test/cp5_test ./test/infinite_exec ./test/bigstack ./test/forktest ./test/zero ./test/infinite_fork
#List all user program source files here.  SHould be the same as the previous list, with ".c" added to each file
USER_SRCS = ./userprog/init.c ./userprog/idle.c ./test/ttyread_test.c ./test/ttywrite_test.c ./test/trapmath_test.c ./test/cp4_test.c ./test/lock_test.c ./test/sem_test.c ./test/cvar_test.c ./test/reclaim_test.c ./test/pipe_test.c ./test/torture.c ./test/test_stack_heap.c ./test/cp6_test.c ./test/cp5_test.c ./test/infinite_exec.c ./test/bigstack.c ./test/forktest.c ./test/zero.c ./test/infinite_fork.c
#List the objects to be formed form the user  source files here.  Should be the same as the prvious list, replacing ".c" with ".o"
USER_OBJS = ./userprog/init.o ./userprog/idle.o ./test/ttyread_test.o ./test/ttywrite_test.o ./test/trapmath_test.o ./test/cp4_test.o ./test/lock_test.o ./test/sem_test.o ./test/cvar_test.o ./test/reclaim_test.o ./test/pipe_test.o ./test/torture.o ./test/test_stack_heap.o ./test/cp6_test.o ./test/cp5_test.o ./test/infinite_exec.o ./test/bigstack.o ./test/forktest.o ./test/zero.o ./test/infinite_fork.o
#List all of the header files necessary for your user programs
USER_INCS = 

#write to output program yalnix
YALNIX_OUTPUT = yalnix

#
#	These definitions affect how your kernel is compiled and linked.
#       The kernel requires -DLINUX, to 
#	to add something like -g here, that's OK.
#

#Set additional parameters.  Students generally should not have to change this next section

#Use the gcc compiler for compiling and linking
CC = gcc

DDIR58 = /net/class/cs58/yalnix
LIBDIR = $(DDIR58)/lib
INCDIR = $(DDIR58)/include
ETCDIR = $(DDIR58)/etc

# any extra loading flags...
LD_EXTRA = 

KERNEL_LIBS = $(LIBDIR)/libkernel.a $(LIBDIR)/libhardware.so

# the "kernel.x" argument tells the loader to use the memory layout
# in the kernel.x file..
KERNEL_LDFLAGS = $(LD_EXTRA) -L$(LIBDIR) -lkernel -lelf -Wl,-T,$(ETCDIR)/kernel.x -Wl,-R$(LIBDIR) -lhardware
LINK_KERNEL = $(LINK.c)

#
#	These definitions affect how your Yalnix user programs are
#	compiled and linked.  Use these flags *only* when linking a
#	Yalnix user program.
#

USER_LIBS = $(LIBDIR)/libuser.a
ASFLAGS = -D__ASM__
CPPFLAGS= -m32 -fno-builtin -I. -I$(INCDIR) -g -DLINUX 


##########################
#Targets for different makes
# all: make all changed components (default)
# clean: remove all output (.o files, temp files, LOG files, TRACE, and yalnix)
# count: count and give info on source files
# list: list all c files and header files in current directory
# kill: close tty windows.  Useful if program crashes without closing tty windows.
# $(KERNEL_ALL): compile and link kernel files
# $(USER_ALL): compile and link user files
# %.o: %.c: rules for setting up dependencies.  Don't use this directly
# %: %.o: rules for setting up dependencies.  Don't use this directly

all: $(ALL)	

clean:
	rm -f *.o *~ TTYLOG* TRACE $(YALNIX_OUTPUT) $(USER_APPS) $(KERNEL_OBJS) $(USER_OBJS)

count:
	wc $(KERNEL_SRCS) $(USER_SRCS)

list:
	ls -l *.c *.h

kill:
	killall yalnixtty yalnixnet yalnix

$(KERNEL_ALL): $(KERNEL_OBJS) $(KERNEL_LIBS) $(KERNEL_INCS)
	$(LINK_KERNEL) -o $@ $(KERNEL_OBJS) $(KERNEL_LDFLAGS)

$(USER_APPS): $(USER_OBJS) $(USER_INCS)
	$(ETCDIR)/yuserbuild.sh $@ $(DDIR58) $@.o










