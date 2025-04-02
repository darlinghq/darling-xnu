#include "./sys/inotify.h"
#include "../errno.h"
#include "../base.h"
#include <linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int inotify_init (void)
{
	#if defined(__NR_inotify_init)
		int rv;

		rv = LINUX_SYSCALL(__NR_inotify_init);
		if (rv < 0)
		{
			cerror(errno_linux_to_bsd(-rv));
			return -1;
		}

		return rv;
	#else
		inotify_init1(0);
	#endif
}

