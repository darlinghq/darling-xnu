#include <darling/emulation/linux_premigration/ext/for-libkqueue.h>

#include <sys/errno.h>

#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>

#include <darlingserver/rpc.h>

int _dserver_rpc_kqchan_mach_port_open_4libkqueue(uint32_t port_name, void* receive_buffer, uint64_t receive_buffer_size, uint64_t saved_filter_flags, int* out_socket) {
	return dserver_rpc_kqchan_mach_port_open(port_name, receive_buffer, receive_buffer_size, saved_filter_flags, out_socket);
};

int _dserver_rpc_kqchan_proc_open_4libkqueue(int32_t pid, uint32_t flags, int* out_socket) {
	return dserver_rpc_kqchan_proc_open(pid, flags, out_socket);
};

int _dup_4libkqueue(int fd) {
	int ret;

	ret = LINUX_SYSCALL1(__NR_dup, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
};

int _signal_watch_4libkqueue(int signum, int eventfd) {
	int linux_signum = signum_bsd_to_linux(signum);
	if (linux_signum <= 0)
		return -EINVAL;
	return sigexc_kqueue_watch(linux_signum, eventfd);
};

void _signal_unwatch_4libkqueue(int signum, int eventfd) {
	int linux_signum = signum_bsd_to_linux(signum);
	if (linux_signum > 0)
		sigexc_kqueue_unwatch(linux_signum, eventfd);
};
