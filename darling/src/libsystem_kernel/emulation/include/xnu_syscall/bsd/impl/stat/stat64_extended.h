#ifndef LINUX_STAT64_EXTENDED_H
#define LINUX_STAT64_EXTENDED_H

#include <darling/emulation/conversion/stat/types.h>

long sys_stat64_extended(const char* path, darling_stat64_t* stat, void* xsec, unsigned long* xsec_size);

#endif // LINUX_STAT64_EXTENDED_H
