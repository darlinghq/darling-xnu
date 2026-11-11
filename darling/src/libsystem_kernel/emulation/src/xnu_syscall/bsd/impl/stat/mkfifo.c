#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkfifo.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkfifoat.h>
#include <darling/emulation/conversion/common_at.h>

long sys_mkfifo(const char* path, unsigned int mode)
{
	return sys_mkfifoat(BSD_AT_FDCWD, path, mode);
}
