#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_ATTR_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_ATTR_H

#include <stdint.h>

typedef uint32_t attrgroup_t;

struct attrlist {
	uint16_t bitmapcount;
	uint16_t reserved;
	attrgroup_t commonattr;
	attrgroup_t volattr;
	attrgroup_t dirattr;
	attrgroup_t fileattr;
	attrgroup_t forkattr;
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_ATTR_H
