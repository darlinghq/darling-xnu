project(emulation)

cmake_policy(SET CMP0005 NEW)
enable_language(ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fvisibility=hidden -fPIC -fno-builtin -ggdb -Wno-int-conversion -Wno-compare-distinct-pointer-types")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -nostdlib")

add_definitions(-DBSDTHREAD_WRAP_LINUX_PTHREAD
	-DEMULATED_SYSNAME="Darwin"
	-DEMULATED_RELEASE="20.6.0"
	-DEMULATED_VERSION="Darwin Kernel Version 20.6.0"
	-DEMULATED_OSVERSION="20G1120"
	-DEMULATED_OSPRODUCTVERSION="11.7.4"
)

# include src/startup for rtsig.h
include_directories(
	${CMAKE_BINARY_DIR}/src/startup
	${CMAKE_BINARY_DIR}/src/external/xnu/darling/src/libsystem_kernel/libsyscall
	${CMAKE_CURRENT_BINARY_DIR}/src/other
	${CMAKE_BINARY_DIR}/src/external/darlingserver/include
	${CMAKE_SOURCE_DIR}/src/external/darlingserver/include
)

mig(src/other/signal/mach_exc.defs)
mig(src/other/signal/exc.defs)

set(other_sources
	src/other/mach/lkm.c

	${CMAKE_CURRENT_BINARY_DIR}/src/other/signal/mach_excUser.c
)

set(common_sources
	src/common/guarded/table.c
	src/common/base.c
	src/common/simple.c
)

set(conversion_sources
	src/conversion/fcntl/open.c
	src/conversion/network/duct.c
	src/conversion/network/getsockopt.c
	src/conversion/network/recvmsg.c
	src/conversion/process/wait4.c
	src/conversion/signal/duct_signals.c
	src/conversion/signal/sigaction.c
	src/conversion/stat/common.c
	src/conversion/errno.c
)

set(linux_premigration_sources
	src/linux_premigration/ext/clock_gettime.c
	src/linux_premigration/ext/clock_nanosleep.c
	src/linux_premigration/ext/epoll_create.c
	src/linux_premigration/ext/epoll_create1.c
	src/linux_premigration/ext/epoll_ctl.c
	src/linux_premigration/ext/epoll_wait.c
	src/linux_premigration/ext/eventfd.c
	src/linux_premigration/ext/eventfd_read.c
	src/linux_premigration/ext/eventfd_write.c
	src/linux_premigration/ext/fanotify.c
	src/linux_premigration/ext/file_handle.c
	src/linux_premigration/ext/for-libelfloader.c
	src/linux_premigration/ext/for-libkqueue.c
	src/linux_premigration/ext/for-xtrace.c
	src/linux_premigration/ext/futex.c
	src/linux_premigration/ext/inotify_add_watch.c
	src/linux_premigration/ext/inotify_init.c
	src/linux_premigration/ext/inotify_init1.c
	src/linux_premigration/ext/inotify_rm_watch.c
	src/linux_premigration/ext/mremap.c
	src/linux_premigration/ext/nanosleep.c
	src/linux_premigration/ext/sched_yield.c
	src/linux_premigration/ext/signalfd.c
	src/linux_premigration/ext/sysinfo.c
	src/linux_premigration/ext/syslog.c
	src/linux_premigration/ext/timerfd_create.c
	src/linux_premigration/ext/timerfd_gettime.c
	src/linux_premigration/ext/timerfd_settime.c
	src/linux_premigration/ext/uname.c
	src/linux_premigration/misc/ioctl.c
	src/linux_premigration/resources/dserver-rpc-defs.c
	src/linux_premigration/signal/sig_restorer.S
	src/linux_premigration/signal/sigexc.c
	src/linux_premigration/elfcalls_wrapper.c
	src/linux_premigration/linux-syscall.S
	src/linux_premigration/vchroot_userspace.c
)

set(xnu_syscall_bsd_sources
	src/xnu_syscall/bsd/bsd_syscall_table.c
	src/xnu_syscall/bsd/bsd_syscall.S

	src/xnu_syscall/bsd/helper/ioctl/termios.c
	src/xnu_syscall/bsd/helper/ioctl/filio.c
	src/xnu_syscall/bsd/helper/ioctl/socket.c
	src/xnu_syscall/bsd/helper/misc/common_at.c
	src/xnu_syscall/bsd/helper/misc/readline.c
	src/xnu_syscall/bsd/helper/misc/fdpath.c
	src/xnu_syscall/bsd/helper/misc/sysctl_hw.c
	src/xnu_syscall/bsd/helper/misc/sysctl_kern.c
	src/xnu_syscall/bsd/helper/misc/sysctl_machdep.c
	src/xnu_syscall/bsd/helper/misc/sysctl_net.c
	src/xnu_syscall/bsd/helper/misc/sysctl_proc.c
	src/xnu_syscall/bsd/helper/misc/sysctl_sysctl.c
	src/xnu_syscall/bsd/helper/misc/sysctl_unspec.c
	src/xnu_syscall/bsd/helper/misc/sysctl_vm.c
	src/xnu_syscall/bsd/helper/network/duct.c

	src/xnu_syscall/bsd/impl/audit/audit_addr.c
	src/xnu_syscall/bsd/impl/audit/audit_session_join.c
	src/xnu_syscall/bsd/impl/audit/audit_session_port.c
	src/xnu_syscall/bsd/impl/audit/audit_session_self.c
	src/xnu_syscall/bsd/impl/bsdthread/bsdthread_create.c
	src/xnu_syscall/bsd/impl/bsdthread/bsdthread_ctl.c
	src/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.c
	src/xnu_syscall/bsd/impl/bsdthread/bsdthread_terminate.c
	src/xnu_syscall/bsd/impl/bsdthread/disable_threadsignal.c
	src/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.c
	src/xnu_syscall/bsd/impl/bsdthread/pthread_chdir.c
	src/xnu_syscall/bsd/impl/bsdthread/pthread_fchdir.c
	src/xnu_syscall/bsd/impl/bsdthread/pthread_kill.c
	src/xnu_syscall/bsd/impl/bsdthread/pthread_markcancel.c
	src/xnu_syscall/bsd/impl/bsdthread/workq_kernreturn.c
	src/xnu_syscall/bsd/impl/bsdthread/workq_open.c
	src/xnu_syscall/bsd/impl/conf/fpathconf.c
	src/xnu_syscall/bsd/impl/conf/pathconf.c
	src/xnu_syscall/bsd/impl/dirent/getdirentries.c
	src/xnu_syscall/bsd/impl/fcntl/fcntl.c
	src/xnu_syscall/bsd/impl/fcntl/open.c
	src/xnu_syscall/bsd/impl/fcntl/openat.c
	src/xnu_syscall/bsd/impl/guarded/guarded_close_np.c
	src/xnu_syscall/bsd/impl/guarded/guarded_kqueue_np.c
	src/xnu_syscall/bsd/impl/guarded/guarded_open_np.c
	src/xnu_syscall/bsd/impl/hfs/stub.c
	src/xnu_syscall/bsd/impl/ioctl/ioctl.c
	src/xnu_syscall/bsd/impl/kqueue/kevent.c
	src/xnu_syscall/bsd/impl/kqueue/kevent64.c
	src/xnu_syscall/bsd/impl/kqueue/kevent_qos.c
	src/xnu_syscall/bsd/impl/kqueue/kqueue.c
	src/xnu_syscall/bsd/impl/misc/terminate_with_payload.c
	src/xnu_syscall/bsd/impl/misc/abort_with_payload.c
	src/xnu_syscall/bsd/impl/misc/clonefile.c
	src/xnu_syscall/bsd/impl/misc/csops.c
	src/xnu_syscall/bsd/impl/misc/csrctl.c
	src/xnu_syscall/bsd/impl/misc/fileport_makefd.c
	src/xnu_syscall/bsd/impl/misc/fileport_makeport.c
	src/xnu_syscall/bsd/impl/misc/fsgetpath.c
	src/xnu_syscall/bsd/impl/misc/getentropy.c
	src/xnu_syscall/bsd/impl/misc/gethostuuid.c
	src/xnu_syscall/bsd/impl/misc/getlogin.c
	src/xnu_syscall/bsd/impl/misc/getrlimit.c
	src/xnu_syscall/bsd/impl/misc/getrusage.c
	src/xnu_syscall/bsd/impl/misc/iopolicysys.c
	src/xnu_syscall/bsd/impl/misc/proc_info.c
	src/xnu_syscall/bsd/impl/misc/ptrace.c
	src/xnu_syscall/bsd/impl/misc/reboot.c
	src/xnu_syscall/bsd/impl/misc/setlogin.c
	src/xnu_syscall/bsd/impl/misc/setrlimit.c
	src/xnu_syscall/bsd/impl/misc/shared_region_check_np.c
	src/xnu_syscall/bsd/impl/misc/syscall.c
	src/xnu_syscall/bsd/impl/misc/sysctl.c
	src/xnu_syscall/bsd/impl/misc/thread_selfid.c
	src/xnu_syscall/bsd/impl/mman/madvise.c
	src/xnu_syscall/bsd/impl/mman/mman.c
	src/xnu_syscall/bsd/impl/mman/msync.c
	src/xnu_syscall/bsd/impl/mount/unmount.c
	src/xnu_syscall/bsd/impl/network/accept.c
	src/xnu_syscall/bsd/impl/network/bind.c
	src/xnu_syscall/bsd/impl/network/connect.c
	src/xnu_syscall/bsd/impl/network/getpeername.c
	src/xnu_syscall/bsd/impl/network/getsockname.c
	src/xnu_syscall/bsd/impl/network/getsockopt.c
	src/xnu_syscall/bsd/impl/network/listen.c
	src/xnu_syscall/bsd/impl/network/recvfrom.c
	src/xnu_syscall/bsd/impl/network/recvmsg.c
	src/xnu_syscall/bsd/impl/network/sendmsg.c
	src/xnu_syscall/bsd/impl/network/sendto.c
	src/xnu_syscall/bsd/impl/network/setsockopt.c
	src/xnu_syscall/bsd/impl/network/shutdown.c
	src/xnu_syscall/bsd/impl/network/socket.c
	src/xnu_syscall/bsd/impl/network/socketpair.c
	src/xnu_syscall/bsd/impl/process/execve.c
	src/xnu_syscall/bsd/impl/process/fork.c
	src/xnu_syscall/bsd/impl/process/getpriority.c
	src/xnu_syscall/bsd/impl/process/posix_spawn.c
	src/xnu_syscall/bsd/impl/process/setpriority.c
	src/xnu_syscall/bsd/impl/process/vfork.c
	src/xnu_syscall/bsd/impl/process/wait4.c
	src/xnu_syscall/bsd/impl/process/waitid.c
	src/xnu_syscall/bsd/impl/psynch/psynch_cvbroad.c
	src/xnu_syscall/bsd/impl/psynch/psynch_cvclrprepost.c
	src/xnu_syscall/bsd/impl/psynch/psynch_cvsignal.c
	src/xnu_syscall/bsd/impl/psynch/psynch_cvwait.c
	src/xnu_syscall/bsd/impl/psynch/psynch_mutexdrop.c
	src/xnu_syscall/bsd/impl/psynch/psynch_mutexwait.c
	src/xnu_syscall/bsd/impl/psynch/psynch_rw_rdlock.c
	src/xnu_syscall/bsd/impl/psynch/psynch_rw_unlock.c
	src/xnu_syscall/bsd/impl/psynch/psynch_rw_wrlock.c
	src/xnu_syscall/bsd/impl/psynch/ulock_wait.c
	src/xnu_syscall/bsd/impl/psynch/ulock_wake.c
	src/xnu_syscall/bsd/impl/select/poll.c
	src/xnu_syscall/bsd/impl/select/pselect.c
	src/xnu_syscall/bsd/impl/select/select.c
	src/xnu_syscall/bsd/impl/signal/kill.c
	src/xnu_syscall/bsd/impl/signal/sigaction.c
	src/xnu_syscall/bsd/impl/signal/sigaltstack.c
	src/xnu_syscall/bsd/impl/signal/sigpending.c
	src/xnu_syscall/bsd/impl/signal/sigprocmask.c
	src/xnu_syscall/bsd/impl/signal/sigreturn.c
	src/xnu_syscall/bsd/impl/signal/sigsuspend.c
	src/xnu_syscall/bsd/impl/signal/sigwait.c
	src/xnu_syscall/bsd/impl/stat/fstat.c
	src/xnu_syscall/bsd/impl/stat/fstat64_extended.c
	src/xnu_syscall/bsd/impl/stat/fstatat.c
	src/xnu_syscall/bsd/impl/stat/fstatfs.c
	src/xnu_syscall/bsd/impl/stat/getfsstat.c
	src/xnu_syscall/bsd/impl/stat/lstat.c
	src/xnu_syscall/bsd/impl/stat/lstat64_extended.c
	src/xnu_syscall/bsd/impl/stat/mkdir.c
	src/xnu_syscall/bsd/impl/stat/mkdirat.c
	src/xnu_syscall/bsd/impl/stat/mkfifo.c
	src/xnu_syscall/bsd/impl/stat/rmdir.c
	src/xnu_syscall/bsd/impl/stat/stat.c
	src/xnu_syscall/bsd/impl/stat/stat64_extended.c
	src/xnu_syscall/bsd/impl/stat/statfs.c
	src/xnu_syscall/bsd/impl/synch/semwait_signal.c
	src/xnu_syscall/bsd/impl/sysv_sem/semctl.c
	src/xnu_syscall/bsd/impl/sysv_sem/semget.c
	src/xnu_syscall/bsd/impl/sysv_sem/semop.c
	src/xnu_syscall/bsd/impl/time/futimes.c
	src/xnu_syscall/bsd/impl/time/getitimer.c
	src/xnu_syscall/bsd/impl/time/gettimeofday.c
	src/xnu_syscall/bsd/impl/time/setitimer.c
	src/xnu_syscall/bsd/impl/time/utimes.c
	src/xnu_syscall/bsd/impl/unistd/access.c
	src/xnu_syscall/bsd/impl/unistd/chdir.c
	src/xnu_syscall/bsd/impl/unistd/chflags.c
	src/xnu_syscall/bsd/impl/unistd/chmod.c
	src/xnu_syscall/bsd/impl/unistd/chmod_extended.c
	src/xnu_syscall/bsd/impl/unistd/chown.c
	src/xnu_syscall/bsd/impl/unistd/chroot.c
	src/xnu_syscall/bsd/impl/unistd/close.c
	src/xnu_syscall/bsd/impl/unistd/dup.c
	src/xnu_syscall/bsd/impl/unistd/dup2.c
	src/xnu_syscall/bsd/impl/unistd/exit.c
	src/xnu_syscall/bsd/impl/unistd/faccessat.c
	src/xnu_syscall/bsd/impl/unistd/fchdir.c
	src/xnu_syscall/bsd/impl/unistd/fchflags.c
	src/xnu_syscall/bsd/impl/unistd/fchmod.c
	src/xnu_syscall/bsd/impl/unistd/fchmodat.c
	src/xnu_syscall/bsd/impl/unistd/fchmod_extended.c
	src/xnu_syscall/bsd/impl/unistd/fchown.c
	src/xnu_syscall/bsd/impl/unistd/fchownat.c
	src/xnu_syscall/bsd/impl/unistd/fdatasync.c
	src/xnu_syscall/bsd/impl/unistd/flock.c
	src/xnu_syscall/bsd/impl/unistd/fsync.c
	src/xnu_syscall/bsd/impl/unistd/ftruncate.c
	src/xnu_syscall/bsd/impl/unistd/getdtablesize.c
	src/xnu_syscall/bsd/impl/unistd/getegid.c
	src/xnu_syscall/bsd/impl/unistd/geteuid.c
	src/xnu_syscall/bsd/impl/unistd/getgid.c
	src/xnu_syscall/bsd/impl/unistd/getgroups.c
	src/xnu_syscall/bsd/impl/unistd/getpgid.c
	src/xnu_syscall/bsd/impl/unistd/getpgrp.c
	src/xnu_syscall/bsd/impl/unistd/getpid.c
	src/xnu_syscall/bsd/impl/unistd/getppid.c
	src/xnu_syscall/bsd/impl/unistd/getsid.c
	src/xnu_syscall/bsd/impl/unistd/gettid.c
	src/xnu_syscall/bsd/impl/unistd/getuid.c
	src/xnu_syscall/bsd/impl/unistd/initgroups.c
	src/xnu_syscall/bsd/impl/unistd/issetugid.c
	src/xnu_syscall/bsd/impl/unistd/lchown.c
	src/xnu_syscall/bsd/impl/unistd/link.c
	src/xnu_syscall/bsd/impl/unistd/linkat.c
	src/xnu_syscall/bsd/impl/unistd/lseek.c
	src/xnu_syscall/bsd/impl/unistd/mknod.c
	src/xnu_syscall/bsd/impl/unistd/pipe.c
	src/xnu_syscall/bsd/impl/unistd/pread.c
	src/xnu_syscall/bsd/impl/unistd/pwrite.c
	src/xnu_syscall/bsd/impl/unistd/read.c
	src/xnu_syscall/bsd/impl/unistd/readlink.c
	src/xnu_syscall/bsd/impl/unistd/readlinkat.c
	src/xnu_syscall/bsd/impl/unistd/readv.c
	src/xnu_syscall/bsd/impl/unistd/rename.c
	src/xnu_syscall/bsd/impl/unistd/renameat.c
	src/xnu_syscall/bsd/impl/unistd/setegid.c
	src/xnu_syscall/bsd/impl/unistd/seteuid.c
	src/xnu_syscall/bsd/impl/unistd/setgid.c
	src/xnu_syscall/bsd/impl/unistd/setgroups.c
	src/xnu_syscall/bsd/impl/unistd/setpgid.c
	src/xnu_syscall/bsd/impl/unistd/setsid.c
	src/xnu_syscall/bsd/impl/unistd/settid.c
	src/xnu_syscall/bsd/impl/unistd/setuid.c
	src/xnu_syscall/bsd/impl/unistd/symlink.c
	src/xnu_syscall/bsd/impl/unistd/symlinkat.c
	src/xnu_syscall/bsd/impl/unistd/sync.c
	src/xnu_syscall/bsd/impl/unistd/truncate.c
	src/xnu_syscall/bsd/impl/unistd/umask.c
	src/xnu_syscall/bsd/impl/unistd/unlink.c
	src/xnu_syscall/bsd/impl/unistd/unlinkat.c
	src/xnu_syscall/bsd/impl/unistd/write.c
	src/xnu_syscall/bsd/impl/unistd/writev.c
	src/xnu_syscall/bsd/impl/wrapped/sem_close.c
	src/xnu_syscall/bsd/impl/wrapped/sem_open.c
	src/xnu_syscall/bsd/impl/wrapped/sem_post.c
	src/xnu_syscall/bsd/impl/wrapped/sem_trywait.c
	src/xnu_syscall/bsd/impl/wrapped/sem_unlink.c
	src/xnu_syscall/bsd/impl/wrapped/sem_wait.c
	src/xnu_syscall/bsd/impl/wrapped/shm_open.c
	src/xnu_syscall/bsd/impl/wrapped/shm_unlink.c
	src/xnu_syscall/bsd/impl/xattr/fgetattrlist.c
	src/xnu_syscall/bsd/impl/xattr/fgetxattr.c
	src/xnu_syscall/bsd/impl/xattr/flistxattr.c
	src/xnu_syscall/bsd/impl/xattr/fremovexattr.c
	src/xnu_syscall/bsd/impl/xattr/fsetattrlist.c
	src/xnu_syscall/bsd/impl/xattr/fsetxattr.c
	src/xnu_syscall/bsd/impl/xattr/getattrlist.c
	src/xnu_syscall/bsd/impl/xattr/getattrlistat.c
	src/xnu_syscall/bsd/impl/xattr/getattrlistbulk.c
	src/xnu_syscall/bsd/impl/xattr/getxattr.c
	src/xnu_syscall/bsd/impl/xattr/listxattr.c
	src/xnu_syscall/bsd/impl/xattr/removexattr.c
	src/xnu_syscall/bsd/impl/xattr/setattrlist.c
	src/xnu_syscall/bsd/impl/xattr/setattrlistat.c
	src/xnu_syscall/bsd/impl/xattr/setxattr.c
)

set(xnu_syscall_mach_sources
	src/xnu_syscall/mach/mach_syscall_table.c
	src/xnu_syscall/mach/mach_syscall.S

	src/xnu_syscall/mach/impl/mach_time.c
	src/xnu_syscall/mach/impl/mach_traps.c
)

set(xnu_syscall_machdep_sources
	src/xnu_syscall/machdep/machdep_syscall_table.c
	src/xnu_syscall/machdep/machdep_syscall.S

	src/xnu_syscall/machdep/impl/tls.c
)

set(xnu_syscall_sources
	src/xnu_syscall/xtrace-hooks.S
	${xnu_syscall_bsd_sources}
	${xnu_syscall_mach_sources}
	${xnu_syscall_machdep_sources}
)

set(emulation_sources
	${common_sources}
	${conversion_sources}
	${linux_premigration_sources}
	${other_sources}
	${xnu_syscall_sources}

	${CMAKE_BINARY_DIR}/src/external/darlingserver/src/rpc.c
)

set_source_files_properties(src/legacy_path/signal/duct_signals.c PROPERTIES COMPILE_FLAGS "-nostdinc")
set_source_files_properties(src/legacy_path/signal/sigexc.c PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/src/legacy_path/signal/exc.h)
set_source_files_properties(${CMAKE_BINARY_DIR}/src/external/darlingserver/src/rpc.c PROPERTIES
	GENERATED TRUE
	COMPILE_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/include/linux_premigration/resources/dserver-rpc-defs.h"
)

include_directories(
	${CMAKE_SOURCE_DIR}/src/libsimple/include
)

add_darling_object_library(emulation ${emulation_sources})
add_darling_object_library(emulation_dyld ${emulation_sources})
set_target_properties(emulation_dyld PROPERTIES COMPILE_FLAGS "-ffunction-sections -DVARIANT_DYLD")
add_dependencies(emulation rtsig_h generate_dserver_rpc_wrappers)
add_dependencies(emulation_dyld rtsig_h generate_dserver_rpc_wrappers)

make_fat(emulation)
make_fat(emulation_dyld)
