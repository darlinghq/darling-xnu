#include <darling/emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

enum {
	BSD_PC_LINK_MAX = 1,
	BSD_PC_MAX_CANON,
	BSD_PC_MAX_INPUT,
	BSD_PC_NAME_MAX,
	BSD_PC_PATH_MAX,
	BSD_PC_PIPE_BUF,
	BSD_PC_CHOWN_RESTRICTED,
	BSD_PC_NO_TRUNC,
	BSD_PC_VDISABLE
};

long sys_fpathconf(int fd, int name)
{
	static const short values[] = {
		[BSD_PC_LINK_MAX] = 8,
		[BSD_PC_MAX_CANON] = 255,
		[BSD_PC_MAX_INPUT] = 255,
		[BSD_PC_NAME_MAX] = 255,
		[BSD_PC_PATH_MAX] = 4096,
		[BSD_PC_PIPE_BUF] = 4096,
		[BSD_PC_CHOWN_RESTRICTED] = 1,
		[BSD_PC_NO_TRUNC] = 1,
		[BSD_PC_VDISABLE] = 0
	};
	
	if (name < 1 || name >= sizeof(values) / sizeof(values[0]))
		return -EINVAL;
	else
		return values[name];
}
