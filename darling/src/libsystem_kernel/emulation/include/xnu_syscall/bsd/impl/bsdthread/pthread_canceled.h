#ifndef PTHREAD_CANCELED_H
#define PTHREAD_CANCELED_H

#include <stdbool.h>

#include <stdint.h>

long sys_pthread_canceled(int action);
bool pthread_testcancel_presyscall(void);

// makes a Linux syscall that blocks at a cancellation point (e.g. a futex wait), so that marking the thread
// for cancellation ends it: if the thread is (or becomes) marked before or while the syscall blocks,
// it returns -LINUX_EINTR, unless cancellation is disabled
long pthread_cancelable_syscall(long a1, long a2, long a3, long a4, long a5, long a6, int nr);

// for the handler of the signal that darlingserver interrupts a cancelable syscall with:
// takes the interrupted instruction pointer and returns the one to resume at
uintptr_t pthread_cancelable_syscall_interrupted(uintptr_t ip);

#endif // PTHREAD_CANCELED_H
