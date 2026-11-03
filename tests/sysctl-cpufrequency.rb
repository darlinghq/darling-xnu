# hw.cpufrequency handlers from sysctl_hw.c on the host with a fake cpufreq file.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
emu="#{root}/darling/src/libsystem_kernel/emulation"
source=File.read("#{emu}/src/xnu_syscall/bsd/helper/misc/sysctl_hw.c")
handlers=source[/^sysctl_handler\(handle_cputhreadtype\)\n\{.*?^\}\n\n(.*?)^sysctl_handler\(handle_cpu64bitcapable\)/m,1] or abort 'cpufrequency handlers missing'
entry=source[/^.*\.name = "cpufrequency",.*$/] or abort 'cpufrequency entry missing'
header=File.read("#{emu}/include/xnu_syscall/bsd/impl/misc/sysctl.h")
size_macro=header[/^#define sysctl_handle_size.*?\n\n/m] or abort 'sysctl_handle_size missing'
Dir.mktmpdir('sysctl-cpufrequency-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <errno.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #{size_macro}
    #define sysctl_handler(_hname) long _hname(const int* name, int nlen, void* old, unsigned long* oldlen, void* _new, unsigned long newlen)
    static const char *khz;
    static bool read_string(const char *path, char *dst, int maxlen) {
      if (!khz) return false;
      strncpy(dst, khz, maxlen);
      return true;
    }
    static unsigned long long __simple_atoi(const char *s, const char **end) { return strtoull(s, NULL, 10); }
    __attribute__((weak)) sysctl_handler(handle_cpufrequency_compat);
    #{handlers}
    int main(void) {
      uint64_t q;
      uint32_t i;
      unsigned long len;

      khz="3500000\\n";
      q=0; len=sizeof(q);
      assert(handle_cpufrequency(NULL,0,&q,&len,NULL,0)==0);
      assert(q==3500000000ULL && len==sizeof(q));
      len=0;
      assert(handle_cpufrequency(NULL,0,NULL,&len,NULL,0)==0 && len==sizeof(q));
      len=sizeof(i);
      assert(handle_cpufrequency(NULL,0,&q,&len,NULL,0)==-EINVAL);

      khz=NULL;
      q=0; len=sizeof(q);
      assert(handle_cpufrequency(NULL,0,&q,&len,NULL,0)==0 && q==2400000000ULL);

      /* Numeric HW_CPU_FREQ stays a 32-bit value and saturates above 2^32 - 1 Hz. */
      assert(handle_cpufrequency_compat);
      khz="3500000\\n";
      i=0; len=sizeof(i);
      assert(handle_cpufrequency_compat(NULL,0,&i,&len,NULL,0)==0);
      assert(i==3500000000U && len==sizeof(i));
      khz="5800000\\n";
      assert(handle_cpufrequency_compat(NULL,0,&i,&len,NULL,0)==0 && i==UINT32_MAX);
      len=0;
      assert(handle_cpufrequency_compat(NULL,0,NULL,&len,NULL,0)==0 && len==sizeof(i));
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  abort "hw.cpufrequency is not a 64-bit OID: #{entry.strip}" unless entry.include?('CTLTYPE_QUAD') && entry.include?('"Q"')
  puts 'PASS hw.cpufrequency is 64-bit Hz and HW_CPU_FREQ is saturated 32-bit Hz at O0/O2'
end
