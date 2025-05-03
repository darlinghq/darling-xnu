#ifndef LINUX_GETITIMER_H
#define LINUX_GETITIMER_H

#include <darling/emulation/xnu_syscall/bsd/impl/time/setitimer.h>

long sys_getitimer(int which, struct bsd_itimerval* oitv);

#endif // LINUX_GETITIMER_H
