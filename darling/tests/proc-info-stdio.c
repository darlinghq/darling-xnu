#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* Interfaces from libsyscall/wrappers/libproc/libproc.h. */
int proc_pidinfo(int, int, uint64_t, void *, int);
int proc_pidfdinfo(int, int, int, void *, int);

int main(void)
{
    int output[2];
    if (pipe(output) != 0) return 2;
    fflush(NULL);
    pid_t child = fork();
    if (child < 0) return 2;
    if (child == 0) {
        close(output[0]);
        if (dup2(output[1], STDOUT_FILENO) < 0) _exit(2);
        close(output[1]);
        proc_pidinfo(getpid(), 0x7fffffff, 0, NULL, 0);
        proc_pidfdinfo(getpid(), 0, 0x7fffffff, NULL, 0);
        _exit(0);
    }
    close(output[1]);
    char buffer[512];
    ssize_t count;
    size_t total = 0;
    while ((count = read(output[0], buffer, sizeof(buffer))) != 0) {
        if (count < 0) {
            if (errno == EINTR) continue;
            return 2;
        }
        total += count;
    }
    close(output[0]);
    int status;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 2;
    if (total != 0) {
        fprintf(stderr, "FAIL: proc_info wrote %zu bytes to stdout\n", total);
        return 1;
    }
    puts("PASS: proc_info leaves stdout clean");
    return 0;
}
