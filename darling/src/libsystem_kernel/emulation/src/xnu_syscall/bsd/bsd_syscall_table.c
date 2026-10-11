#include <darling/emulation/xnu_syscall/bsd/bsd_syscall_table.h>

#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_addr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_session_join.h>
#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_session_port.h>
#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_session_self.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_create.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_ctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_terminate.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/disable_threadsignal.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_chdir.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_fchdir.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_kill.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_markcancel.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/workq_kernreturn.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/workq_open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>
#include <darling/emulation/xnu_syscall/bsd/impl/conf/pathconf.h>
#include <darling/emulation/xnu_syscall/bsd/impl/dirent/getdirentries.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/openat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_close_np.h>
#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_kqueue_np.h>
#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_open_np.h>
#include <darling/emulation/xnu_syscall/bsd/impl/hfs/stub.h>
#include <darling/emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent64.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent_qos.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/terminate_with_payload.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/abort_with_payload.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/clonefile.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/csops.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/csrctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/fileport_makefd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/fileport_makeport.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/fsgetpath.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/getentropy.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/gethostuuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/getlogin.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrlimit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrusage.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/iopolicysys.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/proc_info.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/ptrace.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/reboot.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/setlogin.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/setrlimit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/shared_region_check_np.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/syscall.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/sysctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/thread_selfid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/madvise.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/msync.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mount/unmount.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/accept.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/bind.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/connect.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/getpeername.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockname.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/listen.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/recvfrom.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/recvmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendto.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/setsockopt.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/shutdown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/socket.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/socketpair.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/execve.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/fork.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/getpriority.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/posix_spawn.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/setpriority.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/vfork.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/wait4.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/waitid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_cvbroad.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_cvclrprepost.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_cvsignal.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_cvwait.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_mutexdrop.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_mutexwait.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_rw_rdlock.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_rw_unlock.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_rw_wrlock.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/ulock_wait.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/ulock_wake.h>
#include <darling/emulation/xnu_syscall/bsd/impl/select/poll.h>
#include <darling/emulation/xnu_syscall/bsd/impl/select/pselect.h>
#include <darling/emulation/xnu_syscall/bsd/impl/select/select.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaction.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigpending.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigprocmask.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigreturn.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigsuspend.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigwait.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat64_extended.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatfs.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/getfsstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/lstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/lstat64_extended.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkdir.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkdirat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkfifo.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/rmdir.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat64_extended.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/statfs.h>
#include <darling/emulation/xnu_syscall/bsd/impl/synch/semwait_signal.h>
#include <darling/emulation/xnu_syscall/bsd/impl/sysv_sem/semctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/sysv_sem/semget.h>
#include <darling/emulation/xnu_syscall/bsd/impl/sysv_sem/semop.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/futimes.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/getitimer.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/setitimer.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/utimes.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/access.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chdir.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chflags.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chmod.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chmod_extended.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chroot.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/dup.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/dup2.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/exit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/faccessat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchdir.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchflags.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmod.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmodat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmod_extended.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchownat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fdatasync.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/flock.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fsync.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/ftruncate.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getdtablesize.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getegid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/geteuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgroups.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpgid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpgrp.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getppid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getsid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/gettid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/initgroups.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/issetugid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/lchown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/link.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/linkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/lseek.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/mknod.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pipe.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pread.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pwrite.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlink.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readv.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/rename.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/renameat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setegid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/seteuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setgid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setgroups.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setpgid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setsid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/settid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/symlink.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/symlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/sync.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/truncate.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/umask.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlink.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/writev.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_post.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_trywait.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_unlink.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_wait.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/shm_open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/shm_unlink.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fgetattrlist.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fgetxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/flistxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fremovexattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fsetattrlist.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fsetxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistbulk.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/listxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/removexattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/setattrlist.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/setattrlistat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/setxattr.h>

void* __bsd_syscall_table[600] = {
	[0] = sys_syscall,
	[1] = sys_exit,
	[2] = sys_fork,
	[3] = sys_read,
	[4] = sys_write,
	[5] = sys_open,
	[6] = sys_close,
	[7] = sys_wait4,
	[9] = sys_link,
	[10] = sys_unlink,
	[12] = sys_chdir,
	[13] = sys_fchdir,
	[14] = sys_mknod,
	[15] = sys_chmod,
	[16] = sys_chown,
	[18] = sys_getfsstat,
	[20] = sys_getpid,
	[23] = sys_setuid,
	[24] = sys_getuid,
	[25] = sys_geteuid,
	[26] = sys_ptrace,
	[27] = sys_recvmsg,
	[28] = sys_sendmsg,
	[29] = sys_recvfrom,
	[30] = sys_accept,
	[31] = sys_getpeername,
	[32] = sys_getsockname,
	[33] = sys_access,
	[34] = sys_chflags,
	[35] = sys_fchflags,
	[36] = sys_sync,
	[37] = sys_kill,
	[39] = sys_getppid,
	[41] = sys_dup,
	[42] = sys_pipe,
	[43] = sys_getegid,
	[46] = sys_sigaction,
	[47] = sys_getgid,
	[48] = sys_sigprocmask,
	[49] = sys_getlogin,
	[50] = sys_setlogin,
	[52] = sys_sigpending,
	[53] = sys_sigaltstack,
	[54] = sys_ioctl,
	[55] = sys_reboot,
	[57] = sys_symlink,
	[58] = sys_readlink,
	[59] = sys_execve,
	[60] = sys_umask,
	[61] = sys_chroot,
	[65] = sys_msync,
	[66] = sys_vfork,
	[73] = sys_munmap,
	[74] = sys_mprotect,
	[75] = sys_madvise,
	[78] = sys_mincore,
	[79] = sys_getgroups,
	[80] = sys_setgroups,
	[81] = sys_getpgrp,
	[82] = sys_setpgid,
	[83] = sys_setitimer,
	[86] = sys_getitimer,
	[89] = sys_getdtablesize,
	[90] = sys_dup2,
	[92] = sys_fcntl,
	[93] = sys_select,
	[95] = sys_fsync,
	[96] = sys_setpriority,
	[97] = sys_socket,
	[98] = sys_connect,
	[100] = sys_getpriority,
	[104] = sys_bind,
	[105] = sys_setsockopt,
	[106] = sys_listen,
	[111] = sys_sigsuspend,
	[116] = sys_gettimeofday,
	[117] = sys_getrusage,
	[118] = sys_getsockopt,
	[120] = sys_readv,
	[121] = sys_writev,
	[123] = sys_fchown,
	[124] = sys_fchmod,
	[128] = sys_rename,
	[131] = sys_flock,
	[132] = sys_mkfifo,
	[133] = sys_sendto,
	[134] = sys_shutdown,
	[135] = sys_socketpair,
	[136] = sys_mkdir,
	[137] = sys_rmdir,
	[138] = sys_utimes,
	[139] = sys_futimes,
	[142] = sys_gethostuuid,
	[147] = sys_setsid,
	[151] = sys_getpgid,
	[153] = sys_pread,
	[154] = sys_pwrite,
	[157] = sys_statfs,
	[158] = sys_fstatfs,
	[159] = sys_unmount,
	[169] = sys_csops,
	[170] = sys_csops_audittoken,
	[173] = sys_waitid,
	[181] = sys_setgid,
	[182] = sys_setegid,
	[183] = sys_seteuid,
	[184] = sys_sigreturn,
	[187] = sys_fdatasync,
	[188] = sys_stat,
	[189] = sys_fstat,
	[190] = sys_lstat,
	[191] = sys_pathconf,
	[192] = sys_fpathconf,
	[194] = sys_getrlimit,
	[195] = sys_setrlimit,
	[196] = sys_getdirentries,
	[197] = sys_mmap,
	[199] = sys_lseek,
	[200] = sys_truncate,
	[201] = sys_ftruncate,
	[202] = sys_sysctl,
	[203] = sys_mlock,
	[204] = sys_munlock,
	[216] = sys_open_dprotected_np,
	[217] = sys_statv,
	[218] = sys_lstatv,
	[219] = sys_fstatv,
	[220] = sys_getattrlist,
	[221] = sys_setattrlist,
	[222] = sys_getdirentriesattr,
	[223] = sys_exchangedata,
	[225] = sys_searchfs,
	[226] = sys_delete,
	[227] = sys_copyfile,
	[228] = sys_fgetattrlist,
	[229] = sys_fsetattrlist,
	[230] = sys_poll,
	[234] = sys_getxattr,
	[235] = sys_fgetxattr,
	[236] = sys_setxattr,
	[237] = sys_fsetxattr,
	[238] = sys_removexattr,
	[239] = sys_fremovexattr,
	[240] = sys_listxattr,
	[241] = sys_flistxattr,
	[243] = sys_initgroups,
	[244] = sys_posix_spawn,
	[254] = sys_semctl,
	[255] = sys_semget,
	[256] = sys_semop,
	[266] = sys_shm_open,
	[267] = sys_shm_unlink,
	[268] = sys_sem_open,
	[269] = sys_sem_close,
	[270] = sys_sem_unlink,
	[271] = sys_sem_wait,
	[272] = sys_sem_trywait,
	[273] = sys_sem_post,
	[274] = sys_sysctlbyname,
	[282] = sys_chmod_extended,
	[283] = sys_fchmod_extended,
	[285] = sys_settid,
	[286] = sys_gettid,
	[294] = sys_shared_region_check_np,
	[301] = sys_psynch_mutexwait,
	[302] = sys_psynch_mutexdrop,
	[303] = sys_psynch_cvbroad,
	[304] = sys_psynch_cvsignal,
	[305] = sys_psynch_cvwait,
	[306] = sys_psynch_rw_rdlock,
	[307] = sys_psynch_rw_wrlock,
	[308] = sys_psynch_rw_unlock,
	[310] = sys_getsid,
	[312] = sys_psynch_cvclrprepost,
	[322] = sys_iopolicysys,
	[327] = sys_issetugid,
	[328] = sys_pthread_kill,
	[329] = sys_sigprocmask, // __pthread_sigmask
	[330] = sys_sigwait,
	[331] = sys_disable_threadsignal,
	[332] = sys_pthread_markcancel,
	[333] = sys_pthread_canceled,
	[334] = sys_semwait_signal,
	[336] = sys_proc_info,
	[338] = sys_stat64,
	[339] = sys_fstat64,
	[340] = sys_lstat64,
	[341] = sys_stat64_extended,
	[342] = sys_lstat64_extended,
	[343] = sys_fstat64_extended,
	[344] = sys_getdirentries64,
	[345] = sys_statfs64,
	[346] = sys_fstatfs64,
	[347] = sys_getfsstat64,
	[348] = sys_pthread_chdir,
	[349] = sys_pthread_fchdir,
	[357] = sys_getaudit_addr,
	[358] = sys_setaudit_addr,
	[360] = sys_bsdthread_create,
	[361] = sys_bsdthread_terminate,
	[362] = sys_kqueue,
	[363] = sys_kevent,
	[364] = sys_lchown,
	[366] = sys_bsdthread_register,
	[367] = sys_workq_open,
	[368] = sys_workq_kernreturn,
	[369] = sys_kevent64,
	[372] = sys_thread_selfid,
	[374] = sys_kevent_qos,
	[394] = sys_pselect,
	[395] = sys_pselect_nocancel,
	[396] = sys_read_nocancel,
	[397] = sys_write_nocancel,
	[398] = sys_open_nocancel,
	[399] = sys_close_nocancel,
	[400] = sys_wait4_nocancel,
	[401] = sys_recvmsg_nocancel,
	[402] = sys_sendmsg_nocancel,
	[403] = sys_recvfrom_nocancel,
	[404] = sys_accept_nocancel,
	[405] = sys_msync_nocancel,
	[406] = sys_fcntl_nocancel,
	[407] = sys_select_nocancel,
	[408] = sys_fsync_nocancel,
	[409] = sys_connect_nocancel,
	[410] = sys_sigsuspend_nocancel,
	[411] = sys_readv_nocancel,
	[412] = sys_writev_nocancel,
	[413] = sys_sendto_nocancel,
	[414] = sys_pread_nocancel,
	[415] = sys_pwrite_nocancel,
	[417] = sys_poll_nocancel,
	[420] = sys_sem_wait_nocancel,
	[422] = sys_sigwait_nocancel,
	[423] = sys_semwait_signal_nocancel,
	[427] = sys_fsgetpath,
	[428] = sys_audit_session_self,
	[429] = sys_audit_session_join,
	[430] = sys_fileport_makeport,
	[431] = sys_fileport_makefd,
	[432] = sys_audit_session_port,
	[441] = sys_guarded_open_np,
	[442] = sys_guarded_close_np,
	[443] = sys_guarded_kqueue_np,
	[461] = sys_getattrlistbulk,
	[462] = sys_clonefileat,
	[463] = sys_openat,
	[464] = sys_openat_nocancel,
	[465] = sys_renameat,
	[466] = sys_faccessat,
	[467] = sys_fchmodat,
	[468] = sys_fchownat,
	[469] = sys_fstatat,
	[470] = sys_fstatat64,
	[471] = sys_linkat,
	[472] = sys_unlinkat,
	[473] = sys_readlinkat,
	[474] = sys_symlinkat,
	[475] = sys_mkdirat,
	[476] = sys_getattrlistat,
	[478] = sys_bsdthread_ctl,
	[483] = sys_csrctl,
	[500] = sys_getentropy,
	[515] = sys_ulock_wait,
	[516] = sys_ulock_wake,
	[517] = sys_fclonefileat,
	[520] = sys_terminate_with_payload,
	[521] = sys_abort_with_payload,
	[524] = sys_setattrlistat,
};
