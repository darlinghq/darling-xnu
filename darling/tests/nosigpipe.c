// SO_NOSIGPIPE (setsockopt(2)): a write to a socket whose peer has gone fails with EPIPE and
// does not raise SIGPIPE. MSG_NOSIGNAL asks for the same for one send(2) call. Without either,
// the default action of SIGPIPE ends the process.
//
// Each case runs in a forked child that restores SIGPIPE's default action, closes the peer
// and writes until a write fails. The child exits 0 if that failure was EPIPE, so a child
// killed by SIGPIPE is told apart from one that survived.
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno);          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

enum how { SEND, SENDTO, SENDMSG, WRITE, WRITEV, SEND_MSG_NOSIGNAL, HOWS };
enum prep { NONE, SET, CLEARED, DUP, DUPFD, BEFORE_FORK, REUSED_NUMBER, DUP2_SET, DUP2_CLEAR,
            ACCEPT_FLAGGED, ACCEPT_PLAIN, RAW_CLOSED, PREPS };
static const char *const how_names[] = {"send",  "sendto", "sendmsg", "write",
                                        "writev", "send(MSG_NOSIGNAL)"};
static const char *const prep_names[] = {
    "no option", "option set", "option cleared", "dup()",
    "F_DUPFD",   "set before fork", "descriptor number reused after close",
    "dup2() of a socket with the option over one without", "dup2() of a socket without the option over one with",
    "accepted from a listener with the option", "accepted from a listener without the option",
    "descriptor number reused after a close behind libc's back"};

// close(2) as the host kernel's system call, which the emulation does not see (arm64 only: other
// slices do not reach the host kernel directly).
#if defined(__aarch64__)
static void raw_close(int fd) {
  register long x0 __asm__("x0") = fd;
  register long x8 __asm__("x8") = 57; // Linux __NR_close
  __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
}
#define HAVE_RAW_CLOSE 1
#endif

// With a listener_option of 0 or 1 the listener gets that option before it accepts.
static int set_option(int fd, int value);
static int make_pair(int tcp, int fds[2], int listener_option) {
  if (!tcp)
    return socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  struct sockaddr_in addr = {.sin_len = sizeof(addr), .sin_family = AF_INET};
  socklen_t len = sizeof(addr);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  int l = socket(AF_INET, SOCK_STREAM, 0);
  fds[0] = socket(AF_INET, SOCK_STREAM, 0);
  if (l < 0 || fds[0] < 0 || bind(l, (struct sockaddr *)&addr, len) || listen(l, 1) ||
      getsockname(l, (struct sockaddr *)&addr, &len) ||
      (listener_option >= 0 && set_option(l, listener_option)) ||
      connect(fds[0], (struct sockaddr *)&addr, len))
    return -1;
  fds[1] = accept(l, NULL, NULL);
  close(l);
  return fds[1] < 0 ? -1 : 0;
}

static int set_option(int fd, int value) {
  return setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
}

static int get_option(int fd) {
  int value = -1;
  socklen_t len = sizeof(value);
  return getsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &value, &len) ? -1 : value;
}

static long write_with(enum how how, int fd) {
  char byte = 'x';
  struct iovec iov = {&byte, 1};
  struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};
  switch (how) {
  case SEND: return send(fd, &byte, 1, 0);
  case SENDTO: return sendto(fd, &byte, 1, 0, NULL, 0);
  case SENDMSG: return sendmsg(fd, &msg, 0);
  case WRITE: return write(fd, &byte, 1);
  case WRITEV: return writev(fd, &iov, 1);
  default: return send(fd, &byte, 1, MSG_NOSIGNAL);
  }
}

// On TCP the first write after the peer's close can still succeed; the reset it
// provokes makes a later one fail. Returns 0 if the failure was EPIPE.
static int write_until_failure(enum how how, int fd) {
  for (int i = 0; i < 30; i++) {
    if (write_with(how, fd) < 0)
      return errno == EPIPE ? 0 : 14;
    usleep(20000);
  }
  return 15;
}

static int child_main(enum how how, int tcp, enum prep prep) {
  int fds[2], other[2], fd;
  signal(SIGPIPE, SIG_DFL);
  if (prep == ACCEPT_FLAGGED || prep == ACCEPT_PLAIN) {
    if (make_pair(1, fds, prep == ACCEPT_FLAGGED))
      return 10;
    close(fds[0]);
    return write_until_failure(how, fds[1]);
  }
  if (make_pair(tcp, fds, -1))
    return 10;
  fd = fds[0];
  if (prep != NONE && prep != REUSED_NUMBER && prep != DUP2_SET && prep != DUP2_CLEAR &&
      prep != RAW_CLOSED && set_option(fd, 1))
    return 11;
  if (prep == CLEARED && set_option(fd, 0))
    return 11;
  if (prep == DUP || prep == DUPFD) {
    // The original can go; the copy keeps the option on its own.
    fd = prep == DUP ? dup(fd) : fcntl(fd, F_DUPFD, 20);
    close(fds[0]);
    if (fd < 0)
      return 12;
  }
  if (prep == REUSED_NUMBER) {
    // The option of a closed socket must not stay with its descriptor number.
    if (set_option(fd, 1) || make_pair(tcp, other, -1))
      return 11;
    close(fd), close(other[0]), close(other[1]), close(fds[1]);
    if (make_pair(tcp, fds, -1))
      return 10;
    fd = fds[0];
  }
  if (prep == DUP2_SET || prep == DUP2_CLEAR) {
    // The target's own option is replaced by the source's.
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, other) || set_option(prep == DUP2_SET ? fd : other[0], 1))
      return 11;
    if (dup2(fd, other[0]) < 0)
      return 12;
    close(fd), close(other[1]);
    fd = other[0];
  }
#ifdef HAVE_RAW_CLOSE
  if (prep == RAW_CLOSED) {
    // Nothing told the emulation that the descriptor went away.
    if (set_option(fd, 1))
      return 11;
    raw_close(fd);
    close(fds[1]);
    if (make_pair(tcp, fds, -1))
      return 10;
    fd = fds[0];
  }
#endif
  close(fds[1]);

  if (prep == BEFORE_FORK) { // the grandchild inherits the socket and its option
    int status;
    pid_t inner = fork();
    if (inner == 0)
      _exit(write_until_failure(how, fd));
    return inner > 0 && waitpid(inner, &status, 0) == inner && WIFEXITED(status)
               ? WEXITSTATUS(status) : 16;
  }
  return write_until_failure(how, fd);
}

static int run_case(enum how how, int tcp, enum prep prep) {
  int status;
  int killed = (prep == NONE || prep == CLEARED || prep == REUSED_NUMBER || prep == DUP2_CLEAR ||
                prep == ACCEPT_PLAIN || prep == RAW_CLOSED) &&
               how != SEND_MSG_NOSIGNAL;
  fflush(NULL);
  pid_t child = fork();
  if (child == 0)
    _exit(child_main(how, tcp, prep));
  if (child < 0 || waitpid(child, &status, 0) != child)
    return 1;

  int ok = killed ? WIFSIGNALED(status) && WTERMSIG(status) == SIGPIPE
                  : WIFEXITED(status) && WEXITSTATUS(status) == 0;
  if (!ok)
    fprintf(stderr, "FAIL: %s on %s, %s: expected %s, got %s %d\n", how_names[how],
            tcp ? "tcp loopback" : "socketpair", prep_names[prep],
            killed ? "SIGPIPE" : "EPIPE and survival", WIFSIGNALED(status) ? "signal" : "exit",
            WIFSIGNALED(status) ? WTERMSIG(status) : WEXITSTATUS(status));
  return !ok;
}

static int test_option_values(void) {
  int fds[2], pipefds[2], copy, value = 1;

  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0 && pipe(pipefds) == 0);
  CHECK(get_option(fds[0]) == 0);
  CHECK(set_option(fds[0], 42) == 0 && get_option(fds[0]) == 1); // any non-zero value
  CHECK(get_option(fds[1]) == 0);                                // per socket
  CHECK((copy = dup(fds[0])) >= 0 && get_option(copy) == 1);     // shared by a copy
  CHECK(set_option(fds[0], 0) == 0 && get_option(fds[0]) == 0);
  CHECK(set_option(fds[0], 1) == 0);
  close(fds[0]), close(fds[1]), close(copy);
  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0 && get_option(fds[0]) == 0);

  errno = 0;
  CHECK(setsockopt(fds[0], SOL_SOCKET, SO_NOSIGPIPE, &value, 2) == -1 && errno == EINVAL);
  CHECK(set_option(-1, 1) == -1 && errno == EBADF);
  CHECK(set_option(pipefds[1], 1) == -1 && errno == ENOTSOCK);
  CHECK(get_option(pipefds[0]) == -1 && errno == ENOTSOCK);
  CHECK(write(pipefds[1], "x", 1) == 1);
  return 0;
}

#ifdef HAVE_RAW_CLOSE
// A descriptor number left flagged by a close behind the emulation's back can come back as
// something that is no socket: the write must still work, as a plain write().
static int test_stale_non_socket(void) {
  int sv[2], p[2];

  CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0 && set_option(sv[1], 1) == 0);
  close(sv[0]);
  raw_close(sv[1]);
  CHECK(pipe(p) == 0 && p[1] == sv[1]);
  CHECK(write(p[1], "x", 1) == 1);
  CHECK(writev(p[1], &(struct iovec){"y", 1}, 1) == 1);
  close(p[0]), close(p[1]);
  return 0;
}
#endif

int main(void) {
  int failures = 0;

  if (test_option_values()) {
    fprintf(stderr, "FAIL: option values and errors\n");
    failures++;
  }
#ifdef HAVE_RAW_CLOSE
  if (test_stale_non_socket()) {
    fprintf(stderr, "FAIL: stale option on a descriptor that is no socket\n");
    failures++;
  }
#endif
  for (int tcp = 0; tcp <= 1; tcp++)
    for (int how = SEND; how < HOWS; how++)
      for (int prep = NONE; prep < PREPS; prep++) {
        int accepted = prep == ACCEPT_FLAGGED || prep == ACCEPT_PLAIN;
        if ((how == SEND_MSG_NOSIGNAL && prep != NONE) || (accepted && !tcp))
          continue;
#ifdef HAVE_RAW_CLOSE
        if (prep == RAW_CLOSED && tcp) // the number to be reused is not predictable
          continue;
#else
        if (prep == RAW_CLOSED)
          continue;
#endif
        failures += run_case(how, tcp, prep);
      }

  if (failures)
    fprintf(stderr, "FAILED: %d\n", failures);
  else
    printf("OK\n");
  return failures != 0;
}
