#ifndef DARLING_EMULATION_AIO_H
#define DARLING_EMULATION_AIO_H

// POSIX asynchronous I/O (BSD syscalls 313-320, 421). See aio.c.

long sys_aio_fsync(int op, void* aiocbp);
long sys_aio_return(void* aiocbp);
long sys_aio_suspend(void* const* aiocblist, int nent, const void* timeoutp);
long sys_aio_suspend_nocancel(void* const* aiocblist, int nent, const void* timeoutp);
long sys_aio_cancel(int fd, void* aiocbp);
long sys_aio_error(void* aiocbp);
long sys_aio_read(void* aiocbp);
long sys_aio_write(void* aiocbp);
long sys_lio_listio(int mode, void* const* aiocblist, int nent, const void* sigp);

// The child of a fork has no requests of its own (as on XNU) and nobody holds the table's lock.
void aio_postfork_child(void);

#endif // DARLING_EMULATION_AIO_H
