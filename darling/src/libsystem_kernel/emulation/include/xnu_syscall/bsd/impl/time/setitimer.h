#ifndef LINUX_SETITIMER_H
#define LINUX_SETITIMER_H

#include <darling/emulation/conversion/time/setitimer.h>

long sys_setitimer(int which, struct bsd_itimerval* itv,
		struct bsd_itimerval* oitv);

#endif // LINUX_SETITIMER_H
