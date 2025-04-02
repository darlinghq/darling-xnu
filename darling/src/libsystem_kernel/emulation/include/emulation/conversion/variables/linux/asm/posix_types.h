#ifdef __i386__
    #include <emulation/conversion/variables/linux/asm/posix_types_32.h>
#elif defined(__ILP32__)
    #include <emulation/conversion/variables/linux/asm/posix_types_x32.h>
#else
    #include <emulation/conversion/variables/linux/asm/posix_types_64.h>
#endif
