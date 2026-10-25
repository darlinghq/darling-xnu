# NET_RT_IFLIST2

Compile `route-interface-statistics.c` as a Darwin Mach-O against the configured
SDK and libSystem. Run `darling shell /path/to/route-interface-statistics` in a
private prefix with the rebuilt kernel library. The baseline size query fails
EINVAL. The corrected target test verifies RTM_IFINFO2 and link-address layout,
real indices via if_nametoindex, MTU, index filtering, empty selections, required
buffer lengths, ENOMEM and guard bytes. A real loopback UDP exchange must increase
the reported packet counter, rejecting an implementation that fills fake zeros.
Legacy IFLIST size queries must still work.

Counters are read from Linux /proc/net/dev; unknown fields including link speed
remain zero. Enumeration retains the existing IPv4 SIOCGIFCONF address coverage;
this does not add IPv6-only interfaces. The fixed five-interface cap is replaced
with a bounded dynamic buffer. No Linux network configuration is changed.

Provenance: vendored net/route.h, net/if.h, net/if_var.h and network conversion
headers (rung2), Linux proc network data and authored API controls (rung4).
With the separate NET_RT_DUMP fix, official macOS .NET10.0.13
NetworkInterface.GetAllNetworkInterfaces returns the host interfaces instead of
throwing. The gateway fix is independent and is not included here.
