#include <darling/emulation/xnu_syscall/bsd/impl/time/ntp_adjtime.h>
#include <sys/errno.h>
#include <string.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// Linux struct timex layout (linux/timex.h) contains extra fields `time` and `tick`
// positioned after `tolerance` that are absent in BSD struct timex (RFC 5905).
// Note: Darwin struct timex does not contain a `tai` member (unlike ntptimeval).
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

long sys_ntp_adjtime(struct timex* tp)
{
	if (!tp)
		return -EFAULT;

	struct __darling_linux_timex ltx;
	memset(&ltx, 0, sizeof(ltx));

	ltx.modes = tp->modes;
	ltx.offset = tp->offset;
	ltx.freq = tp->freq;
	ltx.maxerror = tp->maxerror;
	ltx.esterror = tp->esterror;
	ltx.status = tp->status;
	ltx.constant = tp->constant;
	ltx.precision = tp->precision;
	ltx.tolerance = tp->tolerance;
	ltx.ppsfreq = tp->ppsfreq;
	ltx.jitter = tp->jitter;
	ltx.shift = tp->shift;
	ltx.stabil = tp->stabil;
	ltx.jitcnt = tp->jitcnt;
	ltx.calcnt = tp->calcnt;
	ltx.errcnt = tp->errcnt;
	ltx.stbcnt = tp->stbcnt;

	int ret = LINUX_SYSCALL1(__NR_adjtimex, &ltx);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	tp->modes = ltx.modes;
	tp->offset = ltx.offset;
	tp->freq = ltx.freq;
	tp->maxerror = ltx.maxerror;
	tp->esterror = ltx.esterror;
	tp->status = ltx.status;
	tp->constant = ltx.constant;
	tp->precision = ltx.precision;
	tp->tolerance = ltx.tolerance;
	tp->ppsfreq = ltx.ppsfreq;
	tp->jitter = ltx.jitter;
	tp->shift = ltx.shift;
	tp->stabil = ltx.stabil;
	tp->jitcnt = ltx.jitcnt;
	tp->calcnt = ltx.calcnt;
	tp->errcnt = ltx.errcnt;
	tp->stbcnt = ltx.stbcnt;

	return ret; // non-negative clock state (0 to 5)
}
