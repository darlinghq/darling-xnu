#ifndef LINUX_FILEPORT_MAKEPORT_H
#define LINUX_FILEPORT_MAKEPORT_H

struct fileport_makeport_args
{
	int fd;
	int port_out;
};

long sys_fileport_makeport(int fd, int* port);

#endif // LINUX_FILEPORT_MAKEPORT_H
