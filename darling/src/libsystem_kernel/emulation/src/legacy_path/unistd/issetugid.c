#include <darling/emulation/legacy_path/unistd/issetugid.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>
#include <darling/emulation/legacy_path/unistd/geteuid.h>
#include <darling/emulation/legacy_path/unistd/getgid.h>
#include <darling/emulation/legacy_path/unistd/getegid.h>

long sys_issetugid(void)
{
	return sys_getuid() != sys_geteuid() || sys_getgid() != sys_getegid();
}

