#ifndef DARLING_EMULATION_CONVERSION_AT_H
#define DARLING_EMULATION_CONVERSION_AT_H

// Not sure where this comes from...
#define LINUX_AT_INVALID 		0x1

#include <emulation/conversion/variables/bsd/sys/fcntl.h>
#include <emulation/conversion/variables/linux/linux/fcntl.h>

int atflags_bsd_to_linux(int flags);

#endif // DARLING_EMULATION_CONVERSION_AT_H
