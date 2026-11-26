#ifndef DARLING_SENDFILE_H
#define DARLING_SENDFILE_H

struct darwin_iovec;
struct darwin_sf_hdtr;

long sys_sendfile(int fd, int socket, long long offset, long long* nbytes,
		struct darwin_sf_hdtr* hdtr, int flags);

#endif
