#ifndef LINUX_PRIORITY_H
#define LINUX_PRIORITY_H

#include <sys/_types.h>
#include <sys/_types/_id_t.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// Same on BSD and Linux
#define	PRIO_PROCESS	0
#define	PRIO_PGRP	1
#define	PRIO_USER	2

// Unique to Darwin
#define	PRIO_DARWIN_THREAD	3
#define	PRIO_DARWIN_PROCESS	4

// Same on BSD and Linux
#define	PRIO_MIN	-20
#define	PRIO_MAX	20

// Unique to Darwin
#define PRIO_DARWIN_BG 0x1000
#define PRIO_DARWIN_NONUI 0x1001

#endif // LINUX_PRIORITY_H
