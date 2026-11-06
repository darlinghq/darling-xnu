# kevent_qos count handling

`kevent-qos-counts.c` builds the real `kevent_qos.c` on a Linux host with a mocked `sys_kevent64`, and runs it for 0, 1, 16, 17 and 100000 changes/events plus the WORKQ zero-event path and negative counts.

```
T=$(mktemp -d); mkdir -p $T/darling
ln -s $PWD/darling/src/libsystem_kernel/emulation/include $T/darling/emulation
clang -g -fsanitize=address,undefined -I$T darling/tests/kevent-qos-counts.c -o $T/kevent-qos-counts
(ulimit -s 8192; $T/kevent-qos-counts)
```

Before the fix the caller-sized `alloca` overflows the 8 MiB stack at 100000 entries (ASan stack-overflow, exit 141), and a negative count is not rejected. After, it prints `ok`.
