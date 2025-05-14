#include <darling/emulation/xnu_syscall/bsd/impl/misc/getlogin.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern char *getenv(const char *name);
extern unsigned long strlcpy(char* dst, const char* src, unsigned long size);

long sys_getlogin(char* buf, unsigned int len)
{
	int ret;
	char* e;

	e = getenv("USER");
	if (e != NULL)
	{
		return strlcpy(buf, e, len);
	}
	else
	{
		*buf = '\0';
		return 0;
	}
}
