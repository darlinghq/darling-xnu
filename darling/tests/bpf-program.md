# Classic BPF checker and interpreter

`bpf-program.c` tests the program checker and interpreter that `/dev/bpf`
uses for filters it cannot hand to the Linux kernel (DLT_NULL on loopback,
and Ethernet frames whose VLAN tag Linux had taken out). It includes
`bpf_program.c` directly and builds on the host:

    cc -Wall -Wextra -Werror darling/tests/bpf-program.c -o bpf-program
    ./bpf-program

It checks a "udp and dst port" filter over Ethernet and DLT_NULL framing
(matching, other port, other protocol, IP options, truncated capture), the
load widths and byte order, out-of-bounds loads, LEN, indexed loads that
wrap, division by a zero X, arithmetic, scratch memory, TAX/TXA, jumps,
shift counts, and the checker's limits: program length, a final return,
jump ranges, scratch memory indices, division by a constant zero, modulo
(not part of Darwin's instruction set) and packet offsets. Expected output:
`PASS bpf program checker and interpreter`.
