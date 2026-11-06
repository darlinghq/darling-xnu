#include <errno.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
static int compare_linux_ipv4(const unsigned char *buffer, size_t length) {
  FILE *routes = fopen("/proc/net/route", "r");
  CHECK(routes);
  char line[512];
  int checked = 0;
  while (fgets(line, sizeof(line), routes)) {
    char name[IFNAMSIZ];
    unsigned long destination, gateway, mask;
    unsigned flags;
    if (sscanf(line, "%15s %lx %lx %x %*u %*u %*u %lx", name, &destination,
               &gateway, &flags, &mask) != 5)
      continue;
    unsigned index = if_nametoindex(name);
    int found = 0;
    for (size_t offset = 0; offset < length;) {
      const struct rt_msghdr *header = (const void *)(buffer + offset);
      const struct sockaddr_in *addresses = (const void *)(header + 1);
      if (header->rtm_index == index &&
          addresses[0].sin_addr.s_addr == destination &&
          addresses[1].sin_addr.s_addr == gateway &&
          addresses[2].sin_addr.s_addr == mask) {
        CHECK(!!(header->rtm_flags & RTF_GATEWAY) == !!(flags & 2));
        found = 1;
        break;
      }
      offset += header->rtm_msglen;
    }
    CHECK(found);
    checked++;
  }
  CHECK(!ferror(routes));
  fclose(routes);
  CHECK(checked > 0);
  return 0;
}

static int query(int family, int *count) {
  int mib[] = {CTL_NET, PF_ROUTE, 0, family, NET_RT_DUMP, 0};
  size_t length = 0;
  CHECK(!sysctl(mib, 6, NULL, &length, NULL, 0));
  unsigned char *buffer = malloc(length + 16);
  CHECK(buffer);
  memset(buffer, 0xa5, length + 16);
  size_t actual = length;
  CHECK(!sysctl(mib, 6, buffer, &actual, NULL, 0) && actual <= length);
  for (size_t i = length; i < length + 16; i++)
    CHECK(buffer[i] == 0xa5);
  size_t offset = 0;
  *count = 0;
  while (offset < actual) {
    struct rt_msghdr *h = (void *)(buffer + offset);
    CHECK(actual - offset >= sizeof(*h) && h->rtm_msglen >= sizeof(*h) &&
          h->rtm_msglen <= actual - offset);
    CHECK(h->rtm_version == RTM_VERSION && h->rtm_type == RTM_GET);
    CHECK(h->rtm_index &&
          h->rtm_addrs == (RTA_DST | RTA_GATEWAY | RTA_NETMASK));
    char name[IFNAMSIZ];
    CHECK(if_indextoname(h->rtm_index, name));
    struct sockaddr *sa = (void *)(h + 1);
    int af = sa->sa_family;
    CHECK((af == AF_INET || af == AF_INET6) && (!family || af == family));
    size_t addressLength = af == AF_INET ? sizeof(struct sockaddr_in)
                                         : sizeof(struct sockaddr_in6);
    CHECK(h->rtm_msglen == sizeof(*h) + 3 * addressLength);
    for (int i = 0; i < 3; i++) {
      sa = (void *)((char *)(h + 1) + i * addressLength);
      CHECK(sa->sa_family == af && sa->sa_len == addressLength);
    }
    offset += h->rtm_msglen;
    (*count)++;
  }
  CHECK(offset == actual);
  if (family == AF_INET)
    CHECK(!compare_linux_ipv4(buffer, actual));
  if (length) {
    size_t shortlen = length - 1;
    memset(buffer, 0x5a, length + 16);
    CHECK(sysctl(mib, 6, buffer, &shortlen, NULL, 0) == -1 && errno == ENOMEM &&
          shortlen >= length);
    for (size_t i = length - 1; i < length + 16; i++)
      CHECK(buffer[i] == 0x5a);
  }
  free(buffer);
  return 0;
}
int main(void) {
  int all, v4, v6;
  CHECK(!query(0, &all) && !query(AF_INET, &v4) && !query(AF_INET6, &v6));
  CHECK(all == v4 + v6 && all > 0);
  int mib[] = {CTL_NET, PF_ROUTE, 0, AF_UNIX, NET_RT_DUMP, 0};
  size_t length = 0;
  CHECK(sysctl(mib, 6, NULL, &length, NULL, 0) == -1 && errno == EAFNOSUPPORT);
  puts("Route dump layout, real indices, family filters and buffer bounds "
       "passed");
  return 0;
}
