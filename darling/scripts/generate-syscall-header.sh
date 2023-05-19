set -x

# https://stackoverflow.com/a/3220288
# ```
# all: library.cpp main.cpp
# ```
# `$@` evaluates to `all`
# `$<` evaluates to `library.cpp`
# `$^` evaluates to `library.cpp main.cpp`

XNU_SRCROOT="$(cd ../.. && pwd)"
MAKESYSCALLS=$XNU_SRCROOT/bsd/kern/makesyscalls.sh
XNU_TEMP="$XNU_SRCROOT/gen"

mkdir -p $XNU_TEMP

# Based on https://github.com/apple-oss-distributions/xnu/blob/xnu-7195.141.2/makedefs/MakeInc.rule#L386-L392
# $(TARGET)/bsd.syscalls.master: SRCROOT)/bsd/kern/syscalls.master $(MAKESYSCALLS)
# 	@$(LOG_GENERATE) "$(@F)$(Color0) from $(ColorF)$<$(Color0)"
# 	$(_v)$(UNIFDEF) $(SEED_DEFINES) $< > $@;	\
# 	if [ $$? -eq 2 ]; then				\
# 		$(ERR) Parse failure for $<;		\
# 		exit 1;					\
# 	fi

# This actually doesn't seems to make any changes to the copy...
SEED_DEFINES=""
unifdef "$XNU_SRCROOT/bsd/kern/syscalls.master" > "$XNU_TEMP/bsd.syscalls.master"

# Based on https://github.com/apple-oss-distributions/xnu/blob/xnu-7195.141.2/bsd/sys/Makefile#L262-L265
# syscall.h: $(TARGET)/bsd.syscalls.master $(OBJROOT)/cscope.genhdrs
# 	@$(LOG_GENERATE) "$(ColorLF)bsd/sys/$@$(Color0) from $(ColorF)$(<F)$(Color0)"
# 	@echo "$(OBJPATH)/bsd/sys/$@" > $(OBJROOT)/cscope.genhdrs/$@.path
# 	$(_v)$(MAKESYSCALLS) $< header > /dev/null

cd $XNU_TEMP
$MAKESYSCALLS "bsd.syscalls.master" header