#ifndef DARLING_CONVERSION_XATTR_GETATTRLIST
#define DARLING_CONVERSION_XATTR_GETATTRLIST

#include <stdint.h>

typedef uint32_t xnu_attrgroup_t;

struct xnu_attrlist {
	uint16_t     bitmapcount;
	uint16_t   reserved;
	xnu_attrgroup_t commonattr;
	xnu_attrgroup_t volattr;
	xnu_attrgroup_t dirattr;
	xnu_attrgroup_t fileattr;
	xnu_attrgroup_t forkattr;
};

#endif // DARLING_CONVERSION_XATTR_GETATTRLIST
