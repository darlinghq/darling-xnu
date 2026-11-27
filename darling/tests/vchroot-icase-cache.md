# Case-insensitive path lookup follows directory changes

`vchroot-icase-cache.c` is a guest test of the case-insensitive lookup in `vchroot_userspace.c`: a path component that
does not exist as spelled is matched against the directory's entries ignoring ASCII case and canonical
normalization (see `name-equivalence.md`). Whatever the lookup remembers between calls, every result must agree with
the directory as it is at the time of the call.

Build it as a Darwin executable (x86_64 and i386) against libSystem and run it in the guest:

    clang -target x86_64-apple-macos11 -D__DARWIN_ONLY_UNIX_CONFORMANCE=1 -o vchroot-icase-cache darling/tests/vchroot-icase-cache.c
    ./vchroot-icase-cache

The test works in a new directory under `/tmp`, and covers:
- another spelling of an existing name finds it, repeatedly, also after the directory's modification time is moved
  into the past;
- a name that does not exist stays missing, and is found as soon as it is created in any case;
- rename and unlink of a name, and replacing it by another spelling of the same name 20 times in a row;
- a decomposed spelling of a precomposed name;
- 3000 names, each looked up in another case twice;
- a forked child's lookups and the parent's lookups of entries the child created;
- three threads looking up a name while the main thread creates and removes others.

Expected output: `PASS vchroot icase cache`.
