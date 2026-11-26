# Large anonymous mappings

Build `mmap-large-anonymous.c` for the guest and run it in Darling:

```
darling shell <guest-path>/mmap-large-anonymous
```

macOS 26.7.3 on an 8 GB Mac maps up to 4 TB of anonymous memory, read-write or
without access, with mmap() and with mach_vm_allocate(); every line passes.
Before the fix, on a Linux host in the default overcommit mode, read-write
mmap() fails with ENOMEM for any size above the host's memory and swap, as
Linux reserves backing store for such a mapping unless it is MAP_NORESERVE.
