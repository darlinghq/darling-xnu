// The BSD syscall dispatcher must call its C handlers with the stack aligned as
// the i386 ABI requires, whichever way the syscall is entered: through the
// fixed-argument libsystem_kernel stubs or through the variadic syscall()
// (which has a different stack layout). The waitid handler zeroes a large
// local structure with aligned SSE stores, so a misaligned stack faults.
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static int check_exited(pid_t child, siginfo_t *info) {
  return info->si_pid == child && info->si_signo == SIGCHLD &&
         info->si_code == CLD_EXITED && info->si_status == 7;
}

static int run(int variant) {
  pid_t child = fork();
  CHECK(child >= 0);
  if (child == 0)
    _exit(7);
  siginfo_t info;
  memset(&info, 0, sizeof(info));
  errno = 0;
  switch (variant) {
  case 0:
    CHECK(waitid(P_PID, child, &info, WEXITED | WNOWAIT) == 0);
    break;
  case 1:
    CHECK(syscall(SYS_waitid, P_PID, child, &info, WEXITED | WNOWAIT) == 0);
    break;
  case 2:
    CHECK(syscall(SYS_waitid_nocancel, P_PID, child, &info,
                  WEXITED | WNOWAIT) == 0);
    break;
  case 3: // extra (ignored) arguments change the caller's stack padding
    CHECK(syscall(SYS_waitid_nocancel, P_PID, child, &info, WEXITED | WNOWAIT,
                  0, 0) == 0);
    break;
  }
  CHECK(check_exited(child, &info));
  int status;
  CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
        WEXITSTATUS(status) == 7);
  return 0;
}

int main(void) {
  for (int v = 0; v < 4; v++)
    if (run(v))
      return 1;
  puts("syscall stack alignment passed");
  return 0;
}
