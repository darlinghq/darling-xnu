project(emulation_conversion)

set(conversion_sources
    methods/at.c
    methods/errno.c
    methods/signal.c

    methods/stat.c
    
    methods/fd/fcntl.c
    
    methods/network/common.c
    methods/network/sockopt.c
    methods/signal/context.c
    methods/signal/sigaction.c
    methods/signal/sigexc.c
)

add_darling_object_library(emulation_conversion ${conversion_sources})
make_fat(emulation_conversion)