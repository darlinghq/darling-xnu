#ifndef _DARLING_EMULATION_FOR_LIBKQUEUE_H_
#define _DARLING_EMULATION_FOR_LIBKQUEUE_H_

#include <stdint.h>

#include <darling/emulation/common/base.h>

VISIBLE
int _dserver_rpc_kqchan_mach_port_open_4libkqueue(uint32_t port_name, void* receive_buffer, uint64_t receive_buffer_size, uint64_t saved_filter_flags, int* out_socket);

VISIBLE
int _dserver_rpc_kqchan_proc_open_4libkqueue(int32_t pid, uint32_t flags, int* out_socket);

VISIBLE
int _dup_4libkqueue(int fd);

// Counts each delivery of the BSD signal on the eventfd, whichever thread
// takes the signal and whatever its disposition. Returns 0 or a negative errno.
VISIBLE
int _signal_watch_4libkqueue(int signum, int eventfd);

VISIBLE
void _signal_unwatch_4libkqueue(int signum, int eventfd);

#endif // _DARLING_EMULATION_FOR_LIBKQUEUE_H_
