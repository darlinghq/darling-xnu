#ifndef _DARLING_EMULATION_AUDIT_AUDITON_H
#define _DARLING_EMULATION_AUDIT_AUDITON_H

#include <stdint.h>
#include <bsm/audit.h>

long sys_auditon(int cmd, void* data, int length);

#endif // _DARLING_EMULATION_AUDIT_AUDITON_H
