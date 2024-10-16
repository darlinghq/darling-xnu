#ifndef DARLING_EMULATION_LINUX_API_EVENTFD_H
#define DARLING_EMULATION_LINUX_API_EVENTFD_H

#include <stdint.h>

#ifndef __THROW
#	define __THROW
#endif

enum {
	EFD_SEMAPHORE = 00000001,
	EFD_CLOEXEC = 02000000,
	EFD_NONBLOCK = 00004000
};

typedef uint64_t eventfd_t;

#define eventfd_read	__linux_eventfd_read
#define eventfd_write	__linux_eventfd_write

extern int __linux_eventfd(int count, int flags) __THROW;
extern int __linux_eventfd_read(int __fd, eventfd_t *__value);
extern int __linux_eventfd_write(int fd, eventfd_t value);

// We can't do `#define eventfd` because that would mess with `struct eventfd`
static inline int eventfd(int count, int flags)
{
	return __linux_eventfd(count, flags);
}

#endif // DARLING_EMULATION_LINUX_API_EVENTFD_H
