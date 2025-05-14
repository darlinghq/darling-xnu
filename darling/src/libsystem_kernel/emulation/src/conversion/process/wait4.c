#include <darling/emulation/conversion/process/wait4.h>

int waitopts_bsd_to_linux(int options)
{
	int out = 0;

	if (options & BSD_WNOHANG)
		out |= LINUX_WNOHANG;
	if (options & BSD_WUNTRACED)
		out |= LINUX_WUNTRACED;
	if (options & BSD_WEXITED)
		out |= LINUX_WEXITED;
	if (options & BSD_WSTOPPED)
		out |= LINUX_WSTOPPED;
	if (options & BSD_WCONTINUED)
		out |= LINUX_WCONTINUED;
	if (options & BSD_WNOWAIT)
		out |= LINUX_WNOWAIT;

	return out;
}
