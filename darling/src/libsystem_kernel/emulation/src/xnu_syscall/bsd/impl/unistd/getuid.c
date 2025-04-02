#include <emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <emulation/xnu_syscall/shared/unistd/uidgid.h>

#include <stddef.h>
#include <stdint.h>

long sys_getuid(void)
{
	int32_t uid;
	__getuidgid(&uid, NULL);
	return uid;
}

