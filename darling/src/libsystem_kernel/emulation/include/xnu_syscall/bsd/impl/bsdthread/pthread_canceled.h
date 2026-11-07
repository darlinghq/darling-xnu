#ifndef PTHREAD_CANCELED_H
#define PTHREAD_CANCELED_H

#include <stdbool.h>

long sys_pthread_canceled(int action);
bool pthread_testcancel_presyscall(void);

#endif // PTHREAD_CANCELED_H
