# Exercise the real copyout_string used by the string sysctls.
# Usage: ruby darling/tests/sysctl-string-length.rb [path/to/emulation]
# Needs a host libc that declares strlcpy (glibc 2.38 or later, or BSD libc).
require 'tmpdir'
root = File.expand_path(ARGV[0] || '../src/libsystem_kernel/emulation', __dir__)
source = File.read(File.join(root, 'src/xnu_syscall/bsd/impl/misc/sysctl.c'))
body = source[/^long copyout_string\(.*?^\}/m]
truncated = source[/^long copyout_string_truncated\(.*?^\}/m]
abort 'copyout_string not found' unless body && truncated
program = <<~C
  #include <assert.h>
  #include <errno.h>
  #include <stdio.h>
  #include <string.h>
  #{body}
  #{truncated}
  int main(void) {
      unsigned long len = 0;
      copyout_string("Darwin", NULL, &len);
      if (len != 7) { printf("FAIL: size query reports %lu, expected 7\\n", len); return 1; }

      char out[16];
      memset(out, 'x', sizeof out);
      len = sizeof out;
      copyout_string("Darwin", out, &len);
      if (len != 7 || strcmp(out, "Darwin") != 0) {
          printf("FAIL: full copy reports %lu\\n", len); return 1;
      }

      memset(out, 'x', sizeof out);
      len = 7;
      assert(copyout_string("Darwin", out, &len) == 0 && len == 7 && strcmp(out, "Darwin") == 0);

      memset(out, 'x', sizeof out);
      len = 6;
      assert(copyout_string("Darwin", out, &len) == -ENOMEM && out[0] == 'x');

      memset(out, 'x', sizeof out);
      len = 4;
      assert(copyout_string_truncated("Darwin", out, &len) == 0);
      assert(len == 4 && memcmp(out, "Dar", 4) == 0 && out[4] == 'x');
      puts("PASS: lengths include the NUL; a short buffer is ENOMEM, or a terminated prefix for the truncating variant");
      return 0;
  }
C
Dir.mktmpdir('sysctl-string-length') do |dir|
  input = File.join(dir, 'probe.c'); output = File.join(dir, 'probe')
  File.write(input, program)
  abort 'compile failed' unless system(ENV.fetch('CC', 'clang'), '-O1', '-g',
      '-Wall', '-fsanitize=address,undefined', input, '-o', output)
  abort 'sysctl string length regression' unless system(output, rlimit_core: 0)
end
