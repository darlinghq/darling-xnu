#ifndef DARLING_EMULATION_SHARED_UNISTD_UIDGID_H
#define DARLING_EMULATION_SHARED_UNISTD_UIDGID_H

#include <stdint.h>

void __getuidgid(int32_t* uid, int32_t* gid);
long __setuidgid(int32_t uid, int32_t gid);

#endif // DARLING_EMULATION_SHARED_UNISTD_UIDGID_H
