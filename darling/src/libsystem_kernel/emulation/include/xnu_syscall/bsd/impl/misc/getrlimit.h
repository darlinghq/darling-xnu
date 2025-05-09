#ifndef LINUX_GETRLIMIT_H
#define LINUX_GETRLIMIT_H

#include <darling/emulation/conversion/misc/getrlimit.h>

long sys_getrlimit(unsigned int which, struct xnu_rlimit* rlp);

#endif // LINUX_GETRLIMIT_H
