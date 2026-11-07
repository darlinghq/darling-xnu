#ifndef _NOFILE_LIMIT_H
#define _NOFILE_LIMIT_H

#include <darling/emulation/conversion/misc/getrlimit.h>

// Darwin's RLIMIT_NOFILE as the guest sees it. See nofile_limit.c.

// Implement getrlimit/setrlimit for RLIMIT_NOFILE (Darwin values).
long nofile_limit_get(struct xnu_rlimit* rlp);
long nofile_limit_set(const struct xnu_rlimit* rlp);

// Returns 1 if fd is at or above the guest's soft limit.
int nofile_limit_exceeded(int fd);

// For calls that just created fd (or a negative error): returns fd, or closes
// it and returns -EMFILE if it is at or above the guest's soft limit.
long nofile_limit_check_new(long fd);

// Formats "__mldr_nofile=<soft>,<hard>" for the guest's limit into buf.
// Returns 0 if the guest has not set a limit of its own (nothing to pass on).
int nofile_limit_exec_env(char* buf, unsigned long size);

void nofile_limit_postfork_child(void);

#endif // _NOFILE_LIMIT_H
