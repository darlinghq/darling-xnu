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

// Differential test of the openat2() fast path of vchroot_expand() against the component walk.
// See vchroot-fastpath-equiv.md.

#define TEST 1
#include "../src/libsystem_kernel/emulation/src/linux_premigration/name_equivalence.c"
#define main vchroot_cli_main
#include "../src/libsystem_kernel/emulation/src/linux_premigration/vchroot_userspace.c"
#undef main

#include <sched.h>
#include <sys/mount.h>
#include <sys/time.h>

static int failures, compared, engaged, declined, skipped;
static int verbose;

#define P_PROBE(t) ((snprintf(probe, sizeof(probe), "%s/prefix", t), probe))
static char probe[600];
static char T[512];    // test root
static char P[600];    // the prefix (an overlay mount of lower/ and upper/ when --overlay)
static char L[600], U[600], W[600], O[600];

static void die(const char* what)
{
	printf("FAIL setup: %s: %s\n", what, strerror(errno));
	exit(2);
}

static void mkd(const char* base, const char* rel)
{
	char p[1400];
	snprintf(p, sizeof(p), "%s%s", base, rel);
	if (mkdir(p, 0755) && errno != EEXIST)
		die(p);
}

static void mkf(const char* base, const char* rel)
{
	char p[1400];
	snprintf(p, sizeof(p), "%s%s", base, rel);
	int fd = open(p, O_CREAT | O_WRONLY, 0644);
	if (fd < 0)
		die(p);
	close(fd);
}

static void mkl(const char* base, const char* rel, const char* target)
{
	char p[1400];
	snprintf(p, sizeof(p), "%s%s", base, rel);
	if (symlink(target, p))
		die(p);
}

// The layers the prefix is made of. Lower holds most of the tree, upper adds to it and hides parts of it, as a Darling
// prefix does over the installed system.
static void populate_lower(const char* b)
{
	mkd(b, "/usr");
	mkd(b, "/usr/lib");
	mkf(b, "/usr/lib/libSystem.dylib");
	mkd(b, "/usr/lib/Mixed");
	mkf(b, "/usr/lib/Mixed/MiXeD.txt");
	mkd(b, "/usr/lib/sub");
	mkd(b, "/usr/lib/sub/deep");
	mkd(b, "/usr/lib/sub/deep/er");
	mkf(b, "/usr/lib/sub/deep/er/file");
	mkd(b, "/usr/bin");
	mkf(b, "/usr/bin/tool");
	mkf(b, "/usr/bin/gone");
	mkd(b, "/etc");
	mkf(b, "/etc/hosts");
	mkl(b, "/etc/link", "hosts");
	mkl(b, "/etc/abs", "/etc/hosts");
	mkl(b, "/bin", "usr/bin");
	mkl(b, "/lib", "/usr/lib");
	mkl(b, "/Lib2", "./usr/lib");
	mkl(b, "/chain1", "chain2");
	mkl(b, "/chain2", "chain3");
	mkl(b, "/chain3", "usr/lib/libSystem.dylib");
	mkl(b, "/loopa", "loopb");
	mkl(b, "/loopb", "loopa");
	mkl(b, "/up1", "..");
	mkl(b, "/up3", "../../..");
	mkl(b, "/upetc", "../../../etc/hosts");
	mkl(b, "/deepup", "usr/lib/../../../../etc");
	mkl(b, "/toexit", "/Volumes/SystemRoot/etc");
	mkl(b, "/toproc", "/proc/self/stat");
	mkl(b, "/toprocdir", "/proc");
	mkl(b, "/dangling", "/nonexistent/zzz");
	mkl(b, "/dangrel", "nope");
	mkl(b, "/dirlink", "usr/lib/");
	mkl(b, "/usr/lib/relup", "../bin/tool");
	mkl(b, "/usr/lib/absself", "/usr/lib");
	// chains of links: macOS follows 31 (MAXSYMLINKS is 32), the walk the same, the kernel 40
	for (int i = 40; i > 1; i--)
	{
		char n[32], t[32];
		snprintf(n, sizeof(n), "/l%d", i);
		snprintf(t, sizeof(t), "l%d", i - 1);
		mkl(b, n, t);
	}
	mkl(b, "/l1", "usr/lib/libSystem.dylib");
	mkd(b, "/Volumes");
	mkd(b, "/Volumes/SystemRoot");
	mkd(b, "/shadowed");
	mkf(b, "/shadowed/lower-only");
	mkd(b, "/a");
	mkf(b, "/a/b");
	mkd(b, "/CaseDir");
	mkf(b, "/CaseDir/Inner.TXT");
}

static void populate_outside(void)
{
	mkd(O, "");
	mkf(O, "/a");
	mkd(O, "/dir");
	mkf(O, "/dir/x");
	char t[1400];
	snprintf(t, sizeof(t), "%s/a", O);
	mkl(O, "/abslink", t);
	mkl(O, "/rellink", "a");
	mkl(O, "/dotdot", "../outside/a");
	snprintf(t, sizeof(t), "%s/usr/lib", P);
	mkl(O, "/toprefix", t);
	mkl(O, "/todir", "dir");
	mkl(O, "/hostroot", "/");
}

static void populate_upper_through_merged(void)
{
	char p[1400];
	mkf(P, "/usr/lib/newlib.dylib");
	mkd(P, "/upper-only");
	mkf(P, "/upper-only/x");
	mkf(P, "/shadowed/upper-only");
	// hides a lower entry: a whiteout in the upper layer
	snprintf(p, sizeof(p), "%s/usr/bin/gone", P);
	if (unlink(p))
		die("unlink gone");
	// another spelling of a name that exists below
	mkf(P, "/usr/bin/TOOL2");
	mkl(P, "/uplink", "usr/lib/newlib.dylib");
	// replace a lower directory entry by an upper link
	snprintf(p, sizeof(p), "%s/a/b", P);
	if (unlink(p))
		die("unlink a/b");
	mkl(P, "/a/b", "../usr/lib");
}

static const char* const abs_paths[] = {
	"/", "//", "/usr", "/usr/", "/usr/lib/libSystem.dylib", "/usr//lib/./libSystem.dylib", "/usr/lib/",
	"/usr/lib/libsystem.dylib", "/USR/LIB/LIBSYSTEM.DYLIB", "/usr/lib/mixed/MIXED.TXT", "/usr/lib/Mixed/mixed.txt",
	"/usr/lib/nope", "/nope/x/y", "/usr/lib/nope/x", "/usr/nope/lib/libSystem.dylib", "/usr/lib/libSystem.dylib/x",
	"/usr/lib/libSystem.dylib/", "/usr/lib/sub/deep/er/file", "/usr/lib/SUB/DEEP/ER/FILE", "/usr/lib/sub/deep/er/FILE2",
	"/bin", "/bin/tool", "/bin/Tool", "/bin/TOOL", "/lib", "/lib/", "/lib/libSystem.dylib", "/Lib2/libSystem.dylib",
	"/lib2/libSystem.dylib", "/etc/link", "/etc/abs", "/etc/hosts", "/etc/HOSTS", "/chain1", "/chain1/x",
	"/loopa", "/loopa/x", "/up1", "/up1/usr", "/up3", "/up3/usr", "/upetc", "/deepup", "/deepup/hosts",
	"/toexit", "/toexit/passwd", "/toexit/hosts", "/toproc", "/toprocdir", "/toprocdir/self", "/toprocdir/self/stat",
	"/proc", "/proc/", "/proc/self", "/proc/self/stat", "/proc/self/exe", "/proc/self/cwd", "/proc/self/root",
	"/proc/self/fd", "/proc/self/fd/0", "/proc/1/stat", "/proc/cpuinfo", "/proc/nonexistent", "/PROC/self/stat",
	"/dangling", "/dangling/x", "/dangrel", "/dangrel/x", "/dirlink", "/dirlink/libSystem.dylib", "/..",
	"/../x", "/../usr", "/usr/lib/../bin/tool", "/usr/lib/..", "/usr/./lib/.", "/usr/lib/relup", "/usr/lib/absself",
	"/usr/lib/absself/libSystem.dylib", "/l1", "/l5", "/l12", "/l31", "/l32", "/l35", "/l40", "/Volumes", "/Volumes/", "/Volumes/SystemRoot",
	"/Volumes/SystemRoot/", "/volumes/systemroot", "/volumes/systemroot/", "/Volumes/SystemRoot/Volumes/SystemRoot/",
	"/Volumes/SystemRoot/Volumes/SystemRoot/etc", "/Volumes/SystemRoot/nonexistent", "/Volumes/SystemRoot/etc",
	"/Volumes/SystemRoot/etc/", "/Volumes/SystemRoot/etc/ETC", "/Volumes/SystemRoot/proc/self/stat", "/Volumes/SystemRoot/..",
	"/Volumes/SystemRoot/../etc", "/Volumes/SystemRoot/etc/..", "/Volumes/SystemRootX", "/Volumes/SystemRootX/etc",
	"/Volumes/Other", "/usr/lib/newlib.dylib", "/usr/lib/NEWLIB.dylib", "/upper-only/x", "/Upper-Only/X",
	"/shadowed/lower-only", "/shadowed/upper-only", "/shadowed/Lower-Only", "/usr/bin/gone", "/usr/bin/GONE",
	"/usr/bin/tool", "/usr/bin/TOOL2", "/usr/bin/tool2", "/usr/bin/Tool2/x", "/uplink", "/a/b", "/a/b/libSystem.dylib",
	"/A/B/LIBSYSTEM.DYLIB", "/casedir/inner.txt", "/CASEDIR/INNER.TXT", "/CaseDir/Inner.TXT", "/CaseDir/inner.txt/x",
	"/.nofollow/usr/lib", "/.nofollow/chain1", "/.nofollow/lib/libSystem.dylib", "/.nofollow/nope", "/.nofollow/",
	"/dev", "/dev/", "/dev/null", "/dev/NULL", "/dev/stdin", "/dev/fd/0", "/dev/nonexistent", "/dev/pts", "/dev/shm/x",
	"/usr/lib/\xc3\xa9", "/usr/lib/e\xcc\x81", "", "x", ".", "..", "./usr", "../..",
};

// paths below the outside tree, with the Volumes/SystemRoot prefix added
static const char* const outside_rel[] = {
	"", "/", "/a", "/A", "/a/", "/abslink", "/rellink", "/dotdot", "/toprefix", "/toprefix/libSystem.dylib", "/todir",
	"/todir/x", "/dir/x", "/dir/X", "/dir/../a", "/dir/..", "/nope", "/nope/x", "/hostroot", "/hostroot/etc",
	"/a/x", "/dir/./x", "/dir//x",
};

// relative paths, resolved from a directory inside the prefix and from one outside it
static const char* const rel_paths[] = {
	"libSystem.dylib", "LIBSYSTEM.dylib", "./libSystem.dylib", "../bin/tool", "../../etc/hosts", "../../..", "../../../..",
	"Mixed/mixed.txt", "sub/deep/er/file", "nope", "nope/x", "absself/libSystem.dylib", "relup", ".", "..", "../lib",
	"/usr/lib", "a", "abslink", "rellink", "dotdot", "toprefix", "todir/x", "dir/x", "../prefix/usr", "../..", "../../../etc",
};

static void run_expand(struct vchroot_expand_args* a, const char* path, unsigned flags, int dfd, bool fast, bool* used,
		int* rv_out)
{
	memset(a, 0, sizeof(*a));
	strncpy(a->path, path, sizeof(a->path) - 1);
	a->flags = flags;
	a->dfd = dfd;
	*rv_out = vchroot_expand_ex(a, fast, used);
}

static void compare(const char* label, const char* path, unsigned flags, int dfd)
{
	struct vchroot_expand_args w, f;
	bool wused = false, fused = false;
	int wrv, frv;

	run_expand(&w, path, flags, dfd, false, &wused, &wrv);
	run_expand(&f, path, flags, dfd, true, &fused, &frv);
	compared++;
	if (fused)
		engaged++;
	else
		declined++;

	bool same = wrv == frv && (wrv != 0 || strcmp(w.path, f.path) == 0);
	// Documented difference: the kernel follows up to 40 links in a row where the walk gives up after 31, as macOS does
	if (!same && wrv == -LINUX_ELOOP && frv == 0 && fused && strncmp(path, "/l", 2) == 0)
	{
		skipped++;
		if (verbose)
			printf("note %s %s follow=%u: walk ELOOP after 31 links, fast path resolves to %s\n", label, path, flags, f.path);
		return;
	}
	if (!same)
	{
		failures++;
		printf("FAIL %s \"%s\" follow=%u dfd=%d: walk rv=%d path=%s ; fast rv=%d path=%s%s\n", label, path, flags, dfd,
			wrv, wrv == 0 ? w.path : "-", frv, frv == 0 ? f.path : "-", fused ? "" : " (fast path declined)");
	}
	else if (verbose)
		printf("ok   %s \"%s\" follow=%u -> rv=%d %s [%s]\n", label, path, flags, wrv, wrv == 0 ? w.path : "", fused ? "fast" : "walk");
}

static void run_all(const char* label)
{
	char path[1600];

	for (unsigned i = 0; i < sizeof(abs_paths) / sizeof(abs_paths[0]); i++)
		for (unsigned fl = 0; fl < 2; fl++)
			compare(label, abs_paths[i], fl, -100);

	for (unsigned i = 0; i < sizeof(outside_rel) / sizeof(outside_rel[0]); i++)
	{
		snprintf(path, sizeof(path), "/Volumes/SystemRoot%s%s", O, outside_rel[i]);
		for (unsigned fl = 0; fl < 2; fl++)
			compare(label, path, fl, -100);
	}

	// a path of a little under 4096 bytes, and one over
	{
		static char longp[8192];
		for (int n = 0; n < 2; n++)
		{
			strcpy(longp, "/usr/lib/libSystem.dylib");
			char* p = longp + strlen(longp);
			while (p - longp < (n ? 4200 : 3900))
				p += sprintf(p, "/./");
			strcpy(p, "x");
			for (unsigned fl = 0; fl < 2; fl++)
				compare(label, longp, fl, -100);
		}
		static char names[8192];
		char* p = names;
		for (int c = 0; c < 20; c++)
		{
			*p++ = '/';
			memset(p, 'n', 255);
			p += 255;
		}
		*p = 0;
		compare(label, names, 1, -100);
		strcpy(names, "/usr/lib/");
		memset(names + 9, 'z', 300);
		names[309] = 0;
		compare(label, names, 1, -100);
	}

	// relative paths from the current directory: ones in the prefix, then ones outside it
	const char* dirs[] = { "/usr/lib", "/usr/lib/Mixed", "/", "/CaseDir", "/usr/lib/sub/deep" };
	for (unsigned d = 0; d < sizeof(dirs) / sizeof(dirs[0]); d++)
	{
		char host[1400];
		snprintf(host, sizeof(host), "%s%s", P, dirs[d]);
		if (chdir(host))
			die(host);
		int dfd = open(host, O_RDONLY | O_DIRECTORY);
		for (unsigned i = 0; i < sizeof(rel_paths) / sizeof(rel_paths[0]); i++)
			for (unsigned fl = 0; fl < 2; fl++)
			{
				compare(label, rel_paths[i], fl, -100);
				compare(label, rel_paths[i], fl, dfd);
			}
		close(dfd);
	}
	const char* odirs[] = { "", "/dir" };
	for (unsigned d = 0; d < 2; d++)
	{
		char host[1400];
		snprintf(host, sizeof(host), "%s%s", O, odirs[d]);
		if (chdir(host))
			die(host);
		int dfd = open(host, O_RDONLY | O_DIRECTORY);
		for (unsigned i = 0; i < sizeof(rel_paths) / sizeof(rel_paths[0]); i++)
			for (unsigned fl = 0; fl < 2; fl++)
			{
				compare(label, rel_paths[i], fl, -100);
				compare(label, rel_paths[i], fl, dfd);
			}
		close(dfd);
	}
	// a descriptor that is not open
	compare(label, "x", 1, 1000);
	if (chdir("/"))
		die("chdir /");
}

// /proc/<pid>/<file> reaches the right file in both layouts of the prefix's /proc: the procfs mounted on it (the file
// below the prefix) and, without a PID namespace of its own, a link to the host's /proc (the host's file)
static void check_proc_files(bool proc_is_link, bool proc_is_mount)
{
	static const char* const files[] = { "stat", "cmdline", "status", "statm", "comm", "environ", "auxv" };
	char path[64], want[1700];

	if (!proc_is_link && !proc_is_mount)
		return;
	for (unsigned i = 0; i < sizeof(files) / sizeof(files[0]); i++)
		for (int self = 0; self < 2; self++)
			for (int fast = 0; fast < 2; fast++)
			{
				struct vchroot_expand_args a;
				bool used = false;
				int rv;
				// a process in a new PID namespace sees itself as pid 1 only in the mounted procfs
				if (self)
					snprintf(path, sizeof(path), "/proc/self/%s", files[i]);
				else
					snprintf(path, sizeof(path), "/proc/%d/%s", proc_is_link ? (int) getpid() : 1, files[i]);
				run_expand(&a, path, 1, -100, fast, &used, &rv);
				if (proc_is_link)
					snprintf(want, sizeof(want), "/proc/%d/%s", (int) getpid(), files[i]);
				else
					snprintf(want, sizeof(want), "%s/proc/1/%s", P, files[i]);
				if (rv != 0 || strcmp(a.path, want) != 0)
				{
					failures++;
					printf("FAIL %s (fast path %s): rv=%d path=%s, wanted %s\n", path, fast ? "on" : "off", rv, a.path, want);
				}
			}
}

static void expect_engaged_minimum(void)
{
	// the common cases must really use the fast path, or the comparison proves nothing
	static const char* const must[] = { "/usr/lib/libSystem.dylib", "/etc/hosts", "/lib/libSystem.dylib",
		"/usr/lib/sub/deep/er/file", "/a/b/libSystem.dylib" };
	for (unsigned i = 0; i < sizeof(must) / sizeof(must[0]); i++)
	{
		struct vchroot_expand_args a;
		bool used = false;
		int rv;
		run_expand(&a, must[i], 1, -100, true, &used, &rv);
		if (rv != 0 || !used)
		{
			failures++;
			printf("FAIL %s: fast path not used (rv=%d)\n", must[i], rv);
		}
	}
	char outside[1700];
	struct vchroot_expand_args a;
	bool used = false;
	int rv;
	snprintf(outside, sizeof(outside), "/Volumes/SystemRoot%s/dir/x", O);
	run_expand(&a, outside, 1, -100, true, &used, &rv);
	if (rv != 0 || !used)
	{
		failures++;
		printf("FAIL %s: fast path not used (rv=%d)\n", outside, rv);
	}
}

int main(int argc, char** argv)
{
	int overlay = 0, procmount = 0, procsym = 0, devmount = 0;
	const char* dir = NULL;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--overlay"))
			overlay = 1;
		else if (!strcmp(argv[i], "--proc"))
			procmount = 1;
		else if (!strcmp(argv[i], "--dev"))
			devmount = 1;
		else if (!strcmp(argv[i], "--proc-link"))
			procsym = 1;
		else if (!strcmp(argv[i], "-v"))
			verbose = 1;
		else
			dir = argv[i];
	}
	if (!dir)
	{
		printf("usage: %s [--overlay] [--proc | --proc-link] [--dev] [-v] <empty scratch dir>\n", argv[0]);
		return 2;
	}

	// mounts happen in a private mount namespace (run under unshare -Urm, with -pf for --proc)
	if ((overlay || procmount || devmount) && mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL))
		die("make / private (run under unshare -Urm)");

	if (!realpath(dir, T))
		die(dir);
	if (!access(P_PROBE(T), F_OK))
		die("scratch directory is not empty");
	snprintf(P, sizeof(P), "%s/prefix", T);
	snprintf(L, sizeof(L), "%s/lower", T);
	snprintf(U, sizeof(U), "%s/upper", T);
	snprintf(W, sizeof(W), "%s/work", T);
	snprintf(O, sizeof(O), "%s/outside", T);
	mkd(T, "/lower");
	mkd(T, "/upper");
	mkd(T, "/work");
	mkd(T, "/prefix");
	populate_lower(overlay ? L : P);
	if (procsym)
		mkl(overlay ? L : P, "/proc", "/Volumes/SystemRoot/proc");
	else
		mkd(overlay ? L : P, "/proc");
	mkd(overlay ? L : P, "/dev");
	populate_outside();

	if (overlay)
	{
		char opts[2000];
		snprintf(opts, sizeof(opts), "lowerdir=%s,upperdir=%s,workdir=%s", L, U, W);
		if (mount("overlay", P, "overlay", 0, opts))
			die("mount overlay (run under unshare -Urm)");
	}
	populate_upper_through_merged();
	if (procmount)
	{
		char p[700];
		snprintf(p, sizeof(p), "%s/proc", P);
		if (mount("proc", p, "proc", 0, NULL))
			die("mount proc (run under unshare -Urmpf)");
	}

	if (devmount)
	{
		char p[700];
		snprintf(p, sizeof(p), "%s/dev", P);
		if (mount("/dev", p, NULL, MS_BIND | MS_REC, NULL))
			die("bind mount /dev (run under unshare -Urm)");
	}

	int pfd = open(P, O_RDONLY | O_DIRECTORY);
	if (pfd < 0)
		die("open prefix");
	__darling_vchroot(pfd);
	close(pfd);

	char label[64];
	snprintf(label, sizeof(label), "[%s%s%s%s]", overlay ? "overlay" : "plain", procmount ? "+procfs" : "", procsym ? "+proc-link" : "", devmount ? "+dev" : "");
	run_all(label);
	check_proc_files(procsym, procmount);
	expect_engaged_minimum();

	printf("%s compared %d cases: fast path used for %d, walk for %d; %d documented symlink-depth differences\n", label,
		compared, engaged, declined, skipped);
	if (failures)
	{
		printf("FAILED %d\n", failures);
		return 1;
	}
	printf("PASS vchroot fast path equivalence %s\n", label);
	return 0;
}
