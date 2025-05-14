#ifndef DARLING_CONVERSION_PROCESS_POSIX_SPAWN
#define DARLING_CONVERSION_PROCESS_POSIX_SPAWN

#include <sys/types.h>
#include <limits.h>
#include <mach/mach_port.h>

typedef enum {
	XNU_PSFA_OPEN = 0,
	XNU_PSFA_CLOSE = 1,
	XNU_PSFA_DUP2 = 2,
	XNU_PSFA_INHERIT = 3,
	XNU_PSFA_FILEPORT_DUP2 = 4,
	XNU_PSFA_CHDIR = 5,
	XNU_PSFA_FCHDIR = 6
} xnu_psfa_t;

struct xnu__posix_spawnattr
{
	short psa_flags;
	unsigned int psa_sigdefault;
	unsigned int psa_sigmask;
	unsigned int psa_pgroup;
	int psa_binprefs[4];
	int psa_pcontrol;
	int psa_apptype;
	/* more fields that we don't support anyway */
};
typedef struct xnu__posix_spawnattr* xnu_posix_spawnattr_t;

struct xnu__psfa_action
{
	xnu_psfa_t psfaa_type;
	union {
		int psfaa_filedes;
		mach_port_name_t psfaa_fileport;
	};
	union {
		struct _psfaa_open {
			int psfao_oflag;
			mode_t psfao_mode;
			char psfao_path[PATH_MAX];
		} psfaa_openargs;
		struct {
			int psfad_newfiledes;
		} psfaa_dup2args;
		struct {
			char psfac_path[PATH_MAX];
		} psfaa_chdirargs;
	};
};

struct xnu__posix_spawn_file_actions
{
	int psfa_act_alloc;
	int psfa_act_count;
	struct xnu__psfa_action psfa_act_acts[];
};
typedef struct xnu__posix_spawn_file_actions* xnu_posix_spawn_file_actions_t;

#endif // DARLING_CONVERSION_PROCESS_POSIX_SPAWN
