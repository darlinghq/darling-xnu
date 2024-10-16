#ifndef DARLING_BSD_SYSCALL_TABLE_H
#define DARLING_BSD_SYSCALL_TABLE_H

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_create.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>
#include <darling/emulation/xnu_syscall/bsd/impl/dirent/getdirentries.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/openat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/abort_with_payload.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/syscall.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/thread_selfid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/recvmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendto.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigprocmask.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/access.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/exit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/faccessat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pipe.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>

extern void* __bsd_syscall_table[600];

#endif // DARLING_BSD_SYSCALL_TABLE_H
