// Copyright (C) 2026 Lubos Dolezel
// getattrlist(ATTR_VOL_NAME) names the volume that holds an object, whichever object on the volume is asked about,
// with or without ATTR_CMN_RETURNED_ATTRS, and ATTR_VOL_ATTRIBUTES reports it as supported; statfs and getfsstat
// report the volume's file system kind. "/" has a name too. statfs and getfsstat give the volume's owner as the
// user who mounted it (the caller, who attached the image), and root as the owner of "/".
//
// Usage: volume-name-kind <mount point> <expected volume name> <expected kind> [<a file on that volume>]
// The program also passes on macOS (e.g. with a mounted disk image: /Volumes/X X hfs).
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/attr.h>
#include <sys/mount.h>
#include <unistd.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d, errno %d)\n", #c, __LINE__, errno); failures++; } } while (0)

struct name_reply {
	uint32_t length;
	attrreference_t name;
	char data[1024];
} __attribute__((packed));

struct returned_name_reply {
	uint32_t length;
	attribute_set_t returned;
	attrreference_t name;
	char data[1024];
} __attribute__((packed));

struct attributes_reply {
	uint32_t length;
	vol_attributes_attr_t attributes;
} __attribute__((packed));

// the volume name getattrlist gives for path, or NULL
static const char* volume_name(const char* path, int returned, char* out, size_t size)
{
	struct attrlist list = { .bitmapcount = ATTR_BIT_MAP_COUNT, .volattr = ATTR_VOL_INFO | ATTR_VOL_NAME };
	if (returned) {
		struct returned_name_reply reply;
		list.commonattr = ATTR_CMN_RETURNED_ATTRS;
		if (getattrlist(path, &list, &reply, sizeof(reply), 0) != 0)
			return NULL;
		if (!(reply.returned.volattr & ATTR_VOL_NAME))
			return NULL;
		snprintf(out, size, "%s", (const char*)&reply.name + reply.name.attr_dataoffset);
	} else {
		struct name_reply reply;
		if (getattrlist(path, &list, &reply, sizeof(reply), 0) != 0)
			return NULL;
		if (reply.name.attr_length == 0)
			return NULL;
		snprintf(out, size, "%s", (const char*)&reply.name + reply.name.attr_dataoffset);
	}
	return out;
}

int main(int argc, char** argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s <mount point> <volume name> <kind> [<file on it>]\n", argv[0]);
		return 2;
	}
	const char* mp = argv[1];
	char name[1024];

	const char* got = volume_name(mp, 0, name, sizeof(name));
	CHECK(got != NULL && strcmp(got, argv[2]) == 0);
	if (got) printf("name of %s: %s\n", mp, got);

	got = volume_name(mp, 1, name, sizeof(name));
	CHECK(got != NULL && strcmp(got, argv[2]) == 0);

	if (argc > 4) {
		got = volume_name(argv[4], 0, name, sizeof(name));
		CHECK(got != NULL && strcmp(got, argv[2]) == 0);
	}

	got = volume_name("/", 0, name, sizeof(name));
	CHECK(got != NULL && got[0] != 0);

	struct attrlist list = { .bitmapcount = ATTR_BIT_MAP_COUNT, .volattr = ATTR_VOL_INFO | ATTR_VOL_ATTRIBUTES };
	struct attributes_reply attributes;
	CHECK(getattrlist(mp, &list, &attributes, sizeof(attributes), 0) == 0);
	CHECK(attributes.attributes.validattr.volattr & ATTR_VOL_NAME);

	struct statfs sfs;
	CHECK(statfs(mp, &sfs) == 0);
	CHECK(strcmp(sfs.f_fstypename, argv[3]) == 0);
	CHECK(sfs.f_owner == geteuid());
	printf("kind of %s: %s, owner %u\n", mp, sfs.f_fstypename, (unsigned) sfs.f_owner);

	struct statfs root;
	CHECK(statfs("/", &root) == 0);
	CHECK(root.f_owner == 0);

	struct statfs all[64];
	int count = getfsstat(all, sizeof(all), MNT_NOWAIT);
	int found = 0;
	for (int i = 0; i < count; i++) {
		if (strcmp(all[i].f_mntonname, mp) == 0) {
			found = 1;
			CHECK(strcmp(all[i].f_fstypename, argv[3]) == 0);
			CHECK(all[i].f_owner == geteuid());
		}
	}
	CHECK(found);

	printf("%s\n", failures ? "FAILED" : "PASSED");
	return failures != 0;
}
