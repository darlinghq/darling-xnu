// sigpending() stores the set of signals that are blocked and pending for the caller (POSIX sigpending,
// sigpending(2)). It reports no errors and accepts a NULL set. A blocked signal sent to the process
// stays pending, other blocked signals do not show up, and once the signal is unblocked its handler runs and
// the pending set is empty again.
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static volatile sig_atomic_t usr1_runs;

static void on_usr1(int sig) {
  (void)sig;
  usr1_runs++;
}

int main(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = on_usr1;
  sigemptyset(&sa.sa_mask);
  CHECK(sigaction(SIGUSR1, &sa, NULL) == 0);

  sigset_t block, pending;
  sigemptyset(&block);
  sigaddset(&block, SIGUSR1);
  sigaddset(&block, SIGUSR2);
  CHECK(sigprocmask(SIG_BLOCK, &block, NULL) == 0);

  sigfillset(&pending);
  CHECK(sigpending(&pending) == 0);
  CHECK(!sigismember(&pending, SIGUSR1));
  CHECK(!sigismember(&pending, SIGUSR2));

  CHECK(kill(getpid(), SIGUSR1) == 0);
  CHECK(usr1_runs == 0);

  sigemptyset(&pending);
  CHECK(sigpending(&pending) == 0);
  CHECK(sigismember(&pending, SIGUSR1));
  CHECK(!sigismember(&pending, SIGUSR2));

  sigset_t usr1;
  sigemptyset(&usr1);
  sigaddset(&usr1, SIGUSR1);
  CHECK(sigprocmask(SIG_UNBLOCK, &usr1, NULL) == 0);
  CHECK(usr1_runs == 1);

  sigfillset(&pending);
  CHECK(sigpending(&pending) == 0);
  CHECK(!sigismember(&pending, SIGUSR1));
  CHECK(!sigismember(&pending, SIGUSR2));

  CHECK(sigpending(NULL) == 0);

  printf("OK\n");
  return 0;
}
