#ifndef EMU_SOCKET_H
#define EMU_SOCKET_H

#include <darling/emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>

int handle_socket(int fd, unsigned int cmd, void* arg, int* retval);

#endif // EMU_SOCKET_H
