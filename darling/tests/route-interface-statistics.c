#include <errno.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
static int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0};
static uint64_t loopPackets;
static unsigned loopIndex;
static int scan(void *bytes, size_t length, unsigned wanted) {
  size_t offset = 0;
  int count = 0;
  while (offset < length) {
    struct if_msghdr *prefix = (void *)((char *)bytes + offset);
    CHECK(length - offset >= 4 && prefix->ifm_msglen >= 4 &&
          prefix->ifm_msglen <= length - offset);
    CHECK(prefix->ifm_version == RTM_VERSION);
    if (prefix->ifm_type == RTM_IFINFO2) {
      struct if_msghdr2 *h = (void *)prefix;
      CHECK(h->ifm_msglen >= sizeof(*h) + sizeof(struct sockaddr_dl));
      CHECK(h->ifm_index && (!wanted || h->ifm_index == wanted));
      CHECK(h->ifm_data.ifi_mtu > 0);
      struct sockaddr_dl *dl = (void *)(h + 1);
      CHECK(dl->sdl_family == AF_LINK && dl->sdl_index == h->ifm_index &&
            dl->sdl_nlen < IFNAMSIZ);
      CHECK(sizeof(*h) + dl->sdl_len <= h->ifm_msglen);
      char name[IFNAMSIZ] = {0};
      memcpy(name, dl->sdl_data, dl->sdl_nlen);
      CHECK(if_nametoindex(name) == h->ifm_index);
      if (dl->sdl_type == IFT_LOOP) {
        loopIndex = h->ifm_index;
        loopPackets = h->ifm_data.ifi_ipackets;
      }
      count++;
    }
    offset += prefix->ifm_msglen;
  }
  CHECK(offset == length && count > 0);
  return 0;
}
static int snapshot(void) {
  size_t length = 0;
  CHECK(!sysctl(mib, 6, NULL, &length, NULL, 0) && length > 0);
  unsigned char *bytes = malloc(length + 16);
  CHECK(bytes);
  memset(bytes, 0xa5, length + 16);
  size_t actual = length;
  CHECK(!sysctl(mib, 6, bytes, &actual, NULL, 0) && actual <= length);
  CHECK(!scan(bytes, actual, mib[5]));
  for (size_t i = length; i < length + 16; i++)
    CHECK(bytes[i] == 0xa5);
  free(bytes);
  return 0;
}
int main(void) {
  CHECK(!snapshot() && loopIndex);
  unsigned index = loopIndex;
  uint64_t before = loopPackets;
  mib[5] = index;
  CHECK(!snapshot());
  int receiver = socket(AF_INET, SOCK_DGRAM, 0),
      sender = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK(receiver >= 0 && sender >= 0);
  struct sockaddr_in a = {0};
  a.sin_len = sizeof(a);
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  socklen_t alen = sizeof(a);
  CHECK(!bind(receiver, (struct sockaddr *)&a, alen) &&
        !getsockname(receiver, (struct sockaddr *)&a, &alen));
  CHECK(sendto(sender, "stats", 5, 0, (struct sockaddr *)&a, alen) == 5);
  char data[5];
  CHECK(recv(receiver, data, 5, 0) == 5);
  close(sender);
  close(receiver);
  CHECK(!snapshot() && loopPackets > before);
  mib[5] = 0;
  size_t required = 0;
  CHECK(!sysctl(mib, 6, NULL, &required, NULL, 0));
  size_t shortlen = required - 1;
  unsigned char *shortbuf = malloc(required + 16);
  CHECK(shortbuf);
  memset(shortbuf, 0x5a, required + 16);
  CHECK(sysctl(mib, 6, shortbuf, &shortlen, NULL, 0) == -1 && errno == ENOMEM &&
        shortlen >= required);
  for (size_t i = required - 1; i < required + 16; i++)
    CHECK(shortbuf[i] == 0x5a);
  free(shortbuf);
  mib[5] = 65535;
  size_t empty = 0;
  CHECK(!sysctl(mib, 6, NULL, &empty, NULL, 0) && empty == 0);
  mib[5] = 0;
  mib[4] = NET_RT_IFLIST;
  size_t legacy = 0;
  CHECK(!sysctl(mib, 6, NULL, &legacy, NULL, 0) && legacy > 0);
  puts("Interface statistics layout, indices, counters, filtering and buffer "
       "bounds passed");
  return 0;
}
