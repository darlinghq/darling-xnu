# Exercise the actual ARM64 handler with mocked procfs I/O.
# Usage: ruby darling/tests/arm64-core-count.rb [path/to/emulation]
require 'tmpdir'
root = File.expand_path(ARGV[0] || '../src/libsystem_kernel/emulation', __dir__)
source = File.read(File.join(root, 'src/xnu_syscall/bsd/helper/misc/sysctl_machdep.c'))
body = source[/^sysctl_handler\(handle_core_count\)\n\{.*?^\}/m]
header = File.read(File.join(root, 'include/xnu_syscall/bsd/impl/misc/sysctl.h'))
size_macro = header[/^#define sysctl_handle_size.*?\n\n/m]
abort 'handler/size macro not found' unless body && size_macro
program = <<~C
  #include <assert.h>
  #include <errno.h>
  #include <stdio.h>
  #include <string.h>
  #define sysctl_handler(name) long name(void *old, unsigned long *oldlen)
  #define LINUX_O_RDONLY 0
  #{size_macro}
  struct simple_readline_buf { int unused; };
  static int processors, cursor, fail_open, opens, closes;
  static int sys_open(const char *path, int flags, int mode) {
      assert(strcmp(path, "/proc/cpuinfo") == 0 && flags == 0 && mode == 0);
      ++opens; return fail_open ? -ENOENT : 17;
  }
  static int sys_close(int fd) { assert(fd == 17); ++closes; return 0; }
  static void __simple_readline_init(struct simple_readline_buf *buf) { (void)buf; cursor = 0; }
  static char *__simple_readline(int fd, struct simple_readline_buf *buf, char *line, size_t size) {
      assert(fd == 17); (void)buf;
      if (cursor++ >= processors) return NULL;
      snprintf(line, size, "processor : %d", cursor - 1); return line;
  }
  #define __simple_sprintf sprintf
  static void __attribute__((unused)) copyout_string(const char *src, char *out, unsigned long *len) {
      if (out && *len) snprintf(out, *len, "%s", src);
      if (len) *len = strlen(src);
  }
  #{body}
  static void check(int n, int failure, int expected) {
      processors = n; fail_open = failure; opens = closes = 0;
      unsigned char storage[sizeof(int) + 2]; memset(storage, 0xa5, sizeof storage);
      unsigned long size = sizeof(int);
      assert(handle_core_count(storage + 1, &size) == 0 && size == sizeof(int));
      int actual; memcpy(&actual, storage + 1, sizeof actual); assert(actual == expected);
      assert(storage[0] == 0xa5 && storage[sizeof(storage)-1] == 0xa5);
      assert(opens == 1 && closes == (failure ? 0 : 1));
  }
  int main(void) {
      check(1, 0, 1); check(12, 0, 12); check(0, 0, 1); check(0, 1, 1);
      opens = closes = 0;
      unsigned long size = 0;
      assert(handle_core_count(NULL, &size) == 0 && size == sizeof(int));
      assert(opens == 0 && closes == 0);
      unsigned char small[3] = {0xa5, 0xa5, 0xa5}; size = sizeof small;
      assert(handle_core_count(small, &size) == -EINVAL);
      assert(size == sizeof small && small[0] == 0xa5 && small[2] == 0xa5);
      assert(handle_core_count(small, NULL) == -EINVAL && opens == 0);
      puts("PASS: integer result, fallback, size query, short/null length and unaligned output");
  }
C
Dir.mktmpdir('arm64-core-count') do |dir|
  input = File.join(dir, 'probe.c'); output = File.join(dir, 'probe')
  File.write(input, program)
  abort 'compile failed' unless system(ENV.fetch('CC', 'clang'), '-O2', '-g',
      '-Wall', '-Wextra', '-fsanitize=address,undefined', input, '-o', output)
  abort 'core-count regression' unless system(output, rlimit_core: 0)
end
