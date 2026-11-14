#include <darling/emulation/linux_premigration/elfcalls_wrapper.h>

#include <elfcalls.h>
#include <dlfcn.h>

#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/duct_errno.h>

extern struct elf_calls* _elfcalls;

#if defined(__aarch64__)
// On ARM64 the loader states how large its table is (see elfcalls_size.h), and
// members past that size are not used: a loader built without a member leaves
// whatever follows its table in that slot.
extern size_t _elfcalls_size;

#define ELFCALLS_HAS_FIELD(field) (_elfcalls && \
	_elfcalls_size >= __builtin_offsetof(struct elf_calls, field) + sizeof(_elfcalls->field))
#endif

struct elf_calls* elfcalls(void)
{
	if (!_elfcalls)
	{
		//void* module = dlopen("/usr/lib/darling/libelfloader.dylib", RTLD_NOW);
		// if (!module)
		//	__simple_printf("Load error: %s\n", dlerror());

		// struct elf_calls** ptr = (struct elf_calls**) dlsym(module, "_elfcalls");
		// __simple_printf("_elfcalls is at %p\n", ptr);
		// __simple_printf("*_elfcalls = %p\n", *ptr);
		//_elfcalls = *(struct elf_calls**) dlsym(module, "_elfcalls");
		__simple_printf("elfcalls not found?\n");
		__simple_abort();
	}
	return _elfcalls;
}

void native_exit(int ec)
{
	if (_elfcalls)
		_elfcalls->exit(ec);
}

long native_sysconf(int name)
{
	if (_elfcalls)
		return _elfcalls->sysconf(name);
	return -1;
}

int native_fork(void)
{
#if defined(__aarch64__)
	// The caller reports the error like any failed fork.
	if (!ELFCALLS_HAS_FIELD(native_fork) || !_elfcalls->native_fork)
		return -LINUX_ENOSYS;
	return _elfcalls->native_fork();
#else
	return elfcalls()->native_fork();
#endif
}

void* __darling_thread_create(unsigned long stack_size, unsigned long pthobj_size,
			void* entry_point, uintptr_t arg3,
			uintptr_t arg4, uintptr_t arg5, uintptr_t arg6,
			darling_thread_create_callbacks_t callbacks, void* dthread)
{
	return elfcalls()->darling_thread_create(stack_size, pthobj_size, entry_point,
			arg3, arg4, arg5, arg6, callbacks, dthread);
}

int __darling_thread_terminate(void* stackaddr,
			unsigned long freesize, unsigned long pthobj_size)
{
	return elfcalls()->darling_thread_terminate(stackaddr, freesize, pthobj_size);
}

void* __darling_thread_get_stack(void)
{
	return elfcalls()->darling_thread_get_stack();
}

void* native_dlopen(const char* path)
{
	return elfcalls()->dlopen(path);
}

char* native_dlerror(void)
{
	return elfcalls()->dlerror();
}

void* native_dlsym(void* module, const char* name)
{
	return elfcalls()->dlsym(module, name);
}

int native_dlclose(void* module)
{
	return elfcalls()->dlclose(module);
}

const void* __dserver_socket_address(void) {
	return elfcalls()->dserver_socket_address();
};

int __dserver_per_thread_socket(void) {
	return elfcalls()->dserver_per_thread_socket();
};

void __dserver_per_thread_socket_refresh(void) {
	return elfcalls()->dserver_per_thread_socket_refresh();
};

void __dserver_close_socket(int socket) {
	return elfcalls()->dserver_close_socket(socket);
};

int __dserver_get_process_lifetime_pipe() {
	return elfcalls()->dserver_get_process_lifetime_pipe();
}

int __dserver_process_lifetime_pipe_refresh(void) {
	return elfcalls()->dserver_process_lifetime_pipe_refresh();
};

void __dserver_close_process_lifetime_pipe(int fd) {
	return elfcalls()->dserver_close_process_lifetime_pipe(fd);
};
