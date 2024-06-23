#include "getpgrp.h"
#include "getpgid.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

long sys_getpgrp(void)
{
	return sys_getpgid(0);
}

