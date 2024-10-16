#include <emulation/conversion/at.h>
#include <emulation/darling/tsd/perthread_wd.h>

int atfd(int fd)
{
	if (fd == BSD_AT_FDCWD)
		return get_perthread_wd();
	return fd;
}