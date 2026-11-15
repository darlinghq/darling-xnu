#include <darling/emulation/xnu_syscall/bsd/impl/conf/pathconf.h>

#include <sys/stat.h>

#include <darling/emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat.h>
#include <darling/emulation/common/base.h>

long sys_pathconf(const char* file, int name)
{
	darling_stat64_t st;
	// The path is looked up first (following symlinks): a missing path is ENOENT and so on, whatever the name.
	long ret = sys_stat64(file, &st);

	if (ret < 0)
		return ret;
	return pathconf_for_mode(st.st_mode, name);
}
