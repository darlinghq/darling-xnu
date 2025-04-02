#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SOCKET_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SOCKET_H

#include <emulation/conversion/variables/linux/linux/posix_types.h>
#include <emulation/conversion/variables/linux/asm/sockios.h>

#define LINUX_SOL_SOCKET    1

#define LINUX_SO_DEBUG          1
#define LINUX_SO_REUSEADDR      2
#define LINUX_SO_TYPE           3
#define LINUX_SO_ERROR          4
#define LINUX_SO_DONTROUTE      5
#define LINUX_SO_BROADCAST      6
#define LINUX_SO_SNDBUF         7
#define LINUX_SO_RCVBUF         8
#define LINUX_SO_SNDBUFFORCE    32
#define LINUX_SO_RCVBUFFORCE    33
#define LINUX_SO_KEEPALIVE      9
#define LINUX_SO_OOBINLINE      10
#define LINUX_SO_NO_CHECK       11
#define LINUX_SO_PRIORITY       12
#define LINUX_SO_LINGER         13
#define LINUX_SO_BSDCOMPAT      14
#define LINUX_SO_REUSEPORT      15
#ifndef LINUX_SO_PASSCRED
#define LINUX_SO_PASSCRED       16
#define LINUX_SO_PEERCRED       17
#define LINUX_SO_RCVLOWAT       18
#define LINUX_SO_SNDLOWAT       19
#define LINUX_SO_RCVTIMEO_OLD   20
#define LINUX_SO_SNDTIMEO_OLD   21
#endif // !defined(LINUX_SO_PASSCRED)


#define LINUX_SO_TIMESTAMP_OLD        29
#define LINUX_SO_TIMESTAMPNS_OLD      35
#define LINUX_SO_TIMESTAMPING_OLD     37

#define LINUX_SO_ACCEPTCONN           30

#define LINUX_SO_TIMESTAMP_NEW        63
#define LINUX_SO_TIMESTAMPNS_NEW      64
#define LINUX_SO_TIMESTAMPING_NEW     65

#define LINUX_SO_RCVTIMEO_NEW         66
#define LINUX_SO_SNDTIMEO_NEW         67

#if LINUX___BITS_PER_LONG == 64 || (defined(__x86_64__) && defined(__ILP32__))
#define LINUX_SO_TIMESTAMP      LINUX_SO_TIMESTAMP_OLD
#define LINUX_SO_TIMESTAMPNS    LINUX_SO_TIMESTAMPNS_OLD
#define LINUX_SO_TIMESTAMPING   LINUX_SO_TIMESTAMPING_OLD

#define LINUX_SO_RCVTIMEO       LINUX_SO_RCVTIMEO_OLD
#define LINUX_SO_SNDTIMEO       LINUX_SO_SNDTIMEO_OLD

#else
// TODO: need to verify that `linux_time_t` == `long int`.
#define LINUX_SO_TIMESTAMP (sizeof(long int /*linux_time_t*/) == sizeof(linux___kernel_long_t) ? LINUX_SO_TIMESTAMP_OLD : LINUX_SO_TIMESTAMP_NEW)
#define LINUX_SO_TIMESTAMPNS (sizeof(long int /*linux_time_t*/) == sizeof(linux___kernel_long_t) ? LINUX_SO_TIMESTAMPNS_OLD : LINUX_SO_TIMESTAMPNS_NEW)
#define LINUX_SO_TIMESTAMPING (sizeof(long int /*linux_time_t*/) == sizeof(linux___kernel_long_t) ? LINUX_SO_TIMESTAMPING_OLD : LINUX_SO_TIMESTAMPING_NEW)

#define LINUX_SO_RCVTIMEO (sizeof(long int /*linux_time_t*/) == sizeof(linux___kernel_long_t) ? LINUX_SO_RCVTIMEO_OLD : LINUX_SO_RCVTIMEO_NEW)
#define LINUX_SO_SNDTIMEO (sizeof(long int /*linux_time_t*/) == sizeof(linux___kernel_long_t) ? LINUX_SO_SNDTIMEO_OLD : LINUX_SO_SNDTIMEO_NEW)
#endif

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SOCKET_H
