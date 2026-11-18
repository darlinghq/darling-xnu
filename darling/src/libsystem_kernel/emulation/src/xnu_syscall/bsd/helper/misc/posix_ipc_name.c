/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

// Darwin treats the name of a POSIX shared memory object or named semaphore
// as an opaque key: any byte except NUL is allowed, including further slashes
// ("/MTX/1a2b"), and "a", "/a" and "//a" are three different objects. The
// host's shm_open() and sem_open() map names onto single files, so they
// reject a slash after the leading one with EINVAL and treat "a", "/a" and
// "//a" alike. The guest name is therefore encoded into a host name that
// differs for every guest name:
//   - a name with one leading slash keeps it, and the rest is escaped;
//   - a name without a leading slash gets the prefix "/%." instead, which no
//     escaped name can start with;
//   - in the rest, '/' becomes "%2F", '%' becomes "%25" and a leading '.'
//     becomes "%2E" (the host would take "." and ".." for directories);
//   - the name "/" alone becomes "/%-", which no other name produces.
// A plain "/name" is passed through unchanged.

#include <darling/emulation/xnu_syscall/bsd/helper/misc/posix_ipc_name.h>

#include <sys/errno.h>

int posix_ipc_name_to_host(const char* name, size_t max_len, char* host, size_t host_size)
{
	size_t len = 0, out = 0;
	const char* p;

	if (!name)
		return EFAULT;

	while (name[len])
	{
		if (++len > max_len)
			return ENAMETOOLONG;
	}

	// Worst case: "/%." plus three bytes for each of max_len characters.
	if (host_size < 4 + 3 * max_len)
		return ENAMETOOLONG;

	p = name;
	host[out++] = '/';
	if (*p == '/')
	{
		p++;
		if (!*p)
		{
			host[out++] = '%';
			host[out++] = '-';
		}
		else if (*p == '.')
		{
			host[out++] = '%';
			host[out++] = '2';
			host[out++] = 'E';
			p++;
		}
	}
	else
	{
		host[out++] = '%';
		host[out++] = '.';
	}

	for (; *p; p++)
	{
		if (*p == '/' || *p == '%')
		{
			host[out++] = '%';
			host[out++] = '2';
			host[out++] = (*p == '/') ? 'F' : '5';
		}
		else
			host[out++] = *p;
	}
	host[out] = '\0';
	return 0;
}
