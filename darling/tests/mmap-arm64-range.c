#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
#define DARWIN_LIMIT UINT64_C(0x00007ffffe000000)
#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "FAIL line %d: %s (errno %d)\n", __LINE__, #expr, errno); return 1; } } while (0)
static int in_range(void *p, size_t size) {
    return p != MAP_FAILED && (uintptr_t)p < DARWIN_LIMIT && size <= DARWIN_LIMIT - (uintptr_t)p;
}
int main(void) {
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    unsigned char *p = mmap(NULL, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    CHECK(in_range(p, page));
    p[0] = 0x5a;
    errno = 0;
    CHECK(mmap(p, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE, -1, 0) == MAP_FAILED);
    CHECK(errno == EEXIST && p[0] == 0x5a);
    CHECK(mmap(p, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) == p);
    CHECK(p[0] == 0);
    CHECK(munmap(p, page) == 0);
    CHECK(mmap(p, page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE, -1, 0) == p);
    CHECK(munmap(p, page) == 0);
    CHECK(mmap((void *)DARWIN_LIMIT, page, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) == MAP_FAILED);
    CHECK(mmap((void *)(DARWIN_LIMIT - page), page * 2, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE, -1, 0) == MAP_FAILED);
    CHECK(mmap(NULL, SIZE_MAX, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0) == MAP_FAILED);
    for (int i = 0; i < 64; ++i) {
        size_t size = page * (i + 1);
        p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        CHECK(in_range(p, size));
        p[size - 1] = (unsigned char)i;
        CHECK(munmap(p, size) == 0);
    }
    char path[] = "/tmp/darling-mmap-range-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    CHECK(unlink(path) == 0);
    CHECK(ftruncate(fd, page * 2) == 0);
    unsigned char value = 0x73;
    CHECK(pwrite(fd, &value, 1, page) == 1);
    p = mmap(NULL, page, PROT_READ, MAP_SHARED, fd, page);
    CHECK(in_range(p, page) && p[0] == value);
    CHECK(munmap(p, page) == 0 && close(fd) == 0);
    mach_vm_address_t addr = 0;
    CHECK(mach_vm_allocate(mach_task_self(), &addr, page, VM_FLAGS_ANYWHERE) == KERN_SUCCESS);
    CHECK(addr < DARWIN_LIMIT && page <= DARWIN_LIMIT - addr);
    CHECK(mach_vm_deallocate(mach_task_self(), addr, page) == KERN_SUCCESS);
    puts("ARM64 mmap range and mapping semantics passed");
    return 0;
}
