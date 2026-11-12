/*
 * Copyright (C) 2026 Lubos Dolezel
 */
#ifndef _DARLING_NANOSLEEP_H
#define _DARLING_NANOSLEEP_H

#include <stdint.h>

// Private to Darling's libc (not in any public header): sleeps for the interval on the host
// without a darlingserver round trip. Returns 0 when the time has passed, or a positive errno
// value (EINTR if a guest signal handler ran or a pending cancel must be acted on; EINVAL for a
// negative or out of range interval).
int __darling_nanosleep(int cancelable, int64_t tv_sec, int32_t tv_nsec);

#endif
