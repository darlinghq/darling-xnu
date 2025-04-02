#ifndef DARLING_EMULATION_CONVERSION_FCNTL_H
#define DARLING_EMULATION_CONVERSION_FCNTL_H

#include <emulation/conversion/variables/linux/linux/fcntl.h>
#include <emulation/conversion/variables/bsd/sys/fcntl.h>

int cmd_bsd_to_linux(int bsd);
short int flock_type_linux_to_bsd(short int linux);
short int flock_type_bsd_to_linux(short int bsd);
int oflags_bsd_to_linux(int flags);
int oflags_linux_to_bsd(int flags);

#endif // DARLING_EMULATION_CONVERSION_FCNTL_H