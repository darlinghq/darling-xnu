project(emulation)

# By default we don't want to expose these methods to normal macOS apps/frameworks
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion  -Wno-compare-distinct-pointer-types")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/src/startup # include src/startup for rtsig.h
    ${CMAKE_BINARY_DIR}/src/external/darlingserver/include
	${CMAKE_SOURCE_DIR}/src/external/darlingserver/include
    
    ${CMAKE_SOURCE_DIR}/src/libsimple/include
)

set(DARLING_XNU_EMULATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Some projects may include build time checks for `VARIANT_DYLD`. It's important that
# those projects also have a `*_dyld` varient as well.

add_subdirectory(src/common)
add_subdirectory(src/conversion)
add_subdirectory(src/darling)
add_subdirectory(src/linux_api)
add_subdirectory(src/xnu_syscall)
