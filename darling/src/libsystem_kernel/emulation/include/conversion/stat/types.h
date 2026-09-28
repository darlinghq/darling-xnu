#ifndef DARLING_CONVERSION_STAT_TYPES_H
#define DARLING_CONVERSION_STAT_TYPES_H

struct stat;

// Darwin ARM64 exposes only the 64-bit inode layout. Keep the internal
// syscall/converter type consistent without redefining the stat64 token.
#if defined(__aarch64__) || defined(__arm64__)
typedef struct stat darling_stat64_t;
#else
struct stat64;
typedef struct stat64 darling_stat64_t;
#endif

#endif
