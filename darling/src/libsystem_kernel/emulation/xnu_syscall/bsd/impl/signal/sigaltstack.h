#ifndef DARLING_BSD_EMULATION_SIGALTSTACK_H
#define DARLING_BSD_EMULATION_SIGALTSTACK_H

// TODO: Move into conversion

struct bsd_stack
{
	void* ss_sp;
	unsigned long ss_size;
	int ss_flags;
};

struct linux_stack
{
	void* ss_sp;
	int ss_flags;
	unsigned long ss_size;
};

long sys_sigaltstack(const struct bsd_stack* ss, struct bsd_stack* oss);

#endif // DARLING_BSD_EMULATION_SIGALTSTACK_H
