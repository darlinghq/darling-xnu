# Differences Between Darlingserver's XNU Source And This Repo

Darlingserver contains a striped down version of the XNU source that is only used for darlingserver. In comparison, this repo contains a full copy of the XNU source that can use for any purpose.

This implies the following:
* All non-darlingserver XNU source changes or modifications should be made to this repo (not darlingserver).
* If the Darling SDK need any additional XNU header, the symlinks should point to this repo.

# `libsyscall` Symlinks

When updating XNU source, make sure to also restore the following symlinks:

```
# cd /path/to/darling-xnu
ln -s -t libsyscall/Platforms/MacOSX ../../../darling/src/libsystem_kernel/extra/Platforms/MacOSX/ppc
ln -s -t libsyscall ../gen/bsdsyscalls
ln -s -t libsyscall ../darling/src/libsystem_kernel/extra/nolegacy.h
```