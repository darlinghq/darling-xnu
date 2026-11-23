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

// Tests darling_names_equivalent(), the name comparison path lookup falls back to when a name is not found.
// See name-equivalence.md. With "--pairs", it also reads "<expected 0/1> <hex a> <hex b>" lines from stdin.

#include "../src/libsystem_kernel/emulation/src/linux_premigration/name_equivalence.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int failures;

static void expect(bool want, const char* a, const char* b, const char* what)
{
	bool got = darling_names_equivalent(a, b);
	bool back = darling_names_equivalent(b, a);
	if (got != want || back != want)
	{
		printf("FAIL %s: got %d/%d, want %d\n", what, got, back, want);
		failures++;
	}
}

static int unhex(const char* hex, char* out, size_t max)
{
	size_t n = 0;
	if (strcmp(hex, "-") == 0)
	{
		out[0] = 0;
		return 0;
	}
	for (; hex[0] && hex[1] && n + 1 < max; hex += 2)
	{
		unsigned v;
		if (sscanf(hex, "%2x", &v) != 1)
			return -1;
		out[n++] = (char) v;
	}
	out[n] = 0;
	return 0;
}

int main(int argc, char** argv)
{
	expect(true, "readme.txt", "readme.txt", "identical ASCII");
	expect(true, "ReadMe.TXT", "readme.txt", "ASCII case");
	expect(false, "readme.txt", "readme.tx", "a prefix is not equivalent");
	expect(false, "a", "b", "different ASCII");
	expect(true, "", "", "empty names");

	expect(true, "\xc3\xbc.txt", "u\xcc\x88.txt", "u-umlaut precomposed and decomposed");
	expect(true, "\xc3\x9c.txt", "u\xcc\x88.txt", "U-umlaut and u + diaeresis: ASCII case of the base letter");
	expect(false, "\xc3\xbc.txt", "o\xcc\x88.txt", "u-umlaut is not o-umlaut");
	expect(false, "\xc3\xbc.txt", "u.txt", "the mark counts");
	expect(false, "\xc3\xbc", "\xc3\x9f", "u-umlaut is not sharp s");

	// Case is folded for ASCII letters only, after decomposition (so U-umlaut matches u-umlaut, above). Letters
	// without a decomposition outside ASCII keep their case: Greek alpha (U+03B1) is not capital alpha (U+0391).
	expect(false, "\xce\xb1", "\xce\x91", "non-ASCII case is not folded");

	// U+1EC7 (e with circumflex and dot below) = e + U+0323 (class 220) + U+0302 (class 230), in either order of marks
	expect(true, "\xe1\xbb\x87", "e\xcc\xa3\xcc\x82", "precomposed and canonically ordered marks");
	expect(true, "\xe1\xbb\x87", "e\xcc\x82\xcc\xa3", "precomposed and marks out of canonical order");
	expect(true, "e\xcc\xa3\xcc\x82", "e\xcc\x82\xcc\xa3", "marks of different classes reorder");
	// U+0301 and U+0302 are both class 230: their order is significant
	expect(false, "e\xcc\x81\xcc\x82", "e\xcc\x82\xcc\x81", "marks of the same class do not reorder");

	// Singletons: U+212B ANGSTROM SIGN is canonically U+00C5, which is A + U+030A
	expect(true, "\xe2\x84\xab", "\xc3\x85", "Angstrom sign and A-ring");
	expect(true, "\xe2\x84\xab", "A\xcc\x8a", "Angstrom sign and A + ring");
	// U+2126 OHM SIGN is canonically U+03A9 GREEK CAPITAL OMEGA
	expect(true, "\xe2\x84\xa6", "\xce\xa9", "Ohm sign and Omega");

	// Hangul: U+AC00 is U+1100 U+1161; U+AC01 is U+1100 U+1161 U+11A8
	expect(true, "\xea\xb0\x80", "\xe1\x84\x80\xe1\x85\xa1", "Hangul LV syllable");
	expect(true, "\xea\xb0\x81", "\xe1\x84\x80\xe1\x85\xa1\xe1\x86\xa8", "Hangul LVT syllable");
	expect(false, "\xea\xb0\x80", "\xea\xb0\x81", "different Hangul syllables");

	// Compatibility decompositions are not canonical: U+FB01 (fi ligature) is not "fi"
	expect(false, "\xef\xac\x81", "fi", "compatibility ligature");

	// A character outside the BMP with a canonical decomposition: U+1D15E MUSICAL SYMBOL HALF NOTE = U+1D157 U+1D165
	expect(true, "\xf0\x9d\x85\x9e", "\xf0\x9d\x85\x97\xf0\x9d\x85\xa5", "supplementary-plane decomposition");

	// Invalid UTF-8 compares by bytes, ignoring ASCII case
	expect(true, "\xff.TXT", "\xff.txt", "invalid UTF-8, ASCII case");
	expect(false, "\xff.txt", "\xfe.txt", "invalid UTF-8, different bytes");
	expect(false, "u\xcc\x88\xff", "\xc3\xbc\xff", "invalid UTF-8 is not normalized");
	expect(false, "\xc0\xbc", "<", "an overlong encoding is invalid, not '<'");
	expect(false, "\xed\xa0\x80", "\xef\xbf\xbd", "an encoded surrogate is invalid");

	// Longest names: 255 bytes of u-umlaut (127 characters + 'x') against its decomposed form (382 bytes)
	{
		char a[300], b[400];
		size_t i, na = 0, nb = 0;
		for (i = 0; i < 127; i++)
		{
			a[na++] = '\xc3'; a[na++] = '\xbc';
			b[nb++] = 'u'; b[nb++] = '\xcc'; b[nb++] = '\x88';
		}
		a[na++] = 'x'; b[nb++] = 'x';
		a[na] = b[nb] = 0;
		expect(true, a, b, "a 255-byte name");
		b[nb - 1] = 'y';
		expect(false, a, b, "a 255-byte name, last character different");
	}

	// More than 31 marks after a starter (past Unicode's Stream-Safe Text Format limit of 30) compare by their bytes only
	{
		char a[200], b[200];
		size_t i, na = 0, nb = 0;
		a[na++] = '\xc3'; a[na++] = '\xbc';
		b[nb++] = 'u'; b[nb++] = '\xcc'; b[nb++] = '\x88';
		for (i = 0; i < 29; i++)
		{
			a[na++] = '\xcc'; a[na++] = '\x88';
			b[nb++] = '\xcc'; b[nb++] = '\x88';
		}
		a[na] = b[nb] = 0;
		expect(true, a, b, "30 marks in a row");
		a[na++] = '\xcc'; a[na++] = '\x88'; a[na++] = '\xcc'; a[na++] = '\x88';
		b[nb++] = '\xcc'; b[nb++] = '\x88'; b[nb++] = '\xcc'; b[nb++] = '\x88';
		a[na] = b[nb] = 0;
		expect(false, a, b, "32 marks in a row");
		expect(true, a, a, "32 marks in a row, same bytes");
	}

	if (argc > 1 && strcmp(argv[1], "--pairs") == 0)
	{
		char line[4096], ha[2048], hb[2048], a[1024], b[1024];
		int want;
		unsigned long n = 0;
		while (fgets(line, sizeof(line), stdin))
		{
			if (sscanf(line, "%d %2047s %2047s", &want, ha, hb) != 3 || unhex(ha, a, sizeof(a)) || unhex(hb, b, sizeof(b)))
			{
				printf("FAIL bad input line: %s", line);
				failures++;
				continue;
			}
			bool got = darling_names_equivalent(a, b);
			if (got != (bool) want)
			{
				printf("FAIL pair %s %s: got %d, want %d\n", ha, hb, got, want);
				if (++failures > 20)
					break;
			}
			n++;
		}
		printf("checked %lu pairs\n", n);
	}

	if (failures == 0)
		printf("PASS name equivalence\n");
	return failures != 0;
}
