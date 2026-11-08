#ifndef BSDTHREAD_REGISTER_H
#define BSDTHREAD_REGISTER_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*bsdthread_entry_t)(void* /* self */, int /* thread_port */, void* /* funptr */,
		void* /* funarg */, unsigned long /* stacksize */, unsigned int /* flags */);
typedef void (*bsdwqthread_entry_t)(void* /* self */, int /* thread_port */, void* /* stackaddr */,
		void* /* item */, int /* reuse */, int /* nevents */);

extern int pthread_obj_size;
extern bsdthread_entry_t pthread_entry_point;
extern bsdwqthread_entry_t wqueue_entry_point;

struct _pthread_registration_data;

void set_thread_handles(uintptr_t pthread_handle);

// true once libpthread has registered with us, i.e. every Darwin thread has a TSD we can read
bool bsdthread_registered(void);

// the calling thread's TSD slot that it shares with darlingserver for cancellation (see pthread_canceled.c);
// only valid once bsdthread_registered()
uintptr_t* bsdthread_cancel_marked_slot(void);

long sys_bsdthread_register(void* thread_start, void* wqthread, int pthsize,
		const struct _pthread_registration_data* pth_regdata,
		unsigned long pth_regdata_len, unsigned long long dpq_offset);
void pthread_entry_point_wrapper(void* self, int thread_port, void* funptr,
		void* funarg, unsigned long stacksize, unsigned int flags);
void wqueue_entry_point_wrapper(void* self, int thread_port, void* stackaddr,
		void* item, int reuse, int nevents);
void wqueue_entry_point_asm_jump(void* self, int thread_port, void* stackaddr,
		void* item, int reuse, int nevents);

#endif // BSDTHREAD_REGISTER_H
