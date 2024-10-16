#ifndef DARLING_XTRACE_API
#define DARLING_XTRACE_API

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/mman/mman.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

CPP_EXTERN_C_BEGIN

VISIBLE
void* _mmap_for_xtrace(void* start, unsigned long len, int prot, int flags, int fd, long pos);

VISIBLE
long _munmap_for_xtrace(void* addr, unsigned long len);

VISIBLE
long _abort_with_payload_for_xtrace(unsigned int reason_namespace, unsigned long long reason_code, void *payload, unsigned int payload_size, const char *reason_string, unsigned long long reason_flags);

VISIBLE
long __write_for_xtrace(int fd, const void* mem, int len);

VISIBLE
long _open_for_xtrace(const char* filename, int flags, unsigned int mode);

VISIBLE
long _close_for_xtrace(int fd);

VISIBLE
long _sigaltstack_set_default_size_for_xtrace(size_t new_size);

// used to bypass the sigaltstack size requirements imposed by libc
VISIBLE
long _sigaltstack_for_xtrace(const struct bsd_stack* ss, struct bsd_stack* oss);

CPP_EXTERN_C_END

#endif // DARLING_XTRACE_API
