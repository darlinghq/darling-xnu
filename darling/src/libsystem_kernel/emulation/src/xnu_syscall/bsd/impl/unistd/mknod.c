#include <darling/emulation/xnu_syscall/bsd/impl/unistd/mknod.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/mknodat.h>
#include <darling/emulation/conversion/common_at.h>

long sys_mknod(const char* path, int mode, int dev)
{
	return sys_mknodat(BSD_AT_FDCWD, path, mode, dev);
}
