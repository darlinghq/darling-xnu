#ifndef DARLING_EMULATION_LINUX_API_FUTEX_H
#define DARLING_EMULATION_LINUX_API_FUTEX_H

#include <darling/emulation/common/base.h>

#define FUTEX_WAIT	0
#define FUTEX_WAKE	1

struct timespec;

CPP_EXTERN_C_BEGIN

int __linux_futex(int *uaddr, int op, int val, const struct timespec *timeout,
		                 int *uaddr2, int val3);
int __linux_futex_reterr(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);

CPP_EXTERN_C_END

#endif // DARLING_EMULATION_LINUX_API_FUTEX_H

