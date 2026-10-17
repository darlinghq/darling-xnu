# Call the real guarded_{open,close,kqueue}_np handlers with a guard that sits
# on a read-only page, as a caller's static const guard does, and with a NULL guard
# (EFAULT, as XNU's copyin of the guard fails).
# Usage: ruby darling/tests/guarded-np-readonly-guard.rb [path/to/emulation]
require 'tmpdir'
root = File.expand_path(ARGV[0] || '../src/libsystem_kernel/emulation', __dir__)
dir = File.join(root, 'src/xnu_syscall/bsd/impl/guarded')
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
  static long sys_open(const char *path, int flags, unsigned int mode) { (void)path; (void)flags; (void)mode; return 3; }
  static long sys_close(int fd) { (void)fd; return 0; }
  static long sys_kqueue(void) { return 4; }
  #{bodies.join("\n")}
  int main(void) {
      uint64_t *guard = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (guard == MAP_FAILED) return 2;
      *guard = 0x1122334455667788ULL;
      mprotect(guard, 4096, PROT_READ);
      if (sys_guarded_open_np("/x", guard, 1, 0, 0) != 3 || sys_guarded_kqueue_np(guard, 1) != 4 ||
          sys_guarded_close_np(3, guard) != 0 || *guard != 0x1122334455667788ULL)
          return 1;
      if (sys_guarded_open_np("/x", NULL, 1, 0, 0) != -EFAULT || sys_guarded_kqueue_np(NULL, 1) != -EFAULT ||
          sys_guarded_close_np(3, NULL) != -EFAULT)
          return 3;
      puts("PASS: the guard is only read and a NULL guard is EFAULT");
      return 0;
  }
C
Dir.mktmpdir('guarded-np') do |tmp|
  input = File.join(tmp, 'probe.c'); output = File.join(tmp, 'probe')
  File.write(input, program)
  abort 'compile failed' unless system(ENV.fetch('CC', 'clang'), '-O1', '-g',
      '-Wall', input, '-o', output)
  abort 'guarded_*_np wrote the guard or accepted a NULL guard' unless system(output, rlimit_core: 0)
end
