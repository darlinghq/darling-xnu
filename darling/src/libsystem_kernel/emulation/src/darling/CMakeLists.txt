project(emulation_darling)

set(darling_sources
    legacy_lkm.c
    weak_stubs.c

    c_api/elfcalls_wrapper.c
    c_api/for_libelfloader.c
    c_api/for_libkqueue.c

    init/mach_driver.c
    init/sigexc.c

    server/dserver-rpc-defs.c
    server/vchroot_userspace.c

    ${CMAKE_BINARY_DIR}/src/external/darlingserver/src/rpc.c
)

set_source_files_properties(${CMAKE_BINARY_DIR}/src/external/darlingserver/src/rpc.c PROPERTIES
	GENERATED TRUE
	COMPILE_FLAGS "-include ${DARLING_XNU_EMULATION_DIR}/include/emulation/darling/server/dserver-rpc-defs.h"
)

add_darling_object_library(emulation_darling ${darling_sources})
make_fat(emulation_darling)

add_darling_object_library(emulation_darling_dyld ${darling_sources})
set_target_properties(emulation_darling_dyld PROPERTIES COMPILE_FLAGS "-ffunction-sections -DVARIANT_DYLD")
make_fat(emulation_darling_dyld)
