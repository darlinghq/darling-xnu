#ifndef DARLING_ATTRIBUTE_MOUNTPOINT_H
#define DARLING_ATTRIBUTE_MOUNTPOINT_H

// Parse one complete /proc/self/mountinfo record. The caller supplies integer,
// size and errno definitions. Return 1 for a match, 0 for another mount, or a
// negative errno. On failure leave the destination untouched.
static int attribute_mountpoint(const char* line, __SIZE_TYPE__ length,
	uint64_t mount_id, char* output, __SIZE_TYPE__ capacity)
{
	__SIZE_TYPE__ pos = 0;
	uint64_t id = 0;
	if (!length || line[0] < '0' || line[0] > '9')
		return -EINVAL;
	while (pos < length && line[pos] >= '0' && line[pos] <= '9') {
		unsigned digit = line[pos++] - '0';
		if (id > (~(uint64_t)0 - digit) / 10)
			return -EINVAL;
		id = id * 10 + digit;
	}
	if (pos == length || line[pos] != ' ')
		return -EINVAL;
	if (id != mount_id)
		return 0;
	// Skip parent ID, major:minor device and filesystem root fields.
	for (unsigned field = 0; field < 3; ++field) {
		++pos;
		__SIZE_TYPE__ start = pos;
		while (pos < length && line[pos] != ' ' && line[pos] != '\n')
			++pos;
		if (pos == start || pos == length || line[pos] != ' ')
			return -EINVAL;
	}
	__SIZE_TYPE__ start = ++pos;
	while (pos < length && line[pos] != ' ' && line[pos] != '\n')
		++pos;
	if (pos == start || pos == length || line[pos] != ' ' || line[start] != '/')
		return -EINVAL;
	__SIZE_TYPE__ end = pos;
	// First validate and size; only the second pass writes output.
	for (unsigned pass = 0; pass < 2; ++pass) {
		__SIZE_TYPE__ used = 0;
		for (pos = start; pos < end; ++pos) {
			unsigned value = (unsigned char)line[pos];
			if (value == 0 || value == '\t')
				return -EINVAL;
			if (value == '\\') {
				if (end - pos < 4)
					return -EINVAL;
				value = 0;
				for (unsigned digit = 1; digit <= 3; ++digit) {
					if (line[pos + digit] < '0' || line[pos + digit] > '7')
						return -EINVAL;
					value = value * 8 + (line[pos + digit] - '0');
				}
				if (value != ' ' && value != '\t' && value != '\n' && value != '\\')
					return -EINVAL;
				pos += 3;
			}
			if (pass)
				output[used] = value;
			++used;
		}
		if (used >= capacity)
			return -ENAMETOOLONG;
		if (pass)
			output[used] = '\0';
	}
	return 1;
}
// Read complete records without assuming a read boundary is a line boundary.
// The callback returns bytes or negative errno; caller owns/ closes its handle.
static int attribute_find_mountpoint(long (*read_bytes)(void*, char*, __SIZE_TYPE__),
	void* context, uint64_t mount_id, char* output, __SIZE_TYPE__ capacity,
	char* record, __SIZE_TYPE__ record_capacity)
{
	char chunk[512];
	__SIZE_TYPE__ used = 0;
	for (;;) {
		long count = read_bytes(context, chunk, sizeof(chunk));
		if (count == -EINTR)
			continue;
		if (count < 0)
			return (int)count;
		if (count == 0)
			return used ? -EINVAL : -ENOENT;
		if ((__SIZE_TYPE__)count > sizeof(chunk))
			return -EIO;
		for (long i = 0; i < count; ++i) {
			if (used == record_capacity)
				return -ENAMETOOLONG;
			record[used++] = chunk[i];
			if (chunk[i] == '\n') {
				int result = attribute_mountpoint(record, used, mount_id, output, capacity);
				if (result)
					return result;
				used = 0;
			}
		}
	}
}
#endif
