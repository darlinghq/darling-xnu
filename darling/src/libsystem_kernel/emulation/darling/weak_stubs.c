#include <emulation/common/base.h>

#include <os/lock.h>

WEAK
VISIBLE
int kqueue_close(int kq) { return 0; }

WEAK
VISIBLE
void kqueue_closed_fd(int fd) {}

WEAK
VISIBLE
void kqueue_dup(int oldfd, int newfd) { }

WEAK
void os_unfair_lock_unlock(os_unfair_lock_t lock) {}

WEAK
void os_unfair_lock_lock(os_unfair_lock_t lock) {}
