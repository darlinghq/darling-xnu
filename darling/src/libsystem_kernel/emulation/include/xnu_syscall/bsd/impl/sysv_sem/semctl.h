#ifndef LINUX_SYSV_SEMCTL_H
#define LINUX_SYSV_SEMCTL_H

#include <darling/emulation/conversion/sysv_sem/semctl.h>

long sys_semctl(int semid, int semnum, int cmd, union xnu_semun arg);

#endif // LINUX_SYSV_SEMCTL_H
