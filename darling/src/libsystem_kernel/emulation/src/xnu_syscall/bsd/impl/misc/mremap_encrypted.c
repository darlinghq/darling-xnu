#include <darling/emulation/xnu_syscall/bsd/impl/misc/mremap_encrypted.h>
#include <darling/emulation/common/simple.h>

/*
 * BSD syscall 489: mremap_encrypted (FairPlay decryption registration).
 *
 * On real iOS/macOS this asks the kernel to set up on-the-fly decryption for an
 * encrypted segment of a Mach-O. Darling-shipped Mach-Os always have cryptid=0
 * (not actually encrypted), and dyld still issues the call. For cryptid=0 it is
 * effectively a no-op so we just return success.
 *
 * For cryptid != 0 we have no way to actually decrypt — return success and let
 * the binary's signature/integrity checks catch any real issue.
 */
long sys_mremap_encrypted(void* addr, unsigned long len, unsigned int cryptid, unsigned int cputype, unsigned int cpusubtype)
{
	(void)addr;
	(void)len;
	(void)cputype;
	(void)cpusubtype;
	if (cryptid != 0) {
		__simple_printf("mremap_encrypted: stub ignoring cryptid=%u\n", cryptid);
	}
	return 0;
}
