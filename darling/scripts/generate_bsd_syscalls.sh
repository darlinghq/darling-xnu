export ARCHS="arm64 i386 x86_64 ppc"
XNU_DIR="$(cd ../.. && pwd)"
BSDSYSCALLS_GEN_PATH="$XNU_DIR/gen/bsdsyscalls"

mkdir -p $BSDSYSCALLS_GEN_PATH
"$XNU_DIR/libsyscall/xcodescripts/create-syscalls.pl" \
    "$XNU_DIR/bsd/kern/syscalls.master" \
    "$XNU_DIR/libsyscall/custom/" \
    "$XNU_DIR/libsyscall/Platforms/" \
    "MacOSX" \
    "$BSDSYSCALLS_GEN_PATH"
