#ifndef LINUX_FSTAT64_EXTENDED_H
#define LINUX_FSTAT64_EXTENDED_H

#include <darling/emulation/conversion/stat/types.h>

long sys_fstat64_extended(int fd, darling_stat64_t* stat, void* xsec, unsigned long* xsec_size);

#endif // LINUX_FSTAT64_EXTENDED_H
