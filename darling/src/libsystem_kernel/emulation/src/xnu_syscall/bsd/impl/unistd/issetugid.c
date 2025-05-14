#include <darling/emulation/xnu_syscall/bsd/impl/unistd/issetugid.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/geteuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getegid.h>

long sys_issetugid(void)
{
	return sys_getuid() != sys_geteuid() || sys_getgid() != sys_getegid();
}
