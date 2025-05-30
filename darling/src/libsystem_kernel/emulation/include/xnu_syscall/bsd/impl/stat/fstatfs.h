#ifndef LINUX_FSTATFS_H
#define LINUX_FSTATFS_H

#include <darling/emulation/conversion/stat/common.h>

long sys_fstatfs(int fs, struct bsd_statfs* buf);
long sys_fstatfs64(int fd, struct bsd_statfs64* buf);

#endif // LINUX_FSTATFS_H
