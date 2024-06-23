#ifndef LINUX_SIGALTSTACK_H
#define LINUX_SIGALTSTACK_H

// [xnu]/bsd/sys/signal.h (based on `struct user64_sigaltstack`?)
struct bsd_stack
{
	void* ss_sp;
	unsigned long ss_size;
	int ss_flags;
};

// /usr/include/asm-generic/signal.h (based on `typedef struct sigaltstack {...} stack_t`)
struct linux_stack
{
	void* ss_sp;
	int ss_flags;
	unsigned long ss_size;
};

long sys_sigaltstack(const struct bsd_stack* ss, struct bsd_stack* oss);

#endif

