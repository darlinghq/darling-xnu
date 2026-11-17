#ifndef LINUX_OPENAT_H
#define LINUX_OPENAT_H

#include <stdbool.h>

long sys_openat(int fd, const char* filename, int flags, unsigned int mode);
long sys_openat_nocancel(int fd, const char* filename, int flags, unsigned int mode);

// the body of open(), openat() and their $NOCANCEL variants; with `cancelable`, marking the thread
// for cancellation interrupts the open while it blocks (e.g. on a FIFO)
__attribute__((visibility("hidden")))
long sys_openat_common(int fd, const char* filename, int flags, unsigned int mode, bool cancelable);

#endif // LINUX_OPENAT_H
