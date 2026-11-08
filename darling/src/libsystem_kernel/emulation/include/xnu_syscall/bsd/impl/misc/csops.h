#ifndef LINUX_CSOPS_H
#define LINUX_CSOPS_H

#include <stddef.h>
#include <stdint.h>
#include <mach/message.h>

#include <darlingserver/rpc-supplement.h>

long sys_csops(int pid, unsigned int ops, void* useraddr, size_t usersize);
long sys_csops_audittoken(int pid, unsigned int ops, void* useraddr, size_t usersize, audit_token_t* audit_token);

// The code signing state darlingserver records for a process (0 for the
// calling one). Returns 0 or a negative BSD errno value.
int cs_get_state(int pid, dserver_cs_state_t* state);

// The code signing policy (`dserver_code_signing_policy`) the calling process
// was started with. It is fixed until the next exec, so it is asked for once.
uint32_t cs_own_policy(void);

#endif // LINUX_CSOPS_H
