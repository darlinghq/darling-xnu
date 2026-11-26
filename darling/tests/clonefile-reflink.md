# clonefile reflink passthrough

Build `clonefile-reflink.c` as a Darwin executable against libSystem with the configured Darling toolchain and run it under Darling. No display is needed.

The test writes a source file, clones it with `clonefileat` and again with `fclonefileat`, and checks that each result is a different file with the same size, mode, mtime and contents. It also checks `EEXIST` for an existing destination, `ENOENT` for a missing source and that `CLONE_NOFOLLOW` refuses a symbolic link. Before the reflink change, `clonefileat` returned `ENOSYS` and the test failed at the first clone.

When the host filesystem cannot reflink, the test prints `SKIP` and exits 0: Darling maps the documented `EOPNOTSUPP` from the host rather than hiding it. `COPYFILE_CLONE` still falls back to a normal copy in `libcopyfile`; `COPYFILE_CLONE_FORCE` fails, as it should on such a filesystem.

Run it on a filesystem with reflink support (for example btrfs on the host) to exercise the passthrough. A successful run prints `PASS: clonefileat and fclonefileat use a host reflink`.
