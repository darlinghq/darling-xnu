// Copyright (C) 2026 Lubos Dolezel
#ifndef DARLING_LINUX_STATFS_H
#define DARLING_LINUX_STATFS_H

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// The Linux statfs of a host path or descriptor, with the 64-bit counts on every architecture. The i386 kernel
// has a separate statfs64 call that takes the size of the structure as well; the 64-bit kernels' statfs already
// has 64-bit counts.

static inline long linux_statfs_path(const char* host_path, struct linux_statfs64* lbuf)
{
#ifdef __i386__
	return LINUX_SYSCALL3(__NR_statfs64, host_path, sizeof(*lbuf), lbuf);
#else
	return LINUX_SYSCALL2(__NR_statfs, host_path, lbuf);
#endif
}

static inline long linux_statfs_fd(int fd, struct linux_statfs64* lbuf)
{
#ifdef __i386__
	return LINUX_SYSCALL3(__NR_fstatfs64, fd, sizeof(*lbuf), lbuf);
#else
	return LINUX_SYSCALL2(__NR_fstatfs, fd, lbuf);
#endif
}

#endif // DARLING_LINUX_STATFS_H
