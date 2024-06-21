#ifndef LINUX_EPOLL_H
#define LINUX_EPOLL_H
#include <stdint.h>

#ifndef __THROW
#	define __THROW
#endif

enum EPOLL_EVENTS
{
	EPOLLIN = 0x001,
	EPOLLPRI = 0x002,
	EPOLLOUT = 0x004,
	EPOLLRDNORM = 0x040,
	EPOLLRDBAND = 0x080,
	EPOLLWRNORM = 0x100,
	EPOLLWRBAND = 0x200,
	EPOLLMSG = 0x400,
	EPOLLERR = 0x008,
	EPOLLHUP = 0x010,
	EPOLLRDHUP = 0x2000,
	EPOLLWAKEUP = 1u << 29,
	EPOLLONESHOT = 1u << 30,
	EPOLLET = 1u << 31
};

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

#if defined(__x86_64__) || defined(__i386__)
#define __EPOLL_PACKED __attribute__ ((__packed__))
#else
#error "Missing __EPOLL_PACKED definition for arch"
#endif

typedef union epoll_data
{
	void *ptr;
	int fd;
	uint32_t u32;
	uint64_t u64;
} epoll_data_t;

struct epoll_event
{
	uint32_t events;
	epoll_data_t data;
} __EPOLL_PACKED;

extern int epoll_create (int __size) __THROW;

extern int epoll_create1 (int __flags);

extern int epoll_ctl (int __epfd, int __op, int __fd,
						struct epoll_event *__event) __THROW;

extern int epoll_wait (int __epfd, struct epoll_event *__events,
						int __maxevents, int __timeout);


#endif

