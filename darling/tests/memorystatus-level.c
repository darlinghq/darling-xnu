// SPDX-License-Identifier: GPL-3.0-or-later
#include <sys/sysctl.h>
#include <errno.h>
#include <stdio.h>

int main(void) {
    size_t size = 0;
    if (sysctlbyname("kern.memorystatus_level", NULL, &size, NULL, 0) != 0) {
        perror("kern.memorystatus_level size query");
        return 1;
    }
    if (size != sizeof(int))
        return 1;
    int level = -1;
    if (sysctlbyname("kern.memorystatus_level", &level, &size, NULL, 0) != 0 ||
        level < 0 || level > 100)
        return 2;
    int mib[16];
    size_t count = 16;
    if (sysctlnametomib("kern.memorystatus_level", mib, &count) != 0)
        return 3;
    int via_mib = -1;
    size = sizeof(via_mib);
    if (sysctl(mib, count, &via_mib, &size, NULL, 0) != 0 || via_mib < 0 || via_mib > 100)
        return 4;
    size = 1;
    if (sysctlbyname("kern.memorystatus_level", &level, &size, NULL, 0) != -1 || errno != EINVAL)
        return 5;
    size = sizeof(int);
    if (sysctlbyname("kern.memorystatus_level", &level, &size, &level, sizeof(level)) != -1 || errno != EPERM)
        return 6;
    printf("PASS: available=%d%%; sizing, numeric lookup, short buffer and denied writes\n", level);
    return 0;
}
