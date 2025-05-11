#ifndef DARLING_CONVERSION_SYSV_SEM_SEMCTL
#define DARLING_CONVERSION_SYSV_SEM_SEMCTL

#include <stdint.h>

union xnu_semun
{
	int val;
	struct bsd_semid_ds* buf;
	unsigned short* array;
};

struct linux_ipc_perm
{
	int32_t _key;
	uint32_t uid;
	uint32_t gid;
	uint32_t cuid;
	uint32_t cgid;
	short int mode;
};

struct linux_semid_ds
{
	struct linux_ipc_perm sem_perm;
	unsigned long long sem_otime;
	unsigned long long sem_ctime;
	unsigned long sem_nsems;
};

struct bsd_ipc_perm
{
	uint32_t uid;
	uint32_t gid;
	uint32_t cuid;
	uint32_t cgid;
	short int mode;
};

struct bsd_semid_ds
{
	struct bsd_ipc_perm sem_perm;
	int32_t sem_base;
	unsigned short sem_nsems;
	unsigned long long sem_otime;
	uint32_t sem_pad1;
	unsigned long long sem_ctime;
	uint32_t sem_pad2;
};

typedef union xnu_semun xnu_semun_t;

#endif // DARLING_CONVERSION_SYSV_SEM_SEMCTL
