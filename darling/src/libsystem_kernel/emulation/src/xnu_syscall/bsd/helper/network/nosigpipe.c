// SO_NOSIGPIPE bookkeeping; see nosigpipe.h.
//
// The option lives on the open socket on Darwin, so descriptors made by
// dup(2) share it and a fork()ed child inherits it. A bitmap indexed by
// descriptor gives the same behavior inside one process: dup copies the bit,
// fork copies the memory. Lookups are one load, which is what write(2) pays
// on every call.
//
// Not carried over: a change made through one descriptor after the dup or fork
// is not seen through the other copies, and the option is forgotten by execve.

#include <darling/emulation/xnu_syscall/bsd/helper/network/nosigpipe.h>

#include <sys/errno.h>

#define NOSIGPIPE_MAX_FDS	65536
#define NOSIGPIPE_WORD_BITS	(8 * sizeof(unsigned long))

static unsigned long nosigpipe_bits[NOSIGPIPE_MAX_FDS / NOSIGPIPE_WORD_BITS];

#define WORD(fd)	nosigpipe_bits[(unsigned int)(fd) / NOSIGPIPE_WORD_BITS]
#define BIT(fd)		(1UL << ((unsigned int)(fd) % NOSIGPIPE_WORD_BITS))

int nosigpipe_get(int fd)
{
	if (fd < 0 || fd >= NOSIGPIPE_MAX_FDS)
		return 0;
	return (__atomic_load_n(&WORD(fd), __ATOMIC_RELAXED) & BIT(fd)) != 0;
}

long nosigpipe_set(int fd, int on)
{
	if (fd < 0)
		return -EBADF;
	if (fd >= NOSIGPIPE_MAX_FDS)
	{
		// Clearing an option that was never recorded is a no-op.
		return on ? -EMFILE : 0;
	}

	if (on)
		__atomic_fetch_or(&WORD(fd), BIT(fd), __ATOMIC_RELAXED);
	else
		__atomic_fetch_and(&WORD(fd), ~BIT(fd), __ATOMIC_RELAXED);
	return 0;
}

void nosigpipe_close(int fd)
{
	if (nosigpipe_get(fd))
		nosigpipe_set(fd, 0);
}

void nosigpipe_dup(int oldfd, int newfd)
{
	if (oldfd == newfd)
		return;
	nosigpipe_set(newfd, nosigpipe_get(oldfd));
}
