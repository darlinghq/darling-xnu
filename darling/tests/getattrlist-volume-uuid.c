// Copyright (C) 2026 Lubos Dolezel
// getattrlist(2) answers ATTR_VOL_UUID for a volume request (sys/attr.h): a 16-byte uuid_t after the length word, in
// attribute bit order (after ATTR_VOL_CAPABILITIES, before ATTR_VOL_ATTRIBUTES), the same value for every path on the
// volume and for the descriptor form, and with ATTR_CMN_RETURNED_ATTRS the bit is reported in the returned set.
// The program also passes on macOS, where the same calls succeed.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/attr.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d, errno %d)\n", #c, __LINE__, errno); failures++; } } while (0)

static int get_uuid(const char* path, unsigned char* uuid)
{
	struct { uint32_t length; unsigned char uuid[16]; } __attribute__((packed)) reply;
	struct attrlist list;
	memset(&list, 0, sizeof list);
	list.bitmapcount = ATTR_BIT_MAP_COUNT;
	list.volattr = ATTR_VOL_INFO | ATTR_VOL_UUID;
	memset(&reply, 0, sizeof reply);
	if (getattrlist(path, &list, &reply, sizeof reply, 0) != 0)
		return -1;
	if (reply.length != sizeof reply)
		return -2;
	memcpy(uuid, reply.uuid, 16);
	return 0;
}

int main(void)
{
	unsigned char root[16], again[16], inside[16], zero[16] = { 0 };
	CHECK(get_uuid("/", root) == 0);
	CHECK(memcmp(root, zero, 16) != 0);
	CHECK(get_uuid("/", again) == 0 && memcmp(root, again, 16) == 0);

	// a path inside the volume gives the volume's value (macOS 10.14 refuses volume attributes for a path that is not
	// the volume root with EINVAL; later versions and Darling answer)
	if (get_uuid("/usr", inside) == 0)
		CHECK(memcmp(root, inside, 16) == 0);
	else
		CHECK(errno == EINVAL);

	// the descriptor form
	{
		struct { uint32_t length; unsigned char uuid[16]; } __attribute__((packed)) reply;
		struct attrlist list;
		memset(&list, 0, sizeof list);
		list.bitmapcount = ATTR_BIT_MAP_COUNT;
		list.volattr = ATTR_VOL_INFO | ATTR_VOL_UUID;
		int fd = open("/", O_RDONLY);
		CHECK(fd >= 0);
		CHECK(fgetattrlist(fd, &list, &reply, sizeof reply, 0) == 0);
		CHECK(memcmp(reply.uuid, root, 16) == 0);
		close(fd);
	}

	// ordering: CAPABILITIES (32 bytes), UUID (16), ATTRIBUTES (40)
	{
		struct {
			uint32_t length;
			vol_capabilities_attr_t capabilities;
			unsigned char uuid[16];
			vol_attributes_attr_t attributes;
		} __attribute__((packed)) reply;
		struct attrlist list;
		memset(&list, 0, sizeof list);
		list.bitmapcount = ATTR_BIT_MAP_COUNT;
		list.volattr = ATTR_VOL_INFO | ATTR_VOL_CAPABILITIES | ATTR_VOL_UUID | ATTR_VOL_ATTRIBUTES;
		memset(&reply, 0, sizeof reply);
		CHECK(getattrlist("/", &list, &reply, sizeof reply, 0) == 0);
		CHECK(reply.length == sizeof reply);
		CHECK(memcmp(reply.uuid, root, 16) == 0);
	}

	// with ATTR_CMN_RETURNED_ATTRS the returned set names ATTR_VOL_UUID
	{
		struct {
			uint32_t length;
			attribute_set_t returned;
			unsigned char uuid[16];
		} __attribute__((packed)) reply;
		struct attrlist list;
		memset(&list, 0, sizeof list);
		list.bitmapcount = ATTR_BIT_MAP_COUNT;
		list.commonattr = ATTR_CMN_RETURNED_ATTRS;
		list.volattr = ATTR_VOL_INFO | ATTR_VOL_UUID;
		memset(&reply, 0, sizeof reply);
		CHECK(getattrlist("/", &list, &reply, sizeof reply, FSOPT_PACK_INVAL_ATTRS) == 0);
		CHECK(reply.length == sizeof reply);
		CHECK(reply.returned.volattr & ATTR_VOL_UUID);
		CHECK(memcmp(reply.uuid, root, 16) == 0);
	}

	// a different volume has a different identity: /dev is its own file system on macOS and in the guest
	{
		struct stat a, b;
		unsigned char other[16];
		if (stat("/", &a) == 0 && stat("/dev", &b) == 0 && a.st_dev != b.st_dev && get_uuid("/dev", other) == 0)
			CHECK(memcmp(root, other, 16) != 0);
	}

	if (failures)
		return 1;
	puts("PASS: getattrlist answers ATTR_VOL_UUID");
	return 0;
}
