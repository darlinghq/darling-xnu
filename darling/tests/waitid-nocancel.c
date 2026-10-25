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
int main(void) {
  int gate[2];
  CHECK(pipe(gate) == 0);
  pid_t child = fork();
  CHECK(child >= 0);
  if (child == 0) {
    close(gate[1]);
    char c;
    read(gate[0], &c, 1);
    _exit(42);
  }
  close(gate[0]);
  siginfo_t info;
  memset(&info, 0x5a, sizeof(info));
  long result = syscall(SYS_waitid_nocancel, P_PID, child, &info,
                        WEXITED | WNOHANG | WNOWAIT);
  if (result != 0) {
    close(gate[1]);
    waitpid(child, NULL, 0);
    CHECK(result == 0);
  }
  CHECK(info.si_pid == 0);
  close(gate[1]);
  CHECK(syscall(SYS_waitid_nocancel, P_PID, child, &info, WEXITED | WNOWAIT) ==
        0);
  CHECK(info.si_pid == child && info.si_signo == SIGCHLD &&
        info.si_code == CLD_EXITED && info.si_status == 42);
  int status;
  CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
        WEXITSTATUS(status) == 42);
  errno = 0;
  CHECK(syscall(SYS_waitid_nocancel, P_PID, child, &info, WEXITED | WNOHANG) ==
            -1 &&
        errno == ECHILD);
  puts("waitid_nocancel nohang, exit status, nowait and reaping passed");
  return 0;
}
