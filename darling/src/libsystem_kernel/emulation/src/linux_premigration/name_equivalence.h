#ifndef _DARLING_EMULATION_NAME_EQUIVALENCE_H
#define _DARLING_EMULATION_NAME_EQUIVALENCE_H

#include <stdbool.h>

// Whether two file names are the same name to a normalization-insensitive file system that folds ASCII case only:
// equal ignoring ASCII case, or both valid UTF-8 and canonically equivalent (equal canonical decompositions) ignoring
// ASCII case. Uses no libc and allocates nothing.
bool darling_names_equivalent(const char* a, const char* b);

#endif
