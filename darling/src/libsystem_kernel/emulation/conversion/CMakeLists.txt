project(emulation_conversion)

set(conversion_sources
    at.c
    errno.c
    signal.c
    stat.c
    network/common.c
    signal/context.c
    signal/sigaction.c
    signal/sigexc.c
)

add_darling_object_library(emulation_conversion ${conversion_sources})
make_fat(emulation_conversion)