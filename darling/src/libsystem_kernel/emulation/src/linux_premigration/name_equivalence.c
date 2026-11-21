/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

// File name comparison for path lookup. APFS, the file system macOS names live on, treats names that are canonically
// equivalent in Unicode as the same name (Apple's "Apple File System Guide", FAQ), and Foundation hands the kernel
// decomposed names (-[NSString fileSystemRepresentation]), so programs rely on a decomposed name finding a precomposed
// one. Linux file systems compare bytes. Names are compared here by their canonical decompositions (Unicode Standard
// Annex #15: full canonical decomposition, then canonical ordering of combining marks), with ASCII case folded as the
// lookup did before.

#include "name_equivalence.h"
#include "unicode_decomposition.h"

// The longest segment compared, in code points after decomposition: a starter and 31 combining marks. Unicode's
// Stream-Safe Text Format (UAX #15) allows 30 marks in a row; a longer run makes the names compare by bytes only.
#define SEGMENT_MAX 32

#define HANGUL_S_BASE 0xAC00
#define HANGUL_L_BASE 0x1100
#define HANGUL_V_BASE 0x1161
#define HANGUL_T_BASE 0x11A7
#define HANGUL_T_COUNT 28
#define HANGUL_N_COUNT (21 * HANGUL_T_COUNT)
#define HANGUL_S_COUNT (19 * HANGUL_N_COUNT)

static unsigned int ascii_lower(unsigned int c)
{
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

static bool ascii_case_equal(const unsigned char* a, const unsigned char* b)
{
	while (*a && ascii_lower(*a) == ascii_lower(*b))
	{
		a++;
		b++;
	}
	return *a == *b;
}

static bool has_non_ascii(const unsigned char* s)
{
	for (; *s; s++)
	{
		if (*s >= 0x80)
			return true;
	}
	return false;
}

// Decodes one code point of strict UTF-8 (no overlong forms, surrogates or values past U+10FFFF).
// Returns the number of bytes used, or 0 if the bytes at s are not valid UTF-8.
static int utf8_decode(const unsigned char* s, unsigned int* cp)
{
	unsigned int c = s[0];
	int length, i;
	unsigned char low = 0x80, high = 0xBF; // the range of the second byte

	if (c < 0x80)
	{
		*cp = c;
		return 1;
	}
	else if (c >= 0xC2 && c <= 0xDF)
	{
		length = 2;
		c &= 0x1F;
	}
	else if (c >= 0xE0 && c <= 0xEF)
	{
		length = 3;
		c &= 0x0F;
		if (s[0] == 0xE0)
			low = 0xA0;
		else if (s[0] == 0xED)
			high = 0x9F;
	}
	else if (c >= 0xF0 && c <= 0xF4)
	{
		length = 4;
		c &= 0x07;
		if (s[0] == 0xF0)
			low = 0x90;
		else if (s[0] == 0xF4)
			high = 0x8F;
	}
	else
		return 0;

	for (i = 1; i < length; i++)
	{
		if (s[i] < (i == 1 ? low : 0x80) || s[i] > (i == 1 ? high : 0xBF))
			return 0;
		c = (c << 6) | (s[i] & 0x3F);
	}
	*cp = c;
	return length;
}

static int combining_class(unsigned int cp)
{
	int lo = 0, hi = sizeof(unicode_combining_classes) / sizeof(unicode_combining_classes[0]) - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		unsigned int entry_cp = unicode_combining_classes[mid] >> 8;
		if (entry_cp == cp)
			return unicode_combining_classes[mid] & 0xFF;
		if (entry_cp < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return 0;
}

static int find_decomposition(unsigned int cp)
{
	int lo = 0, hi = sizeof(unicode_decompositions) / sizeof(unicode_decompositions[0]) - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		if (unicode_decompositions[mid].cp == cp)
			return mid;
		if (unicode_decompositions[mid].cp < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return -1;
}

// Reads a name's canonical decomposition one code point at a time.
struct cursor
{
	const unsigned char* s;
	unsigned int pending[UNICODE_DECOMPOSITION_MAX];
	int npending, ipending;
};

// The next code point of the decomposition without consuming it: 0 at the end, -1 if the name is not valid UTF-8.
static long cursor_peek(struct cursor* c)
{
	if (c->ipending == c->npending)
	{
		unsigned int cp;
		int used, d, i;

		if (*c->s == 0)
			return 0;
		used = utf8_decode(c->s, &cp);
		if (used == 0)
			return -1;
		c->s += used;
		c->ipending = c->npending = 0;

		if (cp >= HANGUL_S_BASE && cp < HANGUL_S_BASE + HANGUL_S_COUNT)
		{
			unsigned int index = cp - HANGUL_S_BASE;
			c->pending[c->npending++] = HANGUL_L_BASE + index / HANGUL_N_COUNT;
			c->pending[c->npending++] = HANGUL_V_BASE + (index % HANGUL_N_COUNT) / HANGUL_T_COUNT;
			if (index % HANGUL_T_COUNT != 0)
				c->pending[c->npending++] = HANGUL_T_BASE + index % HANGUL_T_COUNT;
		}
		else if ((d = find_decomposition(cp)) >= 0)
		{
			for (i = 0; i < unicode_decompositions[d].length; i++)
				c->pending[c->npending++] = unicode_decomposition_pool[unicode_decompositions[d].offset + i];
		}
		else
			c->pending[c->npending++] = cp;
	}
	return c->pending[c->ipending];
}

// Writes the next segment of the decomposition (a code point and the combining marks after it) to seg, in canonical
// order. Returns its length: 0 at the end, -1 if the name is not valid UTF-8 or the segment is longer than SEGMENT_MAX.
static int next_segment(struct cursor* c, unsigned int* seg)
{
	int n = 0, i;
	long cp;

	while ((cp = cursor_peek(c)) > 0)
	{
		if (n > 0 && combining_class(cp) == 0)
			break;
		if (n == SEGMENT_MAX)
			return -1;
		seg[n++] = cp;
		c->ipending++;
	}
	if (cp < 0)
		return -1;

	// Canonical ordering: sort the marks by combining class, keeping the order of marks of the same class
	for (i = 1; i < n; i++)
	{
		int ccc = combining_class(seg[i]);
		unsigned int mark = seg[i];
		int j = i;

		if (ccc == 0)
			continue;
		while (j > 0 && combining_class(seg[j - 1]) > ccc)
		{
			seg[j] = seg[j - 1];
			j--;
		}
		seg[j] = mark;
	}
	return n;
}

bool darling_names_equivalent(const char* a, const char* b)
{
	struct cursor ca = { (const unsigned char*) a, { 0 }, 0, 0 };
	struct cursor cb = { (const unsigned char*) b, { 0 }, 0, 0 };
	unsigned int sa[SEGMENT_MAX], sb[SEGMENT_MAX];

	if (ascii_case_equal(ca.s, cb.s))
		return true;
	if (!has_non_ascii(ca.s) && !has_non_ascii(cb.s))
		return false;

	while (true)
	{
		int na = next_segment(&ca, sa);
		int nb = next_segment(&cb, sb);
		int i;

		if (na < 0 || nb < 0 || na != nb)
			return false;
		if (na == 0)
			return true;
		for (i = 0; i < na; i++)
		{
			if (ascii_lower(sa[i]) != ascii_lower(sb[i]))
				return false;
		}
	}
}
