#ifndef LINUX_HFS_STUB_H
#define LINUX_HFS_STUB_H

long sys_mkcomplex();
long sys_statv();
long sys_lstatv();
long sys_fstatv();
long sys_getdirentriesattr();
long sys_exchangedata();
long sys_searchfs();
long sys_delete();
long sys_copyfile();
long sys_open_dprotected_np(const char* path, int flags, int class, int dpflags, int mode);

#endif

