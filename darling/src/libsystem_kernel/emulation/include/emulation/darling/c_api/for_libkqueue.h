#ifndef DARLING_EMULATION_DARLING_FOR_LIBKQUEUE_H
#define DARLING_EMULATION_DARLING_FOR_LIBKQUEUE_H

#include <stdint.h>

#include <darling/emulation/common/base.h>

VISIBLE
int _dserver_rpc_kqchan_mach_port_open_4libkqueue(uint32_t port_name, void* receive_buffer, uint64_t receive_buffer_size, uint64_t saved_filter_flags, int* out_socket);

VISIBLE
int _dserver_rpc_kqchan_proc_open_4libkqueue(int32_t pid, uint32_t flags, int* out_socket);

VISIBLE
int _dup_4libkqueue(int fd);

#endif // DARLING_EMULATION_DARLING_FOR_LIBKQUEUE_H
