.text
.globl _linux_sig_restorer
.private_extern _linux_sig_restorer

#ifdef __x86_64__
_linux_sig_restorer:
	movl $15, %eax
	syscall

#elif defined(__i386__)

_linux_sig_restorer:
	movl $173, %eax
	int $0x80
#endif

