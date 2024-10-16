#include "access.h"

#include <emulation/darling/tsd/perthread_wd.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/faccessat.h>

long sys_access(const char* filename, int amode)
{
	return sys_faccessat(get_perthread_wd(), filename, amode, 0);
}
