#include <emulation/conversion/methods/signal.h>

int signum_linux_to_bsd(int signum)
{
	switch (signum)
	{
		case LINUX_SIGHUP:
			return BSD_SIGHUP;
		case LINUX_SIGINT:
			return BSD_SIGINT;
		case LINUX_SIGQUIT:
			return BSD_SIGQUIT;
		case LINUX_SIGILL:
			return BSD_SIGILL;
		case LINUX_SIGTRAP:
			return BSD_SIGTRAP;
		case LINUX_SIGABRT:
			return BSD_SIGABRT;
		case LINUX_SIGFPE:
			return BSD_SIGFPE;
		case LINUX_SIGKILL:
			return BSD_SIGKILL;
		case LINUX_SIGBUS:
			return BSD_SIGBUS;
		case LINUX_SIGSEGV:
			return BSD_SIGSEGV;
		case LINUX_SIGSYS:
			return BSD_SIGSYS;
		case LINUX_SIGPIPE:
			return BSD_SIGPIPE;
		case LINUX_SIGALRM:
			return BSD_SIGALRM;
		case LINUX_SIGTERM:
			return BSD_SIGTERM;
		case LINUX_SIGURG:
			return BSD_SIGURG;
		case LINUX_SIGSTOP:
			return BSD_SIGSTOP;
		case LINUX_SIGTSTP:
			return BSD_SIGTSTP;
		case LINUX_SIGCONT:
			return BSD_SIGCONT;
		case LINUX_SIGCHLD:
			return BSD_SIGCHLD;
		case LINUX_SIGTTIN:
			return BSD_SIGTTIN;
		case LINUX_SIGTTOU:
			return BSD_SIGTTOU;
		case LINUX_SIGIO:
			return BSD_SIGIO;
		case LINUX_SIGXCPU:
			return BSD_SIGXCPU;
		case LINUX_SIGXFSZ:
			return BSD_SIGXFSZ;
		case LINUX_SIGVTALRM:
			return BSD_SIGVTALRM;
		case LINUX_SIGPROF:
			return BSD_SIGPROF;
		case LINUX_SIGWINCH:
			return BSD_SIGWINCH;
		case LINUX_SIGUSR1:
			return BSD_SIGUSR1;
		case LINUX_SIGUSR2:
			return BSD_SIGUSR2;
		// Hack! This is what we send instead of SIGSTOP to allow the target process to receive the signal
		// and pass it through the LKM.
		case LINUX_SIGSTKFLT:
			return BSD_SIGSTOP;
		default:
			return 0;
	}
}

int signum_bsd_to_linux(int signum)
{
	switch (signum)
	{
		case BSD_SIGHUP:
			return LINUX_SIGHUP;
		case BSD_SIGINT:
			return LINUX_SIGINT;
		case BSD_SIGQUIT:
			return LINUX_SIGQUIT;
		case BSD_SIGILL:
			return LINUX_SIGILL;
		case BSD_SIGTRAP:
			return LINUX_SIGTRAP;
		case BSD_SIGABRT:
			return LINUX_SIGABRT;
		case BSD_SIGFPE:
			return LINUX_SIGFPE;
		case BSD_SIGKILL:
			return LINUX_SIGKILL;
		case BSD_SIGBUS:
			return LINUX_SIGBUS;
		case BSD_SIGSEGV:
			return LINUX_SIGSEGV;
		case BSD_SIGSYS:
			return LINUX_SIGSYS;
		case BSD_SIGPIPE:
			return LINUX_SIGPIPE;
		case BSD_SIGALRM:
			return LINUX_SIGALRM;
		case BSD_SIGTERM:
			return LINUX_SIGTERM;
		case BSD_SIGURG:
			return LINUX_SIGURG;
		case BSD_SIGSTOP:
			// Hack! See above.
			//return LINUX_SIGSTOP;
			return LINUX_SIGSTKFLT;
		case BSD_SIGTSTP:
			return LINUX_SIGTSTP;
		case BSD_SIGCONT:
			return LINUX_SIGCONT;
		case BSD_SIGCHLD:
			return LINUX_SIGCHLD;
		case BSD_SIGTTIN:
			return LINUX_SIGTTIN;
		case BSD_SIGTTOU:
			return LINUX_SIGTTOU;
		case BSD_SIGIO:
			return LINUX_SIGIO;
		case BSD_SIGXCPU:
			return LINUX_SIGXCPU;
		case BSD_SIGXFSZ:
			return LINUX_SIGXFSZ;
		case BSD_SIGVTALRM:
			return LINUX_SIGVTALRM;
		case BSD_SIGPROF:
			return LINUX_SIGPROF;
		case BSD_SIGWINCH:
			return LINUX_SIGWINCH;
		case BSD_SIGUSR1:
			return LINUX_SIGUSR1;
		case BSD_SIGUSR2:
			return LINUX_SIGUSR2;
		case BSD_SIGINFO:
			return LINUX_SIGPWR;
		default:
			return 0;
	}
}

void sigset_linux_to_bsd(const linux_sigset_t* linux, bsd_sigset_t* bsd)
{
	int i;

	*bsd = 0;

	for (i = 0; i < 64; i++)
	{
		if (*linux & (1ull << i))
		{
			int num = signum_linux_to_bsd(i+1);
			if (num)
				*bsd |= 1 << (num-1);
		}
	}
}

void sigset_bsd_to_linux(const bsd_sigset_t* bsd, linux_sigset_t* linux)
{
	int i;

	*linux = 0;
	for (i = 0; i < 64; i++)
	{
		if (*bsd & (1ull << i))
		{
			int num = signum_bsd_to_linux(i+1);
			if (num)
				*linux |= 1 << (num-1);
		}
	}
}

