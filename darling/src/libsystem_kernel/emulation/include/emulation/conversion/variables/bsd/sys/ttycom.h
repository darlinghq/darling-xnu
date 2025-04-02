#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_TTYCOM_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_TTYCOM_H

#include <emulation/conversion/variables/bsd/sys/filio.h>

struct bsd_winsize
{
	unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};

#define BSD_TIOCMODG         BSD__IOR('t', 3, int)
#define BSD_TIOCMODS         BSD__IOW('t', 4, int)
#define   BSD_TIOCM_LE        0001
#define   BSD_TIOCM_DTR       0002
#define   BSD_TIOCM_RTS       0004
#define   BSD_TIOCM_ST        0010
#define   BSD_TIOCM_SR        0020
#define   BSD_TIOCM_CTS       0040
#define   BSD_TIOCM_CAR       0100
#define   BSD_TIOCM_CD        BSD_TIOCM_CAR
#define   BSD_TIOCM_RNG       0200
#define   BSD_TIOCM_RI        BSD_TIOCM_RNG
#define   BSD_TIOCM_DSR       0400

#define BSD_TIOCEXCL          BSD__IO('t', 13)
#define BSD_TIOCNXCL          BSD__IO('t', 14)
/* 15 */
#define BSD_TIOCFLUSH        BSD__IOW('t', 16, int)
/* 17 - 18 */
#define BSD_TIOCGETA         BSD__IOR('t', 19, struct bsd_termios)
#define BSD_TIOCSETA         BSD__IOW('t', 20, struct bsd_termios)
#define BSD_TIOCSETAW        BSD__IOW('t', 21, struct bsd_termios)
#define BSD_TIOCSETAF        BSD__IOW('t', 22, struct bsd_termios)
#define BSD_TIOCGETD         BSD__IOR('t', 26, int)
#define BSD_TIOCSETD         BSD__IOW('t', 27, int)
#define BSD_TIOCIXON          BSD__IO('t', 129)
#define BSD_TIOCIXOFF         BSD__IO('t', 128)

#define BSD_TIOCSBRK          BSD__IO('t', 123)
#define BSD_TIOCCBRK          BSD__IO('t', 122)
#define BSD_TIOCSDTR          BSD__IO('t', 121)
#define BSD_TIOCCDTR          BSD__IO('t', 120)
#define BSD_TIOCGPGRP        BSD__IOR('t', 119, int)
#define BSD_TIOCSPGRP        BSD__IOW('t', 118, int)

#define BSD_TIOCOUTQ         BSD__IOR('t', 115, int)
#define BSD_TIOCSTI          BSD__IOW('t', 114, char)
#define BSD_TIOCNOTTY         BSD__IO('t', 113)
#define BSD_TIOCPKT          BSD__IOW('t', 112, int)
#define   BSD_TIOCPKT_DATA            0x00
#define   BSD_TIOCPKT_FLUSHREAD       0x01
#define   BSD_TIOCPKT_FLUSHWRITE      0x02
#define   BSD_TIOCPKT_STOP            0x04
#define   BSD_TIOCPKT_START           0x08
#define   BSD_TIOCPKT_NOSTOP          0x10
#define   BSD_TIOCPKT_DOSTOP          0x20
#define   BSD_TIOCPKT_IOCTL           0x40
#define BSD_TIOCSTOP          BSD__IO('t', 111)
#define BSD_TIOCSTART         BSD__IO('t', 110)
#define BSD_TIOCMSET         BSD__IOW('t', 109, int)
#define BSD_TIOCMBIS         BSD__IOW('t', 108, int)
#define BSD_TIOCMBIC         BSD__IOW('t', 107, int)
#define BSD_TIOCMGET         BSD__IOR('t', 106, int)
#define BSD_TIOCREMOTE       BSD__IOW('t', 105, int)
#define BSD_TIOCGWINSZ       BSD__IOR('t', 104, struct bsd_winsize)
#define BSD_TIOCSWINSZ       BSD__IOW('t', 103, struct bsd_winsize)
#define BSD_TIOCUCNTL        BSD__IOW('t', 102, int)
#define BSD_TIOCSTAT          BSD__IO('t', 101)
#define BSD_UIOCCMD(n)        BSD__IO('u', n)
#define BSD_TIOCSCONS         BSD__IO('t', 99)
#define BSD_TIOCCONS         BSD__IOW('t', 98, int)
#define BSD_TIOCSCTTY         BSD__IO('t', 97)
#define BSD_TIOCEXT          BSD__IOW('t', 96, int)
#define BSD_TIOCSIG           BSD__IO('t', 95)
#define BSD_TIOCDRAIN         BSD__IO('t', 94)
#define BSD_TIOCMSDTRWAIT    BSD__IOW('t', 91, int)
#define BSD_TIOCMGDTRWAIT    BSD__IOR('t', 90, int)
#define BSD_TIOCTIMESTAMP    BSD__IOR('t', 89, struct bsd_timeval)
#define BSD_TIOCDCDTIMESTAMP BSD__IOR('t', 88, struct bsd_timeval)

#define BSD_TIOCSDRAINWAIT   BSD__IOW('t', 87, int)
#define BSD_TIOCGDRAINWAIT   BSD__IOR('t', 86, int)

#define BSD_TIOCPTYGRANT     BSD__IO('t', 84)
#define BSD_TIOCPTYGNAME    BSD__IOC(BSD_IOC_OUT, 't', 83, 128)
#define BSD_TIOCPTYUNLK      BSD__IO('t', 82)

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_TTYCOM_H
