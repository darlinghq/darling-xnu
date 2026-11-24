// The si_code a SA_SIGINFO handler receives (sigaction(2), <sys/signal.h>).
//
// Signals sent by a process (kill, raise, pthread_kill) and by the system (setitimer, alarm) carry
// no fault information: a Darwin program sees si_code 0, not the SI_USER (0x10001) that the header
// describes (observed on macOS 10.14.4 i386 and x86_64 and on 26.7.3). It never sees the negative
// values (SI_TKILL, SI_QUEUE, ...) or SI_KERNEL (0x80) that the Linux kernel uses for these.
// Fault codes are the ones of <sys/signal.h> even where Linux numbers them differently
// (FPE_INTDIV is 7 here and 1 on Linux), and SIGCHLD reports CLD_EXITED with the child's pid.
#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static volatile sig_atomic_t runs;
static volatile int got_signo, got_code;
static volatile pid_t got_pid;
static void *volatile got_addr;
static sigjmp_buf fault_env;

static void record(int sig, siginfo_t *info, void *context) {
  (void)context;
  runs++;
  got_signo = info->si_signo;
  got_code = info->si_code;
  got_pid = info->si_pid;
  got_addr = info->si_addr;
  if (sig == SIGSEGV || sig == SIGBUS || sig == SIGFPE)
    siglongjmp(fault_env, 1);
}

static int install(int sig) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = record;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  return sigaction(sig, &sa, NULL);
}

static int user_code(int code) { return code == 0; }

static int test_kill(void) {
  runs = 0;
  CHECK(install(SIGUSR1) == 0);
  CHECK(kill(getpid(), SIGUSR1) == 0);
  CHECK(runs == 1);
  CHECK(got_signo == SIGUSR1);
  CHECK(user_code(got_code));
  CHECK(got_pid == getpid());
  return 0;
}

static int test_raise(void) {
  runs = 0;
  CHECK(install(SIGUSR2) == 0);
  CHECK(raise(SIGUSR2) == 0);
  CHECK(runs == 1);
  CHECK(got_signo == SIGUSR2);
  CHECK(user_code(got_code));
  return 0;
}

static int test_pthread_kill(void) {
  runs = 0;
  CHECK(install(SIGUSR1) == 0);
  CHECK(pthread_kill(pthread_self(), SIGUSR1) == 0);
  CHECK(runs == 1);
  CHECK(user_code(got_code));
  return 0;
}

static int test_itimer(void) {
  struct itimerval timer = {{0, 0}, {0, 20000}};
  runs = 0;
  CHECK(install(SIGALRM) == 0);
  CHECK(setitimer(ITIMER_REAL, &timer, NULL) == 0);
  for (int i = 0; i < 200 && !runs; i++)
    usleep(10000);
  CHECK(runs == 1);
  CHECK(user_code(got_code));
  return 0;
}

static int test_child(void) {
  runs = 0;
  CHECK(install(SIGCHLD) == 0);
  pid_t child = fork();
  CHECK(child >= 0);
  if (child == 0)
    _exit(3);
  for (int i = 0; i < 200 && !runs; i++)
    usleep(10000);
  CHECK(runs >= 1);
  CHECK(got_signo == SIGCHLD);
  CHECK(got_code == CLD_EXITED);
  CHECK(got_pid == child);
  int status;
  CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 3);
  signal(SIGCHLD, SIG_DFL);
  return 0;
}

static int test_unmapped(void) {
  size_t size = (size_t)getpagesize();
  char *page = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  CHECK(page != MAP_FAILED);
  CHECK(munmap(page, size) == 0);

  runs = 0;
  CHECK(install(SIGSEGV) == 0 && install(SIGBUS) == 0);
  if (sigsetjmp(fault_env, 1) == 0)
    *(volatile char *)page = 1;
  CHECK(runs == 1);
  CHECK(got_signo == SIGSEGV || got_signo == SIGBUS);
  CHECK(got_code == SEGV_MAPERR);
  CHECK(got_addr == page);
  signal(SIGSEGV, SIG_DFL);
  signal(SIGBUS, SIG_DFL);
  return 0;
}

#if defined(__x86_64__)
static int test_integer_divide_by_zero(void) {
  volatile int zero = 0;
  volatile int result;

  runs = 0;
  CHECK(install(SIGFPE) == 0);
  if (sigsetjmp(fault_env, 1) == 0)
    result = 1 / zero;
  (void)result;
  CHECK(runs == 1);
  CHECK(got_signo == SIGFPE);
  CHECK(got_code == FPE_INTDIV);
  signal(SIGFPE, SIG_DFL);
  return 0;
}
#endif

int main(void) {
  int failures = 0;
  struct {
    const char *name;
    int (*run)(void);
  } cases[] = {
      {"kill", test_kill},
      {"raise", test_raise},
      {"pthread_kill", test_pthread_kill},
      {"setitimer", test_itimer},
      {"SIGCHLD", test_child},
      {"fault on an unmapped page", test_unmapped},
#if defined(__x86_64__)
      {"integer divide by zero", test_integer_divide_by_zero},
#endif
  };

  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    if (cases[i].run()) {
      fprintf(stderr, "FAIL: %s (si_code %d)\n", cases[i].name, got_code);
      failures++;
    }
  }
  if (failures)
    fprintf(stderr, "FAILED: %d\n", failures);
  else
    printf("OK\n");
  return failures != 0;
}
