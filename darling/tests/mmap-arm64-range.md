# ARM64 mmap address range

Compile `mmap-arm64-range.c` with the configured Darling ARM64 cross compiler,
SDK, startup object and libSystem link mapping, then run the resulting Mach-O:

```
DPREFIX=<private-prefix> DARLING_INSTALL_PREFIX=<private-runtime>/usr/local \
  <build>/src/startup/darling shell <guest-path>/mmap-arm64-range
```

The original BSD mmap implementation on an Asahi host with 48-bit Linux user
addresses fails the first range assertion. The fixed runtime prints:

```
ARM64 mmap range and mapping semantics passed
```

This checks anonymous and file-backed mappings, retained file offsets, fixed
replacement, no-replace collision/data preservation, reuse of a freed range,
fixed requests outside the Darwin limit, overflow, repeated allocations and Mach
ANYWHERE allocation. The range is the macOS `MACH_VM_MAX_ADDRESS_RAW` in Apple's
published `osfmk/mach/arm/vm_param.h`.

An additional application-level reproducer is an own-source C# executable which
throws and catches `InvalidOperationException`, hosted through the public
`coreclr_initialize` / `coreclr_execute_assembly` API using the official macOS
ARM64 .NET 10.0.13 runtime. Before the fix the exception is unhandled; after the
fix the catch executes and the process exits 0, without interposition. The .NET
v10.0.13 `UnwindContextToWinContext` source masks macOS ARM64 PCs to 47 bits;
returning wider Linux mmap addresses violates that assumption.

The larger managed exception suite is separate from this mapping regression.
