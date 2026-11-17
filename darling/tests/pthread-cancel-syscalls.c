// Copyright (C) 2026 Lubos Dolezel
// pthread_cancel() acts on a thread blocked in a cancellation point (pthread_cancel(3), the list of
// cancellation points in the POSIX standard): the blocked call is abandoned, the thread's cleanup
// handlers run and the thread exits with PTHREAD_CANCELED. With cancellation disabled the call stays
// blocked until it completes, and the cancel is acted on once cancellation is enabled again. The
// $NOCANCEL variants of the calls are not cancellation points.
//
// Each case blocks a thread in a call that nothing completes. Then:
//   cancel   - the thread is cancelled while blocked
//   disabled - with PTHREAD_CANCEL_DISABLE the call stays blocked until the test completes it; it returns its
//              result, and the thread is cancelled when it enables cancellation
//   nocancel - the $NOCANCEL variant stays blocked, completes and returns normally
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// The $NOCANCEL variants are exported from libSystem, but not under the same names on every architecture
// (i386 has the $UNIX2003 forms), so they are looked up by name at run time.
static void* nocancel_symbol(const char* name)
{
	char buf[64];
	void* p;

	snprintf(buf, sizeof(buf), "%s$NOCANCEL", name);
	p = dlsym(RTLD_DEFAULT, buf);
	if (!p) {
		snprintf(buf, sizeof(buf), "%s$NOCANCEL$UNIX2003", name);
		p = dlsym(RTLD_DEFAULT, buf);
	}
	if (!p) {
		printf("FAIL: %s$NOCANCEL is not exported\n", name);
		exit(2);
	}
	return p;
}

#define read_nocancel(fd, buf, n) ((ssize_t (*)(int, void*, size_t)) nocancel_symbol("read"))(fd, buf, n)
#define recvfrom_nocancel(fd, buf, n, fl, from, len) \
	((ssize_t (*)(int, void*, size_t, int, struct sockaddr*, socklen_t*)) nocancel_symbol("recvfrom"))(fd, buf, n, fl, from, len)
#define accept_nocancel(fd, from, len) \
	((int (*)(int, struct sockaddr*, socklen_t*)) nocancel_symbol("accept"))(fd, from, len)
#define poll_nocancel(fds, n, t) ((int (*)(struct pollfd*, nfds_t, int)) nocancel_symbol("poll"))(fds, n, t)

struct tcase {
	const char* name;
	void (*setup)(void);
	void (*block)(void);    // runs in the thread
	void (*release)(void);  // completes the blocked call
	void (*teardown)(void);
	int nocancel;           // block() is a $NOCANCEL call
};

static int fds[2], lsock = -1;
static pid_t child = -1;
static char path[128], fifo[160], lockfile[160];
static int failures;

static atomic_int started, returned, cleaned, block_errno;

static void sleep_ms(long ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
	while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
		;
}

static void fail(const char* name, const char* mode, const char* what)
{
	printf("FAIL %s [%s]: %s\n", name, mode, what);
	failures++;
}

static void fill_pipe(int fd)
{
	int fl = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);
	while (write(fd, "x", 1) == 1)
		;
	fcntl(fd, F_SETFL, fl);
}

static void drain(int fd)
{
	char buf[4096];
	int fl = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, fl | O_NONBLOCK);
	while (read(fd, buf, sizeof(buf)) > 0)
		;
	fcntl(fd, F_SETFL, fl);
}

static void fill_socket(int fd)
{
	fill_pipe(fd);  // writes until the socket is full
}

// a pipe that nothing is written to
static void setup_pipe(void) { if (pipe(fds)) exit(2); }
static void teardown_pipe(void) { close(fds[0]); close(fds[1]); }
static void release_pipe(void) { if (write(fds[1], "x", 1) != 1) exit(2); }

static void block_read(void) { char c; if (read(fds[0], &c, 1) == 1) atomic_store(&returned, 1); }
// A $NOCANCEL call must not be cancelled, but macOS does interrupt some of them (with EINTR) when the thread
// is marked for cancellation; the call is then repeated, which is what a caller of a $NOCANCEL call
// has to do for any signal.
#define RETRY(call) ({ long r_; do { r_ = (long) (call); } while (r_ == -1 && errno == EINTR); r_; })

static void note_result(long r, long expected)
{
	// no close() or other cancellation point here: the thread is marked for cancellation
	if (r == expected)
		atomic_store(&returned, 1);
	else
		atomic_store(&block_errno, r == -1 ? errno : (int) r);
}
static void block_read_nc(void) { char c; note_result(RETRY(read_nocancel(fds[0], &c, 1)), 1); }
static void block_readv(void)
{
	char c;
	struct iovec iov = { &c, 1 };
	if (readv(fds[0], &iov, 1) == 1) atomic_store(&returned, 1);
}
static void block_poll(void)
{
	struct pollfd p = { fds[0], POLLIN, 0 };
	if (poll(&p, 1, -1) == 1) atomic_store(&returned, 1);
}
static void block_poll_nc(void)
{
	struct pollfd p = { fds[0], POLLIN, 0 };
	note_result(RETRY(poll_nocancel(&p, 1, -1)), 1);
}
static void block_select(void)
{
	fd_set s;
	FD_ZERO(&s);
	FD_SET(fds[0], &s);
	if (select(fds[0] + 1, &s, NULL, NULL, NULL) == 1) atomic_store(&returned, 1);
}
static void block_pselect(void)
{
	fd_set s;
	FD_ZERO(&s);
	FD_SET(fds[0], &s);
	if (pselect(fds[0] + 1, &s, NULL, NULL, NULL, NULL) == 1) atomic_store(&returned, 1);
}

// a full pipe: write() blocks
static void setup_full_pipe(void) { setup_pipe(); fill_pipe(fds[1]); }
static void release_full_pipe(void) { drain(fds[0]); }
static void block_write(void) { if (write(fds[1], "y", 1) == 1) atomic_store(&returned, 1); }
static void block_writev(void)
{
	struct iovec iov = { "y", 1 };
	if (writev(fds[1], &iov, 1) == 1) atomic_store(&returned, 1);
}

// a connected socket pair
static void setup_pair(void) { if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) exit(2); }
static void block_recv(void) { char c; if (recv(fds[0], &c, 1, 0) == 1) atomic_store(&returned, 1); }
static void block_recvfrom(void) { char c; if (recvfrom(fds[0], &c, 1, 0, NULL, NULL) == 1) atomic_store(&returned, 1); }
static void block_recvfrom_nc(void)
{
	char c;
	note_result(RETRY(recvfrom_nocancel(fds[0], &c, 1, 0, NULL, NULL)), 1);
}
static void block_recvmsg(void)
{
	char c;
	struct iovec iov = { &c, 1 };
	struct msghdr m;
	memset(&m, 0, sizeof(m));
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	if (recvmsg(fds[0], &m, 0) == 1) atomic_store(&returned, 1);
}
static void setup_full_pair(void) { setup_pair(); fill_socket(fds[0]); }
static void release_full_pair(void) { drain(fds[1]); }
static void block_send(void) { if (send(fds[0], "y", 1, 0) == 1) atomic_store(&returned, 1); }
static void block_sendto(void) { if (sendto(fds[0], "y", 1, 0, NULL, 0) == 1) atomic_store(&returned, 1); }
static void block_sendmsg(void)
{
	struct iovec iov = { "y", 1 };
	struct msghdr m;
	memset(&m, 0, sizeof(m));
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
	if (sendmsg(fds[0], &m, 0) == 1) atomic_store(&returned, 1);
}

// a listening socket that nobody connects to
static void setup_listen(void)
{
	struct sockaddr_un sa;
	snprintf(path, sizeof(path), "/tmp/pcs-%d.sock", getpid());
	unlink(path);
	lsock = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);
	if (bind(lsock, (struct sockaddr*) &sa, sizeof(sa)) || listen(lsock, 1)) exit(2);
}
static void release_listen(void)
{
	struct sockaddr_un sa;
	int c = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);
	if (connect(c, (struct sockaddr*) &sa, sizeof(sa))) exit(2);
	sleep_ms(200);
	close(c);
}
static void teardown_listen(void) { close(lsock); unlink(path); }
static void block_accept(void) { int c = accept(lsock, NULL, NULL); if (c >= 0) { close(c); atomic_store(&returned, 1); } }
static void block_accept_nc(void)
{
	int c = RETRY(accept_nocancel(lsock, NULL, NULL));
	// no close() here: it is a cancellation point, and the thread is marked for cancellation
	if (c >= 0) atomic_store(&returned, 1); else atomic_store(&block_errno, errno);
}

// a connect() that blocks: the queue of a listening socket is full. A child process fills it up (its own
// connect() blocks once the queue is full), so that the test thread's connect() queues up behind it.
static volatile int connect_skipped;
static void setup_connect(void)
{
	struct sockaddr_un sa;
	snprintf(path, sizeof(path), "/tmp/pcs-%d.sock", getpid());
	unlink(path);
	connect_skipped = 0;
	lsock = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);
	if (bind(lsock, (struct sockaddr*) &sa, sizeof(sa)) || listen(lsock, 0)) exit(2);
	child = fork();
	if (child == 0) {
		for (int i = 0; i < 4; i++) {
			int c = socket(AF_UNIX, SOCK_STREAM, 0);
			connect(c, (struct sockaddr*) &sa, sizeof(sa));
		}
		pause();
		_exit(0);
	}
	sleep_ms(300);
}
static void release_connect(void)
{
	// accepting the queued connections makes room for the blocked ones
	int fl = fcntl(lsock, F_GETFL);
	fcntl(lsock, F_SETFL, fl | O_NONBLOCK);
	for (int i = 0; i < 20; i++) {
		int c = accept(lsock, NULL, NULL);
		if (c >= 0)
			close(c);
		sleep_ms(50);
	}
}
static void teardown_connect(void)
{
	int st;
	kill(child, SIGKILL);
	waitpid(child, &st, 0);
	close(lsock);
	unlink(path);
}
static void block_connect(void)
{
	struct sockaddr_un sa;
	int c = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);
	if (connect(c, (struct sockaddr*) &sa, sizeof(sa)) == 0)
		atomic_store(&returned, 1);
	else if (errno == ECONNREFUSED || errno == EAGAIN)
		connect_skipped = 1;  // this system does not block a connect() on a full queue
	close(c);
}

// a child process that exits when released
static void setup_child(void)
{
	if (pipe(fds)) exit(2);
	child = fork();
	if (child == 0) {
		char c;
		close(fds[1]);
		if (read(fds[0], &c, 1)) {}
		_exit(0);
	}
}
static void release_child(void) { release_pipe(); }
static void teardown_child(void)
{
	int st;
	close(fds[1]);
	close(fds[0]);
	waitpid(child, &st, 0);
}
static void block_wait4(void) { int st; note_result(RETRY(wait4(child, &st, 0, NULL)), child); }
static void block_waitpid(void) { int st; if (waitpid(child, &st, 0) == child) atomic_store(&returned, 1); }
static void block_waitid(void)
{
	siginfo_t si;
	if (waitid(P_PID, child, &si, WEXITED) == 0) atomic_store(&returned, 1);
}

// SIGUSR2 is blocked in every thread; sigwait() takes it
static void setup_sig(void)
{
	sigset_t s;
	sigemptyset(&s);
	sigaddset(&s, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &s, NULL);
}
static void release_sig(void) { kill(getpid(), SIGUSR2); }
// the signal is only sent when the case completes the call, and sigwait() takes it
static void teardown_sig(void) {}
static void block_sigwait(void)
{
	sigset_t s;
	int sig;
	sigemptyset(&s);
	sigaddset(&s, SIGUSR2);
	if (sigwait(&s, &sig) == 0) atomic_store(&returned, 1);
}

// open() of a FIFO for reading blocks until there is a writer
static void setup_fifo(void)
{
	snprintf(fifo, sizeof(fifo), "/tmp/pcs-%d.fifo", getpid());
	unlink(fifo);
	if (mkfifo(fifo, 0600)) exit(2);
}
static void release_fifo(void)
{
	int w = open(fifo, O_WRONLY);
	sleep_ms(200);
	close(w);
}
static void teardown_fifo(void) { unlink(fifo); }
static void block_open(void) { int f = open(fifo, O_RDONLY); if (f >= 0) { close(f); atomic_store(&returned, 1); } }

// fcntl(F_SETLKW) on a file that another process has locked
static void setup_lock(void)
{
	snprintf(lockfile, sizeof(lockfile), "/tmp/pcs-%d.lock", getpid());
	if (pipe(fds)) exit(2);
	int ready[2];
	if (pipe(ready)) exit(2);
	child = fork();
	if (child == 0) {
		char c;
		int f;
		close(fds[1]);
		f = open(lockfile, O_RDWR | O_CREAT, 0600);
		struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET };
		fcntl(f, F_SETLK, &fl);
		if (write(ready[1], "x", 1)) {}
		if (read(fds[0], &c, 1)) {}
		_exit(0);
	}
	char c;
	if (read(ready[0], &c, 1) != 1) exit(2);
	close(ready[0]);
	close(ready[1]);
}
static void teardown_lock(void)
{
	int st;
	close(fds[0]);
	close(fds[1]);
	waitpid(child, &st, 0);
	unlink(lockfile);
}
static void block_lock(void)
{
	int f = open(lockfile, O_RDWR);
	struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET };
	if (fcntl(f, F_SETLKW, &fl) == 0) atomic_store(&returned, 1);
	close(f);
}

static const struct tcase cases[] = {
	{ "read(pipe)", setup_pipe, block_read, release_pipe, teardown_pipe, 0 },
	{ "readv(pipe)", setup_pipe, block_readv, release_pipe, teardown_pipe, 0 },
	{ "write(full pipe)", setup_full_pipe, block_write, release_full_pipe, teardown_pipe, 0 },
	{ "writev(full pipe)", setup_full_pipe, block_writev, release_full_pipe, teardown_pipe, 0 },
	{ "recv", setup_pair, block_recv, release_pipe, teardown_pipe, 0 },
	{ "recvfrom", setup_pair, block_recvfrom, release_pipe, teardown_pipe, 0 },
	{ "recvmsg", setup_pair, block_recvmsg, release_pipe, teardown_pipe, 0 },
	{ "send(full socket)", setup_full_pair, block_send, release_full_pair, teardown_pipe, 0 },
	{ "sendto(full socket)", setup_full_pair, block_sendto, release_full_pair, teardown_pipe, 0 },
	{ "sendmsg(full socket)", setup_full_pair, block_sendmsg, release_full_pair, teardown_pipe, 0 },
	{ "accept", setup_listen, block_accept, release_listen, teardown_listen, 0 },
	{ "connect(full queue)", setup_connect, block_connect, release_connect, teardown_connect, 0 },
	{ "poll", setup_pipe, block_poll, release_pipe, teardown_pipe, 0 },
	{ "select", setup_pipe, block_select, release_pipe, teardown_pipe, 0 },
	{ "pselect", setup_pipe, block_pselect, release_pipe, teardown_pipe, 0 },
	// wait4() itself is not a cancellation point on macOS (waitpid() and waitid() are)
	{ "wait4", setup_child, block_wait4, release_child, teardown_child, 1 },
	{ "waitpid", setup_child, block_waitpid, release_child, teardown_child, 0 },
	{ "waitid", setup_child, block_waitid, release_child, teardown_child, 0 },
	{ "sigwait", setup_sig, block_sigwait, release_sig, teardown_sig, 0 },
	{ "open(fifo)", setup_fifo, block_open, release_fifo, teardown_fifo, 0 },
	{ "fcntl(F_SETLKW)", setup_lock, block_lock, release_child, teardown_lock, 0 },
	{ "read$NOCANCEL", setup_pipe, block_read_nc, release_pipe, teardown_pipe, 1 },
	{ "recvfrom$NOCANCEL", setup_pair, block_recvfrom_nc, release_pipe, teardown_pipe, 1 },
	{ "accept$NOCANCEL", setup_listen, block_accept_nc, release_listen, teardown_listen, 1 },
	{ "poll$NOCANCEL", setup_pipe, block_poll_nc, release_pipe, teardown_pipe, 1 },
};

enum mode { CANCEL, DISABLED, NOCANCEL };
static const char* const mode_names[] = { "cancel", "disabled", "nocancel" };

static void cleanup(void* arg)
{
	(void) arg;
	atomic_store(&cleaned, 1);
}

static void* thread_main(void* arg)
{
	const struct tcase* tc = arg;
	int old;

	pthread_cleanup_push(cleanup, NULL);
	if (!tc->nocancel && getenv("PCS_DISABLED"))
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
	atomic_store(&started, 1);
	tc->block();
	if (!tc->nocancel && getenv("PCS_DISABLED")) {
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
		pthread_testcancel();
	}
	pthread_cleanup_pop(0);
	return NULL;
}

static int wait_flag(atomic_int* flag, long ms)
{
	for (long t = 0; t < ms; t += 20) {
		if (atomic_load(flag))
			return 1;
		sleep_ms(20);
	}
	return atomic_load(flag);
}

static void run(const struct tcase* tc, enum mode mode)
{
	pthread_t th;
	void* result = NULL;
	const char* name = tc->name;
	const char* m = mode_names[mode];
	int cancelled_in_time;

	atomic_store(&started, 0);
	atomic_store(&returned, 0);
	atomic_store(&cleaned, 0);
	if (mode == DISABLED)
		setenv("PCS_DISABLED", "1", 1);
	else
		unsetenv("PCS_DISABLED");

	tc->setup();
	pthread_create(&th, NULL, thread_main, (void*) tc);
	wait_flag(&started, 2000);
	sleep_ms(300);  // let it block
	if (connect_skipped || atomic_load(&returned)) {
		pthread_join(th, NULL);
		printf("skip %s [%s]: the call does not block on this system\n", name, m);
		tc->teardown();
		return;
	}
	pthread_cancel(th);

	if (mode == CANCEL) {
		cancelled_in_time = wait_flag(&cleaned, 3000);
		if (!cancelled_in_time) {
			fail(name, m, "the thread blocked in the call was not cancelled");
			tc->release();
		}
		pthread_join(th, &result);
		if (cancelled_in_time) {
			if (result != PTHREAD_CANCELED)
				fail(name, m, "the thread did not exit with PTHREAD_CANCELED");
			else if (atomic_load(&returned))
				fail(name, m, "the call returned");
			else
				printf("ok   %s [%s]\n", name, m);
		}
	} else {
		sleep_ms(500);
		if (atomic_load(&returned) || atomic_load(&cleaned)) {
			fail(name, m, mode == DISABLED ? "the call did not stay blocked with cancellation disabled"
				: "the $NOCANCEL call was interrupted");
		}
		tc->release();
		pthread_join(th, &result);
		if (!atomic_load(&returned))
		{
			char what[96];
			snprintf(what, sizeof(what), "the call did not return its result once completed (errno %d)", atomic_load(&block_errno));
			fail(name, m, what);
		}
		else if (mode == DISABLED && (!atomic_load(&cleaned) || result != PTHREAD_CANCELED))
			fail(name, m, "the thread was not cancelled when it enabled cancellation");
		else if (mode == NOCANCEL && (atomic_load(&cleaned) || result == PTHREAD_CANCELED))
			fail(name, m, "the thread was cancelled");
		else
			printf("ok   %s [%s]\n", name, m);
	}
	tc->teardown();
}

int main(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
	signal(SIGPIPE, SIG_IGN);
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		// an optional argument selects the cases whose name contains it
		if (argc > 1 && !strstr(cases[i].name, argv[1]))
			continue;
		// an optional second argument selects one mode
		if (cases[i].nocancel) {
			if (argc <= 2 || !strcmp(argv[2], "nocancel"))
				run(&cases[i], NOCANCEL);
		} else {
			if (argc <= 2 || !strcmp(argv[2], "cancel"))
				run(&cases[i], CANCEL);
			if (argc <= 2 || !strcmp(argv[2], "disabled"))
				run(&cases[i], DISABLED);
		}
	}
	puts(failures ? "FAIL" : "PASS");
	return failures != 0;
}
