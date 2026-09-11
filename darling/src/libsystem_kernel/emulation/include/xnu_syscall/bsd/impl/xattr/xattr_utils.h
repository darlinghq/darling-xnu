#ifndef LINUX_XATTR_UTILS_H
#define LINUX_XATTR_UTILS_H

#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>

#ifndef ENOATTR
#define ENOATTR 93
#endif

#ifndef ERANGE
#define ERANGE 34
#endif

#ifndef E2BIG
#define E2BIG 7
#endif

#ifndef ENOMEM
#define ENOMEM 12
#endif

#define LINUX_ENODATA 61
#define LINUX_ERANGE  34

extern void* malloc(__SIZE_TYPE__ size);
extern void free(void* ptr);

static inline int errno_linux_xattr_to_bsd(int ret)
{
	if (ret == -LINUX_ENODATA)
		return -ENOATTR;
	return errno_linux_to_bsd(ret);
}

#define DARWIN_XATTR_NOFOLLOW   0x0001
#define DARWIN_XATTR_CREATE     0x0002
#define DARWIN_XATTR_REPLACE    0x0004

#ifndef XATTR_NOFOLLOW
#define XATTR_NOFOLLOW          DARWIN_XATTR_NOFOLLOW
#endif
#ifndef XATTR_CREATE
#define XATTR_CREATE            DARWIN_XATTR_CREATE
#endif
#ifndef XATTR_REPLACE
#define XATTR_REPLACE           DARWIN_XATTR_REPLACE
#endif

#define LINUX_XATTR_CREATE      0x1
#define LINUX_XATTR_REPLACE     0x2

#define XATTR_NAME_MAX_LEN      512

static inline int xattr_str_has_prefix(const char* str, const char* prefix)
{
	while (*prefix)
	{
		if (*str++ != *prefix++)
			return 0;
	}
	return 1;
}

static inline const char* xattr_name_to_linux(const char* name, char* buf, unsigned long bufsize)
{
	if (!name)
		return NULL;
	if (xattr_str_has_prefix(name, "user.") ||
	    xattr_str_has_prefix(name, "system.") ||
	    xattr_str_has_prefix(name, "security.") ||
	    xattr_str_has_prefix(name, "trusted."))
	{
		return name;
	}
	__simple_snprintf(buf, bufsize, "user.%s", name);
	return buf;
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

static inline long translate_linux_xattr_list(const char* l_buf, long l_size, char* namebuf, unsigned long size)
{
	unsigned long total = 0;
	const char* cur = l_buf;
	const char* end = l_buf + l_size;

	while (cur < end && *cur != '\0')
	{
		unsigned long len = __simple_strlen(cur);
		const char* out_name = cur;
		unsigned long out_len = len;

		if (xattr_str_has_prefix(cur, "user."))
		{
			out_name = cur + 5;
			out_len = len - 5;
		}
		else if (xattr_str_has_prefix(cur, "security.") ||
		         xattr_str_has_prefix(cur, "system.") ||
		         xattr_str_has_prefix(cur, "trusted."))
		{
			// Skip internal Linux xattr namespaces
			cur += len + 1;
			continue;
		}

		if (namebuf != NULL && size > 0)
		{
			if (total + out_len + 1 > size)
				return -ERANGE;
			__builtin_memcpy(namebuf + total, out_name, out_len + 1);
		}
		total += out_len + 1;
		cur += len + 1;
	}

	return (long)total;
}

static inline long fetch_linux_xattr_list(int call_nr, const char* path, int fd, char** out_buf, char* stack_buf, unsigned long stack_buf_size)
{
	long l_size;
	unsigned long buf_size = stack_buf_size;
	char* buf = stack_buf;
	int retries = 0;

	if (path)
		l_size = LINUX_SYSCALL(call_nr, path, NULL, 0);
	else
		l_size = LINUX_SYSCALL(call_nr, fd, NULL, 0);

	if (l_size < 0)
		return errno_linux_xattr_to_bsd(l_size);
	if (l_size == 0)
	{
		*out_buf = NULL;
		return 0;
	}

	if ((unsigned long)l_size > buf_size)
	{
		buf_size = (unsigned long)l_size;
		buf = (char*)malloc(buf_size);
		if (!buf)
			return -ENOMEM;
	}

	while (retries++ < 5)
	{
		if (path)
			l_size = LINUX_SYSCALL(call_nr, path, buf, buf_size);
		else
			l_size = LINUX_SYSCALL(call_nr, fd, buf, buf_size);

		if (l_size >= 0)
		{
			*out_buf = buf;
			return l_size;
		}

		if (l_size == -LINUX_ERANGE)
		{
			if (buf_size >= 65536)
			{
				if (buf != stack_buf)
					free(buf);
				return -E2BIG;
			}
			buf_size *= 2;
			if (buf_size > 65536)
				buf_size = 65536;

			char* new_buf = (char*)malloc(buf_size);
			if (!new_buf)
			{
				if (buf != stack_buf)
					free(buf);
				return -ENOMEM;
			}
			if (buf != stack_buf)
				free(buf);
			buf = new_buf;
			continue;
		}

		if (buf != stack_buf)
			free(buf);
		return errno_linux_xattr_to_bsd(l_size);
	}

	if (buf != stack_buf)
		free(buf);
	return -ERANGE;
}

#endif // LINUX_XATTR_UTILS_H
