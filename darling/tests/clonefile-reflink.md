# clonefile reflink passthrough

Build `clonefile-reflink.c` as a Darwin executable against libSystem with the configured Darling toolchain and run it under Darling. No display is needed.

The checks that do not need a reflink always run: an existing destination is `EEXIST` and is not touched, a missing source or an empty path is `ENOENT`, an undefined flag bit is `EINVAL` and creates nothing, a FIFO or a device is `EINVAL` (and a FIFO source does not block), and `CLONE_NOFOLLOW` clones a symbolic link as a link (a dangling one too) while a link is followed without it. These are the results `clonefile(2)` gives on macOS 26.7.3 and 10.14.4; the program passes unchanged there.

On a filesystem that can reflink (for example btrfs or XFS on the host) the test also clones a file with `clonefileat` and `fclonefileat` and checks that each result is a different file with the same size, mode, atime, mtime and contents, including with a directory descriptor and for a read-only source. When the host filesystem cannot reflink, `clonefileat` fails with `ENOTSUP` (the documented result of `clonefile` on a filesystem without cloning), leaves no destination behind, and that part prints `SKIP`. `COPYFILE_CLONE` in `libcopyfile` still falls back to a normal copy; `COPYFILE_CLONE_FORCE` fails, as it should on such a filesystem.

A successful run prints `PASS: clonefileat and fclonefileat`.
