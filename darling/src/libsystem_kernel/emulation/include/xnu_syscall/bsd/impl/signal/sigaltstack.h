#ifndef LINUX_SIGALTSTACK_H
#define LINUX_SIGALTSTACK_H

#define BSD_SS_ONSTACK 1
#define BSD_SS_DISABLE 4
#define LINUX_SS_ONSTACK 1
#define LINUX_SS_DISABLE 2

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

#endif // LINUX_SIGALTSTACK_H
