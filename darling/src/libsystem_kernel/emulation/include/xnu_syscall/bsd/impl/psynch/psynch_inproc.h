#ifndef LINUX_PSYNCH_INPROC_H
#define LINUX_PSYNCH_INPROC_H

#include <stdbool.h>
#include <stdint.h>

// Process-private pthread objects (the ones XNU keeps in its per-process table of
// wait queues) are synchronized inside the process; process-shared ones by
// darlingserver. The decision depends only on the flags of the call.
bool psynch_inproc_handles(uint32_t flags);

// at libsystem_kernel initialization: reads DARLING_PSYNCH_SERVER
void psynch_inproc_init(const char* envp[]);

long psynch_inproc_mutexwait(void* mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags);
long psynch_inproc_mutexdrop(void* mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags);
long psynch_inproc_cvwait(void* cv, uint64_t cvlsgen, uint32_t cvugen, void* mutex, uint64_t mugen,
		uint32_t flags, int64_t sec, uint32_t nsec);
long psynch_inproc_cvsignal(void* cv, uint64_t cvlsgen, uint32_t cvugen, int32_t thread_port, void* mutex,
		uint64_t mugen, uint64_t tid, uint32_t flags);
long psynch_inproc_cvbroad(void* cv, uint64_t cvlsgen, uint64_t cvudgen, uint32_t flags, void* mutex,
		uint64_t mugen, uint64_t tid);
long psynch_inproc_cvclrprepost(void* cv, uint32_t cvgen, uint32_t cvugen, uint32_t cvsgen, uint32_t prepocnt,
		uint32_t preposeq, uint32_t flags);
long psynch_inproc_rw_rdlock(void* rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags);
long psynch_inproc_rw_wrlock(void* rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags);
long psynch_inproc_rw_unlock(void* rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags);

// before a guest signal handler runs on this thread (sigexc): ends a psynch wait
// the thread sleeps in, as XNU ends it before it delivers the signal
void psynch_inproc_signal_arrived(void);

// thread exit (bsdthread_terminate)
void psynch_inproc_thread_exit(void);

// around fork (sys_fork): the child starts with no wait queues. prefork blocks
// signals and saves the previous mask in *sigmask; the postfork calls restore it.
void psynch_inproc_prefork(unsigned long long* sigmask);
void psynch_inproc_postfork_parent(const unsigned long long* sigmask);
void psynch_inproc_postfork_child(const unsigned long long* sigmask);

#endif // LINUX_PSYNCH_INPROC_H
