#ifndef DARLING_EMULATION_CONVERSION_BSD_I386__TYPES_H
#define DARLING_EMULATION_CONVERSION_BSD_I386__TYPES_H

typedef signed char             bsd___int8_t;
typedef short                   bsd___int16_t;
typedef int                     bsd___int32_t;
typedef long long               bsd___int64_t;

typedef unsigned char           bsd___uint8_t;
typedef unsigned short          bsd___uint16_t;
typedef unsigned int            bsd___uint32_t;
typedef unsigned long long      bsd___uint64_t;

typedef unsigned long           bsd___darwin_size_t;

typedef unsigned long           bsd___darwin_clock_t;
typedef bsd___uint32_t          bsd___darwin_socklen_t;
typedef long                    bsd___darwin_ssize_t;
typedef long                    bsd___darwin_time_t;

#endif // DARLING_EMULATION_CONVERSION_BSD_I386__TYPES_H