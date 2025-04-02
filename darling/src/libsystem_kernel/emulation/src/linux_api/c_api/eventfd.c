#include <emulation/linux_api/c_api/sys/eventfd.h>

#include <emulation/linux_api/linux_syscall.h>

extern long cerror(int __err);
extern long write(int fd, const void *buf, unsigned long count);

VISIBLE
int __linux_eventfd(int count, int flags)
{
	int rv;

	#if defined(__x86_64__) || defined(__i386__)
		rv = LINUX_SYSCALL(flags ? __NR_eventfd2 : __NR_eventfd, count, flags);
	#else
		// It's not clear if 0 is offically a valid flag argument.
		// But we don't really have a choice.
		rv = LINUX_SYSCALL(__NR_eventfd2, count, flags);
	#endif
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int __linux_eventfd_write(int fd, eventfd_t value)
{
	return write(fd, &value, sizeof(eventfd_t)) == sizeof(eventfd_t) ? 0 : -1;
}
