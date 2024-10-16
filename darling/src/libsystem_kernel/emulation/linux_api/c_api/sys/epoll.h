#ifndef DARLING_EMULATION_LINUX_API_EPOLL_H
#define DARLING_EMULATION_LINUX_API_EPOLL_H

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
} __attribute__((packed));

#define epoll_create	__linux_epoll_create
#define epoll_create1	__linux_epoll_create1
#define epoll_ctl		__linux_epoll_ctl
#define epoll_wait		__linux_epoll_wait
#define epoll_pwait		__linux_epoll_pwait

extern int __linux_epoll_create(int size) __THROW;
extern int __linux_epoll_create1(int flags);
extern int __linux_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) __THROW;
extern int __linux_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif // DARLING_EMULATION_LINUX_API_EPOLL_H
