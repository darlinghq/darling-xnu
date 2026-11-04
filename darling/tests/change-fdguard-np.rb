# Call the real sys_change_fdguard_np body against a mocked sys_fcntl and check the
# fd flag side effects of guarding and unguarding (see bsd/kern/kern_guarded.c).
# Usage: ruby darling/tests/change-fdguard-np.rb [path/to/emulation]
require 'tmpdir'
root = File.expand_path(ARGV[0] || '../src/libsystem_kernel/emulation', __dir__)
src = File.read(File.join(root, 'src/xnu_syscall/bsd/impl/guarded/change_fdguard_np.c'))
body = src[/^long sys_change_fdguard_np\(.*?^\}/m] or abort 'sys_change_fdguard_np not found'
program = <<~C
  #include <errno.h>
  #include <fcntl.h>
  #include <stdint.h>
  #include <stdio.h>
  static int cloexec[8];
  static long sys_fcntl(int fd, int cmd, long arg) {
      if (fd < 0 || fd >= 8) return -EBADF;
      if (cmd == F_GETFD) return cloexec[fd];
      if (cmd == F_SETFD) { cloexec[fd] = (int)arg; return 0; }
      return -EINVAL;
  }
  #{body}
  #define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\\n", __LINE__, #c); return 1; } } while (0)
  int main(void) {
      uint64_t g = 0x1122334455667788ULL;
      int saved;
      /* Example 1: guard an unguarded fd, then restore it. */
      cloexec[3] = 0; saved = 0;
      CHECK(sys_change_fdguard_np(3, NULL, 0, &g, 1, &saved) == 0);
      CHECK(saved == 0 && cloexec[3] == FD_CLOEXEC);
      CHECK(sys_change_fdguard_np(3, &g, 1, NULL, 0, &saved) == 0);
      CHECK(saved == FD_CLOEXEC && cloexec[3] == 0);
      /* fd that already had close-on-exec keeps it after the round trip. */
      cloexec[4] = FD_CLOEXEC; saved = 0;
      CHECK(sys_change_fdguard_np(4, NULL, 0, &g, 1, &saved) == 0);
      CHECK(saved == FD_CLOEXEC && cloexec[4] == FD_CLOEXEC);
      CHECK(sys_change_fdguard_np(4, &g, 1, NULL, 0, &saved) == 0);
      CHECK(cloexec[4] == FD_CLOEXEC);
      /* NULL fdflagsp is accepted. */
      cloexec[5] = 0;
      CHECK(sys_change_fdguard_np(5, NULL, 0, &g, 1, NULL) == 0 && cloexec[5] == FD_CLOEXEC);
      /* A bad fd fails and leaves the caller's flags alone. */
      saved = 42;
      CHECK(sys_change_fdguard_np(7 + 1, NULL, 0, &g, 1, &saved) == -EBADF && saved == 42);
      puts("PASS: change_fdguard_np returns the old close-on-exec state and sets it when guarding");
      return 0;
  }
C
Dir.mktmpdir('change-fdguard') do |tmp|
  input = File.join(tmp, 'probe.c'); output = File.join(tmp, 'probe')
  File.write(input, program)
  abort 'compile failed' unless system(ENV.fetch('CC', 'clang'), '-O1', '-g', '-Wall', input, '-o', output)
  abort 'change_fdguard_np flag handling is wrong' unless system(output)
end
