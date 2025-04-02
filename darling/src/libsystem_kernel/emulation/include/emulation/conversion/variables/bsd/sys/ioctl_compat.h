#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_IOCTL_COMPAT_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_IOCTL_COMPAT_H

struct bsd_tchars {
    char t_intrc;
    char t_quitc;
    char t_startc;
    char t_stopc;
    char t_eofc;
    char t_brkc;
};

struct bsd_ltchars {
    char t_suspc;
    char t_dsuspc;
    char t_rprntc;
    char t_flushc;
    char t_werasc;
    char t_lnextc;
};

struct bsd_sgttyb {
    char  sg_ispeed;
    char  sg_ospeed;
    char  sg_erase;
    char  sg_kill;
    short sg_flags;
};

#define BSD_TIOCGETP BSD__IOR('t', 8,struct bsd_sgttyb)
#define BSD_TIOCSETP BSD__IOW('t', 9,struct bsd_sgttyb)
#define BSD_TIOCSETC BSD__IOW('t',17,struct bsd_tchars)
#define BSD_TIOCGETC BSD__IOR('t',18,struct bsd_tchars)

#define BSD_TIOCSLTC BSD__IOW('t',117,struct bsd_ltchars)
#define BSD_TIOCGLTC BSD__IOR('t',116,struct bsd_ltchars)

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_IOCTL_COMPAT_H
