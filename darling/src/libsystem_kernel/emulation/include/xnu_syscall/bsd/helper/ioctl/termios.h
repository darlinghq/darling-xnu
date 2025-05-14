#ifndef EMU_TERMIOS_H
#define EMU_TERMIOS_H

#include <darling/emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>

int handle_termios(int fd, unsigned int cmd, void* arg, int* retval);

#endif // EMU_TERMIOS_H
