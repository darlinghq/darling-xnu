# vchroot: openat2 fast path against the component walk

`vchroot-fastpath-equiv.c` is a host test of `vchroot_expand()` in `vchroot_userspace.c`. The function resolves a
guest path to a Linux path in one of two ways: the fast path (one `openat2(RESOLVE_IN_ROOT)` on a descriptor of the
prefix, or `RESOLVE_NO_SYMLINKS` for paths below `/Volumes/SystemRoot`) and the component walk (an `lstat` per
component, which also does the case-insensitive lookup). The test runs the same inputs through both and requires the
same result. It includes `vchroot_userspace.c` and `name_equivalence.c` directly, so it runs on the host, with
`vchroot_expand_ex(args, false, ...)` forcing the walk. Build it with the SDK-less include shim and run it in a
private user namespace (the mounts need one):

    T=$(mktemp -d) && mkdir -p $T/inc/darling && ln -s $PWD/darling/src/libsystem_kernel/emulation/include $T/inc/darling/emulation
    cc -g -O1 -Wall -I$T/inc -o $T/equiv darling/tests/vchroot-fastpath-equiv.c
    mkdir $T/work && unshare -Urmpf $T/equiv --overlay --proc --dev $T/work   # procfs mounted on the prefix's /proc
    mkdir $T/work2 && unshare -Urm $T/equiv --overlay --proc-link $T/work2    # /proc a link to the host's (no PID namespace)
    mkdir $T/work3 && $T/equiv $T/work3                                       # a plain directory as the prefix

The prefix is a tree built in a scratch directory, optionally an overlay mount of a lower and an upper layer (upper
entries over lower ones, a whiteout, another spelling of a lower name), with a procfs, a link to `/proc` or a bind
mount of `/dev` below it. About 1100 cases are resolved: each path with and without following the last link, from absolute
paths and from relative paths with the working directory and with a descriptor inside and outside the prefix. They
cover:
- symbolic links inside the prefix (relative, absolute, chains, loops, dangling, to a directory with a trailing
  slash), absolute links that must stay inside the prefix, and links that climb above its root;
- `..` and `.` at and below the root, `//`, trailing slashes, paths of 3900 and 4200 bytes, 255-byte names, 300-byte
  names;
- missing components (last and middle), a file used as a directory;
- another spelling of a name (ASCII case, the walk's case-insensitive lookup) at every depth, and of `/Volumes/SystemRoot`;
- `/Volumes/SystemRoot/...` into a tree outside the prefix: plain, through relative and absolute links, `..`, and a
  nested `/Volumes/SystemRoot`;
- `/proc` and `/dev`: `/proc/self/...`, `/proc/<pid>/<file>` for stat, cmdline, status, statm, comm, environ and auxv
  with the file of the procfs mounted on the prefix, or the host's file when the prefix's `/proc` is a link,
  `/dev/null`, `/dev/stdin`;
- the `/.nofollow/` prefix.

It also requires the fast path to answer the common cases (an existing deep path, a path through a link, a path
outside the prefix); otherwise the comparison would prove nothing.

The one difference allowed is the number of symbolic links followed in a row: the walk gives up on the 32nd, as macOS
does (`MAXSYMLINKS` is 32, a chain of 31 resolves; observed on macOS 26.7.3), the kernel follows 40, so a chain of 32
to 40 resolves through the fast path. The test lists such cases as notes.

Expected output: `PASS vchroot fast path equivalence [<layout>]` for each layout.
