#include "../src/libsystem_kernel/emulation/src/xnu_syscall/bsd/helper/misc/proc_boottime.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  const char *bad[] = {"",
                       "0",
                       "-1",
                       "+1",
                       " 1",
                       "123bad",
                       "12 3",
                       "18446744073709551616",
                       "99999999999999999999999999"};
  unsigned long value = 0;
  if (!parse_proc_boottime("1700000000", &value) || value != 1700000000UL)
    return 1;
  if (!parse_proc_boottime("123 \t", &value) || value != 123)
    return 1;
  char limit[64];
  snprintf(limit, sizeof(limit), "%ld", LONG_MAX);
  if (!parse_proc_boottime(limit, &value) || value != LONG_MAX)
    return 1;
  snprintf(limit, sizeof(limit), "%lu", (unsigned long)LONG_MAX + 1UL);
  if (parse_proc_boottime(limit, &value))
    return 1;
  for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    value = 42;
    if (parse_proc_boottime(bad[i], &value) || value != 42) {
      fprintf(stderr, "malformed input accepted or output modified at %u\n", i);
      return 1;
    }
  }
  puts("PASS boot timestamp parser: valid, maximum, malformed and overflow");
  return 0;
}
