// Copyright (C) 2026 Lubos Dolezel
// thread_selfusage (syscall 482) returns the calling thread's CPU time in mach
// absolute time units: it tracks CLOCK_THREAD_CPUTIME_ID scaled by the
// timebase (1/1 on macOS 26.7.3 x86_64 and here), is monotonic, grows with
// CPU work of this thread only, and does not grow while the thread sleeps or
// while another thread burns CPU.
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <mach/mach_time.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d)\n", #c, __LINE__); failures++; } } while (0)

#ifdef __i386__
// syscall(2) returns an int on i386, so only the low 32 bits are visible
// here (about 4.3 s of CPU time wrap); the handler itself returns 64 bits in
// %edx:%eax. Keep the test within the 32-bit window.
static uint64_t usage(void) { return (uint32_t) syscall(SYS_thread_selfusage); }
#else
static uint64_t usage(void) { return (uint64_t) syscall(SYS_thread_selfusage); }
#endif

static uint64_t cputime_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
	return (uint64_t) ts.tv_sec * 1000000000ull + (uint64_t) ts.tv_nsec;
}

static volatile unsigned long sink;
static void burn(unsigned long n) { for (unsigned long i = 0; i < n; i++) sink += i; }

static void *burner(void *arg)
{
	(void) arg;
	burn(300000000UL);
	return NULL;
}

int main(void)
{
	mach_timebase_info_data_t tb;
	mach_timebase_info(&tb);
	CHECK(tb.numer == 1 && tb.denom == 1);

	uint64_t u0 = usage(), c0 = cputime_ns();
	CHECK(u0 > 0);
	burn(50000000UL);
	uint64_t u1 = usage(), c1 = cputime_ns();
	CHECK(u1 > u0);
	// both clocks measured the same CPU time, to within 20 ms of scheduling noise
	uint64_t du = u1 - u0, dc = c1 - c0;
	CHECK(du + 20000000ull >= dc && dc + 20000000ull >= du);

	// sleeping uses no CPU
	uint64_t s0 = usage();
	usleep(200000);
	uint64_t s1 = usage();
	CHECK(s1 >= s0 && s1 - s0 < 50000000ull);

	// another thread's CPU time is not counted
	pthread_t t;
	pthread_create(&t, NULL, burner, NULL);
	uint64_t o0 = usage();
	usleep(300000);
	uint64_t o1 = usage();
	CHECK(o1 - o0 < 50000000ull);
	pthread_join(t, NULL);

	if (failures) return 1;
	printf("PASS\n");
	return 0;
}
