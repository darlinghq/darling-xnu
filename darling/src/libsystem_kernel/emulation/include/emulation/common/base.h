#ifndef DARLING_EMULATION_COMMON_BASE_H
#define DARLING_EMULATION_COMMON_BASE_H

#define VISIBLE __attribute__((visibility("default")))
#define WEAK __attribute__((weak))

#ifdef __cplusplus
#define CPP_EXTERN_C_BEGIN extern "C" {
#define CPP_EXTERN_C_END }
#else
#define CPP_EXTERN_C_BEGIN
#define CPP_EXTERN_C_END
#endif

#endif // DARLING_EMULATION_COMMON_BASE_H
