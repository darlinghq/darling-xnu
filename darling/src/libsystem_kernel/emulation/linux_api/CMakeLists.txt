project(emulation_linux_api)

set(linux_api_sources
	linux_syscall.S

	c_api/clock.c
	c_api/epoll.c
	c_api/eventfd.c
	c_api/fanotify.c
	c_api/file_handle.c
	c_api/futex.c
	c_api/inotify.c
	c_api/ioctl.c
	c_api/mremap.c
	c_api/nanosleep.c
	c_api/sched_yield.c
	c_api/signalfd.c
	c_api/sig_restorer.S
	c_api/timerfd.c
)

add_darling_object_library(emulation_linux_api ${linux_api_sources})
make_fat(emulation_linux_api)
