# sys_getrlimit/sys_setrlimit translate RLIM_INFINITY, reject NULL limits and keep the guest's
# RLIMIT_NOFILE apart from the Linux one; host build with a fake prlimit64.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
emu="#{root}/darling/src/libsystem_kernel/emulation"
strip=->(path){File.read("#{emu}/src/xnu_syscall/bsd/#{path}").lines.reject{|l|l.start_with?('#include ')}.join}
Dir.mktmpdir('rlimit-infinity-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <errno.h>
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>
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
    /* stand-ins for what nofile_limit.c uses */
    typedef int libsimple_lock_t;
    #define LIBSIMPLE_LOCK_INITIALIZER 0
    #define libsimple_lock_lock(l) ((void)(l))
    #define libsimple_lock_unlock(l) ((void)(l))
    #define libsimple_lock_init(l) ((void)(l))
    #define __simple_snprintf snprintf
    struct elf_calls { int (*inherited_nofile_limit)(unsigned int limits[2]); };
    static unsigned int inherited[2];
    static int have_inherited;
    static int fake_inherited(unsigned int limits[2]) {
      if (!have_inherited) return -1;
      limits[0]=inherited[0]; limits[1]=inherited[1];
      return 0;
    }
    static struct elf_calls fake_calls = { fake_inherited };
    struct elf_calls* _elfcalls = &fake_calls;
    static long euid = 501;
    static long sys_geteuid(void) { return euid; }
    static int last_closed = -1;
    static long close_internal(int fd) { last_closed=fd; return 0; }
    #{strip['helper/misc/nofile_limit.c']}
    #{strip['impl/misc/getrlimit.c']}
    #{strip['impl/misc/setrlimit.c']}
    /* forget the guest's RLIMIT_NOFILE, as in a new process */
    static void reset_nofile(void) { nofile_state=nofile_state_unknown; nofile_enforced=-1; }
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

      assert(sys_getrlimit(BSD_RLIMIT_CORE,NULL)==-EFAULT);
      assert(sys_setrlimit(BSD_RLIMIT_CORE,NULL)==-EFAULT);
      assert(sys_getrlimit(100,NULL)==-EINVAL);

      /* RLIMIT_NOFILE: until the guest sets one, the host's limit capped at OPEN_MAX */
      reset_nofile();
      host_cur=256; host_max=524288;
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==256 && lim.rlim_max==10240);
      assert(!nofile_limit_exceeded(100000));
      char env[64];
      assert(nofile_limit_exec_env(env,sizeof(env))==0);
      /* QUIRK: RLIM_INFINITY is accepted while the hard limit is not lowered;
         the host soft limit is raised as needed */
      lim.rlim_cur=lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(host_cur==10240 && host_max==524288);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==10240 && lim.rlim_max==10240);
      lim.rlim_cur=DARWIN_INF; lim.rlim_max=10240;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==-EINVAL);
      assert(host_cur==10240);

      /* lowering never lowers the host limit (Darling's RPC sockets sit below
         the host soft limit); the guest's limit is reported and enforced */
      reset_nofile();
      host_cur=host_max=524288;
      lim.rlim_cur=10240; lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(host_cur==524288 && host_max==524288);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==10240 && lim.rlim_max==10240);
      assert(!nofile_limit_exceeded(20000));
      lim.rlim_cur=256; lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(host_cur==524288 && host_max==524288);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE|BSD_RLIMIT_POSIX,&lim)==0);
      assert(lim.rlim_cur==256 && lim.rlim_max==10240);
      assert(nofile_limit_exceeded(256) && !nofile_limit_exceeded(255));
      assert(nofile_limit_check_new(300)==-EMFILE && last_closed==300);
      assert(nofile_limit_check_new(255)==255 && last_closed==300);
      assert(nofile_limit_check_new(-EBADF)==-EBADF);
      assert(nofile_limit_exec_env(env,sizeof(env))==1 && strcmp(env,"__mldr_nofile=256,10240")==0);
      lim.rlim_cur=lim.rlim_max=256;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(host_cur==524288 && host_max==524288);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==256 && lim.rlim_max==256);
      /* raising a lowered hard limit is EPERM for non-root, also with RLIM_INFINITY */
      lim.rlim_cur=256; lim.rlim_max=512;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==-EPERM);
      lim.rlim_cur=lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==-EPERM);
      lim.rlim_cur=512; lim.rlim_max=256;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==-EINVAL);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==256 && lim.rlim_max==256);
      euid=0;
      lim.rlim_cur=256; lim.rlim_max=512;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(host_cur==524288 && host_max==524288);
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==256 && lim.rlim_max==512);
      euid=501;

      /* inherited through execve */
      reset_nofile();
      have_inherited=1; inherited[0]=64; inherited[1]=64;
      assert(sys_getrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(lim.rlim_cur==64 && lim.rlim_max==64);
      assert(nofile_limit_exceeded(64) && !nofile_limit_exceeded(63));
      assert(host_cur==524288 && host_max==524288);
      have_inherited=0;

      /* a host hard limit below OPEN_MAX: RLIM_INFINITY is a raise */
      reset_nofile();
      host_cur=host_max=64;
      lim.rlim_cur=64; lim.rlim_max=DARWIN_INF;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==-EPERM);
      assert(host_cur==64 && host_max==64);
      euid=0;
      assert(sys_setrlimit(BSD_RLIMIT_NOFILE,&lim)==0);
      assert(host_cur==64 && host_max==10240);
      euid=501;
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  puts 'PASS RLIM_INFINITY round-trip, NULL limits and the guest RLIMIT_NOFILE through getrlimit/setrlimit at O0/O2'
end
