// An EXC_BAD_ACCESS handler that inspects and suspends the faulting thread, as garbage collectors do for
// safepoint faults (thread_get_state(3), thread_suspend(3)).
//
// A thread reads, then writes, a page it has no access to. For each fault, its exception handler reads the
// thread's x86_EXCEPTION_STATE64: a page fault (trap 14) at the page's address, with the write bit (2) of the
// error code set for the write only. The handler then suspends the thread, makes the page accessible and
// replies. The thread stays suspended until the handler resumes it (later after the read, at once after the
// write), then carries on. Observed on macOS 27 (x86_64 under Rosetta).
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#pragma pack(4)
typedef struct {
	mach_msg_header_t head;
	mach_msg_body_t body;
	mach_msg_port_descriptor_t thread, task;
	NDR_record_t ndr;
	exception_type_t exception;
	mach_msg_type_number_t code_count;
	int64_t code[2];
	mach_msg_trailer_t trailer;
} request_t;
typedef struct {
	mach_msg_header_t head;
	NDR_record_t ndr;
	kern_return_t ret;
} reply_t;
#pragma pack()

static int failures;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); __atomic_add_fetch(&failures, 1, __ATOMIC_RELAXED); } } while (0)

static mach_port_t port;
static volatile int* page;
static volatile int read_done; // the worker has resumed after the read fault
static volatile int armed; // the page has been protected again for the write

static void handle_one(int write) {
	request_t req;
	kern_return_t kr = mach_msg(&req.head, MACH_RCV_MSG, 0, sizeof req, port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	CHECK(kr == KERN_SUCCESS, "mach_msg receive: %s", mach_error_string(kr));
	if (kr != KERN_SUCCESS)
		exit(1);
	mach_port_t thread = req.thread.name;

#if defined(__x86_64__)
	x86_exception_state64_t es;
	mach_msg_type_number_t count = x86_EXCEPTION_STATE64_COUNT;
	kr = thread_get_state(thread, x86_EXCEPTION_STATE64, (thread_state_t)&es, &count);
	CHECK(kr == KERN_SUCCESS, "thread_get_state(x86_EXCEPTION_STATE64): %s", mach_error_string(kr));
	if (kr == KERN_SUCCESS) {
		CHECK(count == x86_EXCEPTION_STATE64_COUNT, "count %u", count);
		CHECK(es.__trapno == 14, "trapno %u, expected 14", es.__trapno);
		CHECK(es.__faultvaddr == (uintptr_t)page, "faultvaddr %#llx, expected %p", (unsigned long long)es.__faultvaddr, page);
		CHECK(!!(es.__err & 2) == write, "err %#x: the write bit says %s", es.__err, write ? "read" : "write");
	}

	x86_exception_state_t ues;
	count = x86_EXCEPTION_STATE_COUNT;
	kr = thread_get_state(thread, x86_EXCEPTION_STATE, (thread_state_t)&ues, &count);
	CHECK(kr == KERN_SUCCESS && ues.esh.flavor == x86_EXCEPTION_STATE64 && ues.esh.count == x86_EXCEPTION_STATE64_COUNT
		&& ues.ues.es64.__faultvaddr == (uintptr_t)page, "thread_get_state(x86_EXCEPTION_STATE): %s", mach_error_string(kr));
#endif

	// the thread waits for this reply, but suspending it must not wait for the reply
	kr = thread_suspend(thread);
	CHECK(kr == KERN_SUCCESS, "thread_suspend: %s", mach_error_string(kr));
	mprotect((void*)page, getpagesize(), write ? PROT_READ | PROT_WRITE : PROT_READ);

	reply_t rep;
	memset(&rep, 0, sizeof(rep));
	rep.head.msgh_bits = MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(req.head.msgh_bits), 0);
	rep.head.msgh_size = sizeof rep;
	rep.head.msgh_remote_port = req.head.msgh_remote_port;
	rep.head.msgh_id = req.head.msgh_id + 100;
	rep.ndr = NDR_record;
	rep.ret = KERN_SUCCESS;
	kr = mach_msg(&rep.head, MACH_SEND_MSG, sizeof rep, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	CHECK(kr == KERN_SUCCESS, "mach_msg reply: %s", mach_error_string(kr));

	// after the read fault, the thread stays suspended for a while; after the write fault, it is resumed at once
	if (!write) {
		usleep(100000);
		CHECK(!__atomic_load_n(&read_done, __ATOMIC_ACQUIRE), "the thread ran on while suspended");
	}
	kr = thread_resume(thread);
	CHECK(kr == KERN_SUCCESS, "thread_resume: %s", mach_error_string(kr));
}

static void* handler(void* arg) {
	handle_one(0);
	while (!__atomic_load_n(&read_done, __ATOMIC_ACQUIRE))
		usleep(1000);
	mprotect((void*)page, getpagesize(), PROT_NONE);
	__atomic_store_n(&armed, 1, __ATOMIC_RELEASE);
	handle_one(1);
	return NULL;
}

static void* worker(void* arg) {
	thread_set_exception_ports(mach_thread_self(), EXC_MASK_BAD_ACCESS, port,
	                           EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, THREAD_STATE_NONE);
	int v = *page;
	CHECK(v == 0, "read %d", v);
	__atomic_store_n(&read_done, 1, __ATOMIC_RELEASE);
	while (!__atomic_load_n(&armed, __ATOMIC_ACQUIRE))
		usleep(1000);
	*page = 1;
	return NULL;
}

static void timed_out(int sig) {
	static const char msg[] = "FAIL: timed out\nFAIL exception state and suspension in an exception handler\n";
	write(1, msg, sizeof(msg) - 1);
	_exit(1);
}

int main(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	signal(SIGALRM, timed_out);
	alarm(10);
	page = mmap(NULL, getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
	mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
	pthread_t h, w;
	pthread_create(&h, NULL, handler, NULL);
	pthread_create(&w, NULL, worker, NULL);
	pthread_join(w, NULL);
	pthread_join(h, NULL);
	CHECK(*page == 1, "the write did not happen");
	printf("%s exception state and suspension in an exception handler\n", failures ? "FAIL" : "PASS");
	return failures != 0;
}
