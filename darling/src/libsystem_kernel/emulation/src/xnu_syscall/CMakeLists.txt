project(emulation_xnu_syscall)

cmake_policy(SET CMP0005 NEW)
enable_language(ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fPIC -fno-builtin -ggdb")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -nostdlib")

add_definitions(-DBSDTHREAD_WRAP_LINUX_PTHREAD
	-DEMULATED_SYSNAME="Darwin"
	-DEMULATED_RELEASE="20.6.0"
	-DEMULATED_VERSION="Darwin Kernel Version 20.6.0"
	-DEMULATED_OSVERSION="20G1120"
	-DEMULATED_OSPRODUCTVERSION="11.7.4"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_BINARY_DIR}/src/external/xnu/darling/src/libsystem_kernel/libsyscall
	${CMAKE_CURRENT_BINARY_DIR}
)

# mig(signal/mach_exc.defs)
# mig(signal/exc.defs)

set(bsd_sources
	bsd/bsd_syscall.c
	bsd/bsd_syscall_arch_asmbridge.S
	bsd/bsd_syscall_table.c

	bsd/impl/bsdthread/bsdthread_create.c
	bsd/impl/bsdthread/bsdthread_register.c
	bsd/impl/bsdthread/bsdthread_terminate.c
	bsd/impl/bsdthread/disable_threadsignal.c
	bsd/impl/bsdthread/pthread_canceled.c
	bsd/impl/bsdthread/pthread_kill.c
	bsd/impl/bsdthread/workq_kernreturn.c
	bsd/impl/conf/fpathconf.c
	bsd/impl/dirent/getdirentries.c
	bsd/impl/fcntl/fcntl.c
	bsd/impl/fcntl/open.c
	bsd/impl/fcntl/openat.c
	bsd/impl/hfs/stub.c
	bsd/impl/ioctl/ioctl.c
	bsd/impl/ioctl/ioctl-helper_filio.c
	bsd/impl/ioctl/ioctl-helper_socket.c
	bsd/impl/ioctl/ioctl-helper_termios.c
	bsd/impl/misc/abort_with_payload.c
	bsd/impl/misc/syscall.c
	bsd/impl/misc/thread_selfid.c
	bsd/impl/mman/mman.c
	bsd/impl/network/getsockopt.c
	bsd/impl/network/recvmsg.c
	bsd/impl/network/sendmsg.c
	bsd/impl/network/sendto.c
	bsd/impl/process/execve.c
	bsd/impl/process/fork.c
	bsd/impl/process/posix_spawn.c
	bsd/impl/signal/kill.c
	bsd/impl/signal/sigaction.c
	bsd/impl/signal/sigaltstack.c
	bsd/impl/signal/sigprocmask.c
	bsd/impl/time/gettimeofday.c
	bsd/impl/unistd/access.c
	bsd/impl/unistd/chdir.c
	bsd/impl/unistd/close.c
	bsd/impl/unistd/dup.c
	bsd/impl/unistd/dup2.c
	bsd/impl/unistd/exit.c
	bsd/impl/unistd/faccessat.c
	bsd/impl/unistd/fchdir.c
	bsd/impl/unistd/fchmodat.c
	bsd/impl/unistd/getgid.c
	bsd/impl/unistd/getpid.c
	bsd/impl/unistd/getuid.c
	bsd/impl/unistd/pipe.c
	bsd/impl/unistd/read.c
	bsd/impl/unistd/setgid.c
	bsd/impl/unistd/setpgid.c
	bsd/impl/unistd/setuid.c
	bsd/impl/unistd/write.c
	bsd/impl/wrapped/sem_close.c
	bsd/impl/wrapped/sem_open.c
	bsd/impl/wrapped/sem_post.c
	bsd/impl/wrapped/sem_trywait.c
	bsd/impl/wrapped/sem_unlink.c
	bsd/impl/wrapped/sem_wait.c
	bsd/impl/wrapped/shm_open.c
	bsd/impl/wrapped/shm_unlink.c
	bsd/impl/xattr/getattrlist.c
	bsd/impl/xattr/getattrlistat.c
	bsd/impl/xattr/setattrlist.c
	bsd/impl/xattr/setattrlistat.c
	bsd/impl/xattr/setxattr.c
)

set(mach_sources
	mach/mach_syscall_arch_asmbridge.S
	mach/mach_syscall_table.c
	mach/mach_syscall.c

	mach/impl/mach_other.c
	mach/impl/mach_time.c
	mach/impl/macx.c
	mach/impl/mk_timer.c
	mach/impl/trap/kernelrpc.c
	mach/impl/trap/semaphore.c
)

set(machdep_sources
	machdep/machdep_syscall_arch_asmbridge.S
	machdep/machdep_syscall_table.c
	machdep/machdep_syscall.c

	machdep/impl/tls.c
)

set(shared_sources
	shared/at.c
	shared/fdpath.c

	shared/guarded/table.c

	shared/unistd/fchmodat.c
	shared/unistd/uidgid.c
)

set(trace_sources
	trace/xtrace_api.c
	trace/xtrace_hook.S
)

set(emulation_sources
	${bsd_sources}
	${mach_sources}
	${machdep_sources}
	${shared_sources}
	${trace_sources}

	# ${CMAKE_CURRENT_BINARY_DIR}/signal/mach_excUser.c
	# readline.c
	# fdpath.c
	# wrapped/shm_unlink.c
	# guarded/guarded_open_np.c
	# guarded/guarded_close_np.c
	# guarded/guarded_kqueue_np.c
	# guarded/table.c
	# unistd/pread.c
	# unistd/pwrite.c
	# unistd/readv.c
	# unistd/writev.c
	# unistd/flock.c
	# unistd/initgroups.c
	# mount/unmount.c
	# mman/madvise.c
	# mman/msync.c
	# kqueue/kqueue.c
	# kqueue/kevent.c
	# kqueue/kevent64.c
	# kqueue/kevent_qos.c
	# unistd/getsid.c
	# unistd/fsync.c
	# unistd/sync.c
	# unistd/fdatasync.c
	# unistd/fchown.c
	# unistd/fchownat.c
	# unistd/fchmod.c
	# unistd/getegid.c
	# unistd/settid.c
	# unistd/setegid.c
	# unistd/seteuid.c
	# unistd/setsid.c
	# unistd/getuid.c
	# unistd/gettid.c
	# unistd/geteuid.c
	# unistd/lseek.c
	# unistd/ftruncate.c
	# unistd/truncate.c
	# unistd/readlink.c
	# unistd/readlinkat.c
	# unistd/symlink.c
	# unistd/symlinkat.c
	# unistd/link.c
	# unistd/linkat.c
	# unistd/unlink.c
	# unistd/unlinkat.c
	# unistd/mknod.c
	# unistd/chmod.c
	# unistd/chown.c
	# unistd/lchown.c
	# unistd/umask.c
	# unistd/chroot.c
	# unistd/getppid.c
	# unistd/rename.c
	# unistd/renameat.c
	# unistd/getpgrp.c
	# unistd/getdtablesize.c
	# unistd/setgroups.c
	# unistd/getgroups.c
	# unistd/getpgid.c
	# unistd/chmod_extended.c
	# unistd/fchmod_extended.c
	# unistd/fchflags.c
	# unistd/chflags.c
	# unistd/issetugid.c
	# select/select.c
	# select/pselect.c
	# select/poll.c
	# process/vfork.c
	# process/wait4.c
	# process/waitid.c
	# process/getpriority.c
	# process/setpriority.c
	# signal/duct_signals.c
	# signal/sigreturn.c
	# signal/sigsuspend.c
	# signal/sigpending.c
	# signal/sigwait.c
	# misc/ptrace.c
	# misc/getentropy.c
	# misc/shared_region_check_np.c
	# misc/proc_info.c
	# misc/sysctl.c
	# misc/sysctl_proc.c
	# misc/sysctl_hw.c
	# misc/sysctl_kern.c
	# misc/sysctl_unspec.c
	# misc/sysctl_machdep.c
	# misc/sysctl_sysctl.c
	# misc/sysctl_net.c
	# misc/sysctl_vm.c
	# misc/getrlimit.c
	# misc/setrlimit.c
	# misc/gethostuuid.c
	# misc/getrusage.c
	# misc/getlogin.c
	# misc/setlogin.c
	# misc/reboot.c
	# misc/iopolicysys.c
	# misc/csops.c
	# misc/fileport_makeport.c
	# misc/fileport_makefd.c
	# misc/csrctl.c
	# misc/fsgetpath.c
	# misc/abort_with_payload.c
	# misc/clonefile.c
	# network/socket.c
	# network/socketpair.c
	# network/connect.c
	# network/duct.c
	# network/recvfrom.c
	# network/accept.c
	# network/getpeername.c
	# network/getsockname.c
	# network/shutdown.c
	# network/setsockopt.c
	# network/bind.c
	# network/listen.c
	# stat/fstat.c
	# stat/fstatat.c
	# stat/lstat.c
	# stat/stat.c
	# stat/stat64_extended.c
	# stat/lstat64_extended.c
	# stat/fstat64_extended.c
	# stat/getfsstat.c
	# stat/statfs.c
	# stat/fstatfs.c
	# stat/mkdir.c
	# stat/mkdirat.c
	# stat/mkfifo.c
	# stat/rmdir.c
	# stat/common.c
	# xattr/getattrlistbulk.c
	# xattr/fgetattrlist.c
	# xattr/fsetattrlist.c
	# xattr/listxattr.c
	# xattr/flistxattr.c
	# xattr/removexattr.c
	# xattr/fremovexattr.c
	# xattr/getxattr.c
	# xattr/fgetxattr.c
	# xattr/fsetxattr.c
	# synch/semwait_signal.c
	# time/utimes.c
	# time/futimes.c
	# time/setitimer.c
	# time/getitimer.c
	# ext/sysinfo.c
	# ext/uname.c
	# ext/nanosleep.c
	# ext/inotify_rm_watch.c
	# ext/eventfd_read.c
	# ext/eventfd_write.c
	# ext/syslog.c
	# ext/for-xtrace.c
	# ext/for-libelfloader.c
	# bsdthread/bsdthread_register.c
	# bsdthread/bsdthread_ctl.c
	# bsdthread/workq_open.c
	# bsdthread/pthread_chdir.c
	# bsdthread/pthread_fchdir.c
	# bsdthread/pthread_markcancel.c
	# bsdthread/pthread_canceled.c
	# psynch/psynch_rw_rdlock.c
	# psynch/psynch_rw_wrlock.c
	# psynch/psynch_rw_unlock.c
	# psynch/psynch_cvclrprepost.c
	# psynch/psynch_mutexwait.c
	# psynch/psynch_mutexdrop.c
	# psynch/psynch_cvwait.c
	# psynch/psynch_cvbroad.c
	# psynch/psynch_cvsignal.c
	# psynch/ulock_wait.c
	# psynch/ulock_wake.c
	# sysv_sem/semget.c
	# sysv_sem/semctl.c
	# sysv_sem/semop.c
	# conf/pathconf.c
	# audit/audit_session_self.c
	# audit/audit_addr.c
	# audit/audit_session_join.c
	# audit/audit_session_port.c
	# syscalls-table.S
	# linux-syscall.S
)
	
# set_source_files_properties(signal/duct_signals.c PROPERTIES COMPILE_FLAGS "-nostdinc")
# set_source_files_properties(signal/sigexc.c PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/signal/exc.h)

add_darling_object_library(emulation_xnu_syscall ${emulation_sources})
add_dependencies(emulation_xnu_syscall rtsig_h generate_dserver_rpc_wrappers)
make_fat(emulation_xnu_syscall)

add_darling_object_library(emulation_xnu_syscall_dyld ${emulation_sources})
set_target_properties(emulation_xnu_syscall_dyld PROPERTIES COMPILE_FLAGS "-ffunction-sections -DVARIANT_DYLD")
add_dependencies(emulation_xnu_syscall_dyld rtsig_h generate_dserver_rpc_wrappers)
make_fat(emulation_xnu_syscall_dyld)

