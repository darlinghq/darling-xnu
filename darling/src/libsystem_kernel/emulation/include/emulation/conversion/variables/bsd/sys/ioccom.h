#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_IOCCOM_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_IOCCOM_H

#include <stdint.h>

#define BSD_IOCPARM_MASK    0x1fff
#define BSD_IOCPARM_LEN(x)  (((x) >> 16) & BSD_IOCPARM_MASK)
#define BSD_IOCBASECMD(x)   ((x) & ~(IBSD_OCPARM_MASK << 16))
#define BSD_IOCGROUP(x)     (((x) >> 8) & 0xff)

#define BSD_IOCPARM_MAX     (BSD_IOCPARM_MASK + 1)
#define BSD_IOC_VOID        (uint32_t)0x20000000
#define BSD_IOC_OUT         (uint32_t)0x40000000
#define BSD_IOC_IN          (uint32_t)0x80000000
#define BSD_IOC_INOUT       (BSD_IOC_IN|BSD_IOC_OUT)
#define BSD_IOC_DIRMASK     (uint32_t)0xe0000000

#define BSD__IOC(inout,group,num,len) \
        (int)(inout | ((len & BSD_IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define BSD__IO(g,n)        BSD__IOC(BSD_IOC_VOID,  (g), (n), 0)
#define BSD__IOR(g,n,t)     BSD__IOC(BSD_IOC_OUT,   (g), (n), sizeof(t))
#define BSD__IOW(g,n,t)     BSD__IOC(BSD_IOC_IN,    (g), (n), sizeof(t))
#define BSD__IOWR(g,n,t)    BSD__IOC(BSD_IOC_INOUT, (g), (n), sizeof(t))

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_IOCCOM_H
