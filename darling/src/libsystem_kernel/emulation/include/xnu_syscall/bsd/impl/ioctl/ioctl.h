#ifndef EMU_IOCTL_H
#define EMU_IOCTL_H

#include <stdint.h>

extern long __real_ioctl(int fd, int cmd, void* arg);
extern long sys_ioctl(int fd, int cmd, void* arg);

enum {
	IOCTL_PASS,			/** ioctl cmd not known */
	IOCTL_HANDLED		/** ioctl cmd now already handled */
};

#endif // EMU_IOCTL_H
