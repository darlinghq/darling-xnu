#include <darling/emulation/xnu_syscall/bsd/impl/process/execve.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include <mach-o/loader.h>
#include <mach-o/fat.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlink.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/linux_premigration/elfcalls_wrapper.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/host_platform.h>

#include <darlingserver/rpc.h>


#undef memcpy
#include <darling/emulation/linux_premigration/resources/dserver-rpc-defs.h>
extern bool isspace(char c);

extern void _xtrace_execve_inject(const char*** envp_ptr);

static inline bool istext(char c)
{
	return c >= 0x20 && c < 0x7F;
}

static bool env_has_name(const char* entry, const char* name)
{
	size_t i = 0;
	for (; name[i]; i++)
		if (entry[i] != name[i])
			return false;
	return entry[i] == '=';
}

static const char* env_value(const char** envp, const char* name)
{
	if (!envp)
		return NULL;
	for (int i = 0; envp[i]; i++)
	{
		if (env_has_name(envp[i], name))
			return envp[i] + strlen(name) + 1;
	}
	return NULL;
}

static bool env_append(char* out, size_t size, size_t* pos, const char* text, size_t length)
{
	if (length >= size - *pos)
		return false;
	memcpy(out + *pos, text, length);
	*pos += length;
	out[*pos] = '\0';
	return true;
}

// Android only (host_platform_is_android()); on any other host this does
// nothing and the environment is passed on unchanged.
//
// A Darwin program started on Android has to be able to load the host
// libraries mldr opens. They sit in the directory of mldr (an application's
// native library directory) and in the library directory of the Termux-style
// prefix, which the environment names with TERMUX_PREFIX or PREFIX (or which
// lies next to HOME). mldr keeps the LD_LIBRARY_PATH of Darwin programs, so
// it is set in the environment of the program that is being started. The
// value the program was given is kept after ours.
//
// Fills out with "LD_LIBRARY_PATH=..." and returns 1, or returns 0 when there
// is nothing to add.
static int android_ld_library_path(char* out, size_t size, const char* mldr_path, const char** envp)
{
	static const char variable[] = "LD_LIBRARY_PATH=";
	size_t pos = 0;
	size_t mldr_dir_length = 0;
	bool have_entry = false;

	if (!host_platform_is_android())
		return 0;

	for (size_t i = 0; mldr_path[i]; i++)
		if (mldr_path[i] == '/')
			mldr_dir_length = i;

	out[0] = '\0';
	if (!env_append(out, size, &pos, variable, sizeof(variable) - 1))
		return 0;

	if (mldr_dir_length > 0)
	{
		if (!env_append(out, size, &pos, mldr_path, mldr_dir_length))
			return 0;
		have_entry = true;
	}

	const char* prefix = env_value(envp, "TERMUX_PREFIX");
	if (!prefix || !prefix[0])
		prefix = env_value(envp, "PREFIX");

	char lib[4096];
	size_t lib_length = 0;
	lib[0] = '\0';
	if (prefix && prefix[0])
	{
		if (!env_append(lib, sizeof(lib), &lib_length, prefix, strlen(prefix)) ||
				!env_append(lib, sizeof(lib), &lib_length, "/lib", 4))
			lib[0] = '\0';
	}
	else
	{
		const char* home = env_value(envp, "HOME");
		if (home && home[0])
		{
			if (!env_append(lib, sizeof(lib), &lib_length, home, strlen(home)) ||
					!env_append(lib, sizeof(lib), &lib_length, "/../usr/lib", 11))
				lib[0] = '\0';
		}
	}

	if (lib[0] && LINUX_SYSCALL(__NR_faccessat, LINUX_AT_FDCWD, lib, 0, 0) == 0)
	{
		if ((have_entry && !env_append(out, size, &pos, ":", 1)) ||
				!env_append(out, size, &pos, lib, lib_length))
			return 0;
		have_entry = true;
	}

	if (!have_entry)
		return 0;

	const char* existing = env_value(envp, "LD_LIBRARY_PATH");
	if (existing && existing[0])
	{
		if (!env_append(out, size, &pos, ":", 1) || !env_append(out, size, &pos, existing, strlen(existing)))
			return 0;
	}
	return 1;
}

long sys_execve(const char* fname, const char** argvp, const char** envp)
{
	int ret;
	static const char* empty_envp[] = { NULL };

	// a null environment pointer means an empty environment (Linux accepts it
	// too), and the code below walks the array
	if (envp == NULL)
		envp = empty_envp;

	struct vchroot_expand_args vc;
	char mldr_path[4096];
	uint64_t mldr_path_length;
	const char* path_to_exec = vc.path;
	// referenced from the new environment until the execve below, so they
	// must not be scoped to the block that fills them in
	char mldr_lifetime_pipe_env[32] = { '\0' };
	char mldr_nofile_env[48] = { '\0' };
	char android_ld_env[3 * 4096] = { '\0' };

	ret = dserver_rpc_mldr_path(mldr_path, sizeof(mldr_path), &mldr_path_length);
	if (ret < 0) {
		return errno_linux_to_bsd(ret);
	}

	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();

	strcpy(vc.path, fname);

	ret = vchroot_expand(&vc);
	__simple_kprintf("execve expand %s -> %s, ret %d", fname, vc.path, ret);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	char shebang[256];
	int fd = sys_open_nocancel(fname, BSD_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	ret = sys_read_nocancel(fd, shebang, sizeof(shebang));
	if (ret < 0)
		return ret;

	close_internal(fd);

	bool is_script = false;
	bool is_macho = false;

	if (ret < 4) {
		return -ENOEXEC;
	}

	//if (ret >= 4)
	{
		is_script = shebang[0] == '#' && shebang[1] == '!';
		if (!is_script)
		{
			if (istext(shebang[0]) && istext(shebang[1]) && istext(shebang[2]) && istext(shebang[3]))
			{
				strcpy(shebang, "#!/bin/sh\n");
				is_script = true;
			}
		}
	}

	uint32_t magic = *(uint32_t*)shebang;
	is_macho = magic == MH_MAGIC || magic == MH_CIGAM || magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || magic == FAT_MAGIC || magic == FAT_CIGAM;

	if (is_script)
	{
		char *nl, *interp, *arg;
		const char** modargvp;
		int i, j;

		nl = memchr(shebang, '\n', ret);
		if (!nl)
			return -ENOEXEC;

		*nl = '\0';
		for (i = 2; isspace(shebang[i]); i++)
			continue;

		interp = &shebang[i];

		for (i = 0; !isspace(interp[i]) && interp[i]; i++)
			continue;

		if (interp[i] == '\0')
			arg = NULL;
		else
			arg = &interp[i];

		if (arg != NULL)
		{
			*arg = '\0'; // terminate interp
			arg++;
			while (isspace(*arg) && *arg)
				arg++;
			if (*arg == '\0')
				arg = NULL; // no argument, just whitespace
		}

		// Count original arguments
		int orig_argc = 0;
		while (argvp[orig_argc])
			orig_argc++;

		// Allocate a new argvp: mldr_path, interp (vc.path), [arg], fname,
		// argvp[1..orig_argc-1], NULL. That is at most orig_argc + 4 entries,
		// or 5 when the caller passed an empty argv.
		modargvp = (const char**) __builtin_alloca(sizeof(void*) * (orig_argc + 5));

		i = 0;
		modargvp[i++] = mldr_path;
		modargvp[i++] = vc.path; // expanded later
		if (arg != NULL)
			modargvp[i++] = arg;
		modargvp[i++] = fname;

		// Append original arguments (skipping argvp[0])
		for (j = 1; j < orig_argc; j++)
			modargvp[i++] = argvp[j];
		modargvp[i++] = NULL;

		argvp = modargvp;
		vc.flags = 0;
		strcpy(vc.path, interp);

		ret = vchroot_expand(&vc);
		if (ret < 0)
			return errno_linux_to_bsd(ret);

		path_to_exec = mldr_path;
	} else if (is_macho) {
		const char** modargvp;
		char *buf;
		int len = 0;

		// count original arguments
		while (argvp[len++]);

		// allocate a new argvp and argv0
		modargvp = (const char**) __builtin_alloca(sizeof(void*) * (len+1));
		buf = __builtin_alloca(strlen(mldr_path) + 2 + strlen(vc.path));

		// set up the new argv0 (mldr path + "!" + executable path)
		strcpy(buf, mldr_path);
		strcat(buf, "!");
		strcat(buf, vc.path);
		modargvp[0] = buf;

		// append original arguments
		for (int i = 1; i < len+1; i++)
			modargvp[i] = argvp[i-1];

		argvp = modargvp;
		path_to_exec = mldr_path;
	}

	// set up the __mldr_sockpath env var if we're executing mldr
	if (is_script || is_macho) {
		const char** modenvp;
		char* buf;
		int len = 0;
		struct linux_sockaddr_un* server_socket_address = dserver_rpc_hooks_get_server_address();
		const char* server_socket_path = server_socket_address->sun_path;

		__simple_snprintf(mldr_lifetime_pipe_env, sizeof(mldr_lifetime_pipe_env) - 1, "__mldr_lifetime_pipe=%d", __dserver_get_process_lifetime_pipe());

		// RLIMIT_NOFILE survives execve; mldr hands it to the new image
		int have_nofile_env = nofile_limit_exec_env(mldr_nofile_env, sizeof(mldr_nofile_env) - 1);

		int add_android_ld = android_ld_library_path(android_ld_env, sizeof(android_ld_env), mldr_path, envp);

		// count original env vars (handling NULL envp safely)
		if (envp) {
			while (envp[len]) len++;
		}

		const int new_env_count = (have_nofile_env ? 3 : 2) + add_android_ld;

		// allocate a new envp and env0, env1 (+1 for trailing NULL)
		modenvp = (const char**)__builtin_alloca(sizeof(void*) * (len + new_env_count + 1));
		buf = __builtin_alloca(strlen(server_socket_path) + sizeof("__mldr_sockpath="));

		// set up the new env0
		strcpy(buf, "__mldr_sockpath=");
		strcat(buf, server_socket_path);
		int env_idx = 0;
		modenvp[env_idx++] = buf;
		modenvp[env_idx++] = mldr_lifetime_pipe_env;
		if (add_android_ld)
			modenvp[env_idx++] = android_ld_env;
		if (have_nofile_env)
			modenvp[env_idx++] = mldr_nofile_env;

		// append original env vars
		if (envp) {
			for (int i = 0; i < len; i++)
			{
				// replaced by the entry that android_ld_library_path built from it
				if (add_android_ld && env_has_name(envp[i], "LD_LIBRARY_PATH"))
					continue;
				modenvp[env_idx++] = envp[i];
			}
		}
		modenvp[env_idx] = NULL;

		envp = modenvp;
	}

	// otherwise it's a Linux executable (ELF or something else binfmt handles);
	// this is the default

	linux_sigset_t set;
	set = (1ull << (SIGNAL_SIGEXC_SUSPEND-1));
	set |= (1ull << (SIGNAL_S2C-1));

	// darlingserver needs to know whether the execve completes successfully or not.
	// since pidfds don't notify on execve, we have to use a pipe with close-on-exec
	// that darlingserver will monitor. if it reads EOF, it knows the execve succeeded.
	// if it reads a single byte (that we send it), it knows the execve failed.

	int dserver_execve_pipe[2];

	// open a pipe with FD_CLOEXEC set
	ret = LINUX_SYSCALL(__NR_pipe2, dserver_execve_pipe, LINUX_O_CLOEXEC);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	// send a copy of the read end to the server (along with whether or not we're executing another Darling-managed binary)
	ret = dserver_rpc_checkout(dserver_execve_pipe[0], is_script || is_macho);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	// close the read end for ourselves
	close_internal(dserver_execve_pipe[0]);

	LINUX_SYSCALL(__NR_rt_sigprocmask, 0 /* LINUX_SIG_BLOCK */,
			&set, NULL, sizeof(linux_sigset_t));

	// mldr takes over the lifetime pipe named in __mldr_lifetime_pipe, so it
	// must survive the execve even if the program marked every descriptor
	// close-on-exec before exec'ing (APT does, for its download methods).
	if (is_script || is_macho) {
		int lifetime_pipe = __dserver_get_process_lifetime_pipe();
		if (lifetime_pipe >= 0)
			LINUX_SYSCALL(__NR_fcntl, lifetime_pipe, 2 /* F_SETFD */, 0);
	}

	// let xtrace inject itself into the execve, if necessary
	_xtrace_execve_inject(&envp);

	ret = LINUX_SYSCALL(__NR_execve, path_to_exec, argvp, envp);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	// the execve failed; write to the write end of the pipe.
	// ignore errors.
	sys_write_nocancel(dserver_execve_pipe[1], "\x01", 1);
	close_internal(dserver_execve_pipe[1]);

	LINUX_SYSCALL(__NR_rt_sigprocmask, 1 /* LINUX_SIG_UNBLOCK */,
			&set, NULL, sizeof(linux_sigset_t));

	return ret;
}
