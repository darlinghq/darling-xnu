#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_TERMBITS_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_TERMBITS_H

#include <emulation/conversion/variables/linux/asm-generic/termbits-common.h>

typedef unsigned int linux_tcflag_t;


#define NCCS 19
struct linux_termios {
	linux_tcflag_t c_iflag;
	linux_tcflag_t c_oflag;
	linux_tcflag_t c_cflag;
	linux_tcflag_t c_lflag;
	linux_cc_t c_line;
	linux_cc_t c_cc[NCCS];
};

struct linux_termios2 {
	linux_tcflag_t c_iflag;
	linux_tcflag_t c_oflag;
	linux_tcflag_t c_cflag;
	linux_tcflag_t c_lflag;
	linux_cc_t c_line;
	linux_cc_t c_cc[NCCS];
	linux_speed_t c_ispeed;
	linux_speed_t c_ospeed;
};


#define LINUX_VINTR      0
#define LINUX_VQUIT      1
#define LINUX_VERASE     2
#define LINUX_VKILL      3
#define LINUX_VEOF       4
#define LINUX_VTIME      5
#define LINUX_VMIN       6
#define LINUX_VSWTC      7
#define LINUX_VSTART     8
#define LINUX_VSTOP      9
#define LINUX_VSUSP     10
#define LINUX_VEOL      11
#define LINUX_VREPRINT  12
#define LINUX_VDISCARD  13
#define LINUX_VWERASE   14
#define LINUX_VLNEXT    15
#define LINUX_VEOL2     16


#define LINUX_IUCLC    0x0200
#define LINUX_IXON     0x0400
#define LINUX_IXOFF    0x1000
#define LINUX_IMAXBEL  0x2000
#define LINUX_IUTF8    0x4000


#define LINUX_OLCUC    0x00002
#define LINUX_ONLCR    0x00004
#define LINUX_NLDLY    0x00100
#define   LINUX_NL0    0x00000
#define   LINUX_NL1    0x00100
#define LINUX_CRDLY    0x00600
#define   LINUX_CR0    0x00000
#define   LINUX_CR1    0x00200
#define   LINUX_CR2    0x00400
#define   LINUX_CR3    0x00600
#define LINUX_TABDLY   0x01800
#define   LINUX_TAB0   0x00000
#define   LINUX_TAB1   0x00800
#define   LINUX_TAB2   0x01000
#define   LINUX_TAB3   0x01800
#define   LINUX_XTABS  0x01800
#define LINUX_BSDLY    0x02000
#define   LINUX_BS0    0x00000
#define   LINUX_BS1    0x02000
#define LINUX_VTDLY    0x04000
#define   LINUX_VT0    0x00000
#define   LINUX_VT1    0x04000
#define LINUX_FFDLY    0x08000
#define   LINUX_FF0    0x00000
#define   LINUX_FF1    0x08000


#define LINUX_CBAUD       0x0000100f
#define LINUX_CSIZE       0x00000030
#define   LINUX_CS5       0x00000000
#define   LINUX_CS6       0x00000010
#define   LINUX_CS7       0x00000020
#define   LINUX_CS8       0x00000030
#define LINUX_CSTOPB      0x00000040
#define LINUX_CREAD       0x00000080
#define LINUX_PARENB      0x00000100
#define LINUX_PARODD      0x00000200
#define LINUX_HUPCL       0x00000400
#define LINUX_CLOCAL      0x00000800
#define LINUX_CBAUDEX     0x00001000
#define LINUX_BOTHER      0x00001000
#define     LINUX_B57600  0x00001001
#define    LINUX_B115200  0x00001002
#define    LINUX_B230400  0x00001003
#define    LINUX_B460800  0x00001004
#define    LINUX_B500000  0x00001005
#define    LINUX_B576000  0x00001006
#define    LINUX_B921600  0x00001007
#define   LINUX_B1000000  0x00001008
#define   LINUX_B1152000  0x00001009
#define   LINUX_B1500000  0x0000100a
#define   LINUX_B2000000  0x0000100b
#define   LINUX_B2500000  0x0000100c
#define   LINUX_B3000000  0x0000100d
#define   LINUX_B3500000  0x0000100e
#define   LINUX_B4000000  0x0000100f
#define LINUX_CIBAUD      0x100f0000


#define LINUX_ISIG      0x00001
#define LINUX_ICANON    0x00002
#define LINUX_XCASE     0x00004
#define LINUX_ECHO      0x00008
#define LINUX_ECHOE     0x00010
#define LINUX_ECHOK     0x00020
#define LINUX_ECHONL    0x00040
#define LINUX_NOFLSH    0x00080
#define LINUX_TOSTOP    0x00100
#define LINUX_ECHOCTL   0x00200
#define LINUX_ECHOPRT   0x00400
#define LINUX_ECHOKE    0x00800
#define LINUX_FLUSHO    0x01000
#define LINUX_PENDIN    0x04000
#define LINUX_IEXTEN    0x08000
#define LINUX_EXTPROC   0x10000


#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_TERMBITS_H