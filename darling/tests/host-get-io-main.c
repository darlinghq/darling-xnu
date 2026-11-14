// host_get_io_main is the current name of host_get_io_master (same request, same reply). Current macOS libraries import
// it from libsystem_kernel; without the export Darling's libsystem_kernel is bypassed by the cache's own copy, which
// runs against Darling's kernel. The kernel half of Darling has no IO master port of its own (Darling's IOKit reaches
// iokitd through the bootstrap server), so the routine answers success with MACH_PORT_NULL, exactly as
// host_get_io_master does; this test pins that the two agree and that IOMasterPort/IOMainPort still hand out a port.
// Symbols are looked up at run time so the same binary reports a missing export instead of failing to load.
#include <dlfcn.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>

#define IO_KIT "/System/Library/Frameworks/IOKit.framework/IOKit"

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s\n", what);
		exit(1);
	}
}

int main(void)
{
	kern_return_t (*get_io_main)(host_t, mach_port_t*) = dlsym(RTLD_DEFAULT, "host_get_io_main");
	expect(get_io_main != NULL, "libsystem_kernel exports host_get_io_main");

	mach_port_t main_port = 0x7777, master_port = 0x7777;
	kern_return_t main_kr = get_io_main(mach_host_self(), &main_port);
	kern_return_t master_kr = host_get_io_master(mach_host_self(), &master_port);
	expect(main_kr == KERN_SUCCESS, "host_get_io_main succeeds");
	expect(main_kr == master_kr && main_port == master_port, "host_get_io_main answers as host_get_io_master does");

	mach_port_t unused = 0x7777;
	expect(get_io_main(MACH_PORT_NULL, &unused) == host_get_io_master(MACH_PORT_NULL, &unused) &&
	        get_io_main(MACH_PORT_NULL, &unused) != KERN_SUCCESS,
	    "a null host is refused the same way");

	void* iokit = dlopen(IO_KIT, RTLD_NOW);
	expect(iokit != NULL, "IOKit loads");
	kern_return_t (*io_master_port)(mach_port_t, mach_port_t*) = dlsym(iokit, "IOMasterPort");
	kern_return_t (*io_main_port)(mach_port_t, mach_port_t*) = dlsym(iokit, "IOMainPort");
	expect(io_master_port != NULL && io_main_port != NULL, "IOKit exports IOMasterPort and IOMainPort");
	mach_port_t via_master = MACH_PORT_NULL, via_main = MACH_PORT_NULL;
	expect(io_master_port(MACH_PORT_NULL, &via_master) == KERN_SUCCESS && MACH_PORT_VALID(via_master), "IOMasterPort returns a port");
	expect(io_main_port(MACH_PORT_NULL, &via_main) == KERN_SUCCESS && MACH_PORT_VALID(via_main), "IOMainPort returns a port");
	expect(via_master == via_main, "IOMasterPort and IOMainPort return the same port");

	printf("PASS: host_get_io_main agrees with host_get_io_master (port %u); IOMasterPort and IOMainPort return a port\n", main_port);
	return 0;
}
