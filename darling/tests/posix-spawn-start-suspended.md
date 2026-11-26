# POSIX_SPAWN_START_SUSPENDED and SIGSTOP

Build `posix-spawn-start-suspended.c` as a Darwin executable (x86_64 and i386) and run it in the guest.
It starts a child with `POSIX_SPAWN_START_SUSPENDED`, waits for its stop with `waitpid(WUNTRACED)`,
checks that it stays stopped, resumes it with `SIGCONT` and checks its exit status; then it stops a
running child with `kill(SIGSTOP)` and checks the `WIFSTOPPED` / `WSTOPSIG` status.

Reference behaviour on macOS 26.7.3 (probe `vd-upstream/mac-probes/sp134-spawn-suspended.c`): the suspended
child is reported stopped (with a zero stop signal), is not run until SIGCONT and then exits normally;
`kill(SIGSTOP)` is reported stopped with signal SIGSTOP. The stop signal of the suspended child is 0.
