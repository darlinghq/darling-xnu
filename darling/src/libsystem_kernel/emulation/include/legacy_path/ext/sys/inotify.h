#ifndef EXT_INOTIFY_H
#define EXT_INOTIFY_H
#include <stdint.h>

#ifndef __THROW
#	define __THROW
#endif

#define inotify_init		__linux_inotify_init
#define inotify_init1		__linux_inotify_init1
#define inotify_add_watch	__linux_inotify_add_watch
#define inotify_rm_watch	__linux_inotify_rm_watch

struct inotify_event
{
	int wd;
	uint32_t mask;
	uint32_t cookie;
	uint32_t len;
	char name[];
};

#define IN_ACCESS        0x00000001
#define IN_MODIFY        0x00000002
#define IN_ATTRIB        0x00000004
#define IN_CLOSE_WRITE   0x00000008
#define IN_CLOSE_NOWRITE 0x00000010
#define IN_CLOSE         (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
#define IN_OPEN          0x00000020
#define IN_MOVED_FROM    0x00000040
#define IN_MOVED_TO      0x00000080
#define IN_MOVE          (IN_MOVED_FROM | IN_MOVED_TO)
#define IN_CREATE        0x00000100
#define IN_DELETE        0x00000200
#define IN_DELETE_SELF   0x00000400
#define IN_MOVE_SELF     0x00000800

#define IN_UNMOUNT       0x00002000
#define IN_Q_OVERFLOW    0x00004000
#define IN_IGNORED       0x00008000

#define IN_CLOSE         (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
#define IN_MOVE          (IN_MOVED_FROM | IN_MOVED_TO)

#define IN_ONLYDIR       0x01000000
#define IN_DONT_FOLLOW   0x02000000
#define IN_EXCL_UNLINK   0x04000000
#define IN_MASK_ADD      0x20000000

#define IN_ISDIR         0x40000000
#define IN_ONESHOT       0x80000000

#define IN_ALL_EVENTS    (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE  \
                          | IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM        \
                          | IN_MOVED_TO | IN_CREATE | IN_DELETE               \
                          | IN_DELETE_SELF | IN_MOVE_SELF)
#define IN_NONBLOCK		00004000

extern int inotify_init (void) __THROW;

extern int inotify_init1 (int __flags) __THROW;

extern int inotify_add_watch (int __fd, const char *__name, uint32_t __mask)
  __THROW;

extern int inotify_rm_watch (int __fd, int __wd) __THROW;

#endif


