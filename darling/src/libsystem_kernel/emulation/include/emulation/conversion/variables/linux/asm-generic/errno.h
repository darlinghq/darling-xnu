#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_ERRNO_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_ERRNO_H

#include <darling/emulation/conversion/variables/linux/asm-generic/errno-base.h>

#define LINUX_EDEADLK      35
#define LINUX_ENAMETOOLONG 36
#define LINUX_ENOLCK       37

#define LINUX_ENOSYS 38

#define LINUX_ENOTEMPTY 39
#define LINUX_ELOOP     40
// EWOULDBLOCK
#define LINUX_ENOMSG 42
#define LINUX_EIDRM  43
#define LINUX_ECHRNG 44
// EL2NSYNC
// EL3HLT
// EL3RST
#define LINUX_ELNRNG  48
#define LINUX_EUNATCH 49
#define LINUX_ENOCSI  50
// EL2HLT
#define LINUX_EBADE   52
#define LINUX_EBADR   53
#define LINUX_EXFULL  54
#define LINUX_ENOANO  55
#define LINUX_EBADRQC 56
#define LINUX_EBADSLT 57

// EDEADLOCK

#define LINUX_EBFONT          59
#define LINUX_ENOSTR          60
#define LINUX_ENODATA         61
#define LINUX_ETIME           62
#define LINUX_ENOSR           63
#define LINUX_ENONET          64
#define LINUX_ENOPKG          65
#define LINUX_EREMOTE         66
#define LINUX_ENOLINK         67
#define LINUX_EADV            68
#define LINUX_ESRMNT          69
#define LINUX_ECOMM           70
#define LINUX_EPROTO          71
#define LINUX_EMULTIHOP       72
#define LINUX_EDOTDOT         73
#define LINUX_EBADMSG         74
#define LINUX_EOVERFLOW       75
#define LINUX_ENOTUNIQ        76
#define LINUX_EBADFD          77
#define LINUX_EREMCHG         78
#define LINUX_ELIBACC         79
#define LINUX_ELIBBAD         80
#define LINUX_ELIBSCN         81
#define LINUX_ELIBMAX         82
#define LINUX_ELIBEXEC        83
#define LINUX_EILSEQ          84
#define LINUX_ERESTART        85
#define LINUX_ESTRPIPE        86
#define LINUX_EUSERS          87
#define LINUX_ENOTSOCK        88
#define LINUX_EDESTADDRREQ    89
#define LINUX_EMSGSIZE        90
#define LINUX_EPROTOTYPE      91
#define LINUX_ENOPROTOOPT     92
#define LINUX_EPROTONOSUPPORT 93
#define LINUX_ESOCKTNOSUPPORT 94
#define LINUX_EOPNOTSUPP      95
#define LINUX_EPFNOSUPPORT    96
#define LINUX_EAFNOSUPPORT    97
#define LINUX_EADDRINUSE      98
#define LINUX_EADDRNOTAVAIL   99
#define LINUX_ENETDOWN        100
#define LINUX_ENETUNREACH     101
#define LINUX_ENETRESET       102
#define LINUX_ECONNABORTED    103
#define LINUX_ECONNRESET      104
#define LINUX_ENOBUFS         105
#define LINUX_EISCONN         106
#define LINUX_ENOTCONN        107
#define LINUX_ESHUTDOWN       108
#define LINUX_ETOOMANYREFS    109
#define LINUX_ETIMEDOUT       110
#define LINUX_ECONNREFUSED    111
#define LINUX_EHOSTDOWN       112
#define LINUX_EHOSTUNREACH    113
#define LINUX_EALREADY        114
#define LINUX_EINPROGRESS     115
#define LINUX_ESTALE          116
#define LINUX_EUCLEAN         117
#define LINUX_ENOTNAM         118
#define LINUX_ENAVAIL         119
#define LINUX_EISNAM          120
#define LINUX_EREMOTEIO       121
#define LINUX_EDQUOT          122

#define LINUX_ENOMEDIUM    123
#define LINUX_EMEDIUMTYPE  124
#define LINUX_ECANCELED    125
#define LINUX_ENOKEY       126
#define LINUX_EKEYEXPIRED  127
#define LINUX_EKEYREVOKED  128
#define LINUX_EKEYREJECTED 129

#define LINUX_EOWNERDEAD      130
#define LINUX_ENOTRECOVERABLE 131

#define LINUX_ERFKILL         132

#define LINUX_EHWPOISON       133

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_ERRNO_H