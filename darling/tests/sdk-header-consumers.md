# SDK headers for third-party consumers

`sdk-header-consumers.c` is a compile-only check of Darling's SDK headers as
a project outside Darling's own build uses them: without `-DDARLING` and
without `-DPLATFORM_MacOSX`. With `SDK` set to the SDK's `usr/include`
directory (`Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include`
in the Darling source tree, which links to this repository's headers), run:

```
for t in arm64-apple-macos11 x86_64-apple-macos10.15 i386-apple-macos10.13 aarch64-linux-gnu; do
  clang -target $t -fsyntax-only -nostdinc \
    -isystem "$(clang -print-resource-dir)/include" -isystem "$SDK" \
    sdk-header-consumers.c || echo "FAIL: $t"
done
```

Every target compiles without errors. It checks that:
- `sys/cdefs.h` applies the macOS conformance settings when no platform
  macro is given, so 64-bit code gets no `$UNIX2003` symbol suffixes;
- `os/trace.h` and `os/trace_base.h` declare `__dso_handle` with the same
  type;
- `stdatomic.h` provides the C11 atomics when it defers to Clang's header;
- the `machine/` and `mach/machine/` dispatchers accept `__aarch64__`, the
  arm64 spelling of Linux compilers (`aarch64-linux-gnu`). For that target
  only the dispatchers are checked; the `arm/` headers behind most of them
  still test `__arm64__`.

Before these fixes, every target fails.
