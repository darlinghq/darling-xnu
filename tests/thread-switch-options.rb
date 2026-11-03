# syscall_thread_switch_impl honours the option and option_time units; host build with fake yield/sleep.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
source=File.read("#{root}/darling/src/libsystem_kernel/emulation/src/xnu_syscall/mach/impl/mach_traps.c")
impl=source[/^extern int __linux_nanosleep\(.*?^kern_return_t syscall_thread_switch_impl\(.*?^\}\n/m] or abort 'syscall_thread_switch_impl missing'
switch_h=File.read("#{root}/osfmk/mach/thread_switch.h").scan(/^#define SWITCH_OPTION_\w+\s+\d+.*$/).join("\n")
Dir.mktmpdir('thread-switch-options-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <time.h>
    typedef int kern_return_t;
    typedef unsigned int mach_port_name_t;
    typedef unsigned int mach_msg_timeout_t;
    #define KERN_SUCCESS 0
    #define KERN_INVALID_ARGUMENT 4
    #{switch_h}
    static int yields, sleeps;
    static long long slept_ns;
    static void __linux_sched_yield(void) { yields++; }
    #{impl}
    int __linux_nanosleep(struct timespec *tv, struct timespec *rem) {
      sleeps++; slept_ns=tv->tv_sec*1000000000LL+tv->tv_nsec;
      return 0;
    }
    #define SWITCH(option, time) (yields=sleeps=0, slept_ns=-1, syscall_thread_switch_impl(0, (option), (time)))
    int main(void) {
      /* NONE and the depress options give up the CPU without sleeping, whatever option_time says. */
      assert(SWITCH(0,0)==KERN_SUCCESS && yields==1 && sleeps==0);
      assert(SWITCH(0,5)==KERN_SUCCESS && yields==1 && sleeps==0);
      assert(SWITCH(1,1)==KERN_SUCCESS && yields==1 && sleeps==0);
      assert(SWITCH(4,1)==KERN_SUCCESS && yields==1 && sleeps==0);
      /* WAIT and OSLOCK_WAIT: option_time is in milliseconds. */
      assert(SWITCH(2,3)==KERN_SUCCESS && sleeps==1 && yields==0 && slept_ns==3000000LL);
      assert(SWITCH(5,1500)==KERN_SUCCESS && sleeps==1 && slept_ns==1500000000LL);
      assert(SWITCH(2,0)==KERN_SUCCESS && sleeps==0 && yields==1);
      /* DISPATCH_CONTENTION: option_time is in microseconds. */
      assert(SWITCH(3,250)==KERN_SUCCESS && sleeps==1 && slept_ns==250000LL);
      /* Unknown options are rejected, as XNU does. */
      assert(SWITCH(6,1)==KERN_INVALID_ARGUMENT && yields==0 && sleeps==0);
      assert(SWITCH(-1,1)==KERN_INVALID_ARGUMENT);
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  puts 'PASS thread_switch yields for NONE/DEPRESS and waits ms (us for DISPATCH_CONTENTION) at O0/O2'
end
