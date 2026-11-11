#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_kern.h>

#include <sys/errno.h>
#include <sys/syslimits.h>

#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_proc.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrlimit.h>
#include <darling/emulation/linux_premigration/ext/sysinfo.h>
#include <darling/emulation/linux_premigration/ext/syslog.h>
#include <darling/emulation/linux_premigration/ext/sys/utsname.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/linux_premigration/elfcalls_wrapper.h>

#include <darling/emulation/xnu_syscall/bsd/helper/misc/readline.h>
#include <darling/emulation/common/string.h>
#include "proc_boottime.h"

#include <darling-config.h>

enum {
	_KERN_MSGBUF = 1000,
	_KERN_OSPRODUCTVERSION,
	_KERN_NUM_FILES,

	_KERN_SEMMNS = 1000,
};

static sysctl_handler(handle_msgbuf);
static sysctl_handler(handle_proc);
static sysctl_handler(handle_procargs32);
static sysctl_handler(handle_argmax);
static sysctl_handler(handle_boottime);
static sysctl_handler(handle_ostype);
static sysctl_handler(handle_hostname);
static sysctl_handler(handle_domainname);
static sysctl_handler(handle_osrelease);
static sysctl_handler(handle_version);
static sysctl_handler(handle_osversion);
static sysctl_handler(handle_maxproc);
static sysctl_handler(handle_maxfiles);
static sysctl_handler(handle_maxfilesperproc);
static sysctl_handler(handle_num_files);
static sysctl_handler(handle_netboot);
static sysctl_handler(handle_safeboot);
static sysctl_handler(handle_usrstack32);
static sysctl_handler(handle_usrstack64);
static sysctl_handler(handle_sysv_semmns);
static sysctl_handler(handle_osproductversion);
static sysctl_handler(handle_osrevision);

extern int _sysctl_proc(int what, int flag, struct kinfo_proc* out, unsigned long* buflen);
extern int _sysctl_procargs(int pid, char* buf, unsigned long* buflen);
extern int strlcpy(char* dst, const char* src, __SIZE_TYPE__ dst_size);
extern __SIZE_TYPE__ strlen(const char* str);

const struct known_sysctl sysctls_kern_sysv[] = {
	{ .oid = _KERN_SEMMNS, .type = CTLTYPE_INT, .exttype = "I", .name = "semmns", .handler = handle_sysv_semmns },
	{ .oid = -1 }
};

const struct known_sysctl sysctls_kern[] = {
	{ .oid = _KERN_MSGBUF, .type = CTLTYPE_STRING, .exttype = "S", .name = "msgbuf", .handler = handle_msgbuf },
	{ .oid = KERN_PROC, .type = CTLTYPE_STRUCT, .exttype = "", .name = "proc", .handler = handle_proc },
	{ .oid = KERN_PROCARGS2, .type = CTLTYPE_STRUCT, .exttype = "", .name = "procargs2", .handler = handle_procargs32 },
	{ .oid = KERN_ARGMAX, .type = CTLTYPE_INT, .exttype = "I", .name = "argmax", .handler = handle_argmax },
	{ .oid = KERN_MAXPROC, .type = CTLTYPE_INT, .exttype = "I", .name = "maxproc", .handler = handle_maxproc },
	{ .oid = KERN_MAXFILES, .type = CTLTYPE_INT, .exttype = "I", .name = "maxfiles", .handler = handle_maxfiles },
	{ .oid = KERN_MAXFILESPERPROC, .type = CTLTYPE_INT, .exttype = "I", .name = "maxfilesperproc", .handler = handle_maxfilesperproc },
	{ .oid = _KERN_NUM_FILES, .type = CTLTYPE_INT, .exttype = "I", .name = "num_files", .handler = handle_num_files },
	{ .oid = KERN_NETBOOT, .type = CTLTYPE_INT, .exttype = "I", .name = "netboot", .handler = handle_netboot },
	{ .oid = KERN_SAFEBOOT, .type = CTLTYPE_INT, .exttype = "I", .name = "safeboot", .handler = handle_safeboot },
	{ .oid = KERN_BOOTTIME, .type = CTLTYPE_STRUCT, .exttype = "S,timeval", .name = "boottime", .handler = handle_boottime },
	{ .oid = KERN_OSTYPE, .type = CTLTYPE_STRING, .exttype = "S", .name = "ostype", .handler = handle_ostype },
	{ .oid = KERN_HOSTNAME, .type = CTLTYPE_STRING, .exttype = "S", .name = "hostname", .handler = handle_hostname },
	{ .oid = KERN_DOMAINNAME, .type = CTLTYPE_STRING, .exttype = "S", .name = "domainname", .handler = handle_domainname },
	{ .oid = KERN_OSRELEASE, .type = CTLTYPE_STRING, .exttype = "S", .name = "osrelease", .handler = handle_osrelease },
	{ .oid = KERN_VERSION, .type = CTLTYPE_STRING, .exttype = "S", .name = "version", .handler = handle_version },
	{ .oid = KERN_OSVERSION, .type = CTLTYPE_STRING, .exttype = "S", .name = "osversion", .handler = handle_osversion },
	{ .oid = KERN_USRSTACK32, .type = CTLTYPE_INT, .exttype = "I", .name = "usrstack", .handler = handle_usrstack32 },
	{ .oid = KERN_USRSTACK64, .type = CTLTYPE_QUAD, .exttype = "Q", .name = "usrstack64", .handler = handle_usrstack64 },
	{ .oid = _KERN_OSPRODUCTVERSION, .type = CTLTYPE_STRING, .exttype = "S", .name = "osproductversion", .handler = handle_osproductversion },
	{ .oid = KERN_OSREV, .type = CTLTYPE_INT, .exttype = "I", .name = "osrevision", .handler = handle_osrevision },
	{ .oid = KERN_SYSV, .type = CTLTYPE_NODE, .exttype = "", .name = "sysv", .subctls = sysctls_kern_sysv },
	{ .oid = -1 }
};

sysctl_handler(handle_msgbuf)
{
	if (*oldlen <= 8)
	{
		// Caller is asking for buffer size
		if (*oldlen < sizeof(int))
			return -EINVAL;

		*((int*) old) = __linux_syslog(SYSLOG_ACTION_SIZE_BUFFER, NULL, 0);
	}
	else
	{
		// Caller is asking for buffer contents
		int ret;

		ret = __linux_syslog(SYSLOG_ACTION_READ_ALL, (char*) old, *oldlen);
		if (ret < 0)
			return errno_linux_to_bsd(ret);

		*oldlen = ret;
	}
	return 0;
}

sysctl_handler(handle_proc)
{
	// KERN_PROC_ALL takes no argument, so its name has three components.
	if (nlen == 3 && name[2] == KERN_PROC_ALL)
		return _sysctl_proc(KERN_PROC_ALL, 0, (struct kinfo_proc*) old, oldlen);
	if (nlen < 4)
		return -ENOENT;
	return _sysctl_proc(name[2], name[3], (struct kinfo_proc*) old, oldlen);
}

sysctl_handler(handle_procargs32)
{
	if (nlen < 3)
		return -ENOENT;
	return _sysctl_procargs(name[2], (char*) old, oldlen);
}

sysctl_handler(handle_argmax)
{
	struct xnu_rlimit lim;
	unsigned long long argmax;
	int r;
	int* ovalue = (int*) old;

	r = sys_getrlimit(BSD_RLIMIT_STACK, &lim);
	if (r < 0)
		return r;

	// macOS reports the fixed ARG_MAX whatever the stack limit is. Linux
	// lets exec() take a quarter of the stack limit, so report that when it
	// is lower; an unlimited stack must not overflow the int.
	argmax = lim.rlim_cur / 4;
	if (argmax > ARG_MAX)
		argmax = ARG_MAX;

	if (ovalue != NULL)
		*ovalue = (int) argmax;
	*oldlen = sizeof(int);
	return 0;
}

// Linux clock IDs (Darwin's <time.h> numbers its clocks differently).
#define LINUX_CLOCK_REALTIME 0
#define LINUX_CLOCK_BOOTTIME 7

// Last boot time read from /proc/stat, in seconds; 0 until the first read.
static unsigned long boottime_cache;

static int read_proc_boottime(unsigned long* seconds)
{
	int fd = sys_open_nocancel("/proc/stat", BSD_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	struct rdline_buffer buffer;
	_readline_init(&buffer);
	const char* line;
	*seconds = 0;
	while ((line = _readline(fd, &buffer)) != NULL)
	{
		if (strncmp(line, "btime ", 6) == 0)
		{
			if (!parse_proc_boottime(line + 6, seconds))
				*seconds = 0;
			break;
		}
	}
	close_internal(fd);
	return *seconds ? 0 : -EIO;
}

// Linux derives btime from the offset between CLOCK_REALTIME and
// CLOCK_BOOTTIME, which only moves when the wall clock is set. Libc reads
// kern.boottime twice for every CLOCK_MONOTONIC query, so keep the last
// btime and only go back to /proc/stat when the current offset has left
// the second that btime describes.
static bool boottime_cache_valid(unsigned long seconds)
{
	struct timespec boot, real;

	if (seconds == 0)
		return false;
	if (LINUX_SYSCALL(__NR_clock_gettime, LINUX_CLOCK_BOOTTIME, &boot) < 0)
		return false;
	if (LINUX_SYSCALL(__NR_clock_gettime, LINUX_CLOCK_REALTIME, &real) < 0)
		return false;

	long long offset_ns = (long long)(real.tv_sec - boot.tv_sec) * 1000000000LL
		+ (real.tv_nsec - boot.tv_nsec);
	long long cached_ns = (long long)seconds * 1000000000LL;

	return offset_ns >= cached_ns && offset_ns < cached_ns + 1000000000LL;
}

sysctl_handler(handle_boottime)
{
	if (_new != NULL || newlen != 0)
		return -EPERM;
	if (old == NULL)
	{
		*oldlen = sizeof(struct bsd_timeval);
		return 0;
	}
	if (*oldlen < sizeof(struct bsd_timeval))
		return -EINVAL;

	unsigned long seconds = __atomic_load_n(&boottime_cache, __ATOMIC_RELAXED);
	if (!boottime_cache_valid(seconds))
	{
		int err = read_proc_boottime(&seconds);
		if (err < 0)
			return err;
		__atomic_store_n(&boottime_cache, seconds, __ATOMIC_RELAXED);
	}

	struct bsd_timeval* value = old;
	value->tv_sec = seconds;
	value->tv_usec = 0;
	*oldlen = sizeof(*value);
	return 0;
}

sysctl_handler(handle_ostype)
{
	const char* s = EMULATED_SYSNAME;

	return copyout_string(s, (char*) old, oldlen);
}

sysctl_handler(handle_osrelease)
{
	const char* s = EMULATED_RELEASE;

	return copyout_string(s, (char*) old, oldlen);
}

sysctl_handler(handle_version)
{
	const char* s = EMULATED_VERSION;

	return copyout_string(s, (char*) old, oldlen);
}

sysctl_handler(handle_osversion)
{
	const char* s = EMULATED_OSVERSION;

	return copyout_string(s, (char*) old, oldlen);
}


static struct linux_utsname lu;
struct linux_utsname* need_uname(void)
{
#ifndef VARIANT_DYLD
	// Cache __linux_uname results
	if (!lu.sysname[0])
	{
		__linux_uname(&lu);
	}
#endif
	return &lu;
}

sysctl_handler(handle_hostname)
{
	int rv = 0;
	
	need_uname();
	copyout_string_truncated(lu.nodename, (char*) old, oldlen);
	
	if (_new && newlen > 0)
	{
		rv = LINUX_SYSCALL(__NR_sethostname, _new, newlen);
		if (rv < 0)
			rv = errno_linux_to_bsd(rv);
	}
	
	return rv;
}

sysctl_handler(handle_domainname)
{
	int rv = 0;
	
	need_uname();
	copyout_string_truncated(lu.domainname, (char*) old, oldlen);
	
	if (_new && newlen > 0)
	{
		rv = LINUX_SYSCALL(__NR_setdomainname, _new, newlen);
		if (rv < 0)
			rv = errno_linux_to_bsd(rv);
	}
	
	return rv;
}

sysctl_handler(handle_maxproc)
{
	char buf[16];
	int fd = sys_open_nocancel("/proc/sys/kernel/pid_max", BSD_O_RDONLY, 0);
	int value = 1024;
	int* ovalue = (int*) old;
	
	if (!oldlen)
		return -EINVAL;
	
	if (fd >= 0)
	{
		int rd = sys_read_nocancel(fd, buf, sizeof(buf) - 1);
		close_internal(fd);
		
		if (rd > 0)
		{
			buf[rd] = '\0';
			value = __simple_atoi(buf, NULL);
		}
	}
	
	if (ovalue != NULL)
		*ovalue = value;
	*oldlen = sizeof(int);
	
	return 0;
}

sysctl_handler(handle_netboot)
{
	int* ovalue = (int*) old;
	if (!oldlen)
		return -EINVAL;

	if (*oldlen >= sizeof(int))
		*ovalue = 0;
	*oldlen = sizeof(int);

	return 0;
}

sysctl_handler(handle_safeboot)
{
	int* ovalue = (int*) old;
	if (!oldlen)
		return -EINVAL;

	if (*oldlen >= sizeof(int))
		*ovalue = 0;
	*oldlen = sizeof(int);

	return 0;
}

sysctl_handler(handle_sysv_semmns)
{
	int* ovalue = (int*) old;
	if (!oldlen)
		return -EINVAL;
	if (*oldlen >= sizeof(int))
		*ovalue = -1;
	*oldlen = sizeof(int);
	return 0;
}

sysctl_handler(handle_usrstack32)
{
	unsigned int* ovalue = (unsigned int*) old;
	if (ovalue)
		*ovalue = __darling_thread_get_stack();
	*oldlen = sizeof(*ovalue);
	return 0;
}

sysctl_handler(handle_usrstack64)
{
	unsigned long long* ovalue = (unsigned long long*) old;
	if (ovalue)
		*ovalue = __darling_thread_get_stack();
	*oldlen = sizeof(*ovalue);
	return 0;
}

static sysctl_handler(handle_osproductversion) {
	return copyout_string(EMULATED_OSPRODUCTVERSION, (char*)old, oldlen);
};

// Apple no longer changes this value
// https://lists.apple.com/archives/darwin-dev/2005/Jul/msg00029.html
#define EMULATED_OSREVISION 199506

static sysctl_handler(handle_osrevision) {
	int* ovalue = (int*)old;
	if (ovalue)
		*ovalue = EMULATED_OSREVISION;
	*oldlen = sizeof(*ovalue);
	return 0;
}

// Linux's fs.file-max does not fit in an int and is no useful limit for
// a guest; report macOS's value for a machine with little memory.
static sysctl_handler(handle_maxfiles) {
	sysctl_handle_size(sizeof(int));
	*((int*) old) = 49152;
	return 0;
}

// The most a guest's RLIMIT_NOFILE can be raised to (see nofile_limit.c).
static sysctl_handler(handle_maxfilesperproc) {
	sysctl_handle_size(sizeof(int));
	*((int*) old) = DARWIN_OPEN_MAX;
	return 0;
}

// Open files in the system: the first field of /proc/sys/fs/file-nr.
static sysctl_handler(handle_num_files) {
	char buf[64];

	sysctl_handle_size(sizeof(int));
	if (!read_string("/proc/sys/fs/file-nr", buf, sizeof(buf)))
		return -ENOENT;
	*((int*) old) = (int) __simple_atoi(buf, NULL);
	return 0;
}
