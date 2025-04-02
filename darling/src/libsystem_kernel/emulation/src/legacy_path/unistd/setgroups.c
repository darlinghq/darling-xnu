#include "setgroups.h"
#include "getgroups.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

long sys_setgroups(unsigned int size, int* gidset)
{
	return __setgroups(size, (const unsigned int*)gidset);
}

