# Stable boot timestamp regression

Build `boottime-monotonic.c` as a Darwin executable against libSystem using the configured Darling toolchain. Run it under Darling for about four seconds. No AppKit, display or Roon is needed. It repeatedly queries kern.boottime and CLOCK_MONOTONIC, using CLOCK_UPTIME_RAW only for elapsed-time reporting. Clock/sysctl errors fail the fixture.

On the old implementation, mismatched whole-second boundaries in wall time and rounded Linux uptime repeatedly change the reported boot timestamp. Baseline observed eight changes and four backward monotonic jumps of about 0.98 seconds in 3.64 seconds, exit 1. With the fix, no changes/backward jumps in 3.65 seconds, exit 0. Avoid an administrator wall-clock adjustment while running this check. This fixes the rounding artifact, not every behavior of the Libc wall-clock-derived monotonic fallback.

Run `cc -Wall -Wextra -Werror darling/tests/proc-boottime-parser.c -o proc-boottime-parser` and execute it natively for valid/trailing-space, signed maximum, malformed and overflow cases. The output parameter must stay unchanged on rejected input.

Actual AppKit/CGL redraw and resize showed 1–2 second pacing stalls before and a worst observed frame interval of 21ms after; the user confirmed improved responsiveness in the unchanged Roon app before these regression tests were finalized.
