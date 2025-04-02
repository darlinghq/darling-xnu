#include <emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>

#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/variables/bsd/sys/unistd.h>


long sys_fpathconf(int fd, int name)
{
	static const short values[] = {
		[BSD__PC_LINK_MAX] = 8,
		[BSD__PC_MAX_CANON] = 255,
		[BSD__PC_MAX_INPUT] = 255,
		[BSD__PC_NAME_MAX] = 255,
		[BSD__PC_PATH_MAX] = 4096,
		[BSD__PC_PIPE_BUF] = 4096,
		[BSD__PC_CHOWN_RESTRICTED] = 1,
		[BSD__PC_NO_TRUNC] = 1,
		[BSD__PC_VDISABLE] = 0
	};
	
	if (name < 1 || name >= sizeof(values) / sizeof(values[0]))
		return -BSD_EINVAL;
	else
		return values[name];
}
