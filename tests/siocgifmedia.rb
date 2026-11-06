# handle_socket's SIOCGIFMEDIA/SIOCGIFXMEDIA on the host with a fake Linux SIOCGIFFLAGS.
require 'tmpdir'
require 'open3'
root=File.realpath(ARGV.fetch(0))
emu="#{root}/darling/src/libsystem_kernel/emulation"
strip=->(path){File.read(path).lines.reject{|l|l.start_with?('#include ')}.join}
source=strip["#{emu}/src/xnu_syscall/bsd/helper/ioctl/socket.c"]
ifreq=strip["#{emu}/include/conversion/network/ifreq.h"]
# The tree's real Linux-to-Darwin errno table (host errno names equal Darwin's for the codes it maps).
errno_conv=strip["#{emu}/include/conversion/duct_errno.h"]+strip["#{emu}/src/conversion/errno.c"]
# Darwin definitions from this tree's headers (the host's <net/if.h> is Linux's).
if_h=File.read("#{root}/bsd/net/if.h")
iff=if_h.scan(/^#define IFF_\w+\s+0x\h+.*$/).join("\n")
ifmediareq=if_h[/^struct ifmediareq \{.*?^\};/m] or abort 'struct ifmediareq missing'
media=File.read("#{root}/bsd/net/if_media.h").scan(/^#define IFM_(?:ETHER|IEEE80211|AUTO|AVALID|ACTIVE)\s.*$/).join("\n")
Dir.mktmpdir('siocgifmedia-') do |dir|
  File.write("#{dir}/probe.c", <<~C)
    #include <assert.h>
    #include <errno.h>
    #include <stddef.h>
    #include <string.h>
    #define IFNAMSIZ 16
    #{iff}
    #{media}
    #pragma pack(4)
    #{ifmediareq}
    #pragma pack()
    struct ifreq { char ifr_name[IFNAMSIZ]; union { short ifru_flags; int ifru_mtu; } ifr_ifru; };
    #define ifr_flags ifr_ifru.ifru_flags
    #define ifr_mtu ifr_ifru.ifru_mtu
    enum { IOCTL_PASS, IOCTL_HANDLED };
    enum { SIOCGIFFLAGS=1, SIOCGIFMTU, SIOCSIFFLAGS, SIOCAIFADDR, SIOCAIFADDR_IN6, SIOCGIFMEDIA, SIOCGIFXMEDIA };
    #{ifreq}
    #{errno_conv}
    /* Linux interface flags: IFF_UP 0x1, IFF_LOOPBACK 0x8, IFF_RUNNING 0x40 */
    static long __real_ioctl(int fd, int req, void *arg) {
      struct linux_ifreq *r=arg;
      assert(req==LINUX_SIOCGIFFLAGS);
      if (!strcmp(r->lifr_name,"lo")) { r->lifr_flags=0x1|0x8|0x40; return 0; }
      if (!strcmp(r->lifr_name,"eth0")) { r->lifr_flags=0x1|0x40; return 0; }
      if (!strcmp(r->lifr_name,"eth1")) { r->lifr_flags=0x1; return 0; }
      if (!strcmp(r->lifr_name,"wlan0")) { r->lifr_flags=0x1|0x40; return 0; }
      return -19; /* Linux ENODEV */
    }
    #{source}
    static struct ifmediareq req(const char *name, int count, int *list) {
      struct ifmediareq r;
      memset(&r,0x5a,sizeof(r));
      memset(r.ifm_name,0,sizeof(r.ifm_name));
      strcpy(r.ifm_name,name);
      r.ifm_count=count; r.ifm_ulist=list;
      return r;
    }
    int main(void) {
      int ret, list[4]={-1,-1,-1,-1};
      struct ifmediareq r;

      /* Apple ifconfig's media_status(): count query with a NULL list, then the list. */
      r=req("eth0",0,NULL);
      assert(handle_socket(3,SIOCGIFMEDIA,&r,&ret)==IOCTL_HANDLED && ret==0);
      assert(r.ifm_count==1);
      assert(r.ifm_current==(IFM_ETHER|IFM_AUTO) && r.ifm_active==r.ifm_current && r.ifm_mask==0);
      assert(r.ifm_status==(IFM_AVALID|IFM_ACTIVE));
      r.ifm_ulist=list; r.ifm_count=4;
      assert(handle_socket(3,SIOCGIFXMEDIA,&r,&ret)==IOCTL_HANDLED && ret==0);
      assert(r.ifm_count==1 && list[0]==(IFM_ETHER|IFM_AUTO) && list[1]==-1);

      /* Up but no carrier. */
      r=req("eth1",0,NULL);
      assert(handle_socket(3,SIOCGIFMEDIA,&r,&ret)==IOCTL_HANDLED && ret==0);
      assert(r.ifm_status==IFM_AVALID && r.ifm_count==1);

      r=req("wlan0",0,NULL);
      assert(handle_socket(3,SIOCGIFMEDIA,&r,&ret)==IOCTL_HANDLED && ret==0);
      assert(r.ifm_current==(IFM_IEEE80211|IFM_AUTO));

      /* A non-NULL list with ifm_count 0 is left alone. */
      list[0]=-1;
      r=req("eth0",0,list);
      assert(handle_socket(3,SIOCGIFMEDIA,&r,&ret)==IOCTL_HANDLED && ret==0);
      assert(r.ifm_count==0 && list[0]==-1);

      /* lo0 has no media in XNU. */
      r=req("lo",0,NULL);
      assert(handle_socket(3,SIOCGIFMEDIA,&r,&ret)==IOCTL_HANDLED && ret==-EOPNOTSUPP);

      r=req("nope0",0,NULL);
      assert(handle_socket(3,SIOCGIFMEDIA,&r,&ret)==IOCTL_HANDLED && ret==-6); /* Darwin ENXIO, as XNU if.c:3110 */
      assert(handle_socket(3,SIOCGIFMEDIA,NULL,&ret)==IOCTL_HANDLED && ret==-EFAULT);
    }
  C
  %w[-O0 -O2].each do |opt|
    out,status=Open3.capture2e('clang',opt,'-fsanitize=address,undefined',"#{dir}/probe.c",'-o',"#{dir}/probe")
    abort out unless status.success?
    out,status=Open3.capture2e("#{dir}/probe")
    abort out unless status.success?
  end
  puts 'PASS SIOCGIFMEDIA/SIOCGIFXMEDIA report one autoselect medium and reject loopback at O0/O2'
end
