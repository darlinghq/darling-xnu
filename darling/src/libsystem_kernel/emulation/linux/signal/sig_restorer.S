.text
.globl _sig_restorer
.private_extern _sig_restorer

#ifdef __x86_64__
#define __NR_rt_sigreturn 15
_sig_restorer:
	movl $__NR_rt_sigreturn, %eax
	syscall

#elif defined(__i386__)
#define __NR_rt_sigreturn 173
_sig_restorer:
	movl $__NR_rt_sigreturn, %eax
	int $0x80

#elif defined(__arm64__)
#define __NR_rt_sigreturn 139
_sig_restorer:
	mov w8, #__NR_rt_sigreturn
	svc #0

#else
#error "Missing sig_restorer implementation for arch"
#endif

