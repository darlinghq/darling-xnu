#ifndef LINUX_GETDIRENTRIES_H
#define LINUX_GETDIRENTRIES_H

long sys_getdirentries(int fd, char* buf, unsigned int len, long* basep);
long sys_getdirentries64(int fd, char* buf, unsigned int len, long* basep);

struct linux_dirent64
{
	unsigned long long d_ino;
	unsigned long long d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

struct bsd_dirent
{
	unsigned int d_ino;
	unsigned short d_reclen;
	unsigned char d_type;
	unsigned char d_namlen;
	char d_name[];
};

struct bsd_dirent64
{
	unsigned long long d_ino;
	unsigned long long d_seekoff;
	unsigned short d_reclen;
	unsigned short d_namlen;
	unsigned char d_type;
	char d_name[];
};

#endif

