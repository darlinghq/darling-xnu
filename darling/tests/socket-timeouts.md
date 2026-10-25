# Socket timeout ABI regression

Compile `socket-timeouts.c` as a Darwin executable linked to the configured
Darling libSystem, then run with `darling shell /path/to/socket-timeouts`.
Compare the original kernel library with the rebuilt library via a test-only
`DYLD_LIBRARY_PATH`; no installation is required.

On ARM64 Asahi the baseline exits1: SO_SNDTIMEO with valid fields and poisoned
padding fails EDOM, while SO_RCVTIMEO succeeds. With the change it exits0.
Controls cover zero/valid fields with different padding, option round trips,
short/negative lengths, null pointers, negative seconds/microseconds and
microseconds >=1000000. A full local socketpair send buffer then demonstrates
that a real100ms send timeout takes effect (observed106ms), rather than merely
returning success. No network server or guest application is required.

Specification: published/vendored `bsd/kern/uipc_socket.c`'s
`sooptcopyin_timeval` and SO_SNDTIMEO/SO_RCVTIMEO cases (rung1); existing
`conversion/time/gettimeofday.h` declarations and authored API controls
(rungs2/4). Darwin's32-bit tv_usec plus padding must become Linux's long
microsecond field. Validate length/pointer/domain before reading the input.
The legacy internal seconds member is unsigned; interpret its bits as signed
long for the documented negative-seconds rejection.
