#ifndef DARLING_EMULATION_CONVERSION_ERRNO_H
#define DARLING_EMULATION_CONVERSION_ERRNO_H

#include <darling/emulation/conversion/variables/bsd/sys/errno.h>
#include <darling/emulation/conversion/variables/linux/asm-generic/errno.h>

int errno_linux_to_bsd(int err);

#endif // DARLING_EMULATION_CONVERSION_ERRNO_H
