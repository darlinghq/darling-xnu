#ifndef LINUX_SELECT_H
#define LINUX_SELECT_H

#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

long sys_select(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout);
long sys_select_nocancel(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout);

// Wake a select() waiting on fd; Darwin reports EBADF after a watched descriptor is closed.
void select_fd_closed(int fd);

#endif // LINUX_SELECT_H
