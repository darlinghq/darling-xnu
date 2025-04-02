#include <emulation/xnu_syscall/bsd/impl/unistd/setuid.h>

#include <emulation/xnu_syscall/shared/unistd/uidgid.h>

long sys_setgid(int gid)
{
	return __setuidgid(-1, gid);
}

