#ifndef _DARLING_EMULATION_MACH_SQ_H_
#define _DARLING_EMULATION_MACH_SQ_H_

#include <stdbool.h>
#include <stdint.h>
#include <mach/message.h>

// Simple messages a task sends to its own ports, and receives from them, without darlingserver.
// The ports stay darlingserver's; only the message queue of a port the server put in shared-queue mode
// lives in memory the task shares with the server (darlingserver/mach-sq.h). Anything this code does
// not cover goes to the server as before.

// at libsystem_kernel initialization: reads the kill switch DARLING_MACH_SERVER=1
void mach_sq_init(const char* envp[]);

// before an RPC that creates a receive right for this task: makes sure the task has its region
void mach_sq_prepare(void);

// mach_msg_overwrite_trap(): returns true when the call was done here (`*result` set), false when it
// has to go to the server unchanged
bool mach_sq_msg(mach_msg_header_t* msg, mach_msg_option_t option, mach_msg_size_t send_size,
		mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
		mach_msg_header_t* rcv_msg, mach_msg_return_t* result);

// from the suspend and sigexc signal handlers, for a signal not caused by this thread: if the thread is
// between making a message visible and waking its waiter, keeps the signal to be raised again right
// after the wake and returns true (the handler then returns at once)
struct linux_siginfo;
bool mach_sq_defer_signal(int linux_signum, const struct linux_siginfo* info);

// in the child of a fork: the region was not inherited
void mach_sq_postfork_child(void);

#endif // _DARLING_EMULATION_MACH_SQ_H_
