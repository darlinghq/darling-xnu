#include <darling/emulation/xnu_syscall/bsd/impl/misc/reboot.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/exit.h>

long sys_reboot(int opt, const char* cmd)
{
	__simple_printf("ALERT: The process has asked for system reboot with opt %d and cmd '%s' - terminating\n", cmd);
	sys_exit(1);
	return -ENOTSUP;
}
