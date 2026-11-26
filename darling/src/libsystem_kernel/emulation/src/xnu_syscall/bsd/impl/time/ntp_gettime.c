#include <darling/emulation/xnu_syscall/bsd/impl/time/ntp_gettime.h>

#include <sys/errno.h>
#include <stddef.h>
#include <string.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// Linux struct timex has `time` and `tick` between tolerance and ppsfreq,
// which Darwin's struct timex lacks. See impl/time/ntp_adjtime.c.
struct __darling_linux_timex {
	unsigned int modes;
	long offset;
	long freq;
	long maxerror;
	long esterror;
	int status;
	long constant;
	long precision;
	long tolerance;
	struct {
		long tv_sec;
		long tv_usec;
	} time;
	long tick;
	long ppsfreq;
	long jitter;
	int shift;
	long stabil;
	long jitcnt;
	long calcnt;
	long errcnt;
	long stbcnt;
	int tai;
	int pad[11];
};

// bsd/sys/timex.h: struct timespec (two longs) plus four longs and state.
struct bsd_ntptimeval {
	struct {
		long tv_sec;
		long tv_nsec;
	} time;
	long maxerror;
	long esterror;
	long tai;
	int time_state;
};

// Both Darwin and Linux use the same numeric TIME_* clock states.
#define DARLING_TIME_OK 0
#define DARLING_TIME_INS 1
#define DARLING_TIME_DEL 2
#define DARLING_TIME_OOP 3
#define DARLING_TIME_WAIT 4
#define DARLING_TIME_ERROR 5

static int clock_state(long linux_state)
{
	switch (linux_state)
	{
		case 0: return DARLING_TIME_OK;
		case 1: return DARLING_TIME_INS;
		case 2: return DARLING_TIME_DEL;
		case 3: return DARLING_TIME_OOP;
		case 4: return DARLING_TIME_WAIT;
		case 5: return DARLING_TIME_ERROR;
		default: return DARLING_TIME_ERROR;
	}
}

long sys_ntp_gettime(struct ntptimeval* ntvp)
{
	struct __darling_linux_timex ltx;
	struct bsd_ntptimeval bsd;
	long ret, usec;

	if (ntvp == NULL)
		return -EFAULT;

	// adjtimex() with a zeroed modes field only reads the clock; it does
	// not change any NTP parameters. Its non-negative return value is the
	// clock state.
	memset(&ltx, 0, sizeof(ltx));
	ret = LINUX_SYSCALL1(__NR_adjtimex, &ltx);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	usec = ltx.time.tv_usec;
	if (usec < 0)
		usec = 0;
	else if (usec > 999999)
		usec = 999999;

	memset(&bsd, 0, sizeof(bsd));
	bsd.time.tv_sec = ltx.time.tv_sec;
	bsd.time.tv_nsec = usec * 1000;
	bsd.maxerror = ltx.maxerror;
	bsd.esterror = ltx.esterror;
	bsd.tai = ltx.tai;
	bsd.time_state = clock_state(ret);
	memcpy(ntvp, &bsd, sizeof(bsd));

	return 0;
}
