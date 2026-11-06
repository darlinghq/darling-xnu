#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int failures;
static void check(int pass, const char *name) {
    printf("%s %s\n", pass ? "PASS" : "FAIL", name);
    if (!pass) failures++;
}
static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
int main(void) {
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) return 2;
    int options[] = {SO_RCVTIMEO, SO_SNDTIMEO};
    for (unsigned i = 0; i < 2; i++) {
        for (unsigned pattern = 0; pattern < 2; pattern++) {
            struct timeval tv;
            memset(&tv, pattern ? 0xa5 : 0, sizeof(tv));
            tv.tv_sec = pattern ? 1 : 0;
            tv.tv_usec = pattern ? 250000 : 0;
            int result = setsockopt(pair[0], SOL_SOCKET, options[i], &tv, sizeof(tv));
            printf("option=%x pattern=%u errno=%d\n", options[i], pattern,
                   result < 0 ? errno : 0);
            check(result == 0, "set timeout independently of padding");
            struct timeval actual = {0};
            socklen_t size = sizeof(actual);
            check(getsockopt(pair[0], SOL_SOCKET, options[i], &actual, &size) == 0 &&
                  size == sizeof(actual) && actual.tv_sec == tv.tv_sec &&
                  actual.tv_usec == tv.tv_usec, "timeout fields round trip");
        }
    }
    // Avoid exercising unsafe baseline pointer handling after the primary failure.
    if (failures) return 1;
    for (unsigned i = 0; i < 2; i++) {
        struct timeval tv = {0};
        check(setsockopt(pair[0], SOL_SOCKET, options[i], &tv, sizeof(tv)-1) == -1 &&
              errno == EINVAL, "short timeout rejected");
        check(setsockopt(pair[0], SOL_SOCKET, options[i], &tv, (socklen_t)-1) == -1 &&
              errno == EINVAL, "negative timeout length rejected");
        check(setsockopt(pair[0], SOL_SOCKET, options[i], NULL, sizeof(tv)) == -1 &&
              errno == EFAULT, "null timeout rejected");
        tv.tv_sec = -1;
        check(setsockopt(pair[0], SOL_SOCKET, options[i], &tv, sizeof(tv)) == -1 &&
              errno == EDOM, "negative seconds rejected");
        tv.tv_sec = 0; tv.tv_usec = -1;
        check(setsockopt(pair[0], SOL_SOCKET, options[i], &tv, sizeof(tv)) == -1 &&
              errno == EDOM, "negative microseconds rejected");
        tv.tv_usec = 1000000;
        check(setsockopt(pair[0], SOL_SOCKET, options[i], &tv, sizeof(tv)) == -1 &&
              errno == EDOM, "un-normalized microseconds rejected");
    }
    struct timeval timeout;
    memset(&timeout, 0xa5, sizeof(timeout));
    timeout.tv_sec = 0; timeout.tv_usec = 100000;
    check(setsockopt(pair[0], SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0,
          "set real send timeout");
    char data[16384] = {0};
    ssize_t sent = 0;
    unsigned n;
    for (n = 0; n < 4096; n++) {
        sent = send(pair[0], data, sizeof(data), MSG_DONTWAIT);
        if (sent < 0) break;
    }
    check(n < 4096 && sent == -1 && errno == EAGAIN, "fill send buffer");
    if (n < 4096 && sent == -1 && errno == EAGAIN) {
        double start = now();
        sent = send(pair[0], data, 1, 0);
        int error = errno;
        double elapsed = now() - start;
        printf("blocked send elapsed=%.3f\n", elapsed);
        check(sent == -1 && error == EAGAIN && elapsed >= .02 && elapsed < .8,
              "send timeout actually bounds blocking send");
    }
    close(pair[0]); close(pair[1]);
    return failures ? 1 : 0;
}
