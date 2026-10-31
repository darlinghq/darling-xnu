# darling-xnu

## Upstream URL

https://github.com/apple-oss-distributions/xnu/tree/xnu-7195.141.2

## Updating Source

When updating the source code, make sure to refer to the [`Updating Sources`](https://docs.darlinghq.org/contributing/updating-sources/index.html#updating-sources) section from the Darling Docs website. Additional details (if any) can be found in [`darling/notes/UPDATE_SOURCE.md`](darling/notes/UPDATE_SOURCE.md).

## Memory advice capability contract

`MADV_ZERO` (11 in newer Darwin SDKs) is not implemented. Success would promise zeroed storage that allocators can
reuse without clearing it, so the emulation returns `ENOTSUP` and leaves the memory unchanged. Unknown advice values
remain advisory no-ops for compatibility, as in the existing Darwin-specific advice range.

Run `darling/src/libsystem_kernel/tests/madvise_advice_guest.c` inside a Darling image to check the contract: a zero
request either zeroes the selected bytes or fails without changing them, implemented advice remains usable, and
neighbouring private writes survive.
