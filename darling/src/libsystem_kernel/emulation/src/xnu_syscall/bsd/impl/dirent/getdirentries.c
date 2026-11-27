#include <darling/emulation/xnu_syscall/bsd/impl/dirent/getdirentries.h>

#include <sys/dirent.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/dirent/getdirentries.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define LINUX_SEEK_SET 0
#define LINUX_SEEK_CUR 1

extern __SIZE_TYPE__ strlen(const char* s);
extern char* strcpy(char* dest, const char* src);
extern int strcmp(const char* str1, const char* str2);

#ifndef min
#	define min(a,b) ((a) < (b)) ? (a) : (b)
#endif

#define ALIGN(x, alignment) (((x) + (alignment - 1)) & ~(alignment - 1))

static inline void round_to_4(unsigned short* reclen)
{
	if (*reclen & 3)
	{
		*reclen += 3;
		*reclen /= 4;
		*reclen *= 4;
	}
}

// Both calls read Linux records into a host buffer, convert them and stop at
// the first converted record that does not fit in the caller's buffer. A
// getdirentries64 record (ALIGN(22 + namelen, 8)) can be 8 bytes longer than
// its Linux record (ALIGN(20 + namelen, 8)), and the host read must be able to
// return a longest Linux record (255-byte name) even when the caller's buffer
// is smaller, so the host read can return records that do not fit. The
// directory offset is then put back on the first record not returned, so the
// next call returns it and no entry is ever dropped.
#define LINUX_DIRENT64_LARGEST_RECORD 280
#define GETDIRENTRIES_HOST_BUF_MAX 2048

// The largest converted record of either call: a getdirentries64 record with
// a 255-byte name. Smaller caller buffers may not fit the first record.
#define BSD_DIRENT_LARGEST_RECORD 280

static unsigned int getdirentries_host_len(unsigned int len)
{
	if (len < LINUX_DIRENT64_LARGEST_RECORD)
		return LINUX_DIRENT64_LARGEST_RECORD;
	if (len > GETDIRENTRIES_HOST_BUF_MAX)
		return GETDIRENTRIES_HOST_BUF_MAX;
	return len;
}

// Called when records were read but not returned. resume is the d_off of the
// last returned record, or the offset before the read (start) when none was.
// Returns opos, or an error when no record could be returned.
static long getdirentries_unread(int fd, int opos, long long resume, long long start)
{
	long ret;

	if (opos == 0)
	{
		// The first record does not fit in the caller's buffer. start is
		// always known here: it is read whenever len is small enough for
		// that to happen.
		if (start < 0)
			return errno_linux_to_bsd(-LINUX_EINVAL);
		resume = start;
	}

	ret = LINUX_SYSCALL(__NR_lseek, fd, resume, LINUX_SEEK_SET);
	if (opos == 0)
		return errno_linux_to_bsd(ret < 0 ? ret : -LINUX_EINVAL);
	return opos;
}

long sys_getdirentries(int fd, char* ibuf, unsigned int len, long* basep)
{
	int ret, bpos = 0, opos = 0;
	long long start = -1, resume = 0;

	// bounded, so this doesn't blow up the stack
	char buf[GETDIRENTRIES_HOST_BUF_MAX];

	if (basep)
		*basep = 0;

	// The offset before the read, needed only when the caller's buffer may be
	// too small for the first record.
	if (len < BSD_DIRENT_LARGEST_RECORD)
	{
		start = LINUX_SYSCALL(__NR_lseek, fd, 0, LINUX_SEEK_CUR);
		if (start < 0)
			return errno_linux_to_bsd(start);
	}

	ret = LINUX_SYSCALL(__NR_getdents64, fd, buf, getdirentries_host_len(len));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	while (bpos < ret && opos < len)
	{
		struct linux_dirent64* l64;
		struct bsd_dirent* bsd;
		int slen;

		l64 = (struct linux_dirent64*) (buf + bpos);
		bsd = (struct bsd_dirent*) (ibuf + opos);
		slen = strlen(l64->d_name);

		if (len-opos < ALIGN(sizeof(struct bsd_dirent) + slen + 1, 4))
			break;

		bsd->d_ino = l64->d_ino;
		bsd->d_type = l64->d_type;
		strcpy(bsd->d_name, l64->d_name);

		bsd->d_reclen = sizeof(struct bsd_dirent) + slen + 1;
		bsd->d_namlen = slen;

		round_to_4(&bsd->d_reclen);

		opos += bsd->d_reclen;
		bpos += l64->d_reclen;
		resume = l64->d_off;
	}

	if (bpos < ret)
		return getdirentries_unread(fd, opos, resume, start);

	return opos;
}

struct dirent64 __DARWIN_STRUCT_DIRENTRY;

long sys_getdirentries64(int fd, char* ibuf, unsigned int len, long* basep)
{
	int ret, bpos = 0, opos = 0;
	long long start = -1, resume = 0;

	// bounded, so this doesn't blow up the stack
	char buf[GETDIRENTRIES_HOST_BUF_MAX];

	if (basep)
		*basep = 0;

	// The offset before the read, needed only when the caller's buffer may be
	// too small for the first record.
	if (len < BSD_DIRENT_LARGEST_RECORD)
	{
		start = LINUX_SYSCALL(__NR_lseek, fd, 0, LINUX_SEEK_CUR);
		if (start < 0)
			return errno_linux_to_bsd(start);
	}

	ret = LINUX_SYSCALL(__NR_getdents64, fd, buf, getdirentries_host_len(len));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	while (bpos < ret && opos < len)
	{
		struct linux_dirent64* l64;
		struct bsd_dirent64* bsd;
		int slen;
		unsigned short reclen;

		l64 = (struct linux_dirent64*) (buf + bpos);
		bsd = (struct bsd_dirent64*) (ibuf + opos);
		slen = strlen(l64->d_name);

		reclen = ALIGN(__builtin_offsetof(struct bsd_dirent64, d_name) + slen + 1, 8);
		if (len - opos < reclen)
			break;

		bsd->d_ino = l64->d_ino;
		bsd->d_type = l64->d_type;
		bsd->d_namlen = slen;
		bsd->d_seekoff = 0;
		bsd->d_reclen = reclen;
		strcpy(bsd->d_name, l64->d_name);

		opos += bsd->d_reclen;
		bpos += l64->d_reclen;
		resume = l64->d_off;
	}

	if (bpos < ret)
		return getdirentries_unread(fd, opos, resume, start);

	return opos;
}
