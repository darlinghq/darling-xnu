# NET_RT_DUMP

Compile `route-dump.c` as a Darwin Mach-O against the configured SDK and
libSystem. Run `darling shell /path/to/route-dump` in a private prefix with the
rebuilt kernel library. The baseline size query fails EINVAL. The corrected test
also compares every Linux IPv4 destination, gateway, netmask and interface with
the translated dump, rejecting fabricated empty routes. It checks route-message and sockaddr lengths, real interface indices, IPv4/IPv6/all
family filters, unsupported-family errors, ENOMEM/required lengths and buffer
guards. The test expects a stable host route table with at least one route.

Routes come from Linux /proc/net/route and /proc/net/ipv6_route; this covers the
proc-visible tables, not every policy-routing table. Source-specific IPv6 routes
are omitted because the legacy output cannot represent their source selector.
Known flags are translated; IPv6 link-local addresses get the actual scope ID.
This is a read-only query, not route socket mutation support.

Provenance: vendored net/route.h and netinet/in.h layouts and Linux UAPI `linux/sockios.h`, `linux/route.h` and conversion headers (rungs1–2), proc network data and authored API controls (rung4). With the
separate IFLIST2 fix, official macOS .NET10.0.13 managed interface/gateway
enumeration passes. IFLIST2 itself is not implemented in this change.
