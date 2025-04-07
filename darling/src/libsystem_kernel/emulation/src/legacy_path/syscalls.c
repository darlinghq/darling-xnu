#include <darling/emulation/legacy_path/syscalls.h>

#include <darling/emulation/legacy_path/kqueue/kqueue.h>
#include <darling/emulation/legacy_path/kqueue/kevent.h>
#include <darling/emulation/legacy_path/kqueue/kevent64.h>
#include <darling/emulation/legacy_path/kqueue/kevent_qos.h>
#include <darling/emulation/legacy_path/unistd/write.h>
#include <darling/emulation/legacy_path/unistd/read.h>
#include <darling/emulation/legacy_path/mount/unmount.h>
#include <darling/emulation/legacy_path/guarded/guarded_open_np.h>
#include <darling/emulation/legacy_path/guarded/guarded_close_np.h>
#include <darling/emulation/legacy_path/guarded/guarded_kqueue_np.h>
#include <darling/emulation/legacy_path/wrapped/shm_open.h>
#include <darling/emulation/legacy_path/wrapped/shm_unlink.h>
#include <darling/emulation/legacy_path/wrapped/sem_open.h>
#include <darling/emulation/legacy_path/wrapped/sem_unlink.h>
#include <darling/emulation/legacy_path/wrapped/sem_close.h>
#include <darling/emulation/legacy_path/wrapped/sem_wait.h>
#include <darling/emulation/legacy_path/wrapped/sem_trywait.h>
#include <darling/emulation/legacy_path/wrapped/sem_post.h>
#include <darling/emulation/legacy_path/mman/mman.h>
#include <darling/emulation/legacy_path/mman/madvise.h>
#include <darling/emulation/legacy_path/mman/msync.h>
#include <darling/emulation/legacy_path/unistd/initgroups.h>
#include <darling/emulation/legacy_path/unistd/getsid.h>
#include <darling/emulation/legacy_path/unistd/flock.h>
#include <darling/emulation/legacy_path/unistd/sync.h>
#include <darling/emulation/legacy_path/unistd/fsync.h>
#include <darling/emulation/legacy_path/unistd/fdatasync.h>
#include <darling/emulation/legacy_path/unistd/dup.h>
#include <darling/emulation/legacy_path/unistd/dup2.h>
#include <darling/emulation/legacy_path/unistd/exit.h>
#include <darling/emulation/legacy_path/unistd/close.h>
#include <darling/emulation/legacy_path/unistd/fchdir.h>
#include <darling/emulation/legacy_path/unistd/fchown.h>
#include <darling/emulation/legacy_path/unistd/fchownat.h>
#include <darling/emulation/legacy_path/unistd/fchmod.h>
#include <darling/emulation/legacy_path/unistd/fchmodat.h>
#include <darling/emulation/legacy_path/unistd/setgid.h>
#include <darling/emulation/legacy_path/unistd/setegid.h>
#include <darling/emulation/legacy_path/unistd/seteuid.h>
#include <darling/emulation/legacy_path/unistd/setsid.h>
#include <darling/emulation/legacy_path/unistd/setuid.h>
#include <darling/emulation/legacy_path/unistd/settid.h>
#include <darling/emulation/legacy_path/unistd/gettid.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>
#include <darling/emulation/legacy_path/unistd/geteuid.h>
#include <darling/emulation/legacy_path/unistd/pread.h>
#include <darling/emulation/legacy_path/unistd/pwrite.h>
#include <darling/emulation/legacy_path/unistd/getpid.h>
#include <darling/emulation/legacy_path/unistd/access.h>
#include <darling/emulation/legacy_path/unistd/faccessat.h>
#include <darling/emulation/legacy_path/unistd/lseek.h>
#include <darling/emulation/legacy_path/unistd/truncate.h>
#include <darling/emulation/legacy_path/unistd/ftruncate.h>
#include <darling/emulation/legacy_path/unistd/readlink.h>
#include <darling/emulation/legacy_path/unistd/readlinkat.h>
#include <darling/emulation/legacy_path/unistd/readv.h>
#include <darling/emulation/legacy_path/unistd/writev.h>
#include <darling/emulation/legacy_path/unistd/getegid.h>
#include <darling/emulation/legacy_path/unistd/symlink.h>
#include <darling/emulation/legacy_path/unistd/symlinkat.h>
#include <darling/emulation/legacy_path/unistd/umask.h>
#include <darling/emulation/legacy_path/unistd/chroot.h>
#include <darling/emulation/legacy_path/unistd/link.h>
#include <darling/emulation/legacy_path/unistd/linkat.h>
#include <darling/emulation/legacy_path/unistd/unlink.h>
#include <darling/emulation/legacy_path/unistd/unlinkat.h>
#include <darling/emulation/legacy_path/unistd/chdir.h>
#include <darling/emulation/legacy_path/unistd/mknod.h>
#include <darling/emulation/legacy_path/unistd/chmod.h>
#include <darling/emulation/legacy_path/unistd/getpgid.h>
#include <darling/emulation/legacy_path/unistd/setpgid.h>
#include <darling/emulation/legacy_path/unistd/chown.h>
#include <darling/emulation/legacy_path/unistd/lchown.h>
#include <darling/emulation/legacy_path/unistd/getgid.h>
#include <darling/emulation/legacy_path/unistd/getppid.h>
#include <darling/emulation/legacy_path/unistd/rename.h>
#include <darling/emulation/legacy_path/unistd/renameat.h>
#include <darling/emulation/legacy_path/unistd/getpgrp.h>
#include <darling/emulation/legacy_path/unistd/getdtablesize.h>
#include <darling/emulation/legacy_path/unistd/setpgid.h>
#include <darling/emulation/legacy_path/unistd/getgroups.h>
#include <darling/emulation/legacy_path/unistd/setgroups.h>
#include <darling/emulation/legacy_path/unistd/pipe.h>
#include <darling/emulation/legacy_path/unistd/chmod_extended.h>
#include <darling/emulation/legacy_path/unistd/fchmod_extended.h>
#include <darling/emulation/legacy_path/unistd/fchflags.h>
#include <darling/emulation/legacy_path/unistd/chflags.h>
#include <darling/emulation/legacy_path/unistd/issetugid.h>
#include <darling/emulation/legacy_path/signal/kill.h>
#include <darling/emulation/legacy_path/signal/sigaltstack.h>
#include <darling/emulation/legacy_path/signal/sigaction.h>
#include <darling/emulation/legacy_path/signal/sigreturn.h>
#include <darling/emulation/legacy_path/signal/sigprocmask.h>
#include <darling/emulation/legacy_path/signal/sigsuspend.h>
#include <darling/emulation/legacy_path/signal/sigpending.h>
#include <darling/emulation/legacy_path/signal/sigwait.h>
#include <darling/emulation/legacy_path/process/vfork.h>
#include <darling/emulation/legacy_path/process/wait4.h>
#include <darling/emulation/legacy_path/process/waitid.h>
#include <darling/emulation/legacy_path/process/execve.h>
#include <darling/emulation/legacy_path/process/getpriority.h>
#include <darling/emulation/legacy_path/process/setpriority.h>
#include <darling/emulation/legacy_path/misc/getentropy.h>
#include <darling/emulation/legacy_path/misc/ptrace.h>
#include <darling/emulation/legacy_path/misc/getlogin.h>
#include <darling/emulation/legacy_path/misc/setlogin.h>
#include <darling/emulation/legacy_path/misc/reboot.h>
#include <darling/emulation/legacy_path/misc/iopolicysys.h>
#include <darling/emulation/legacy_path/misc/fsgetpath.h>
#include <darling/emulation/legacy_path/misc/abort_with_payload.h>
#include <darling/emulation/legacy_path/misc/shared_region_check_np.h>
#include <darling/emulation/legacy_path/ioctl/ioctl.h>
#include <darling/emulation/legacy_path/misc/getrlimit.h>
#include <darling/emulation/legacy_path/misc/setrlimit.h>
#include <darling/emulation/legacy_path/misc/thread_selfid.h>
#include <darling/emulation/legacy_path/misc/sysctl.h>
#include <darling/emulation/legacy_path/misc/proc_info.h>
#include <darling/emulation/legacy_path/misc/gethostuuid.h>
#include <darling/emulation/legacy_path/misc/getrusage.h>
#include <darling/emulation/legacy_path/misc/syscall.h>
#include <darling/emulation/legacy_path/misc/csops.h>
#include <darling/emulation/legacy_path/misc/fileport_makeport.h>
#include <darling/emulation/legacy_path/misc/fileport_makefd.h>
#include <darling/emulation/legacy_path/misc/csrctl.h>
#include <darling/emulation/legacy_path/misc/clonefile.h>
#include <darling/emulation/legacy_path/synch/semwait_signal.h>
#include <darling/emulation/legacy_path/fcntl/open.h>
#include <darling/emulation/legacy_path/fcntl/openat.h>
#include <darling/emulation/legacy_path/fcntl/fcntl.h>
#include <darling/emulation/legacy_path/network/socket.h>
#include <darling/emulation/legacy_path/network/connect.h>
#include <darling/emulation/legacy_path/network/recvmsg.h>
#include <darling/emulation/legacy_path/network/sendmsg.h>
#include <darling/emulation/legacy_path/network/recvfrom.h>
#include <darling/emulation/legacy_path/network/getpeername.h>
#include <darling/emulation/legacy_path/network/getsockname.h>
#include <darling/emulation/legacy_path/network/accept.h>
#include <darling/emulation/legacy_path/network/shutdown.h>
#include <darling/emulation/legacy_path/network/getsockopt.h>
#include <darling/emulation/legacy_path/network/setsockopt.h>
#include <darling/emulation/legacy_path/network/sendto.h>
#include <darling/emulation/legacy_path/network/bind.h>
#include <darling/emulation/legacy_path/network/listen.h>
#include <darling/emulation/legacy_path/network/socketpair.h>
#include <darling/emulation/legacy_path/dirent/getdirentries.h>
#include <darling/emulation/legacy_path/stat/fstat.h>
#include <darling/emulation/legacy_path/stat/fstatat.h>
#include <darling/emulation/legacy_path/stat/stat.h>
#include <darling/emulation/legacy_path/stat/lstat.h>
#include <darling/emulation/legacy_path/stat/statfs.h>
#include <darling/emulation/legacy_path/stat/mkdir.h>
#include <darling/emulation/legacy_path/stat/mkdirat.h>
#include <darling/emulation/legacy_path/stat/mkfifo.h>
#include <darling/emulation/legacy_path/stat/rmdir.h>
#include <darling/emulation/legacy_path/stat/getfsstat.h>
#include <darling/emulation/legacy_path/stat/fstatfs.h>
#include <darling/emulation/legacy_path/stat/stat64_extended.h>
#include <darling/emulation/legacy_path/stat/lstat64_extended.h>
#include <darling/emulation/legacy_path/stat/fstat64_extended.h>
#include <darling/emulation/legacy_path/time/gettimeofday.h>
#include <darling/emulation/legacy_path/time/utimes.h>
#include <darling/emulation/legacy_path/time/futimes.h>
#include <darling/emulation/legacy_path/time/setitimer.h>
#include <darling/emulation/legacy_path/time/getitimer.h>
#include <darling/emulation/legacy_path/process/vfork.h>
#include <darling/emulation/legacy_path/process/fork.h>
#include <darling/emulation/legacy_path/process/posix_spawn.h>
#include <darling/emulation/legacy_path/bsdthread/bsdthread_register.h>
#include <darling/emulation/legacy_path/bsdthread/bsdthread_create.h>
#include <darling/emulation/legacy_path/bsdthread/bsdthread_ctl.h>
#include <darling/emulation/legacy_path/bsdthread/bsdthread_terminate.h>
#include <darling/emulation/legacy_path/bsdthread/disable_threadsignal.h>
#include <darling/emulation/legacy_path/bsdthread/pthread_chdir.h>
#include <darling/emulation/legacy_path/bsdthread/pthread_fchdir.h>
#include <darling/emulation/legacy_path/bsdthread/pthread_markcancel.h>
#include <darling/emulation/legacy_path/bsdthread/pthread_canceled.h>
#include <darling/emulation/legacy_path/hfs/stub.h>
#include <darling/emulation/legacy_path/xattr/getattrlistbulk.h>
#include <darling/emulation/legacy_path/xattr/getattrlist.h>
#include <darling/emulation/legacy_path/xattr/fgetattrlist.h>
#include <darling/emulation/legacy_path/xattr/getattrlistat.h>
#include <darling/emulation/legacy_path/xattr/setattrlistat.h>
#include <darling/emulation/legacy_path/xattr/setattrlist.h>
#include <darling/emulation/legacy_path/xattr/fsetattrlist.h>
#include <darling/emulation/legacy_path/xattr/listxattr.h>
#include <darling/emulation/legacy_path/xattr/flistxattr.h>
#include <darling/emulation/legacy_path/xattr/removexattr.h>
#include <darling/emulation/legacy_path/xattr/fremovexattr.h>
#include <darling/emulation/legacy_path/xattr/getxattr.h>
#include <darling/emulation/legacy_path/xattr/fgetxattr.h>
#include <darling/emulation/legacy_path/xattr/setxattr.h>
#include <darling/emulation/legacy_path/xattr/fsetxattr.h>
#include <darling/emulation/legacy_path/select/select.h>
#include <darling/emulation/legacy_path/select/pselect.h>
#include <darling/emulation/legacy_path/select/poll.h>
#include <darling/emulation/legacy_path/psynch/psynch_rw_rdlock.h>
#include <darling/emulation/legacy_path/psynch/psynch_rw_wrlock.h>
#include <darling/emulation/legacy_path/psynch/psynch_rw_unlock.h>
#include <darling/emulation/legacy_path/psynch/psynch_cvclrprepost.h>
#include <darling/emulation/legacy_path/psynch/psynch_mutexwait.h>
#include <darling/emulation/legacy_path/psynch/psynch_mutexdrop.h>
#include <darling/emulation/legacy_path/psynch/psynch_cvwait.h>
#include <darling/emulation/legacy_path/psynch/psynch_cvsignal.h>
#include <darling/emulation/legacy_path/psynch/psynch_cvbroad.h>
#include <darling/emulation/legacy_path/psynch/ulock_wait.h>
#include <darling/emulation/legacy_path/psynch/ulock_wake.h>
#include <darling/emulation/legacy_path/bsdthread/workq_kernreturn.h>
#include <darling/emulation/legacy_path/bsdthread/workq_open.h>
#include <darling/emulation/legacy_path/bsdthread/pthread_kill.h>
#include <darling/emulation/legacy_path/conf/pathconf.h>
#include <darling/emulation/legacy_path/conf/fpathconf.h>
#include <darling/emulation/legacy_path/sysv_sem/semctl.h>
#include <darling/emulation/legacy_path/sysv_sem/semget.h>
#include <darling/emulation/legacy_path/sysv_sem/semop.h>
#include <darling/emulation/legacy_path/audit/audit_session_self.h>
#include <darling/emulation/legacy_path/audit/audit_addr.h>
#include <darling/emulation/legacy_path/audit/audit_session_join.h>
#include <darling/emulation/legacy_path/audit/audit_session_port.h>

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
	[521] = sys_abort_with_payload,
	[524] = sys_setattrlistat,
};

