# Actual pselect wrapper with controlled syscall adapter; not signal delivery.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
source=File.read("#{root}/darling/src/libsystem_kernel/emulation/src/xnu_syscall/bsd/impl/select/pselect.c").lines.reject{|l|l.start_with?('#include ')}.join
Dir.mktmpdir('pselect-mask-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <stdint.h>
    #include <stddef.h>
    typedef unsigned sigset_t;
    typedef unsigned long long linux_sigset_t;
    struct bsd_timeval { long tv_sec; int tv_usec; };
    #define CANCELATION_POINT() ((void)0)
    #define LINUX_SYSCALL(n, ...) probe(__VA_ARGS__)
    static linux_sigset_t expected;
    static int expect_mask;
    static void sigset_bsd_to_linux(const sigset_t *in, linux_sigset_t *out) { *out=*in; }
    static int errno_linux_to_bsd(int e) { return e; }
    static int probe(int n, void *r, void *w, void *e, const void *t, const void *mask) {
      assert(!!mask==expect_mask);
      if (mask) {
        const long *data=mask;
        assert(data[1]==sizeof(linux_sigset_t));
        assert((uintptr_t)data[0]>4096);
        assert(*(const linux_sigset_t *)data[0]==expected);
      }
      return 0;
    }
    long sys_pselect_nocancel(int,void*,void*,void*,struct bsd_timeval*,const sigset_t*);
    #{source}
    int main(void) {
      sigset_t mask=4; expected=4; expect_mask=1;
      assert(sys_pselect_nocancel(0,0,0,0,0,&mask)==0);
      mask=0; expected=0;
      assert(sys_pselect_nocancel(0,0,0,0,0,&mask)==0);
      expect_mask=0;
      assert(sys_pselect_nocancel(0,0,0,0,0,0)==0);
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,'-fsanitize=address,undefined',"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  puts 'PASS nonempty, empty and absent pselect signal masks at O0/O2'
end
