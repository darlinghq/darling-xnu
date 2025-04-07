#include <darling/emulation/legacy_path/ext/sys/eventfd.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

extern long cerror(int __err);
extern long write(int fd, const void *buf, unsigned long count);

VISIBLE
int eventfd_write (int __fd, eventfd_t __value)
{
	return write(__fd, &__value, sizeof(eventfd_t)) == sizeof(eventfd_t)
		? 0 : -1;
}

