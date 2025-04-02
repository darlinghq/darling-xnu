#include "sysctl_kern.h"
#include "sysctl_proc.h"
#include "getrlimit.h"
#include "../ext/sysinfo.h"
#include "../ext/syslog.h"
#include "../ext/sys/utsname.h"
#include <linux-syscalls/linux.h>
#include "../base.h"
#include "../time/gettimeofday.h"
#include "darling-config.h"
#include "../errno.h"
#include "../simple.h"
#include "../fcntl/open.h"
#include "../unistd/read.h"
#include "../unistd/close.h"
#include <sys/errno.h>
#include "../elfcalls_wrapper.h"

enum {
	_KERN_MSGBUF = 1000,
	_KERN_OSPRODUCTVERSION,

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
	struct rlimit lim;
	int r;
	int* ovalue = (int*) old;

	r = sys_getrlimit(BSD_RLIMIT_STACK, &lim);
	if (r < 0)
		return r;

	if (ovalue != NULL)
		*ovalue = lim.rlim_cur / 4;
	*oldlen = sizeof(int);
	return 0;
}

sysctl_handler(handle_boottime)
{
	// Fill in struct timeval { tv_sec, tv_usec }
	struct sysinfo info;
	struct bsd_timeval* tv;

	if (__linux_sysinfo(&info) == -1)
		return -errno;

	tv = (struct bsd_timeval*) old;
	if (tv != NULL)
	{
		if (*oldlen < sizeof(*tv))
			return -EINVAL;
		sys_gettimeofday(tv, NULL);

		tv->tv_sec -= info.uptime;

		// we don't want the microseconds to be constantly jumping around
		// (that actually causes an infinite loop in `_mach_boottime_usec` in libc)
		// but Linux doesn't give us microseconds, so just settle for 0
		tv->tv_usec = 0;
	}
	*oldlen = sizeof(*tv);
	
	return 0;
}

sysctl_handler(handle_ostype)
{
	const char* s = EMULATED_SYSNAME;

	copyout_string(s, (char*) old, oldlen);
	return 0;
}

sysctl_handler(handle_osrelease)
{
	const char* s = EMULATED_RELEASE;

	copyout_string(s, (char*) old, oldlen);
	return 0;
}

sysctl_handler(handle_version)
{
	const char* s = EMULATED_VERSION;

	copyout_string(s, (char*) old, oldlen);
	return 0;
}

sysctl_handler(handle_osversion)
{
	const char* s = EMULATED_OSVERSION;

	copyout_string(s, (char*) old, oldlen);
	return 0;
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
	copyout_string(lu.nodename, (char*) old, oldlen);
	
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
	copyout_string(lu.domainname, (char*) old, oldlen);
	
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
	int fd = sys_open("/proc/sys/kernel/pid_max", BSD_O_RDONLY, 0);
	int value = 1024;
	int* ovalue = (int*) old;
	
	if (!oldlen)
		return -EINVAL;
	
	if (fd >= 0)
	{
		int rd = sys_read(fd, buf, sizeof(buf) - 1);
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
	copyout_string(EMULATED_OSPRODUCTVERSION, (char*)old, oldlen);
	return 0;
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