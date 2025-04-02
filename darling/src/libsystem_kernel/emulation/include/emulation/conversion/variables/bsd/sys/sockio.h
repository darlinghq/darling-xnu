#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_SOCKIO_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_SOCKIO_H

#include <emulation/conversion/variables/bsd/net/if.h>
#include <emulation/conversion/variables/bsd/sys/filio.h>

// TODO: Duplicate ifreq/ifaliasreq struct
#define BSD_SIOCSIFFLAGS     BSD__IOW ('i', 16, struct bsd_ifreq)
#define BSD_SIOCGIFFLAGS     BSD__IOWR('i', 17, struct bsd_ifreq)
#define BSD_SIOCAIFADDR      BSD__IOW ('i', 26, struct bsd_ifaliasreq)

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_SOCKIO_H
