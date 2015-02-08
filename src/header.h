//    Filename: header.h
//
//    Description: Some utilites for the TinySearchEngine engine project - MACROs for 
//              general memory allocation and initialization and some 
//              program exceptions processing
//


// Note, the header check below makes sure you do not include a header file twice. Use it.

#ifndef _HEADER_H_
#define _HEADER_H_

int nonemptytrasnlationunit;

#define min(x,y)   ((x)>(y))?(y):(x)

// Print  s together with the source file name and the current line number.
#define LOG(s)  TracePrintf(100, "[%s:%d]%s\n", __FILE__, __LINE__, s)

// malloc a new data structure t

#define NEW(t) malloc(sizeof(t))

// Check whether  s is NULL or not. Quit this program if it is NULL.
#define ERRCHECK(s)  if (s)   {						\
	TracePrintf(1, "Non-zero exit code %d at %s:line%d\n", s, __FILE__, __LINE__); \
	Halt();								\
    }


// Check whether s is NULL or not on a memory allocation. Exit the current process if malloc fails.
#define MALLOC_CHECK(s)  if ((s) == NULL)   {				\
	TracePrintf(1, "No enough memory at %s:line%d ", __FILE__, __LINE__); \
	perror(":");							\
	SysExit(ERROR_MALLOC_FAILED);					\
    }

// Set memory space starts at pointer \a n of size \a m to zero. 
#define BZERO(n,m)  memset(n, 0, m)

#endif
