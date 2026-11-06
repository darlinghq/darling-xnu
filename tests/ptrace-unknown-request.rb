# sys_ptrace on a request it does not implement; host build of the real function.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
emu="#{root}/darling/src/libsystem_kernel/emulation"
source=File.read("#{emu}/src/xnu_syscall/bsd/impl/misc/ptrace.c")
function=source[/^long sys_ptrace\(.*?^\}/m] or abort 'sys_ptrace missing'
Dir.mktmpdir('ptrace-unknown-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <errno.h>
    #include <signal.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include "#{emu}/include/xnu_syscall/bsd/impl/misc/ptrace.h"
    static int calls;
    static int errno_linux_to_bsd(int e) { return e; }
    static void __simple_printf(const char *f, ...) { }
    static void __simple_kprintf(const char *f, ...) { }
    static void __simple_abort(void) { assert(0); }
    static int getpid(void) { return 1; }
    static long sys_getppid(void) { return 1; }
    static long sys_kill(int pid, int sig, int posix) { ++calls; return 0; }
    static int dserver_rpc_set_tracer(int a, int b) { ++calls; return 0; }
    static int dserver_rpc_ptrace_sigexc(int pid, int on) { ++calls; return 0; }
    static int dserver_rpc_tid_for_thread(void *t, int *tid) { ++calls; return 0; }
    static int dserver_rpc_ptrace_thupdate(int tid, int sig) { ++calls; return 0; }
    #{function}
    int main(void) {
      /* PT_STEP (9) is a Darwin request Darling does not implement; -1 is no request at all. */
      const int unknown[]={9,-1};
      for (size_t i=0;i<sizeof(unknown)/sizeof(*unknown);++i)
        assert(sys_ptrace(unknown[i],0,NULL,0)==-EINVAL);
      assert(calls==0);
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,'-Werror=conditional-uninitialized',"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  puts 'PASS unimplemented ptrace requests return EINVAL at O0/O2'
end
