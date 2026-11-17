// host_get_io_main is the current name of host_get_io_master (same request, same reply). Current macOS libraries import
// it from libsystem_kernel, so the export must exist. The routine hands out the host's IO main port: on macOS the
// kernel's own, on Darling the port the IOKit daemon registers with host_set_special_port.
// Run with the argument "darling" to also exercise host_set_special_port; do not do that on macOS.
// Symbols are looked up at run time so the same binary reports a missing export instead of failing to load.
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Newer SDKs declare only the new name.
extern kern_return_t host_get_io_master(host_t, mach_port_t*);

#define IO_KIT "/System/Library/Frameworks/IOKit.framework/IOKit"
#define IO_MASTER_PORT_ID 3
#define IO_NO_SUCH_ID 5

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s\n", what);
		exit(1);
	}
}

int main(int argc, char** argv)
{
	int darling = argc > 1 && strcmp(argv[1], "darling") == 0;

	kern_return_t (*get_io_main)(host_t, mach_port_t*) = dlsym(RTLD_DEFAULT, "host_get_io_main");
	expect(get_io_main != NULL, "libsystem_kernel exports host_get_io_main");

	mach_port_t main_port = MACH_PORT_NULL, master_port = MACH_PORT_NULL;
	expect(get_io_main(mach_host_self(), &main_port) == KERN_SUCCESS, "host_get_io_main succeeds");
	expect(MACH_PORT_VALID(main_port), "host_get_io_main returns a port");
	expect(host_get_io_master(mach_host_self(), &master_port) == KERN_SUCCESS, "host_get_io_master succeeds");
	expect(MACH_PORT_VALID(master_port), "host_get_io_master returns a port");

	mach_port_t unused = 0x7777;
	expect(get_io_main(MACH_PORT_NULL, &unused) == host_get_io_master(MACH_PORT_NULL, &unused) &&
	        get_io_main(MACH_PORT_NULL, &unused) != KERN_SUCCESS,
	    "a null host is refused the same way");

	// the port works for IOKit calls
	void* iokit = dlopen(IO_KIT, RTLD_NOW);
	expect(iokit != NULL, "IOKit loads");
	CFMutableDictionaryRef (*service_matching)(const char*) = dlsym(iokit, "IOServiceMatching");
	kern_return_t (*get_matching)(mach_port_t, CFDictionaryRef, io_iterator_t*) = dlsym(iokit, "IOServiceGetMatchingServices");
	io_object_t (*iterator_next)(io_iterator_t) = dlsym(iokit, "IOIteratorNext");
	kern_return_t (*object_release)(io_object_t) = dlsym(iokit, "IOObjectRelease");
	expect(service_matching && get_matching && iterator_next && object_release, "IOKit exports the matching calls");

	io_iterator_t iter = 0;
	expect(get_matching(main_port, service_matching("IOService"), &iter) == KERN_SUCCESS && iter != 0,
	    "IOServiceGetMatchingServices works with the port from host_get_io_main");
	io_object_t first = iterator_next(iter);
	expect(first != 0, "the IO main port's registry has services");
	object_release(first);
	object_release(iter);

	if (darling) {
		mach_port_t priv = MACH_PORT_NULL;
		expect(host_get_host_priv_port(mach_host_self(), &priv) == KERN_SUCCESS && MACH_PORT_VALID(priv), "host priv port is available");

		// registering the current port again leaves the answer unchanged
		expect(host_set_special_port(priv, IO_MASTER_PORT_ID, main_port) == KERN_SUCCESS,
		    "host_set_special_port accepts the kernel-reserved IO main port ID");
		mach_port_t again = MACH_PORT_NULL;
		expect(get_io_main(mach_host_self(), &again) == KERN_SUCCESS && again == main_port,
		    "host_get_io_main returns the registered port");

		// the IDs that identify the host, and unassigned reserved IDs, stay refused
		expect(host_set_special_port(priv, 1, main_port) == KERN_INVALID_ARGUMENT, "the host port ID is refused");
		expect(host_set_special_port(priv, 2, main_port) == KERN_INVALID_ARGUMENT, "the host priv port ID is refused");
		expect(host_set_special_port(priv, IO_NO_SUCH_ID, main_port) == KERN_INVALID_ARGUMENT, "an unassigned reserved ID is refused");
	}

	printf("PASS: host_get_io_main returns a usable port (%u)\n", main_port);
	return 0;
}
