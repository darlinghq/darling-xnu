#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int64_t clock_ns(clockid_t id)
{
	struct timespec value;
	if (clock_gettime(id, &value))
	{
		perror("clock_gettime");
		_Exit(2);
	}
	return (int64_t)value.tv_sec * 1000000000LL + value.tv_nsec;
}

int main(void)
{
	struct timeval last = {0};
	int changes = 0, backwards = 0;
	int64_t previous = clock_ns(CLOCK_MONOTONIC);
	int64_t start = clock_ns(CLOCK_UPTIME_RAW);
	for (int i = 0; i < 180; i++)
	{
		struct timeval boot = {0};
		size_t size = sizeof(boot);
		if (sysctlbyname("kern.boottime", &boot, &size, NULL, 0))
			return 2;
		if (i && (last.tv_sec != boot.tv_sec || last.tv_usec != boot.tv_usec))
			changes++;
		last = boot;
		int64_t now = clock_ns(CLOCK_MONOTONIC);
		if (now < previous)
		{
			backwards++;
			printf("monotonic backwards by %.3f seconds\n", (previous-now)/1e9);
		}
		previous = now;
		usleep(20000);
	}
	printf("boot timestamp changes=%d backwards=%d raw elapsed=%.3f seconds\n",
		changes, backwards, (clock_ns(CLOCK_UPTIME_RAW)-start)/1e9);
	return changes || backwards ? 1 : 0;
}
