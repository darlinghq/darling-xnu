#include <darling/emulation/legacy_path/misc/getlogin.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

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

