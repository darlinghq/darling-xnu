#include "./sys/eventfd.h"
#include "../errno.h"
#include "../base.h"
#include <linux-syscalls/linux.h>

extern long cerror(int __err);
extern long read(int fd, void *buf, unsigned long count);

VISIBLE
int eventfd_read (int __fd, eventfd_t *__value)
{
	return read(__fd, __value, sizeof(eventfd_t)) == sizeof(eventfd_t)
		? 0 : -1;
}

