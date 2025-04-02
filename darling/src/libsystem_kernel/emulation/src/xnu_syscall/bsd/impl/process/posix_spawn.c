#include <emulation/xnu_syscall/bsd/impl/process/posix_spawn.h>

// #include "execve.h"
// #include "../base.h"
// #include "../errno.h"
// #include "../unistd/read.h"
// #include "../unistd/write.h"
// #include "../unistd/exit.h"
// #include "../unistd/close.h"
// #include "../unistd/pipe.h"
// #include "../unistd/dup2.h"
// #include "../unistd/dup.h"
// #include "../mach/lkm.h"
// #include "../unistd/chdir.h"
// #include "../unistd/fchdir.h"
// #include "../dirent/getdirentries.h"

// for debugging only; remove before committing
// #include "../signal/kill.h"
// #include "../unistd/getpid.h"

#include <emulation/common/simple.h>
#include <emulation/conversion/variables/bsd/sys/fcntl.h>
#include <emulation/conversion/variables/linux/dirent.h>
#include <emulation/conversion/variables/linux/linux/fcntl.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/linux_api/c_api/close.h>
#include <emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <emulation/xnu_syscall/bsd/impl/process/execve.h>
#include <emulation/xnu_syscall/bsd/impl/process/fork.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigprocmask.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/chdir.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/dup.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/dup2.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/exit.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/fchdir.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/getgid.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/setgid.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/setpgid.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/setuid.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/write.h>

#include <_libkernel_init.h>
#include <darlingserver/rpc.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/spawn.h>

#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR 0x0100
#endif

#define LINUX_ADDR_NO_RANDOMIZE 0x40000

extern _libkernel_functions_t _libkernel_functions;

long sys_posix_spawn(int* pid, const char* path, const struct _posix_spawn_args_desc* desc,
		char** argvp, char** envp)
{
	int ret, my_pid;
	int pipe[2] = { -1, -1 };

	if (desc && desc->attrp && desc->attrp->psa_flags & POSIX_SPAWN_SETEXEC)
		goto no_fork;

	ret = LINUX_SYSCALL(__NR_pipe2, pipe, LINUX_O_CLOEXEC);
	if (ret < 0)
		return ret;

	// we don't want to call user atfork callbacks, but we *do* want to call most
	// libsystem atfork callbacks to set up the environment for some of the calls we need
	// to make for the setup

	_libkernel_functions->posix_spawn_prepare();

	if ((my_pid = sys_fork()) == 0)
	{
		// child
		// close the reading side
		close_internal(pipe[0]);

		_libkernel_functions->posix_spawn_child();

no_fork:
		if (desc && desc->attrp)
		{
			if (desc->attrp->psa_flags & POSIX_SPAWN_RESETIDS)
			{
				sys_setgid(sys_getgid());
				sys_setuid(sys_getuid());
			}
			if (desc->attrp->psa_flags & POSIX_SPAWN_SETPGROUP)
			{
				ret = sys_setpgid(0, desc->attrp->psa_pgroup);
				if (ret != 0)
					goto fail;
			}
			if (desc->attrp->psa_flags & POSIX_SPAWN_SETSIGMASK)
			{
				ret = sys_sigprocmask(3 /* SIG_SETMASK */, &desc->attrp->psa_sigmask, NULL);
				if (ret != 0)
					goto fail;
			}
			if (desc->attrp->psa_flags & _POSIX_SPAWN_DISABLE_ASLR)
			{
				unsigned int pers = LINUX_SYSCALL(__NR_personality, 0xffffffff);
				if (!(pers & LINUX_ADDR_NO_RANDOMIZE))
				{
					pers |= LINUX_ADDR_NO_RANDOMIZE;
					LINUX_SYSCALL(__NR_personality, pers);
				}
			}
			if (desc->attrp->psa_flags & POSIX_SPAWN_START_SUSPENDED)
			{
				int status = dserver_rpc_stop_after_exec();
				if (status < 0) {
					__simple_printf("Failed to tell darlingserver to stop us after exec: %d", status);
					__simple_abort();
				}
			}

			if (desc->attrp->psa_flags & POSIX_SPAWN_CLOEXEC_DEFAULT) {
				// set O_CLOEXEC on everything
				int dir = sys_open("/proc/self/fd", BSD_O_RDONLY, 0);
				char buf[4096];
				int len = 0;

				//__simple_kprintf("asked for cloexec default\n");

				if (dir < 0) {
					ret = dir;
					goto fail;
				}

				while ((len = LINUX_SYSCALL(__NR_getdents64, dir, buf, sizeof(buf))) > 0) {
					struct linux_dirent64* dirent = (struct linux_dirent64*)&buf[0];
					for (int i = 0; i < len; i += dirent->d_reclen, dirent = (struct linux_dirent64*)&buf[i]) {
						int fd = -1;

						//__simple_kprintf("dentry name: %s\n", dirent->d_name);
						if (dirent->d_name[0] == '.') {
							continue;
						}

						fd = __simple_atoi(dirent->d_name, NULL);

						// FIXME: pretty sure this should not be hardcoded
						if (fd == 1023) {
							// special commpage fd
							continue;
						}

						if (desc && desc->factp) {
							// if this FD is one that we're creating, don't set O_CLOEXEC on it
							for (size_t j = 0; j < desc->factp->psfa_act_count; j++) {
								const struct _psfa_action* act = &desc->factp->psfa_act_acts[j];
								int maybe_fd = act->psfaa_filedes;

								switch (act->psfaa_type) {
									case PSFA_DUP2:
									case PSFA_FILEPORT_DUP2:
										maybe_fd = act->psfaa_dup2args.psfad_newfiledes;
										// fall through

									case PSFA_OPEN:
									case PSFA_INHERIT:
										if (fd == maybe_fd) {
											maybe_fd = -1;
										}
										break;

									default:
										break;
								}

								// maybe_fd was set to -1, indicating the descriptors matched
								if (maybe_fd == -1) {
									fd = -1;
									break;
								}
							}
						}

						// fd was set to -1, indicating we should skip it
						if (fd == -1) {
							continue;
						}

						//__simple_kprintf("setting cloexec on %d\n", fd);

						ret = sys_fcntl(fd, BSD_F_GETFD, 0);
						if (ret < 0) {
							close_internal(dir);
							goto fail;
						}

						ret = sys_fcntl(fd, BSD_F_SETFD, ret | BSD_FD_CLOEXEC);
						if (ret < 0) {
							close_internal(dir);
							goto fail;
						}
					}
				}

				close_internal(dir);

				if (len < 0) {
					ret = len;
					goto fail;
				}
			}

			// TODO: other attributes
		}
		if (desc && desc->factp)
		{
			int i;

			//__simple_kprintf("act count: %d\n", desc->factp->psfa_act_count);

			//sys_kill(sys_getpid(), SIGSTOP, 0);

			for (i = 0; i < desc->factp->psfa_act_count; i++)
			{
				const struct _psfa_action* act;

				act = &desc->factp->psfa_act_acts[i];

				//__simple_kprintf("act count (on iter %d): %d\n", i, desc->factp->psfa_act_count);

				if (act->psfaa_filedes == pipe[1] || (act->psfaa_type == PSFA_DUP2 && act->psfaa_dup2args.psfad_newfiledes == pipe[1]))
				{
					ret = sys_dup(pipe[1]);
					if (ret < 0)
						goto fail;

					close_internal(pipe[1]);
					pipe[1] = ret;
				}

				switch (act->psfaa_type)
				{
					case PSFA_CLOSE:
						//__simple_kprintf("closing %d\n", act->psfaa_filedes);
						ret = close_internal(act->psfaa_filedes);
						if (ret != 0)
							goto fail;
						break;

					case PSFA_DUP2:
						//__simple_kprintf("duping %d to %d\n", act->psfaa_filedes, act->psfaa_dup2args.psfad_newfiledes);
						ret = sys_dup2(act->psfaa_filedes, act->psfaa_dup2args.psfad_newfiledes);
						if (ret < 0)
							goto fail;
						break;

					case PSFA_OPEN:
					{
						//__simple_kprintf("opening %s to %d\n", act->psfaa_openargs.psfao_path, act->psfaa_filedes);
						ret = sys_open(act->psfaa_openargs.psfao_path,
								act->psfaa_openargs.psfao_oflag, act->psfaa_openargs.psfao_mode);
						if (ret < 0)
							goto fail;

						if (ret != act->psfaa_filedes)
						{
							int fd = ret;
							ret = sys_dup2(ret, act->psfaa_filedes);

							if (ret < 0)
							{
								close_internal(fd);
								goto fail;
							}
						}
						break;
					}

					case PSFA_CHDIR: {
						//__simple_kprintf("chdiring to %s\n", act->psfaa_chdirargs.psfac_path);
						ret = sys_chdir(act->psfaa_chdirargs.psfac_path);
						if (ret < 0) {
							goto fail;
						}
					} break;

					case PSFA_FCHDIR: {
						//__simple_kprintf("fchdiring to %d\n", act->psfaa_filedes);
						ret = sys_fchdir(act->psfaa_filedes);
						if (ret < 0) {
							goto fail;
						}
					} break;

					// unset CLOEXEC on this fd
					case PSFA_INHERIT: {
						//__simple_kprintf("inheriting %d\n", act->psfaa_filedes);
						ret = sys_fcntl(act->psfaa_filedes, BSD_F_GETFD, 0);
						if (ret < 0) {
							goto fail;
						}
						ret = sys_fcntl(act->psfaa_filedes, BSD_F_SETFD, ret & ~BSD_FD_CLOEXEC);
						if (ret < 0) {
							goto fail;
						}
					} break;

					default:
						//__simple_kprintf("unknown PSFA type %d\n", act->psfaa_type);
						;
				}
			}

			//__simple_kprintf("act count (on finish): %d\n", desc->factp->psfa_act_count);
		}

		char binprefs[64];
		if (desc && desc->attrp && desc->attrp->psa_binprefs[0])
		{
			char** new_envp;
			int i, env_len = 0;

			while (envp[env_len])
				env_len++;
			env_len++; // terminating NULL

			// +1 for our new entry
			new_envp = (char**) __builtin_alloca((env_len + 1) * sizeof(char*));

			__simple_sprintf(binprefs, "__mldr_bprefs=%x,%x,%x,%x",
				desc->attrp->psa_binprefs[0],
				desc->attrp->psa_binprefs[1],
				desc->attrp->psa_binprefs[2],
				desc->attrp->psa_binprefs[3]);

			new_envp[0] = binprefs;
			for (i = 0; i < env_len; i++)
				new_envp[i+1] = envp[i];
			envp = new_envp;
		}

		ret = sys_execve((char*) path, argvp, envp);
fail:
		//__simple_kprintf("posix_spawn is failing with %d\n", ret);
		if (desc && desc->attrp && desc->attrp->psa_flags & POSIX_SPAWN_SETEXEC)
		{
			// no_fork case
			ret = errno_linux_to_bsd(ret);
			return ret;
		}
		else
		{
			sys_write(pipe[1], &ret, sizeof(ret));
			sys_exit(127);
			__builtin_unreachable();
		}
	}
	else
	{
		// parent
		int err;

		// close the writing side
		close_internal(pipe[1]);

		_libkernel_functions->posix_spawn_parent();

		if (my_pid < 0)
		{
			ret = my_pid;
			goto out;
		}

		if (pid != NULL)
			*pid = my_pid;

		if (sys_read(pipe[0], &ret, sizeof(ret)) != sizeof(ret))
			ret = 0;
		close_internal(pipe[0]);
	}

out:
	return ret;
}
