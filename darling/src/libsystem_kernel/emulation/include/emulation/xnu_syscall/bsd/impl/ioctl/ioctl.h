#ifndef DARLING_BSD_EMULATION_IOCTL_H
#define DARLING_BSD_EMULATION_IOCTL_H

#include <stdint.h>

#include <emulation/xnu_syscall/bsd/impl/ioctl/ioctl-helper_filio.h>
#include <emulation/xnu_syscall/bsd/impl/ioctl/ioctl-helper_socket.h>
#include <emulation/xnu_syscall/bsd/impl/ioctl/ioctl-helper_termios.h>

extern long sys_ioctl(int fd, int cmd, void* arg);

enum {
	IOCTL_PASS,			/** ioctl cmd not known */
	IOCTL_HANDLED		/** ioctl cmd now already handled */
};

#endif // DARLING_BSD_EMULATION_IOCTL_H
