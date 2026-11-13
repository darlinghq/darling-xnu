// getiopolicy_np() must return what setiopolicy_np() stored, in the scope it was stored for, and refuse what
// xnu refuses (bsd/kern/kern_resource.c iopolicysys): unknown types, scopes and policies.
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

// private in the SDK header (xnu bsd/sys/resource.h under PRIVATE)
#define IOPOL_TYPE_VFS_HFS_CASE_SENSITIVITY 1
#define IOPOL_VFS_HFS_CASE_SENSITIVITY_DEFAULT 0
#define IOPOL_VFS_HFS_CASE_SENSITIVITY_FORCE_CASE_SENSITIVE 1

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s (errno %d)\n", what, errno);
		exit(1);
	}
}

static int get(int type, int scope)
{
	errno = 0;
	return getiopolicy_np(type, scope);
}

static int set(int type, int scope, int policy)
{
	errno = 0;
	return setiopolicy_np(type, scope, policy);
}

static int thread_sees_default;

static void* other_thread(void* unused)
{
	// the policy another thread set for itself is not seen here
	thread_sees_default = get(IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES, IOPOL_SCOPE_THREAD) == IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT &&
	                      get(IOPOL_TYPE_VFS_ATIME_UPDATES, IOPOL_SCOPE_THREAD) == IOPOL_ATIME_UPDATES_DEFAULT;
	return NULL;
}

int main(void)
{
	const int materialize = IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES;

	// process scope: the default is not to materialize; a default request means that too
	expect(get(materialize, IOPOL_SCOPE_PROCESS) == IOPOL_MATERIALIZE_DATALESS_FILES_OFF, "process default is OFF");
	expect(set(materialize, IOPOL_SCOPE_PROCESS, IOPOL_MATERIALIZE_DATALESS_FILES_ON) == 0, "set process ON");
	expect(get(materialize, IOPOL_SCOPE_PROCESS) == IOPOL_MATERIALIZE_DATALESS_FILES_ON, "get process ON");
	expect(set(materialize, IOPOL_SCOPE_PROCESS, IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT) == 0, "set process DEFAULT");
	expect(get(materialize, IOPOL_SCOPE_PROCESS) == IOPOL_MATERIALIZE_DATALESS_FILES_OFF, "process DEFAULT reads back as OFF");

	// thread scope: stored per thread and independent of the process scope
	expect(get(materialize, IOPOL_SCOPE_THREAD) == IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT, "thread default inherits");
	expect(set(materialize, IOPOL_SCOPE_THREAD, IOPOL_MATERIALIZE_DATALESS_FILES_OFF) == 0, "set thread OFF");
	expect(get(materialize, IOPOL_SCOPE_THREAD) == IOPOL_MATERIALIZE_DATALESS_FILES_OFF, "get thread OFF");
	expect(set(IOPOL_TYPE_VFS_ATIME_UPDATES, IOPOL_SCOPE_THREAD, IOPOL_ATIME_UPDATES_OFF) == 0, "set thread atime OFF");
	pthread_t thread;
	expect(pthread_create(&thread, NULL, other_thread, NULL) == 0, "create thread");
	pthread_join(thread, NULL);
	expect(thread_sees_default, "another thread does not see this thread's policies");
	expect(get(IOPOL_TYPE_VFS_ATIME_UPDATES, IOPOL_SCOPE_THREAD) == IOPOL_ATIME_UPDATES_OFF, "atime thread policy kept");
	expect(get(IOPOL_TYPE_VFS_ATIME_UPDATES, IOPOL_SCOPE_PROCESS) == IOPOL_ATIME_UPDATES_DEFAULT, "atime process policy untouched");
	expect(set(materialize, IOPOL_SCOPE_THREAD, IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT) == 0, "set thread DEFAULT");
	expect(get(materialize, IOPOL_SCOPE_THREAD) == IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT, "thread DEFAULT");

	// the other types
	expect(set(IOPOL_TYPE_VFS_TRIGGER_RESOLVE, IOPOL_SCOPE_PROCESS, IOPOL_VFS_TRIGGER_RESOLVE_OFF) == 0, "set trigger resolve OFF");
	expect(get(IOPOL_TYPE_VFS_TRIGGER_RESOLVE, IOPOL_SCOPE_PROCESS) == IOPOL_VFS_TRIGGER_RESOLVE_OFF, "get trigger resolve OFF");
	// disk policies read back as xnu's throttle tiers do: tier 0 (unset, DEFAULT or IMPORTANT) is DEFAULT, and a thread
	// reads its own value, never the process's (osfmk/kern/task_policy.c proc_tier_to_iopol)
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS) == IOPOL_DEFAULT, "unset disk process policy is DEFAULT");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD) == IOPOL_DEFAULT, "unset disk thread policy is DEFAULT");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE) == 0, "set disk process THROTTLE");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS) == IOPOL_THROTTLE, "get disk process THROTTLE");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD) == IOPOL_DEFAULT, "a thread without its own disk policy does not read the process's");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_IMPORTANT) == 0, "set disk process IMPORTANT");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS) == IOPOL_DEFAULT, "IMPORTANT is tier 0 and reads back as DEFAULT");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_PASSIVE) == 0 && get(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS) == IOPOL_PASSIVE, "PASSIVE reads back");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, IOPOL_UTILITY) == 0, "set disk thread UTILITY");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD) == IOPOL_UTILITY, "get disk thread UTILITY");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_DEFAULT) == 0, "set disk process DEFAULT");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS) == IOPOL_DEFAULT, "disk process DEFAULT");
	expect(get(IOPOL_TYPE_DISK, IOPOL_SCOPE_DARWIN_BG) == IOPOL_UTILITY, "an unset background disk policy reads UTILITY");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_DARWIN_BG, IOPOL_DEFAULT) == 0 && get(IOPOL_TYPE_DISK, IOPOL_SCOPE_DARWIN_BG) == IOPOL_UTILITY,
	       "the background scope's DEFAULT is UTILITY");
	expect(set(IOPOL_TYPE_DISK, IOPOL_SCOPE_DARWIN_BG, IOPOL_IMPORTANT) == -1 && errno == EINVAL, "IMPORTANT is not valid for the background scope");
	if (geteuid() != 0) {
		expect(set(IOPOL_TYPE_VFS_HFS_CASE_SENSITIVITY, IOPOL_SCOPE_PROCESS, IOPOL_VFS_HFS_CASE_SENSITIVITY_FORCE_CASE_SENSITIVE) == -1 && errno == EPERM,
		       "a privileged type is refused to a non-root process");
		expect(get(IOPOL_TYPE_VFS_HFS_CASE_SENSITIVITY, IOPOL_SCOPE_PROCESS) == IOPOL_VFS_HFS_CASE_SENSITIVITY_DEFAULT, "and stays DEFAULT");
	}

	// what xnu refuses
	{
		struct { int scope; int type; int policy; } param = { IOPOL_SCOPE_PROCESS, materialize, 0 };
		errno = 0;
		expect(syscall(SYS_iopolicysys, 99, &param) == -1 && errno == EINVAL, "unknown command");
	}
	if (geteuid() != 0) {
		struct { int scope; int type; int policy; } mtime = { IOPOL_SCOPE_PROCESS, 8 /* IOPOL_TYPE_VFS_SKIP_MTIME_UPDATE */, 0 };
		errno = 0;
		expect(syscall(SYS_iopolicysys, 99, &mtime) == -1 && errno == EPERM, "an unknown command for the mtime type is refused as unprivileged (EPERM)");
	}
	expect(set(IOPOL_TYPE_DISK, 7, IOPOL_THROTTLE) == -1 && errno == EINVAL, "unknown disk scope");
	expect(set(IOPOL_TYPE_VFS_ATIME_UPDATES, IOPOL_SCOPE_PROCESS, 5) == -1 && errno == EINVAL, "unknown policy for a flag type");
	expect(set(IOPOL_TYPE_VFS_TRIGGER_RESOLVE, IOPOL_SCOPE_PROCESS, 5) == -1 && errno == EINVAL, "unknown policy for another flag type");
	// getiopolicy_np() overwrites errno with the stub's -1 (libsyscall/wrappers/getiopolicy_np.c), so only the result is checked for a get
	expect(get(materialize, 9) == -1, "unknown scope on get");
	expect(set(materialize, IOPOL_SCOPE_PROCESS, 7) == -1 && errno == EINVAL, "unknown policy on set");
	expect(get(99, IOPOL_SCOPE_PROCESS) == -1, "unknown type");
	expect(get(IOPOL_TYPE_VFS_TRIGGER_RESOLVE, IOPOL_SCOPE_THREAD) == -1, "trigger resolve has no thread scope");
	expect(set(IOPOL_TYPE_VFS_ATIME_UPDATES, IOPOL_SCOPE_DARWIN_BG, IOPOL_ATIME_UPDATES_OFF) == -1 && errno == EINVAL, "atime has no background scope");

	printf("PASS: I/O policies are remembered per scope and invalid requests are refused\n");
	return 0;
}
