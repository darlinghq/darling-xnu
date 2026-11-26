// Anonymous mappings far larger than the machine's memory succeed, as on
// macOS, where nothing is set aside when memory is mapped: an 8 GB Mac maps
// 4 TB of read-write anonymous memory (observed on macOS 26.7.3, x86_64).
// Pages are only committed as they are touched; the test touches the first and
// last page of each mapping. JavaScriptCore's Gigacage reserves tens of GB
// this way when it starts.
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

static int failures;

static void check_mmap(unsigned long gigabytes, int prot)
{
    size_t length = (size_t)gigabytes << 30;
    void *p = mmap(NULL, length, prot, MAP_PRIVATE | MAP_ANON, -1, 0);
    int ok = p != MAP_FAILED;
    printf("%s: mmap %lu GB %s%s%s\n", ok ? "PASS" : "FAIL", gigabytes,
           prot == PROT_NONE ? "without access" : "read-write", ok ? "" : ": ", ok ? "" : strerror(errno));
    if (!ok) {
        failures++;
        return;
    }
    if (prot != PROT_NONE) {
        ((volatile char *)p)[0] = 1;
        ((volatile char *)p)[length - 1] = 1;
    }
    munmap(p, length);
}

static void check_vm_allocate(unsigned long gigabytes)
{
    mach_vm_address_t address = 0;
    mach_vm_size_t size = (mach_vm_size_t)gigabytes << 30;
    kern_return_t kr = mach_vm_allocate(mach_task_self(), &address, size, VM_FLAGS_ANYWHERE);
    printf("%s: mach_vm_allocate %lu GB (%d)\n", kr == KERN_SUCCESS ? "PASS" : "FAIL", gigabytes, kr);
    if (kr != KERN_SUCCESS) {
        failures++;
        return;
    }
    ((volatile char *)address)[0] = 1;
    ((volatile char *)address)[size - 1] = 1;
    mach_vm_deallocate(mach_task_self(), address, size);
}

int main(void)
{
    unsigned long sizes[] = { 16, 64, 128, 512, 4096 };
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        check_mmap(sizes[i], PROT_NONE);
        check_mmap(sizes[i], PROT_READ | PROT_WRITE);
        check_vm_allocate(sizes[i]);
    }
    return failures != 0;
}
