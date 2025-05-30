#ifndef LINUX_GETFSSTAT_H
#define LINUX_GETFSSTAT_H

#include <darling/emulation/conversion/stat/common.h>

long sys_getfsstat(struct bsd_statfs* buf, int bufsize, int flags);
long sys_getfsstat64(struct bsd_statfs64* buf, int bufsize, int flags);

#endif // LINUX_GETFSSTAT_H
