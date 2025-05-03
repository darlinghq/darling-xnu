#ifndef LINUX_GETPRIORITY_H
#define LINUX_GETPRIORITY_H

#include <darling/emulation/conversion/process/priority.h>

int sys_getpriority(int which, id_t who);

#endif // LINUX_GETPRIORITY_H
