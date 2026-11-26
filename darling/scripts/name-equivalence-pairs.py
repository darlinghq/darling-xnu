#!/usr/bin/env python3
# This file is part of Darling. Copyright (C) 2026 Lubos Dolezel. GPLv3 or later.
# Prints "<expected> <hex a> <hex b>" lines for tests/name-equivalence.c --pairs: Python's unicodedata (NFD) is the
# reference. Usage: scripts/name-equivalence-pairs.py | ./name-equivalence --pairs
import random
import unicodedata

def h(s):
	return s.encode("utf-8").hex() or "-"

def fold(s):
	return "".join(c.lower() if "A" <= c <= "Z" else c for c in s)

def equivalent(a, b):
	return fold(unicodedata.normalize("NFD", a)) == fold(unicodedata.normalize("NFD", b))

chars = [chr(c) for c in range(0x110000) if not 0xD800 <= c <= 0xDFFF]
# Every character against its decomposition and its composition
for c in chars:
	d = unicodedata.normalize("NFD", c)
	if d != c:
		print(1, h(c), h(d))
		print(1, h("x" + c + "y"), h("x" + d + "y"))
	n = unicodedata.normalize("NFC", c)
	if n != c:
		print(1, h(c), h(n))
# Random strings of bases and marks in random order, against their NFC and NFD forms, and against each other
rng = random.Random(1)
marks = [c for c in chars if unicodedata.combining(c)]
bases = [c for c in chars if unicodedata.normalize("NFD", c) != c] + list("aeiouAEIOU")
for _ in range(50000):
	s = "".join(rng.choice(bases) + "".join(rng.choice(marks) for _ in range(rng.randint(0, 3))) for _ in range(rng.randint(1, 3)))
	t = "".join(rng.choice(bases) + "".join(rng.choice(marks) for _ in range(rng.randint(0, 3))) for _ in range(rng.randint(1, 3)))
	print(1, h(s), h(unicodedata.normalize("NFC", s)))
	print(1, h(unicodedata.normalize("NFD", s)), h(unicodedata.normalize("NFKC", s) if equivalent(s, unicodedata.normalize("NFKC", s)) else s))
	print(int(equivalent(s, t)), h(s), h(t))
