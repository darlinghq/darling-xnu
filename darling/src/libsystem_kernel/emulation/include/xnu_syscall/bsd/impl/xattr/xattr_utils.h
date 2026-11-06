#ifndef LINUX_XATTR_UTILS_H
#define LINUX_XATTR_UTILS_H

#include <stddef.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>

#define DARWIN_XATTR_NOFOLLOW   0x0001
#define DARWIN_XATTR_CREATE     0x0002
#define DARWIN_XATTR_REPLACE    0x0004
#define DARWIN_XATTR_MAXNAMELEN 127

#define LINUX_XATTR_CREATE      0x1
#define LINUX_XATTR_REPLACE     0x2
#define LINUX_XATTR_LIST_MAX    65536

/*
 * Darwin extended attribute names have no namespace, while Linux requires
 * one, and only "user." is available to unprivileged processes. Every
 * Darwin name is stored as "user.<name>" on the host, and only "user."
 * attributes are visible to Darwin code (without the prefix). This keeps
 * the mapping one-to-one: whatever is listed can be read back under the
 * same name, and a Darwin name that happens to start with "user." or
 * "security." does not reach another host namespace.
 */
#define XATTR_LINUX_NAMESPACE       "user."
#define XATTR_LINUX_NAMESPACE_LEN   (sizeof(XATTR_LINUX_NAMESPACE) - 1)
#define XATTR_LINUX_NAME_BUFSIZE    (XATTR_LINUX_NAMESPACE_LEN + DARWIN_XATTR_MAXNAMELEN + 1)

static inline int errno_linux_xattr_to_bsd(int ret)
{
	// a missing attribute is ENOATTR on Darwin, ENODATA on Linux
	if (ret == -LINUX_ENODATA)
		return -ENOATTR;
	return errno_linux_to_bsd(ret);
}

// Writes the host name for the Darwin attribute name `name` into `buf`,
// which must hold XATTR_LINUX_NAME_BUFSIZE bytes. Returns 0 or a negative
// BSD errno.
static inline int xattr_name_to_linux(const char* name, char* buf)
{
	unsigned long i;

	if (name == NULL)
		return -EFAULT;

	for (i = 0; i < XATTR_LINUX_NAMESPACE_LEN; i++)
		buf[i] = XATTR_LINUX_NAMESPACE[i];

	for (i = 0; name[i] != '\0'; i++)
	{
		if (i >= DARWIN_XATTR_MAXNAMELEN)
			return -ENAMETOOLONG;
		buf[XATTR_LINUX_NAMESPACE_LEN + i] = name[i];
	}
	buf[XATTR_LINUX_NAMESPACE_LEN + i] = '\0';

	return 0;
}

static inline int xattr_options_to_linux_flags(int options)
{
	int flags = 0;
	if (options & DARWIN_XATTR_CREATE)
		flags |= LINUX_XATTR_CREATE;
	if (options & DARWIN_XATTR_REPLACE)
		flags |= LINUX_XATTR_REPLACE;
	return flags;
}

static inline int xattr_is_darwin_visible(const char* name, unsigned long len)
{
	unsigned long i;

	if (len <= XATTR_LINUX_NAMESPACE_LEN)
		return 0;
	for (i = 0; i < XATTR_LINUX_NAMESPACE_LEN; i++)
	{
		if (name[i] != XATTR_LINUX_NAMESPACE[i])
			return 0;
	}
	return 1;
}

// Runs the Linux list syscall `call_nr` (listxattr, llistxattr or
// flistxattr) on `target` (a path or a file descriptor) and returns the
// Darwin view of the list in `namebuf`, as listxattr(2) describes it: the
// size of the list when `namebuf` is NULL, ERANGE when `size` is too
// small. Returns a negative BSD errno on failure.
//
// The host list includes the namespace prefixes and attributes from other
// namespaces, so its size says nothing about the size of the Darwin list:
// the whole host list is always read and filtered. It is read into a stack
// buffer or, if it does not fit, into a buffer of the largest size Linux
// returns (XATTR_LIST_MAX). There is no separate size query that could go
// stale before the read, so nothing needs to be retried.
static inline long xattr_list_to_darwin(int call_nr, long target, char* namebuf, unsigned long size)
{
	char stack_buf[2048];
	char* l_buf = stack_buf;
	unsigned long l_bufsize = sizeof(stack_buf);
	unsigned long pos, total = 0;
	long ret;

	ret = LINUX_SYSCALL(call_nr, target, l_buf, l_bufsize);
	if (ret == -LINUX_ERANGE)
	{
		// not malloc: libsystem_kernel sits below libc
		l_bufsize = LINUX_XATTR_LIST_MAX;
		l_buf = (char*) sys_mmap(NULL, l_bufsize, PROT_READ | PROT_WRITE,
				MAP_ANON | MAP_PRIVATE, -1, 0);
		if ((unsigned long) l_buf > (unsigned long) -4096)
			return (long) l_buf;

		ret = LINUX_SYSCALL(call_nr, target, l_buf, l_bufsize);
	}

	if (ret < 0)
	{
		ret = errno_linux_xattr_to_bsd(ret);
		goto out;
	}

	for (pos = 0; pos < (unsigned long) ret; )
	{
		const char* name = l_buf + pos;
		unsigned long len = 0;

		while (pos + len < (unsigned long) ret && name[len] != '\0')
			len++;
		pos += len + 1;

		if (!xattr_is_darwin_visible(name, len))
			continue;

		name += XATTR_LINUX_NAMESPACE_LEN;
		len -= XATTR_LINUX_NAMESPACE_LEN;

		if (namebuf != NULL && total + len + 1 <= size)
		{
			__builtin_memcpy(namebuf + total, name, len);
			namebuf[total + len] = '\0';
		}
		total += len + 1;
	}

	if (namebuf != NULL && total > size)
		ret = -ERANGE;
	else
		ret = total;

out:
	if (l_buf != stack_buf)
		sys_munmap(l_buf, l_bufsize);
	return ret;
}

#endif // LINUX_XATTR_UTILS_H
