# sys_madvise maps Darwin MADV_FREE/REUSABLE/REUSE onto Linux; host build with a fake madvise.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
emu="#{root}/darling/src/libsystem_kernel/emulation"
source=File.read("#{emu}/src/xnu_syscall/bsd/impl/mman/madvise.c").lines.reject{|l|l.start_with?('#include ')}.join
Dir.mktmpdir('madvise-free-reuse-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <stdlib.h>
    #include <string.h>
    #define LINUX_EINVAL 22
    #define LINUX_ENOMEM 12
    #define LINUX_EFAULT 14
    #define BSD_ENOMEM 1012 /* distinct, so translation is visible */
    #define __NR_madvise 0
    #define LINUX_SYSCALL(n, ...) fake_madvise(__VA_ARGS__)
    static int old_kernel, calls, last_advice, next_error;
    static unsigned long last_len;
    static int fake_madvise(void *addr, unsigned long len, int advice) {
      if (old_kernel && advice == 23) return -LINUX_EINVAL; /* Linux < 5.14 */
      if (len == 0) return 0;
      calls++; last_advice=advice; last_len=len;
      int e=next_error; next_error=0;
      return e;
    }
    static int errno_linux_to_bsd(int e) { return e == -LINUX_ENOMEM ? -BSD_ENOMEM : e; }
    #{source}
    #define CALL(advice) (calls=0, last_advice=-1, sys_madvise((void *)0x10000, 0x4000, (advice)))
    int main(int argc, char **argv) {
      old_kernel = argc > 1 && !strcmp(argv[1], "old");

      /* Darwin MADV_FREE (5): Linux MADV_FREE (8) on every kernel. */
      assert(CALL(5)==0 && calls==1 && last_advice==8 && last_len==0x4000);
      /* Mappings Linux cannot lazily free (EINVAL): advisory, still succeeds. */
      next_error=-LINUX_EINVAL;
      assert(CALL(5)==0 && calls==1);
      next_error=-LINUX_ENOMEM;
      assert(CALL(5)==-BSD_ENOMEM);

      if (!old_kernel) {
        /* REUSABLE (7) -> MADV_FREE, REUSE (8) -> MADV_POPULATE_WRITE (23). */
        assert(CALL(7)==0 && calls==1 && last_advice==8);
        assert(CALL(8)==0 && calls==1 && last_advice==23 && last_len==0x4000);
        /* XNU's reuse does not populate, so populate failures other than a hole are not errors. */
        next_error=-LINUX_EFAULT;
        assert(CALL(8)==0);
        next_error=-LINUX_ENOMEM;
        assert(CALL(8)==-BSD_ENOMEM);
      } else {
        /* No way to cancel a lazy free: REUSABLE and REUSE stay no-ops. */
        assert(CALL(7)==0 && calls==0);
        assert(CALL(8)==0 && calls==0);
      }

      /* Shared advice values still pass through; other Darwin-only ones are ignored. */
      assert(CALL(3)==0 && calls==1 && last_advice==3);
      assert(CALL(6)==0 && calls==0);
      assert(CALL(9)==0 && calls==0);
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    %w[new old].each do |kernel|
      out,status=Open3.capture2e("#{dir}/probe",kernel)
      abort "#{kernel} kernel: #{out}" unless status.success?
    end
  end
  puts 'PASS madvise FREE/REUSABLE/REUSE map to Linux MADV_FREE/POPULATE_WRITE, no-op without 5.14, at O0/O2'
end
