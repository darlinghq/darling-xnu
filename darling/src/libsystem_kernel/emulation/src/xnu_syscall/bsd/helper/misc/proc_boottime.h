#ifndef DARLING_PROC_BOOTTIME_H
#define DARLING_PROC_BOOTTIME_H

// Parse the value of a Linux /proc/stat btime record without integer wrapping.
static inline int parse_proc_boottime(const char *text, unsigned long *seconds)
{
	unsigned long value = 0, limit = ~0UL >> 1;
	if (*text < '0' || *text > '9')
		return 0;
	while (*text >= '0' && *text <= '9')
	{
		unsigned digit = *text++ - '0';
		if (value > (limit - digit) / 10)
			return 0;
		value = value * 10 + digit;
	}
	while (*text == ' ' || *text == '\t')
		text++;
	if (*text || !value)
		return 0;
	*seconds = value;
	return 1;
}

#endif
