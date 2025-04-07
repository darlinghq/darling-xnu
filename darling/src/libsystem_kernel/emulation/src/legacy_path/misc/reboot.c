#include <darling/emulation/legacy_path/misc/reboot.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/exit.h>

long sys_reboot(int opt, const char* cmd)
{
	__simple_printf("ALERT: The process has asked for system reboot with opt %d and cmd '%s' - terminating\n", cmd);
	sys_exit(1);
	return -ENOTSUP;
}

