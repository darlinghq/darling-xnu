#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

// Per ADR-0004 a placeholder is private or fails loudly, never a weak definition
// that can satisfy a guest binding with a no-op: the real implementation is
// optional, so import it weakly and test for it explicitly.
#if defined(VARIANT_DYLD)
/* Loader-archive variant (emulation_dyld): the 32-bit and 64-bit loaders have no libkqueue provider, so the
 * references must be satisfied inside the archive -- measured, the link fails otherwise ("ld: symbol(s) not
 * found for architecture i386" for _kqueue_impl and the rest). Unlike the weak definitions they replace, these
 * are HIDDEN, so a guest cannot satisfy a dynamic binding with them: this is the ADR's "private" case, not
 * another public no-op. The dylib variant is built without VARIANT_DYLD and defines none of these names. */
__attribute__((visibility("hidden"))) int kqueue_impl(void) { return -ENOSYS; }
__attribute__((visibility("hidden"))) int kevent_impl(int kq, ...) { (void)kq; return -ENOSYS; }
__attribute__((visibility("hidden"))) int kevent64_impl(int kq, ...) { (void)kq; return -ENOSYS; }
__attribute__((visibility("hidden"))) int kqueue_close(int kq) { (void)kq; return 0; }
__attribute__((visibility("hidden"))) void kqueue_closed_fd(int fd) { (void)fd; }
__attribute__((visibility("hidden"))) void kqueue_dup(int oldfd, int newfd) { (void)oldfd; (void)newfd; }
__attribute__((visibility("hidden"))) void __darling_kqueue_register_listen(int fd) { (void)fd; }
#else
int WEAK_IMPORT kqueue_impl(void);
#endif

long sys_kqueue(void)
{
	static int warned = 0;

	if (!kqueue_impl) {
		if (!warned) {
			warned = 1;
			__simple_printf("no kqueue implementation present\n");
		}
		return -ENOSYS;
	}

	int ret = kqueue_impl();
	if (ret < 0)
		ret = -errno;
	else if (nofile_limit_exceeded(ret)) {
		// through libkqueue, which owns the descriptor
		sys_close_nocancel(ret);
		ret = -EMFILE;
	}
	return ret;
}
