#include "./sys/epoll.h"
#include "../errno.h"
#include "../base.h"
#include <linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int epoll_create (__unused int __size)
{
	return epoll_create1(0);
}

