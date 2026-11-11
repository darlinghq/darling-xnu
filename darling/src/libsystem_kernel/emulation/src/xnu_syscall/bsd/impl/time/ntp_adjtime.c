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

// Linux adjtimex mode bits (linux/timex.h)
#define LINUX_ADJ_OFFSET    0x0001
#define LINUX_ADJ_FREQUENCY 0x0002
#define LINUX_ADJ_MAXERROR  0x0004
#define LINUX_ADJ_ESTERROR  0x0008
#define LINUX_ADJ_STATUS    0x0010
#define LINUX_ADJ_TIMECONST 0x0020
#define LINUX_ADJ_TAI       0x0080
#define LINUX_ADJ_MICRO     0x1000
#define LINUX_ADJ_NANO      0x2000

// Darwin's MOD_* bits are not Linux's ADJ_* bits throughout: MOD_CLKB has
// the value of ADJ_TICK, MOD_CLKA overlaps ADJ_OFFSET_SINGLESHOT, and
// MOD_PPSMAX has no Linux counterpart. Translate the bits that have one and
// drop the others (PPS and clock selection, which XNU also ignores without
// a PPS source).
static unsigned int modes_bsd_to_linux(unsigned int modes)
{
	unsigned int out = 0;

	if (modes & MOD_OFFSET)
		out |= LINUX_ADJ_OFFSET;
	if (modes & MOD_FREQUENCY)
		out |= LINUX_ADJ_FREQUENCY;
	if (modes & MOD_MAXERROR)
		out |= LINUX_ADJ_MAXERROR;
	if (modes & MOD_ESTERROR)
		out |= LINUX_ADJ_ESTERROR;
	if (modes & MOD_STATUS)
		out |= LINUX_ADJ_STATUS;
	if (modes & MOD_TIMECONST)
		out |= LINUX_ADJ_TIMECONST;
	if (modes & MOD_TAI)
		out |= LINUX_ADJ_TAI;
	if (modes & MOD_MICRO)
		out |= LINUX_ADJ_MICRO;
	if (modes & MOD_NANO)
		out |= LINUX_ADJ_NANO;

	return out;
}

long sys_ntp_adjtime(struct timex* tp)
{
	if (!tp)
		return -EFAULT;

	struct __darling_linux_timex ltx;
	memset(&ltx, 0, sizeof(ltx));

	ltx.modes = modes_bsd_to_linux(tp->modes);
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
