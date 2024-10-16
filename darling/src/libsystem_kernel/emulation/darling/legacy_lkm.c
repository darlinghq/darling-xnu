#include "legacy_lkm.h"

#include <emulation/common/simple.h>

__attribute__((visibility("default")))
int lkm_call(int call_nr, void* arg)
{
	__simple_printf("Something called the old LKM API (nr = %d)\n", call_nr);
	__builtin_unreachable();
}

__attribute__((visibility("default")))
int lkm_call_raw(int call_nr, void* arg)
{
	__simple_printf("Something called the old LKM API (nr = %d)\n", call_nr);
	__builtin_unreachable();
}