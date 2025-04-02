#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_FILIO_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_FILIO_H

#include <emulation/conversion/variables/bsd/sys/ioccom.h>

#define BSD_FIOCLEX         BSD__IO('f', 1)
#define BSD_FIONCLEX        BSD__IO('f', 2)
#define BSD_FIONREAD        BSD__IOR('f', 127, int)
#define BSD_FIONBIO         BSD__IOW('f', 126, int)
#define BSD_FIOASYNC        BSD__IOW('f', 125, int)
#define BSD_FIOSETOWN       BSD__IOW('f', 124, int)
#define BSD_FIOGETOWN       BSD__IOR('f', 123, int)
#define BSD_FIODTYPE        BSD__IOR('f', 122, int)

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_FILIO_H
