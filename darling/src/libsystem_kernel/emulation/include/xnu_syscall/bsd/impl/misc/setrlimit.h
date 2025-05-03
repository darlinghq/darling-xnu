#ifndef LINUX_SETRLIMIT_H
#define LINUX_SETRLIMIT_H

#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrlimit.h>

long sys_setrlimit(unsigned int which, const struct rlimit* rlp);

#endif // LINUX_SETRLIMIT_H
