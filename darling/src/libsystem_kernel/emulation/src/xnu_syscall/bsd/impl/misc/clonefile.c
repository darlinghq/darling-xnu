#include <darling/emulation/xnu_syscall/bsd/impl/misc/clonefile.h>

#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/openat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkdirat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/futimes.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmod.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlinkat.h>

long sys_clonefileat(int src_fd, const char* src_path, int dest_fd, const char* dest_path, uint32_t flags) {
	return -ENOSYS;
}

long sys_fclonefileat(int src_fd, int dest_fd, const char* dest_path, uint32_t flags) {
#ifdef __i386__
    struct stat st;
    long fstat_ret = sys_fstat(src_fd, &st);
#else
    struct stat64 st;
    long fstat_ret = sys_fstat64(src_fd, &st);
#endif

    if (fstat_ret < 0) {
        return fstat_ret;
    }

    if (!S_ISREG(st.st_mode)) {
        return -EOPNOTSUPP;
    }

    if ((flags & XNU_CLONE_NOFOLLOW) && S_ISLNK(st.st_mode)) {
        return -ENOTSUP;
    }

    if (flags & XNU_CLONE_NOFOLLOW_ANY) {
#warning darling do not support XNU_CLONE_NOFOLLOW_ANY flag yet
    }

    if (flags & XNU_CLONE_ACL) {
#warning darling do not support XNU_CLONE_ACL flag yet
    }

    if (flags & XNU_CLONE_RESOLVE_BENEATH) {
#warning darling do not support XNU_CLONE_RESOLVE_BENEATH flag yet
    }

    int ret = 0;
    int real_dest_fd = -1;
    char *filename = NULL;

    if (dest_fd == BSD_AT_FDCWD) {
        if (!dest_path || dest_path[0] == '\0') {
            return -EINVAL;
        }

        char *path_buf = strdup(dest_path);

        if (!path_buf) {
            ret = -ENOMEM;
            goto out_close;
        }

        char *path = path_buf;

        if (path[0] == '/') {
            dest_fd = sys_open("/", BSD_O_RDONLY | BSD_O_DIRECTORY | BSD_O_CLOEXEC, 0);

            if (dest_fd < 0) {
                ret = dest_fd;
                goto out_close;
            }

            path++;
        } else {
            dest_fd = LINUX_AT_FDCWD;
        }

        char *token;
        int has_error = 0;

        while ((token = strsep(&path, "/")) != NULL) {
            if (*token == '\0') continue;

            int next_fd = sys_openat(dest_fd, token, BSD_O_RDONLY | BSD_O_DIRECTORY | BSD_O_CLOEXEC, st.st_mode & 0777);

            if (next_fd >= 0) {
                if (dest_fd != LINUX_AT_FDCWD) sys_close(dest_fd);

                dest_fd = next_fd;
                continue;
            }

            if (path == NULL) {
                if (*token == '\0') {
                    ret = -EINVAL;
                    free(path_buf);
                    goto out_close;
                }

                filename = token;
                free(path_buf);
                break;
            }

            if (next_fd < 0) {
                ret = next_fd;
                free(path_buf);
                goto out_close;
            }
        }
    } else {
        filename = dest_path;
    }

    real_dest_fd = sys_openat(dest_fd, filename, BSD_O_CREAT | BSD_O_WRONLY | BSD_O_EXCL, 0644);

    if (real_dest_fd < 0) {
        if (dest_fd >= 0) sys_close(dest_fd);

        return real_dest_fd;
    }

    if (st.st_size > 0) {
        size_t remaining = (size_t)st.st_size;

        while (remaining > 0) {
            size_t copied = LINUX_SYSCALL(__NR_copy_file_range, src_fd, NULL, real_dest_fd, NULL, remaining, 0);

            if (copied < 0) {
                ret = copied;
                goto out_close;
            }

            if (copied == 0) {
                break;
            }

            if (copied > remaining) {
                remaining = 0;
            } else {
                remaining -= copied;
            }
        }
    }

    if (!(flags & XNU_CLONE_NOOWNERCOPY)) {
        long current_uid = sys_getuid();

        if (current_uid != 0 && current_uid != st.st_uid) {
            ret = -EPERM;
            goto out_close;
        }

        long chown_ret = sys_fchown(real_dest_fd, st.st_uid, st.st_gid);

        if (chown_ret < 0) {
            ret = chown_ret;
            goto out_close;
        }
    }

    long chmod_ret = sys_fchmod(real_dest_fd, st.st_mode & 07777);

    if (chmod_ret < 0) {
        ret = chmod_ret;
        goto out_close;
    }

    // copy_file_range doesn't preserve timestamps, so we need to explicitly
    // set them to match the source file's atime and mtime
    struct timespec times[2];
    times[0] = st.st_atimespec;
    times[1] = st.st_mtimespec;
    struct bsd_timeval tv[2];
    tv[0] = (struct bsd_timeval){
        .tv_sec = times[0].tv_sec,
        .tv_usec = times[0].tv_nsec / 1000,
    };
    tv[1] = (struct bsd_timeval){
        .tv_sec = times[1].tv_sec,
        .tv_usec = times[1].tv_nsec / 1000,
    };
    long times_ret = sys_futimes(real_dest_fd, tv);

    if (times_ret < 0) {
        ret = times_ret;
        goto out_close;
    }

out_close:
    {
        if (real_dest_fd >= 0) {
            if (ret == 0) {
                long close_ret = sys_close(real_dest_fd);

                if (close_ret < 0) {
                    ret = close_ret;
                }
            } else {
                long unlinkat_ret = sys_unlinkat(dest_fd, filename, 0);

                if (unlinkat_ret < 0) {
                    ret = unlinkat_ret;
                }
            }
        }

        if (dest_fd >= 0) sys_close(dest_fd);
    }

    return ret;
}
