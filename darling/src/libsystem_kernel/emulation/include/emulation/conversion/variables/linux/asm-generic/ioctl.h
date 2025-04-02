#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_IOCTL_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_IOCTL_H


#define LINUX__IOC_NRBITS    8
#define LINUX__IOC_TYPEBITS  8


#ifndef LINUX__IOC_SIZEBITS
#define LINUX__IOC_SIZEBITS  14
#endif

#ifndef LINUX__IOC_DIRBITS
#define LINUX__IOC_DIRBITS   2
#endif


#define LINUX__IOC_NRSHIFT    0
#define LINUX__IOC_TYPESHIFT  (LINUX__IOC_NRSHIFT+LINUX__IOC_NRBITS)
#define LINUX__IOC_SIZESHIFT  (LINUX__IOC_TYPESHIFT+LINUX__IOC_TYPEBITS)
#define LINUX__IOC_DIRSHIFT   (LINUX__IOC_SIZESHIFT+LINUX__IOC_SIZEBITS)


#ifndef LINUX__IOC_NONE
#define LINUX__IOC_NONE      0U
#endif

#ifndef LINUX__IOC_WRITE
#define LINUX__IOC_WRITE     1U
#endif

#ifndef LINUX__IOC_READ
#define LINUX__IOC_READ      2U
#endif

#define LINUX__IOC(dir,type,nr,size) \
        (((dir)  << LINUX__IOC_DIRSHIFT) | \
         ((type) << LINUX__IOC_TYPESHIFT) | \
         ((nr)   << LINUX__IOC_NRSHIFT) | \
         ((size) << LINUX__IOC_SIZESHIFT))

#define LINUX__IOC_TYPECHECK(t) (sizeof(t))


#define LINUX__IO(type,nr)            LINUX__IOC(LINUX__IOC_NONE,(type),(nr),0)
#define LINUX__IOR(type,nr,size)      LINUX__IOC(LINUX__IOC_READ,(type),(nr),(LINUX__IOC_TYPECHECK(size)))
#define LINUX__IOW(type,nr,size)      LINUX__IOC(LINUX__IOC_WRITE,(type),(nr),(LINUX__IOC_TYPECHECK(size)))
#define LINUX__IOWR(type,nr,size)     LINUX__IOC(LINUX__IOC_READ|LINUX__IOC_WRITE,(type),(nr),(LINUX__IOC_TYPECHECK(size)))
#define LINUX__IOR_BAD(type,nr,size)  LINUX__IOC(LINUX__IOC_READ,(type),(nr),sizeof(size))
#define LINUX__IOW_BAD(type,nr,size)  LINUX__IOC(LINUX__IOC_WRITE,(type),(nr),sizeof(size))
#define LINUX__IOWR_BAD(type,nr,size) LINUX__IOC(LINUX__IOC_READ|LINUX__IOC_WRITE,(type),(nr),sizeof(size))


#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_IOCTL_H