#ifndef EXT_FUTEX_H
#define EXT_FUTEX_H

#include <darling/emulation/common/base.h>

CPP_EXTERN_BEGIN

#define FUTEX_WAIT	0
#define FUTEX_WAKE	1

struct timespec;

int __linux_futex (int *uaddr, int op, int val, const struct timespec *timeout,
		                 int *uaddr2, int val3);
int __linux_futex_reterr(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);

CPP_EXTERN_END

#endif // EXT_FUTEX_H
