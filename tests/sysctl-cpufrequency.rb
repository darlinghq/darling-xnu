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
  abort "hw.cpufrequency handler is not handle_cpufrequency: #{entry.strip}" unless entry =~ /\.handler = handle_cpufrequency \}/

  # Resolve OIDs from the real enum and the real XNU sysctl.h values.
  sysctl_h=File.read("#{root}/bsd/sys/sysctl.h")
  oids={}
  enum=source[/^enum \{\n(.*?)^\};/m,1] or abort 'Darling sysctl enum missing'
  next_oid=nil
  enum.each_line do |line|
    name,value=line.strip.chomp(',').split(/\s*=\s*/)
    next if name.nil? || name.empty?
    next_oid=Integer(value) if value
    abort "enum has no start value before #{name}" unless next_oid
    oids[name]=next_oid
    next_oid+=1
  end
  sysctl_h.scan(/^#define\s+(HW_\w+)\s+(\d+)\b/) { |name,value| oids[name]=value.to_i }
  table=source[/^const struct known_sysctl sysctls_hw\[\] = \{\n(.*?)^\};/m,1] or abort 'sysctls_hw table missing'
  rows=table.lines.map { |line| line[/\.oid = (\w+)/,1] }.compact
  values=rows.map { |name| oids.fetch(name) { abort "unresolved sysctls_hw OID #{name}" } }
  dups=rows.zip(values).group_by(&:last).select { |_,pairs| pairs.size > 1 }
  abort "duplicate sysctls_hw OIDs: #{dups.values.map { |pairs| pairs.map(&:first).join('=') }.join(', ')}" unless dups.empty?
  abort 'HW_CPU_FREQ is not 15 in bsd/sys/sysctl.h' unless oids['HW_CPU_FREQ'] == 15
  abort '_HW_CPUFREQUENCY collides with HW_CPU_FREQ' if oids['_HW_CPUFREQUENCY'] == oids['HW_CPU_FREQ']

  compat=source[/^.*\.name = "cpufrequency_compat",.*$/] or abort 'cpufrequency_compat entry missing'
  abort "cpufrequency_compat is not the 32-bit HW_CPU_FREQ OID: #{compat.strip}" unless
    compat.include?('.oid = HW_CPU_FREQ,') && compat.include?('.type = CTLTYPE_INT,') &&
    compat.include?('.exttype = "I"') && compat.include?('.handler = handle_cpufrequency_compat }')
  puts 'PASS hw.cpufrequency is 64-bit Hz, HW_CPU_FREQ is saturated 32-bit Hz at O0/O2, and sysctls_hw OIDs are unique'
end
