#ifndef DARLING_EMULATION_NOSIGPIPE_H
#define DARLING_EMULATION_NOSIGPIPE_H

// Darwin's per-socket SO_NOSIGPIPE option: a write to a socket whose peer has
// gone fails with EPIPE without raising SIGPIPE. Linux only has MSG_NOSIGNAL
// per call, so the option is kept here, per descriptor, and the write paths
// ask for MSG_NOSIGNAL while it is set.

// Returns 1 if SO_NOSIGPIPE is set on the socket behind fd, else 0.
int nosigpipe_get(int fd);

// Sets or clears the option. Returns 0, or a negative Darwin errno if fd
// cannot be tracked.
long nosigpipe_set(int fd, int on);

// Descriptor bookkeeping: close (called before the descriptor is closed) and
// dup/dup2/F_DUPFD (called after newfd was created; newfd shares oldfd's
// open file and so its option). Until the call returns, a thread that uses
// newfd already sees the option as it was left by the previous owner of the
// number.
void nosigpipe_close(int fd);
void nosigpipe_dup(int oldfd, int newfd);

// A socket was just created at fd (socket, socketpair): forget whatever an
// earlier owner of the number left behind. A descriptor can be closed behind
// this module's back (by a forked child's guard table, or by the host under
// FEX), so close() is not enough. accept() uses nosigpipe_dup() from the
// listener instead, as the accepted socket inherits the option.
void nosigpipe_created(int fd);

#endif // DARLING_EMULATION_NOSIGPIPE_H
