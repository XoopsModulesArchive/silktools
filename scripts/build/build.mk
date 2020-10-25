#####################################################-*- makefile -*-##
# CERT SiLK Analysis Suite Global Makefile
# build.mk,v 1.13 2003/12/22 17:15:35 thomasm Exp
#######################################################################

ifndef SUITEROOT
$(error Please set the SUITEROOT environment variable.)
endif

#######################################################################
# Platform Determination
#######################################################################

# determine hardware and operating system type using uname

ifndef HW_TYPE
HW_TYPE = $(shell uname -m)
endif

ifndef OS_TYPE
OS_TYPE = $(shell uname -s|tr '[:lower:]' '[:upper:]')
endif

# since we use GCC most places, set compilation flags for GCC here
GCC_WARNING_FLAGS = -Wall -W
GCC_RELEASE_FLAGS = $(GCC_WARNING_FLAGS) -O3
GCC_DEBUG_FLAGS = $(GCC_WARNING_FLAGS) -g
GCC_PROFILE_FLAGS = -Wall -Wformat -W -g -pg
GCC_MAKEDEPEND_FLAGS = -MM
GCC_LINK_FLAGS_profile = -pg -ldl

# File extension for shared libraries (.so for everything but darwin so far)
SHLIB_EXT=so

# set compliation flags based on operating system

ifeq ($(OS_TYPE),LINUX)
CC = gcc
RELEASE_FLAGS = $(GCC_RELEASE_FLAGS)
DEBUG_FLAGS = $(GCC_DEBUG_FLAGS)
PROFILE_FLAGS = $(GCC_PROFILE_FLAGS)
LINK_FLAGS_profile = $(GCC_LINK_FLAGS_profile)
MAKEDEPEND_FLAGS = $(GCC_MAKEDEPEND_FLAGS)
LIBDIR_FLAGS = $(LIBDIR_LINUX)
STDLIBS_FLAGS = $(STDLIBS_LINUX)
INCLUDE_FLAGS = $(INCLUDE_LINUX)
USES_DLOPEN_FLAGS = -rdynamic
PYTHON = python
else
ifeq ($(OS_TYPE),SUNOS)
CC = gcc
RELEASE_FLAGS = $(GCC_RELEASE_FLAGS)
DEBUG_FLAGS = $(GCC_DEBUG_FLAGS)
PROFILE_FLAGS = $(GCC_PROFILE_FLAGS)
LINK_FLAGS_profile = $(GCC_LINK_FLAGS_profile)
MAKEDEPEND_FLAGS = $(GCC_MAKEDEPEND_FLAGS)
LIBDIR_FLAGS = $(LIBDIR_SUNOS)
STDLIBS_FLAGS = $(STDLIBS_SUNOS) -lsocket -lnsl
INCLUDE_FLAGS = $(INCLUDE_SUNOS)
PYTHON = python
else
ifeq ($(OS_TYPE),OSF1)
CC = cc
RELEASE_FLAGS = -O4 -D_POSIX_PII
DEBUG_FLAGS = -g2
PROFILE_FLAGS = -p
LINK_FLAGS_profile = $(PROFILE_FLAGS)
MAKEDEPEND_FLAGS = -M
LIBDIR_FLAGS = $(LIBDIR_OSF1)
STDLIBS_FLAGS = $(STDLIBS_OSF1)
INCLUDE_FLAGS = $(INCLUDE_OSF1) -I/usr/include -I/usr/local/include
PYTHON = python
else
ifeq ($(OS_TYPE),AIX)
CC = gcc
RELEASE_FLAGS = $(GCC_RELEASE_FLAGS)
DEBUG_FLAGS = $(GCC_DEBUG_FLAGS)
PROFILE_FLAGS = $(GCC_PROFILE_FLAGS)
LINK_FLAGS_profile = $(GCC_LINK_FLAGS_profile)
MAKEDEPEND_FLAGS = $(GCC_MAKEDEPEND_FLAGS)
LIBDIR_FLAGS = $(LIBDIR_AIX)
STDLIBS_FLAGS = $(STDLIBS_AIX) -lnsl
INCLUDE_FLAGS = $(INCLUDE_AIX)
USES_DLOPEN_FLAGS = -Wl,-brtl,-bexpall,-bgcbypass:100
# The "100" on the above line is arbitrarily large.  We don't want to
# have the linker doing gc on unreferenced functions, since they may
# be referenced by run-time dynamically-loaded libraries
PYTHON = python
else
ifeq ($(OS_TYPE),OPENBSD)
CC = gcc
RELEASE_FLAGS = $(GCC_RELEASE_FLAGS)
DEBUG_FLAGS = $(GCC_DEBUG_FLAGS)
PROFILE_FLAGS = $(GCC_PROFILE_FLAGS)
LINK_FLAGS_profile = $(GCC_LINK_FLAGS_profile)
MAKEDEPEND_FLAGS = $(GCC_MAKEDEPEND_FLAGS)
LIBDIR_FLAGS = $(LIBDIR_OPENBSD)
STDLIBS_FLAGS = $(STDLIBS_OPENBSD)
INCLUDE_FLAGS = $(INCLUDE_OPENBSD)
USES_DLOPEN_FLAGS = -Wl,-export-dynamic
PYTHON = python
else
# AJK. Start with the same values as those for LINUX. Need
# to verify that's all we need to do.
ifeq ($(OS_TYPE),DARWIN)
CC = gcc
DARWIN_CFLAGS = # -fno-common
RELEASE_FLAGS = $(GCC_RELEASE_FLAGS) $(DARWIN_CFLAGS)
DEBUG_FLAGS = $(GCC_DEBUG_FLAGS) $(DARWIN_CFLAGS)
PROFILE_FLAGS = $(GCC_PROFILE_FLAGS) $(DARWIN_CFLAGS)
LINK_FLAGS_profile = $(GCC_LINK_FLAGS_profile)
MAKEDEPEND_FLAGS = $(GCC_MAKEDEPEND_FLAGS)
LIBDIR_FLAGS = $(LIBDIR_LINUX)
STDLIBS_FLAGS = $(STDLIBS_LINUX)
INCLUDE_FLAGS = $(INCLUDE_LINUX)
USES_DLOPEN_FLAGS = -rdynamic
SHLIB_EXT=dylib
PYTHON = python
else
$(error bad OS_TYPE $(OS_TYPE))
endif
endif
endif
endif
endif
endif


#######################################################################
# Global Variables
#######################################################################

# set include and library target dirs

INCDIR=$(SUITEROOT)/src/include
LIBDIR=$(SUITEROOT)/lib
BINDIR=$(SUITEROOT)/bin
OBJDIR=obj/.objdir


# Create variables for debugging and profiling

LIBS = $(foreach lib,$(LIBRARIES),obj/lib$(lib).a)
LIBS_DEBUG = $(foreach lib,$(LIBRARIES),obj/lib$(lib)_g.a)
LIBS_PROFILE = $(foreach lib,$(LIBRARIES),obj/lib$(lib)_p.a)

INSTALLED_LIBS = \
    $(patsubst obj/%, $(LIBDIR)/%, $(LIBS) $(LIBS_DEBUG) $(LIBS_PROFILE))

APPNAMES_DEBUG = $(APPNAMES:%=%_g)
APPNAMES_PROFILE = $(APPNAMES:%=%_p)

INSTALLED_APPS = \
    $(addprefix $(BINDIR)/, $(APPNAMES) $(APPNAMES_DEBUG) $(APPNAMES_PROFILE))

TESTAPPS_DEBUG = $(TESTAPPS:%=%_g)
TESTAPPS_PROFILE = $(TESTAPPS:%=%_p)

DYNALIBS_RELEASE = $(foreach lib,$(DYNALIBS),obj/lib$(lib).$(SHLIB_EXT))
DYNALIBS_DEBUG = $(foreach lib,$(DYNALIBS),obj/lib$(lib)_g.$(SHLIB_EXT))
DYNALIBS_PROFILE = $(foreach lib,$(DYNALIBS),obj/lib$(lib)_p.$(SHLIB_EXT))

INSTALLED_DYNA = $(patsubst obj/%, $(LIBDIR)/%, \
                   $(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE))

# Test harness program and directory where tests reside
TEST_HARNESS_PROG = $(SUITEROOT)/scripts/shell-scripts/test-harness.py
TEST_HARNESS_DIR = test


#######################################################################
# Targets
#######################################################################

# Protect against typos that create empty list of objects and invoke
# infinite recursion
ifdef WITHDEPS_OBJ
ifeq ($(strip $(WITHDEPS_OBJ)),)
$(error No objects given for target--check for typos in your makefile)
endif
endif

.PHONY: default all release debug profile clean common_clean check \
	libraries libraries-release libraries-debug libraries-profile \
	appnames appnames-release appnames-debug appnames-profile \
	testapps testapps-release testapps-debug testapps-profile \
	dynalibs dynalibs-release dynalibs-debug dynalibs-profile \
	install install-release install-debug install-profile \
	install-libraries install-appnames install-dynalibs \
	install-libraries-release install-libraries-debug \
	install-libraries-profile install-appnames-release \
	install-appnames-debug install-appnames-profile \
	install-dynalibs-release install-dynalibs-debug \
	install-dynalibs-profile depend always_rebuild


default:: install-libraries-release appnames-release install-dynalibs-release

all:: libraries appnames dynalibs

libraries:: libraries-release libraries-debug libraries-profile

appnames:: appnames-release appnames-debug appnames-profile

testapps:: testapps-release testapps-debug testapps-profile

dynalibs:: dynalibs-release dynalibs-debug dynalibs-profile

release:: libraries-release appnames-release dynalibs-release

debug:: libraries-debug appnames-debug dynalibs-debug

profile:: libraries-profile appnames-profile dynalibs-profile

libraries-release:: $(LIBS)

libraries-debug:: $(LIBS_DEBUG)

libraries-profile:: $(LIBS_PROFILE)

appnames-release:: $(APPNAMES)

appnames-debug:: $(APPNAMES_DEBUG)

appnames-profile:: $(APPNAMES_PROFILE)

testapps-release:: $(TESTAPPS)

testapps-debug:: $(TESTAPPS_DEBUG)

testapps-profile:: $(TESTAPPS_PROFILE)

dynalibs-release:: $(DYNALIBS_RELEASE)

dynalibs-debug:: $(DYNALIBS_DEBUG)

dynalibs-profile:: $(DYNALIBS_PROFILE)

install:: install-libraries install-appnames install-dynalibs

install-release:: install-libraries-release install-appnames-release install-dynalibs-release

install-debug:: install-libraries-debug install-appnames-debug install-dynalibs-debug

install-profile:: install-libraries-profile install-appnames-profile install-dynalibs-profile

install-libraries:: install-libraries-release install-libraries-debug install-libraries-profile

install-appnames:: install-appnames-release install-appnames-debug install-appnames-profile

install-dynalibs:: install-dynalibs-release install-dynalibs-debug install-dynalibs-profile

install-libraries-release:: $(LIBS:obj/%=$(LIBDIR)/%)

install-libraries-debug:: $(LIBS_DEBUG:obj/%=$(LIBDIR)/%)

install-libraries-profile:: $(LIBS_PROFILE:obj/%=$(LIBDIR)/%)

install-appnames-release:: $(APPNAMES:%=$(BINDIR)/%)

install-appnames-debug:: $(APPNAMES_DEBUG:%=$(BINDIR)/%)

install-appnames-profile:: $(APPNAMES_PROFILE:%=$(BINDIR)/%)

install-dynalibs-release:: $(DYNALIBS_RELEASE:obj/%=$(LIBDIR)/%)

install-dynalibs-debug:: $(DYNALIBS_DEBUG:obj/%=$(LIBDIR)/%)

install-dynalibs-profile:: $(DYNALIBS_PROFILE:obj/%=$(LIBDIR)/%)

$(INSTALLED_LIBS): $(LIBDIR)/%: obj/%
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	cp $^ $@

$(INSTALLED_APPS): $(BINDIR)/%: %
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	cp $^ $@

$(INSTALLED_DYNA): $(LIBDIR)/%: obj/%
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	rm -f $@
	cp $^ $@

check:: release
	if [ -d $(TEST_HARNESS_DIR) ]; then \
	  $(PYTHON) $(TEST_HARNESS_PROG) $(TEST_HARNESS_DIR) ; \
	fi

clean::	common_clean
	rm -f $(LIBS) $(LIBS_DEBUG) $(LIBS_PROFILE) $(APPNAMES) $(APPNAMES_DEBUG) $(APPNAMES_PROFILE) $(TESTAPPS) $(TESTAPPS_DEBUG) $(TESTAPPS_PROFILE) $(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE)

common_clean:
	rm -f core
	rm -rf obj

depend:
# Nothing to do: dependencies are always rebuilt

always_rebuild:

$(OBJDIR):
	mkdir -p obj
	touch $@


# option to make used when calling onesself recursively without
# changing directories.
MAKE_RECURSIVE_OPT = -f $(SUITEROOT)/scripts/build/build.mk
ifndef EMACS
MAKE_RECURSIVE_OPT += --no-print-directory
endif

# utilities
SHELL = /bin/sh
AR = ar
RANLIB = ranlib
AWK = awk
SED = sed



#######################################################################
# Common rules
#######################################################################


#######################################################################
# Compilation
#######################################################################

# Common compilation macros

INCLUDE_FLAGS += -I$(INCDIR) $(INCLUDE_GLOBAL)
LIBDIR_FLAGS += -Lobj -L$(LIBDIR) $(LIBDIR_GLOBAL)
STDLIBS_FLAGS += $(STDLIBS_GLOBAL)
DEFINES = -D$(OS_TYPE) $(BYTE_ORDER_FLAG)

CFLAGS_RELEASE = $(RELEASE_FLAGS) $(INCLUDE_FLAGS) $(DEFINES)
CFLAGS_DEBUG = $(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(DEFINES)
CFLAGS_PROFILE = $(PROFILE_FLAGS) $(INCLUDE_FLAGS) $(DEFINES)

# Add system-dependent libraries to DEPLIBS

DEPLIBS_OS_TYPE = DEPLIBS_$(OS_TYPE)
DEPLIBS += $($(DEPLIBS_OS_TYPE))

# Derive library linking flags from dependent libraries

DEPLIBS_RELEASE = $(DEPLIBS:%=$(SUITEROOT)/src/%)
DEPLIBS_DEBUG = $(DEPLIBS_RELEASE:%.a=%_g.a)
DEPLIBS_PROFILE = $(DEPLIBS_RELEASE:%.a=%_p.a)

ifdef WITHDEPS_LIB
DEPLIBS_FLAGS = $(patsubst lib%.a,-l%,$(notdir $(WITHDEPS_LIB)))
endif

# Dependent library recursive make rules

ifdef WITHDEPS_LIB
# we're using $(sort ...) here to remove duplicates
$(sort $(WITHDEPS_LIB)): %: always_rebuild
	@echo Checking whether $(@F) needs rebuilding
	$(MAKE) -C $(dir $(@D)) install-libraries-$(APP_RELEASE)
endif


#######################################################################
# Functions to be invoked by $(call xxx,args)
#######################################################################

# $(call mkobjs,SUFFIX,NAMES)
#     For each name in NAMES, this function removes any existing
#     suffix, prepends "obj/" to the name and appends SUFFIX to it.
#     SUFFIX should not be blank.
# Ex: $(call mkobjs,.g,foo.o bar.o baz.o)
#         ==> obj/foo.g obj/bar.g obj/baz.g
# Ex: $(call mkobjs,.o,foo.o bar.o baz.o)
#         ==> obj/foo.o obj/bar.o obj/baz.o

mkobjs = $(foreach name,$(2),obj/$(basename $(name))$(strip $(1)))


# $(call mklibs,LABEL,DEPLIBS)
#     For each lib in DEPLIBS, where lib looks like
#     "util/fglob/libfglob.a", this function inserts "obj/" before the
#     library name and inserts LABEL before the ".a" extension.  LABEL
#     may be blank.
# Ex: $(call mklibs,_g,util/fglob/libfglob.a ds/hashlib/libhash.a)
#         ==> util/fglob/obj/libfglob_g.a ds/hashlib/obj/libhash_g.a)
# Ex: $(call mklibs,,util/fglob/libfglob.a ds/hashlib/libhash.a)
#         ==> util/fglob/obj/libfglob.a ds/hashlib/obj/libhash.a)

mklibs = $(foreach lib,$(2),$(SUITEROOT)/src/$(dir $(lib))obj/$(basename $(notdir $(lib)))$(strip $(1)).a)


#######################################################################
# First pass through makefile, recurse with the correct object list
# for the target
#######################################################################

ifndef WITHDEPS_OBJ

ifneq ($(strip $(LIBOBJS) $(foreach lib, $(LIBRARIES), $(LIBOBJS_$(lib)))),)
$(LIBS): obj/lib%.a: always_rebuild
	@$(MAKE) $(MAKE_RECURSIVE_OPT) WITHDEPS_OBJ="$(call mkobjs,.o,$(LIBOBJS) $(LIBOBJS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" LIBRARIES="$(*)" $@

$(LIBS_DEBUG): obj/lib%_g.a: always_rebuild
	@$(MAKE) $(MAKE_RECURSIVE_OPT) WITHDEPS_OBJ="$(call mkobjs,.g,$(LIBOBJS) $(LIBOBJS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" LIBRARIES="$(*)" $@

$(LIBS_PROFILE): obj/lib%_p.a: always_rebuild
	@$(MAKE) $(MAKE_RECURSIVE_OPT) WITHDEPS_OBJ="$(call mkobjs,.P,$(LIBOBJS) $(LIBOBJS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" LIBRARIES="$(*)" $@
endif


ifneq ($(strip $(OBJS) $(foreach app, $(APPNAMES), $(OBJS_$(app)))),)
$(APPNAMES): %: always_rebuild
	@WITHDEPS_OBJ="$(call mkobjs,.o,$(OBJS) $(OBJS_$(*)))" WITHDEPS_LIB="$(call mklibs,,$(DEPLIBS) $(DEPLIBS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(@))" APP_RELEASE="release" APPNAMES="$@" $(MAKE) $(MAKE_RECURSIVE_OPT) $@

$(APPNAMES_DEBUG): %_g: always_rebuild
	@WITHDEPS_OBJ="$(call mkobjs,.g,$(OBJS) $(OBJS_$(*)))" WITHDEPS_LIB="$(call mklibs,_g,$(DEPLIBS) $(DEPLIBS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" APP_RELEASE="debug" APPNAMES="$@" $(MAKE) $(MAKE_RECURSIVE_OPT) $@

$(APPNAMES_PROFILE): %_p: always_rebuild
	@WITHDEPS_OBJ="$(call mkobjs,.P,$(OBJS) $(OBJS_$(*)))" WITHDEPS_LIB="$(call mklibs,_p,$(DEPLIBS) $(DEPLIBS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" APP_RELEASE="profile" APPNAMES="$@" $(MAKE) $(MAKE_RECURSIVE_OPT) $@
endif


ifneq ($(strip $(TESTOBJS) $(foreach app, $(TESTAPPS), $(TESTOBJS_$(app)))),)
$(TESTAPPS): %: always_rebuild
	@WITHDEPS_OBJ="$(call mkobjs,.o,$(TESTOBJS) $(TESTOBJS_$(*)))" WITHDEPS_LIB="$(call mklibs,,$(DEPLIBS) $(DEPLIBS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" APP_RELEASE="release" TESTAPPS="$@" $(MAKE) $(MAKE_RECURSIVE_OPT) $@

$(TESTAPPS_DEBUG): %_g: always_rebuild
	@WITHDEPS_OBJ="$(call mkobjs,.g,$(TESTOBJS) $(TESTOBJS_$(*)))" WITHDEPS_LIB="$(call mklibs,_g,$(DEPLIBS) $(DEPLIBS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" APP_RELEASE="debug" TESTAPPS="$@" $(MAKE) $(MAKE_RECURSIVE_OPT) $@

$(TESTAPPS_PROFILE): %_p: always_rebuild
	@WITHDEPS_OBJ="$(call mkobjs,.P,$(TESTOBJS) $(TESTOBJS_$(*)))" WITHDEPS_LIB="$(call mklibs,_p,$(DEPLIBS) $(DEPLIBS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" APP_RELEASE="profile" TESTAPPS="$@" $(MAKE) $(MAKE_RECURSIVE_OPT) $@
endif


ifneq ($(strip $(DLOBJS) $(foreach lib, $(DYNALIBS), $(DLOBJS_$(lib)))),)
$(DYNALIBS_RELEASE): obj/lib%.$(SHLIB_EXT): always_rebuild
	@$(MAKE) $(MAKE_RECURSIVE_OPT) WITHDEPS_OBJ="$(call mkobjs,.o,$(DLOBJS) $(DLOBJS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" DYNALIBS="$(*)" $@

$(DYNALIBS_DEBUG): obj/lib%_g.$(SHLIB_EXT): always_rebuild
	@$(MAKE) $(MAKE_RECURSIVE_OPT) WITHDEPS_OBJ="$(call mkobjs,.g,$(DLOBJS) $(DLOBJS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" DYNALIBS="$(*)" $@

$(DYNALIBS_PROFILE): obj/lib%_p.$(SHLIB_EXT): always_rebuild
	@$(MAKE) $(MAKE_RECURSIVE_OPT) WITHDEPS_OBJ="$(call mkobjs,.P,$(DLOBJS) $(DLOBJS_$(*)))" WITHDEP_CFLAGS="$(CFLAGS_$(*))" DYNALIBS="$(*)" $@
endif

else

#######################################################################
# Recursive call, actually make the target
#######################################################################

# Protect against typos that create empty list of objects and invoke
# infinite recursion
ifeq ($(strip $(WITHDEPS_OBJ)),)
$(error No objects given for target)
endif

# Include automatically generated dependency rules
-include $(addsuffix .d, $(basename $(WITHDEPS_OBJ)))

# Library linking rules
$(LIBS) $(LIBS_DEBUG) $(LIBS_PROFILE): $(WITHDEPS_OBJ)
	@echo "Creating library $@"
	-rm -f $@
	$(AR) cr $@ $^
	$(RANLIB) $@

# User application linking rules
$(APPNAMES) $(APPNAMES_DEBUG) $(APPNAMES_PROFILE): $(WITHDEPS_OBJ) $(WITHDEPS_LIB)
	@echo "Final link for application $@"
	$(CC) -o $@ $(WITHDEPS_OBJ) $(LIBDIR_FLAGS) $(LINK_FLAGS_$(APP_RELEASE)) $(DEPLIBS_FLAGS) $(STDLIBS_FLAGS) $(if $(strip $(filter $@,$(foreach x,$(USES_DLOPEN),$(x) $(x)_g $(x)_p))),$(USES_DLOPEN_FLAGS))

# Testing application linking rules
$(TESTAPPS) $(TESTAPPS_DEBUG) $(TESTAPPS_PROFILE): $(WITHDEPS_OBJ) $(WITHDEPS_LIB)
	@echo "Final link for testing application $@"
	$(CC) -o $@ $(WITHDEPS_OBJ) $(LIBDIR_FLAGS) $(LINK_FLAGS_$(APP_RELEASE)) $(DEPLIBS_FLAGS) $(STDLIBS_FLAGS)

# Dynamic library rules
ifeq ($(OS_TYPE),AIX)
$(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE): $(WITHDEPS_OBJ)
	@echo "Final link for shared object $@"
	$(LD) -o $@ -G -bnoentry -bexpall -b32 -bM:SRE $^ $(DYNALIBS_FLAGS) -lc -ldl
else
ifeq ($(OS_TYPE),SUNOS)
$(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE): $(WITHDEPS_OBJ)
	@echo "Final link for shared object $@"
	$(CC) -o $@ -DPIC -fpic -G $^ $(DYNALIBS_FLAGS)
else
ifeq ($(OS_TYPE),OSF1)
$(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE): $(WITHDEPS_OBJ)
	@echo "Final link for shared object $@"
	$(LD) -o $@ -shared -expect_unresolved '*' $^ $(DYNALIBS_FLAGS)
else
ifeq ($(OS_TYPE),DARWIN)
$(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE): $(WITHDEPS_OBJ)
	@echo "Final link for shared object $@"
	# Dynamic libraries may need the static utility libraries. FIXME. This should be a complete list
	# or there should be a mechanism for specifying these dependencies per shared library.
	$(CC) -o $@ -dynamiclib  $^ $(DYNALIBS_FLAGS) -install_name $(@F) $(SUITEROOT)/lib/libfglob.a  $(SUITEROOT)/lib/libutils.a  $(SUITEROOT)/lib/librw.a
else
$(DYNALIBS_RELEASE) $(DYNALIBS_DEBUG) $(DYNALIBS_PROFILE): $(WITHDEPS_OBJ)
	@echo "Final link for shared object $@"
	$(CC) -o $@ -DPIC -fPIC -shared $^ $(DYNALIBS_FLAGS)
endif
endif
endif
endif

endif


# Compliation pattern rules

obj/%.o : %.c $(OBJDIR)
	$(CC) $(CFLAGS) $(WITHDEP_CFLAGS) $(CFLAGS_RELEASE) -c -o $@ $<

obj/%.g : %.c $(OBJDIR)
	$(CC) $(CFLAGS) $(WITHDEP_CFLAGS) $(CFLAGS_DEBUG) -c -o $@ $<

obj/%.P : %.c $(OBJDIR)
	$(CC) $(CFLAGS) $(WITHDEP_CFLAGS) $(CFLAGS_PROFILE) -c -o $@ $<

# Additional rule so "make libXXX.a" does something useful
$(subst obj/, ,$(LIBS) $(LIBS_DEBUG) $(LIBS_PROFILE)): %.a: obj/%.a

# Additional rule so "make xxx.o" does something useful
%.o %.g %.P %.d : %.c
	$(MAKE) $(MAKE_RECURSIVE_OPT) obj/$@


#######################################################################
# Automatic dependency generation
#######################################################################

# Rule to build dependency file
obj/%.d : %.c $(OBJDIR)
	@set -e ; $(CC) $(MAKEDEPEND_FLAGS) $(INCLUDE_FLAGS) $(DEFINES) $< 2>/dev/null | $(SED) 's,\($*\)\.o[ :]*,obj/\1.o obj/\1.g obj/\1.P $@ : ,g' > $@; [ -s $@ ] || rm -f $@

# Tell make not to delete dependency files
.SECONDARY: *.d


#######################################################################
# Control which variables get exported to sub-makes
#######################################################################

export	 LIBDIR_GLOBAL STDLIBS_GLOBAL INCLUDE_GLOBAL \
	 LIBDIR_LINUX STDLIBS_LINUX INCLUDE_LINUX \
	 LIBDIR_SUNOS STDLIBS_SUNOS INCLUDE_SUNOS \
	 LIBDIR_OSF1 STDLIBS_OSF1 INCLUDE_OSF1 \
	 LIBDIR_AIX STDLIBS_AIX INCLUDE_AIX \
	 DYNALIBS_FLAGS USES_DLOPEN CFLAGS

unexport LIBRARIES LIBS LIBS_DEBUG LIBS_PROFILE INSTALLED_LIBS \
	 APPNAMES APPNAMES_DEBUG APPNAMES_PROFILE INSTALLED_APPS \
	 TESTAPPS TESTAPPS_DEBUG TESTAPPS_PROFILE \
	 DYNALIBS DYNALIBS_RELEASE DYNALIBS_DEBUG DYNALIBS_PROFILE \
	 WITHDEPS_OBJ WITHDEPS_LIB
