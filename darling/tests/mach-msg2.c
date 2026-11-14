// mach_msg2_internal sends and receives through the register-packed mach_msg2 interface. The packing is the one of
// XNU's own mach_msg2 inline wrapper (osfmk/mach/message.h, APSL), copied below as msg2(): the first named value of
// each word in the low half, data pointing at the whole message. The oracle for the transport is the classic mach_msg:
// a message sent with mach_msg2 must arrive intact through mach_msg, and the other way round. The symbol is looked up
// at run time so the same binary reports a missing export instead of failing to load.
#include <dlfcn.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef mach_msg_return_t (*msg2_fn)(void*, mach_msg_option64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static msg2_fn msg2_internal;

#define PAD sizeof(mach_msg_max_trailer_t)
#define BIG 8192

typedef struct {
	mach_msg_header_t header;
	char body[BIG];
	mach_msg_max_trailer_t trailer;
} buffer_t;

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s\n", what);
		exit(1);
	}
}

static void expect_rc(mach_msg_return_t got, mach_msg_return_t want, const char* what)
{
	if (got != want) {
		printf("FAIL: %s (got 0x%x, want 0x%x)\n", what, got, want);
		exit(1);
	}
}

#define SHIFT_ARGS(lo, hi) ((uint64_t)(hi) << 32 | (uint32_t)(lo))

// the call XNU's static inline mach_msg2 (osfmk/mach/message.h) makes once it has the descriptor count
static mach_msg_return_t msg2_packed(void* data, uint64_t option64, mach_msg_header_t header, mach_msg_size_t send_size,
    mach_msg_size_t descriptors, mach_msg_size_t rcv_size, mach_port_t rcv_name, uint64_t timeout, uint32_t priority)
{
	return msg2_internal(data, option64, SHIFT_ARGS(header.msgh_bits, send_size),
	    SHIFT_ARGS(header.msgh_remote_port, header.msgh_local_port), SHIFT_ARGS(header.msgh_voucher_port, header.msgh_id),
	    SHIFT_ARGS(descriptors, rcv_name), SHIFT_ARGS(rcv_size, priority), timeout);
}

// XNU's static inline mach_msg2: same argument order and packing, data is the whole message
static mach_msg_return_t msg2(void* data, uint64_t option64, mach_msg_header_t header, mach_msg_size_t send_size,
    mach_msg_size_t rcv_size, mach_port_t rcv_name, uint64_t timeout, uint32_t priority)
{
	mach_msg_base_t* base;
	mach_msg_size_t descriptors;
	if (option64 & MACH64_MSG_VECTOR) {
		base = (mach_msg_base_t*)(uintptr_t)((mach_msg_vector_t*)data)->msgv_data;
	} else {
		base = (mach_msg_base_t*)data;
	}
	if ((option64 & MACH_SEND_MSG) && (base->header.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
		descriptors = base->body.msgh_descriptor_count;
	} else {
		descriptors = 0;
	}
	return msg2_packed(data, option64, header, send_size, descriptors, rcv_size, rcv_name, timeout, priority);
}

static mach_msg_header_t mkhdr(mach_msg_bits_t bits, mach_port_t remote, mach_port_t local, mach_msg_id_t id)
{
	mach_msg_header_t h = { .msgh_bits = bits, .msgh_remote_port = remote, .msgh_local_port = local, .msgh_id = id };
	return h;
}

// A buffer header that disagrees with the registers in every field: the registers must win, so a send that read the
// header from the buffer fails or delivers a different message.
static mach_msg_header_t garbage(void)
{
	mach_msg_header_t h = { .msgh_bits = 0, .msgh_size = 0x7770, .msgh_remote_port = MACH_PORT_NULL,
		.msgh_local_port = 0x1234, .msgh_voucher_port = 0x5678, .msgh_id = 0x0bad };
	return h;
}

static void fill(char* p, size_t n, unsigned seed)
{
	for (size_t i = 0; i < n; i++) {
		p[i] = (char)(seed + i * 7);
	}
}

static int filled(const char* p, size_t n, unsigned seed)
{
	for (size_t i = 0; i < n; i++) {
		if (p[i] != (char)(seed + i * 7)) {
			return 0;
		}
	}
	return 1;
}

int main(void)
{
	msg2_internal = dlsym(RTLD_DEFAULT, "mach_msg2_internal");
	expect(msg2_internal != NULL, "libsystem_kernel exports mach_msg2_internal");

	mach_port_t port = MACH_PORT_NULL, other = MACH_PORT_NULL;
	expect_rc(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port), KERN_SUCCESS, "allocate port");
	expect_rc(mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND), KERN_SUCCESS, "make send right");
	expect_rc(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &other), KERN_SUCCESS, "allocate second port");
	expect_rc(mach_port_insert_right(mach_task_self(), other, other, MACH_MSG_TYPE_MAKE_SEND), KERN_SUCCESS, "make second send right");

	static buffer_t in, out;
	const mach_msg_bits_t copy_send = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
	const mach_msg_size_t hdr = sizeof(mach_msg_header_t);

	// 1. mach_msg2 sends, mach_msg receives
	char body[100];
	fill(body, sizeof body, 3);
	memset(&out, 0, sizeof out);
	memcpy(out.body, body, sizeof body);
	out.header = garbage();
	expect_rc(msg2(&out, MACH_SEND_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0x4d32), hdr + sizeof body, 0, MACH_PORT_NULL, 0, 0),
	    MACH_MSG_SUCCESS,
	    "mach_msg2 send");
	memset(&in, 0, sizeof in);
	expect_rc(mach_msg(&in.header, MACH_RCV_MSG, 0, sizeof in, port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL), MACH_MSG_SUCCESS,
	    "mach_msg receives what mach_msg2 sent");
	expect(in.header.msgh_id == 0x4d32 && in.header.msgh_size == hdr + sizeof body, "mach_msg2 header fields arrive");
	expect(MACH_MSGH_BITS_LOCAL(in.header.msgh_bits) == MACH_MSG_TYPE_PORT_SEND, "mach_msg2 bits arrive");
	expect(filled(in.body, sizeof body, 3), "mach_msg2 body arrives intact");

	// 2. mach_msg sends, mach_msg2 receives the whole message into data
	memset(&out, 0, sizeof out);
	out.header.msgh_bits = copy_send;
	out.header.msgh_size = hdr + sizeof body;
	out.header.msgh_remote_port = port;
	out.header.msgh_id = 0x4d33;
	fill(out.body, sizeof body, 9);
	expect_rc(mach_msg(&out.header, MACH_SEND_MSG, out.header.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL),
	    MACH_MSG_SUCCESS, "mach_msg send");
	memset(&in, 0, sizeof in);
	expect_rc(msg2(&in, MACH_RCV_MSG, mkhdr(0, 0, 0, 0), 0, sizeof in, port, 0, 0), MACH_MSG_SUCCESS,
	    "mach_msg2 receives what mach_msg sent");
	expect(in.header.msgh_id == 0x4d33 && in.header.msgh_size == hdr + sizeof body, "received header is in the data buffer");
	expect(filled(in.body, sizeof body, 9), "received body is intact");

	// 3. one mach_msg2 call that sends and receives, both through mach_msg2, with the message above the stack limit
	static buffer_t rpc;
	fill((char*)&rpc.body, BIG - hdr, 5);
	rpc.header = garbage();
	expect_rc(msg2(&rpc, MACH_SEND_MSG | MACH_RCV_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0x4d34), BIG, sizeof rpc, port, MACH_MSG_TIMEOUT_NONE, 0),
	    MACH_MSG_SUCCESS, "mach_msg2 send and receive in one call");
	expect(rpc.header.msgh_id == 0x4d34 && rpc.header.msgh_size == BIG, "the one-call receive replaced the buffer with the message");
	expect(filled(rpc.body, BIG - hdr, 5), "large body is intact");

	// 4. vector mode with separate send and receive buffers (message of 5000 bytes: heap path)
	static struct {
		mach_msg_header_t header;
		char body[5000];
	} vsend;
	vsend.header = garbage();
	fill(vsend.body, sizeof vsend.body, 11);
	mach_msg_vector_t vec = {
		.msgv_data = (mach_vm_address_t)(uintptr_t)&vsend,
		.msgv_rcv_addr = (mach_vm_address_t)(uintptr_t)&in,
		.msgv_send_size = sizeof vsend,
		.msgv_rcv_size = sizeof in,
	};
	memset(&in, 0, sizeof in);
	expect_rc(msg2(&vec, MACH64_MSG_VECTOR | MACH_SEND_MSG | MACH_RCV_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0x4d35), 1, 1, port,
	              MACH_MSG_TIMEOUT_NONE, 0),
	    MACH_MSG_SUCCESS, "vector mode send and receive");
	expect(in.header.msgh_id == 0x4d35 && in.header.msgh_size == sizeof vsend, "vector receive buffer holds the message");
	expect(filled(in.body, sizeof vsend.body, 11), "vector body is intact");

	// 5. complex message: one port descriptor, count taken from the packed word
	struct {
		mach_msg_base_t base;
		mach_msg_port_descriptor_t desc;
		mach_msg_max_trailer_t trailer;
	} cx, csend;
	memset(&csend, 0, sizeof csend);
	const mach_msg_header_t chdr = mkhdr(copy_send | MACH_MSGH_BITS_COMPLEX, port, MACH_PORT_NULL, 0x4d36);
	csend.base.header = garbage();
	csend.base.header.msgh_bits = MACH_MSGH_BITS_COMPLEX; // the buffer says complex, the registers decide the rest
	csend.base.body.msgh_descriptor_count = 7; // the register says 1
	csend.desc = (mach_msg_port_descriptor_t){ .name = other, .disposition = MACH_MSG_TYPE_COPY_SEND, .type = MACH_MSG_PORT_DESCRIPTOR };
	expect_rc(msg2_packed(&csend, MACH_SEND_MSG, chdr, sizeof(mach_msg_base_t) + sizeof csend.desc, 1, 0, MACH_PORT_NULL, 0, 0),
	    MACH_MSG_SUCCESS, "complex mach_msg2 send");
	memset(&cx, 0, sizeof cx);
	expect_rc(mach_msg(&cx.base.header, MACH_RCV_MSG, 0, sizeof cx, port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL), MACH_MSG_SUCCESS,
	    "mach_msg receives the complex message");
	expect((cx.base.header.msgh_bits & MACH_MSGH_BITS_COMPLEX) && cx.base.body.msgh_descriptor_count == 1,
	    "descriptor count arrives");
	expect(cx.desc.type == MACH_MSG_PORT_DESCRIPTOR && MACH_PORT_VALID(cx.desc.name), "port descriptor arrives");
	mach_port_type_t t = 0;
	expect(mach_port_type(mach_task_self(), cx.desc.name, &t) == KERN_SUCCESS && (t & MACH_PORT_TYPE_SEND), "descriptor holds a send right");

	// 6. a complex message shorter than its base is refused, and an empty queue times out
	expect_rc(msg2_packed(&csend, MACH_SEND_MSG, chdr, hdr, 1, 0, MACH_PORT_NULL, 0, 0), MACH_SEND_MSG_TOO_SMALL,
	    "send_size below the base size is refused");
	expect_rc(msg2_packed(&csend, MACH_SEND_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0), hdr, 1, 0, MACH_PORT_NULL, 0, 0),
	    MACH_SEND_TOO_LARGE, "a simple message with a descriptor count is refused");
	expect_rc(msg2(&in, MACH_SEND_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0), hdr + 2, 0, MACH_PORT_NULL, 0, 0),
	    MACH_SEND_MSG_TOO_SMALL, "a send_size that is not a multiple of 4 is refused");
	expect_rc(msg2_packed(NULL, MACH_SEND_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0), hdr, 0, 0, MACH_PORT_NULL, 0, 0),
	    MACH_SEND_INVALID_DATA, "a send with no data is refused");

	// vector counts as mach_msg2_trap validates them; auxiliary data is refused, never dropped
	mach_msg_vector_t vv[2] = { { .msgv_data = (mach_vm_address_t)(uintptr_t)&vsend, .msgv_send_size = sizeof vsend,
	                                .msgv_rcv_size = sizeof in },
		{ .msgv_data = (mach_vm_address_t)(uintptr_t)&in, .msgv_send_size = 16, .msgv_rcv_size = 16 } };
	const mach_msg_header_t vh = mkhdr(copy_send, port, MACH_PORT_NULL, 0x4d37);
	expect_rc(msg2_packed(vv, MACH64_MSG_VECTOR | MACH_SEND_MSG, vh, 0, 0, 0, MACH_PORT_NULL, 0, 0), MACH_SEND_MSG_TOO_SMALL,
	    "a send vector count of zero is refused");
	expect_rc(msg2_packed(vv, MACH64_MSG_VECTOR | MACH_SEND_MSG, vh, 3, 0, 0, MACH_PORT_NULL, 0, 0), MACH_SEND_INVALID_DATA,
	    "a send vector count above two is refused");
	expect_rc(msg2_packed(vv, MACH64_MSG_VECTOR | MACH_SEND_MSG, vh, 2, 0, 2, MACH_PORT_NULL, 0, 0), MACH_SEND_INVALID_DATA,
	    "an auxiliary send vector with data is refused");
	expect_rc(msg2_packed(vv, MACH64_MSG_VECTOR | MACH_RCV_MSG | MACH_RCV_TIMEOUT, vh, 0, 0, 0, other, 10, 0),
	    MACH_RCV_INVALID_ARGUMENTS, "a receive vector count of zero is refused");
	expect_rc(msg2_packed(vv, MACH64_MSG_VECTOR | MACH_RCV_MSG | MACH_RCV_TIMEOUT, vh, 0, 0, 2, other, 10, 0),
	    MACH_RCV_INVALID_ARGUMENTS, "an auxiliary receive vector is refused");
	expect_rc(msg2(&in, MACH_RCV_MSG | MACH_RCV_TIMEOUT, mkhdr(0, 0, 0, 0), 0, sizeof in, other, 10, 0x1234), MACH_RCV_TIMED_OUT,
	    "an empty queue times out");

	// a bad receive vector does not stop the send (mach_msg2_trap validates it after the send): the message arrives and
	// the call reports the receive error
	static const struct { mach_msg_size_t rcv_count; mach_msg_id_t id; } rcv_cases[] = { { 0, 0x4d38 }, { 2, 0x4d39 } };
	for (int i = 0; i < 2; i++) {
		expect_rc(msg2_packed(vv, MACH64_MSG_VECTOR | MACH_SEND_MSG | MACH_RCV_MSG,
		              mkhdr(copy_send, port, MACH_PORT_NULL, rcv_cases[i].id), 1, 0, rcv_cases[i].rcv_count, port,
		              MACH_MSG_TIMEOUT_NONE, 0),
		    MACH_RCV_INVALID_ARGUMENTS, "a send and receive call with a bad receive vector reports the receive error");
		memset(&in, 0, sizeof in);
		expect_rc(mach_msg(&in.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0, sizeof in, port, 1000, MACH_PORT_NULL), MACH_MSG_SUCCESS,
		    "the message of the failed-receive call was sent");
		expect(in.header.msgh_id == rcv_cases[i].id, "the sent message is the one of that call");
	}

	// descriptors must fit in the message and the size is bounded
	expect_rc(msg2_packed(&csend, MACH_SEND_MSG, chdr, sizeof(mach_msg_base_t), 1, 0, MACH_PORT_NULL, 0, 0),
	    MACH_SEND_MSG_TOO_SMALL, "a complex message without room for its descriptor is refused");
	expect_rc(msg2_packed(&csend, MACH_SEND_MSG, chdr, sizeof(mach_msg_base_t) + sizeof csend.desc, 0xffffffffu, 0, MACH_PORT_NULL, 0, 0),
	    MACH_SEND_MSG_TOO_SMALL, "a huge descriptor count is refused");
	expect_rc(msg2_packed(&csend, MACH_SEND_MSG, mkhdr(copy_send, port, MACH_PORT_NULL, 0), 0x40000000, 0, 0, MACH_PORT_NULL, 0, 0),
	    MACH_SEND_TOO_LARGE, "a message above the body limit is refused without being read");

	// a receive-only call with MACH_RCV_SYNC_WAIT takes the sync-send port from the low half of the remote/local word
	// (msgh_remote_port), not from the priority in the high half of rcv_size_and_priority
	mach_port_t (*get_special)(void) = dlsym(RTLD_DEFAULT, "mig_get_special_reply_port");
	expect(get_special != NULL, "libsystem_kernel exports mig_get_special_reply_port");
	mach_port_t special = get_special();
	expect(MACH_PORT_VALID(special), "the thread has a special reply port");
	const uint64_t sync_rcv = MACH_RCV_MSG | MACH_RCV_TIMEOUT | MACH_RCV_SYNC_WAIT;
	expect_rc(msg2_packed(&in, sync_rcv, mkhdr(0, port, 0, 0), 0, 0, sizeof in, special, 10, 0), MACH_RCV_TIMED_OUT,
	    "the sync port in msgh_remote_port is used");
	expect_rc(msg2_packed(&in, sync_rcv, mkhdr(0, MACH_PORT_NULL, 0, 0), 0, 0, sizeof in, special, 10, port), MACH_RCV_INVALID_NOTIFY,
	    "a valid port in the priority half is not the sync port");

	printf("PASS: mach_msg2_internal sends and receives, plain, one-call, vector and complex messages\n");
	return 0;
}
