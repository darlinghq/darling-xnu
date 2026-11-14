#ifndef DARLING_ELFCALLS_SIZE_H
#define DARLING_ELFCALLS_SIZE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Optional loader metadata. Zero means unavailable, never an inferred size. */
static size_t elfcalls_parse_size(const char *value)
{
    size_t result = 0;
    if (!value || !*value) return 0;
    for (; *value; ++value) {
        unsigned digit;
        if (*value >= '0' && *value <= '9') digit = *value - '0';
        else if (*value >= 'a' && *value <= 'f') digit = *value - 'a' + 10;
        else if (*value >= 'A' && *value <= 'F') digit = *value - 'A' + 10;
        else return 0;
        if (result > (SIZE_MAX - digit) / 16) return 0;
        result = result * 16 + digit;
    }
    return result;
}

static size_t elfcalls_size_from_apple(const char **apple)
{
    size_t result = 0;
    int found = 0;
    if (!apple) return 0;
    for (; *apple; ++apple) {
        if (strncmp(*apple, "elf_calls_size=", 15) != 0) continue;
        if (found++) return 0; /* Ambiguous duplicate metadata is not trusted. */
        result = elfcalls_parse_size(*apple + 15);
    }
    return result;
}
#endif
