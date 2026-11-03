# sys_getrlimit/sys_setrlimit translate RLIM_INFINITY; host build with a fake prlimit64.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
emu="#{root}/darling/src/libsystem_kernel/emulation"
strip=->(path){File.read("#{emu}/src/xnu_syscall/bsd/impl/misc/#{path}").lines.reject{|l|l.start_with?('#include ')}.join}
Dir.mktmpdir('rlimit-infinity-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <errno.h>
    #include <stddef.h>
    #include "#{emu}/include/conversion/misc/getrlimit.h"
    #define __NR_prlimit64 0
    #define LINUX_SYSCALL(n, ...) prlimit64(__VA_ARGS__)
    static unsigned long long host_cur, host_max;
    static int prlimit64(int pid, unsigned int which, struct xnu_rlimit *set, struct xnu_rlimit *get) {
      if (set) { host_cur=set->rlim_cur; host_max=set->rlim_max; }
      if (get) { get->rlim_cur=host_cur; get->rlim_max=host_max; }
      return 0;
    }
    static int errno_linux_to_bsd(int e) { return e; }
    #{strip['getrlimit.c']}
    #{strip['setrlimit.c']}
    /* Darwin's RLIM_INFINITY, bsd/sys/resource.h */
    #define DARWIN_INF ((1ULL << 63) - 1)
    int main(void) {
      struct xnu_rlimit lim;
      host_cur=host_max=~0ULL;
      assert(sys_getrlimit(BSD_RLIMIT_STACK,&lim)==0);
      assert(lim.rlim_cur==DARWIN_INF && lim.rlim_max==DARWIN_INF);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==10240 && lim.rlim_max==10240);
      host_cur=8<<20; host_max=~0ULL;
      assert(sys_getrlimit(BSD_RLIMIT_STACK|BSD_RLIMIT_POSIX,&lim)==0);
      assert(lim.rlim_cur==8<<20 && lim.rlim_max==DARWIN_INF);
      host_cur=host_max=1ULL<<63;
      assert(sys_getrlimit(BSD_RLIMIT_CORE,&lim)==0);
      assert(lim.rlim_cur==DARWIN_INF && lim.rlim_max==DARWIN_INF);

      lim.rlim_cur=4096; lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_CORE,&lim)==0);
      assert(host_cur==4096 && host_max==~0ULL);
      lim.rlim_cur=lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_CORE,&lim)==0);
      assert(host_cur==~0ULL && host_max==~0ULL);
      assert(sys_getrlimit(BSD_RLIMIT_CORE,&lim)==0);
      assert(lim.rlim_cur==DARWIN_INF && lim.rlim_max==DARWIN_INF);
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  puts 'PASS Darwin and Linux RLIM_INFINITY round-trip through getrlimit/setrlimit at O0/O2'
end
