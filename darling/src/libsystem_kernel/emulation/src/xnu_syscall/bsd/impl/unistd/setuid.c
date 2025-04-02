#include <emulation/xnu_syscall/bsd/impl/unistd/setuid.h>

#include <emulation/xnu_syscall/shared/unistd/uidgid.h>

long sys_setuid(int uid)
{
	return __setuidgid(uid, -1);
}

