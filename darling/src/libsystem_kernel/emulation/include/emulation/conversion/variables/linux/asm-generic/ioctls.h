#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_IOCTLS_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_IOCTLS_H

#include <emulation/conversion/variables/linux/linux/ioctl.h>

#define LINUX_TCGETS          0x5401
#define LINUX_TCSETS          0x5402
#define LINUX_TCSETSW         0x5403
#define LINUX_TCSETSF         0x5404
#define LINUX_TCGETA          0x5405
#define LINUX_TCSETA          0x5406
#define LINUX_TCSETAW         0x5407
#define LINUX_TCSETAF         0x5408
#define LINUX_TCSBRK          0x5409
#define LINUX_TCXONC          0x540A
#define LINUX_TCFLSH          0x540B
#define LINUX_TIOCEXCL        0x540C
#define LINUX_TIOCNXCL        0x540D
#define LINUX_TIOCSCTTY       0x540E
#define LINUX_TIOCGPGRP       0x540F
#define LINUX_TIOCSPGRP       0x5410
#define LINUX_TIOCOUTQ        0x5411
#define LINUX_TIOCSTI         0x5412
#define LINUX_TIOCGWINSZ      0x5413
#define LINUX_TIOCSWINSZ      0x5414
#define LINUX_TIOCMGET        0x5415
#define LINUX_TIOCMBIS        0x5416
#define LINUX_TIOCMBIC        0x5417
#define LINUX_TIOCMSET        0x5418
#define LINUX_TIOCGSOFTCAR    0x5419
#define LINUX_TIOCSSOFTCAR    0x541A
#define LINUX_FIONREAD        0x541B
#define LINUX_TIOCINQ         LINUX_FIONREAD
#define LINUX_TIOCLINUX       0x541C
#define LINUX_TIOCCONS        0x541D
#define LINUX_TIOCGSERIAL     0x541E
#define LINUX_TIOCSSERIAL     0x541F
#define LINUX_TIOCPKT         0x5420
#define LINUX_FIONBIO         0x5421
#define LINUX_TIOCNOTTY       0x5422
#define LINUX_TIOCSETD        0x5423
#define LINUX_TIOCGETD        0x5424
// LINUX_TCSBRKP
#define LINUX_TIOCSBRK        0x5427
#define LINUX_TIOCCBRK        0x5428
// LINUX_TIOCGSID
#define LINUX_TCGETS2         LINUX__IOR('T', 0x2A, struct linux_termios2)
#define LINUX_TCSETS2         LINUX__IOW('T', 0x2B, struct linux_termios2)
#define LINUX_TCSETSW2        LINUX__IOW('T', 0x2C, struct linux_termios2)
#define LINUX_TCSETSF2        LINUX__IOW('T', 0x2D, struct linux_termios2)

#define LINUX_TIOCGPTN        LINUX__IOR('T', 0x30, unsigned int)
#define LINUX_TIOCSPTLCK      LINUX__IOW('T', 0x31, int)

// TODO: Does this actually exist on Linux? It looks like this
// value was taken from macOS
#define LINUX_TIOCSTOP    LINUX__IO('t', 111)
#define LINUX_TIOCSTART   LINUX__IO('t', 110)

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_IOCTLS_H