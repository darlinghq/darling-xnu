#ifndef DARLING_CONVERSION_NETWORK_SOCKET
#define DARLING_CONVERSION_NETWORK_SOCKET

int sfamily_linux_to_bsd(int family);
int sfamily_bsd_to_linux(int family);

#define LINUX_SYS_SOCKET	1

#endif // DARLING_CONVERSION_NETWORK_SOCKET
