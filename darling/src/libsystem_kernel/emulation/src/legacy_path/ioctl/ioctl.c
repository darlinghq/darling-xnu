#include <darling/emulation/legacy_path/ioctl/ioctl.h>

#include <sys/errno.h>
#include <stddef.h>
#include <stdbool.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/ioctl/termios.h>
#include <darling/emulation/legacy_path/ioctl/filio.h>
#include <darling/emulation/legacy_path/ioctl/termios.h>
#include <darling/emulation/legacy_path/ioctl/socket.h>

#define IOCTL_STEP(x) { int state, retval; state = (x); \
	if (state == IOCTL_HANDLED) return retval; }

// Emulated ioctl implementation
long sys_ioctl(int fd, int cmd, void* arg)
{
	IOCTL_STEP(handle_filio(fd, cmd, arg, &retval));
	IOCTL_STEP(handle_socket(fd, cmd, arg, &retval));
	IOCTL_STEP(handle_termios(fd, cmd, arg, &retval));
	
run_ioctl:
	return __real_ioctl(fd, cmd, arg);
}



