project(emulation_common)

set(common_sources
    simple.c
)

add_darling_object_library(emulation_common ${common_sources})
make_fat(emulation_common)