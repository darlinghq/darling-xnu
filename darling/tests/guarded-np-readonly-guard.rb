# Call the real guarded_{open,close,kqueue}_np handlers with a guard that sits
# on a read-only page, as a caller's static const guard does, and with a NULL guard
# (EFAULT, as XNU's copyin of the guard fails), an unmapped or PROT_NONE guard (EFAULT, found with a futex
# probe), a zero guard and bad guardflags (EINVAL, checked before the guard is read).
# Usage: ruby darling/tests/guarded-np-readonly-guard.rb [path/to/emulation]
require 'tmpdir'
root = File.expand_path(ARGV[0] || '../src/libsystem_kernel/emulation', __dir__)
dir = File.join(root, 'src/xnu_syscall/bsd/impl/guarded')
guard_args = File.read(File.join(root, 'include/common/guarded/guard_args.h'))
bodies = %w[open close kqueue].map do |kind|
  File.read(File.join(dir, "guarded_#{kind}_np.c"))[/^long sys_guarded_#{kind}_np\(.*?^\}/m] or
    abort "sys_guarded_#{kind}_np not found"
end
program = <<~C
  #include <errno.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <string.h>
  #include <sys/mman.h>
  #include <sys/syscall.h>
  #include <unistd.h>
  // stand-in for the emulation LINUX_SYSCALL, which returns a negative errno (__NR_futex comes from sys/syscall.h)
  static long linux_syscall_probe(long nr, long a, long b, long c, long d) { long r = syscall(nr, a, b, c, d); return r < 0 ? -errno : r; }
  #define LINUX_SYSCALL(nr, a, b, c, d) linux_syscall_probe(nr, (long)(a), (long)(b), (long)(c), (long)(d))
  #{guard_args}
  static long sys_open(const char *path, int flags, unsigned int mode) { (void)path; (void)flags; (void)mode; return 3; }
  static long sys_close(int fd) { (void)fd; return 0; }
  static long sys_kqueue(void) { return 4; }
  #{bodies.join("\n")}
  int main(void) {
      uint64_t *guard = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (guard == MAP_FAILED) return 2;
      *guard = 0x1122334455667788ULL;
      mprotect(guard, 4096, PROT_READ);
      if (sys_guarded_open_np("/x", guard, 3, 0, 0) != 3 || sys_guarded_kqueue_np(guard, 3) != 4 ||
          sys_guarded_close_np(3, guard) != 0 || *guard != 0x1122334455667788ULL)
          return 1;
      if (sys_guarded_open_np("/x", NULL, 3, 0, 0) != -EFAULT || sys_guarded_kqueue_np(NULL, 3) != -EFAULT ||
          sys_guarded_close_np(3, NULL) != -EFAULT)
          return 3;
      long ps = sysconf(_SC_PAGESIZE);
      char *two = mmap(NULL, 2 * ps, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (two == MAP_FAILED) return 2;
      uint64_t *rw = (uint64_t*) two;
      *rw = 7;
      uint64_t *gap = (uint64_t*) (two + ps - 4);   // 4-byte aligned, second half on the next page
      uint64_t *unmapped = (uint64_t*) (two + ps);
      uint64_t *none = (uint64_t*) (two + ps);
      munmap(two + ps, ps);
      if (sys_guarded_open_np("/x", unmapped, 3, 0, 0) != -EFAULT || sys_guarded_kqueue_np(unmapped, 3) != -EFAULT)
          return 7;
      if (sys_guarded_open_np("/x", gap, 3, 0, 0) != -EFAULT || sys_guarded_kqueue_np(gap, 3) != -EFAULT)
          return 7;
      if (mmap(two + ps, ps, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED) return 2;
      if (sys_guarded_open_np("/x", none, 3, 0, 0) != -EFAULT || sys_guarded_kqueue_np(none, 3) != -EFAULT)
          return 8;
      if (sys_guarded_open_np("/x", rw, 3, 0, 0) != 3 || sys_guarded_kqueue_np(rw, 3) != 4)
          return 9;
      if (sys_guarded_open_np("/x", NULL, 0, 0, 0) != -EINVAL || sys_guarded_kqueue_np(NULL, 1) != -EINVAL ||
          sys_guarded_open_np("/x", unmapped, 1, 0, 0) != -EINVAL)
          return 10;
      uint64_t zero = 0;
      if (sys_guarded_open_np("/x", &zero, 3, 0, 0) != -EINVAL || sys_guarded_kqueue_np(&zero, 3) != -EINVAL)
          return 4;
      uint64_t one = 1;
      unsigned bad[] = { 0, 1, 1u << 2, 3 | (1u << 5), 3 | (1u << 31) };
      for (unsigned i = 0; i < sizeof bad / sizeof *bad; ++i)
          if (sys_guarded_open_np("/x", &one, bad[i], 0, 0) != -EINVAL || sys_guarded_kqueue_np(&one, bad[i]) != -EINVAL)
              return 5;
      if (sys_guarded_open_np("/x", &one, 0x1f, 0, 0) != 3 || sys_guarded_kqueue_np(&one, 2) != 4)
          return 6;
      puts("PASS: the guard is only read, NULL, unmapped and PROT_NONE guards are EFAULT, zero guard and bad guardflags are EINVAL");
      return 0;
  }
C
Dir.mktmpdir('guarded-np') do |tmp|
  input = File.join(tmp, 'probe.c'); output = File.join(tmp, 'probe')
  File.write(input, program)
  abort 'compile failed' unless system(ENV.fetch('CC', 'clang'), '-O1', '-g',
      '-Wall', input, '-o', output)
  abort "guarded_*_np wrote the guard or accepted a NULL, unmapped or zero guard or bad guardflags (probe exit #{$?.exitstatus})" unless system(output, rlimit_core: 0)
end
