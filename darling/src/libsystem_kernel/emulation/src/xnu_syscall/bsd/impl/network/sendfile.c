#include <darling/emulation/xnu_syscall/bsd/impl/network/sendfile.h>

#include <limits.h>
#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

struct darwin_iovec {
	void* iov_base;
	size_t iov_len;
};

struct darwin_sf_hdtr {
	struct darwin_iovec* headers;
	int hdr_cnt;
	struct darwin_iovec* trailers;
	int trl_cnt;
};

// Linux limits a single sendfile() to this many bytes. Darwin's off_t is
// always 64-bit, so larger requests are split into multiple host calls.
#define LINUX_SENDFILE_CHUNK 0x7ffff000ULL

#ifdef __NR_sendfile64
#define DARLING_NR_SENDFILE __NR_sendfile64
#else
#define DARLING_NR_SENDFILE __NR_sendfile
#endif

static long file_end(int fd, long long* end)
{
	struct linux_stat st;
	long ret;

#ifdef __NR_fstat64
	ret = LINUX_SYSCALL(__NR_fstat64, fd, &st);
#else
	ret = LINUX_SYSCALL(__NR_fstat, fd, &st);
#endif
	if (ret < 0)
		return ret;
	*end = st.st_size;
	return 0;
}

// writev() may perform a partial write. Darwin's sendfile() reports the
// number of bytes sent so far even when a later write fails, so keep a
// running total and return the host error to the caller.
static long write_iov(int socket, struct darwin_iovec* iov, int count, long long* written)
{
	int i;

	*written = 0;
	if (iov == NULL)
		return count == 0 ? 0 : -EFAULT;
	if (count < 0)
		return -EINVAL;

	for (i = 0; i < count; ++i)
	{
		const char* data = iov[i].iov_base;
		size_t left = iov[i].iov_len;

		if (left != 0 && data == NULL)
			return -EFAULT;

		while (left != 0)
		{
			size_t chunk = left > INT_MAX ? INT_MAX : left;
			long ret = sys_write_nocancel(socket, data, (int) chunk);

			if (ret < 0)
				return ret;
			if (ret == 0)
				return -EIO;
			*written += ret;
			data += ret;
			left -= (size_t) ret;
		}
	}

	return 0;
}

long sys_sendfile(int fd, int socket, long long offset, long long* nbytes,
		struct darwin_sf_hdtr* hdtr, int flags)
{
	long long count, sent = 0, file_sent = 0;
	long long position, end;
	long ret;

	CANCELATION_POINT();

	if (fd < 0 || socket < 0)
		return -EBADF;
	if (nbytes == NULL || offset < 0 || flags != 0)
		return -EINVAL;

	count = *nbytes;
	if (count < 0)
		return -EINVAL;

	if (hdtr != NULL)
	{
		long long header_bytes;

		if (hdtr->hdr_cnt < 0 || hdtr->trl_cnt < 0)
			return -EINVAL;
		ret = write_iov(socket, hdtr->headers, hdtr->hdr_cnt, &header_bytes);
		sent += header_bytes;
		if (ret < 0)
		{
			*nbytes = sent;
			return ret;
		}
	}

	if (count == 0)
	{
		ret = file_end(fd, &end);
		if (ret < 0)
		{
			*nbytes = sent;
			return errno_linux_to_bsd(ret);
		}
		count = end - offset;
		if (count < 0)
			count = 0;
	}

	while (file_sent < count)
	{
		unsigned long long chunk = (unsigned long long) (count - file_sent);
		long transferred;

		if (chunk > LINUX_SENDFILE_CHUNK)
			chunk = LINUX_SENDFILE_CHUNK;

		position = offset + file_sent;
		transferred = LINUX_SYSCALL(DARLING_NR_SENDFILE, socket, fd, &position,
				(size_t) chunk);
		if (transferred < 0)
		{
			*nbytes = sent;
			return errno_linux_to_bsd(transferred);
		}
		if (transferred == 0)
			break; // end of file
		file_sent += transferred;
		sent += transferred;
	}

	if (hdtr != NULL)
	{
		long long trailer_bytes;

		ret = write_iov(socket, hdtr->trailers, hdtr->trl_cnt, &trailer_bytes);
		sent += trailer_bytes;
		if (ret < 0)
		{
			*nbytes = sent;
			return ret;
		}
	}

	*nbytes = sent;
	return 0;
}
