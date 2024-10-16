#ifndef DARLING_BSD_EMULATION_GETUID_H
#define DARLING_BSD_EMULATION_GETUID_H

#include <stdint.h>

long sys_getuid(void);

// TODO: this should be relocated into the shared folder...

void __getuidgid(int32_t* uid, int32_t* gid);
long __setuidgid(int32_t uid, int32_t gid);

#endif // DARLING_BSD_EMULATION_GETUID_H
