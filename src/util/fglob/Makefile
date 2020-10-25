##################################################-*- makefile -*-##
# CERT Analysis Suite Build Makefile
# Makefile,v 1.4 2003/11/21 19:37:30 thomasm Exp
####################################################################

# Set SUITEROOT to the root of the tree if not already set
ifndef SUITEROOT
export SUITEROOT := $(CURDIR)/../../..
endif

# Libraries to be made.  (For some library libXXX.a, only put XXX).
# LIBOBJS lists objects for every library in $(LIBRARIES)---list .o
# files only; .g and .P are handled automagically.
# LIBOBJS_XXX lists objects for libXXX.a; must appear in $(LIBRARIES)
LIBRARIES = fglob
LIBOBJS = fglob.o
#LIBOBJS_XXX = 

# User applications to be made.  OBJS lists objects for every app in
# $(APPNAMES); OBJ_XXX is for app XXX; XXX must appear in $(APPNAMES)
APPNAMES = rwfglob
OBJS = fglob.o rwfglobapp.o
#OBJS_XXX = 

# Testing Applications to be made on request; these are never
# installed.
TESTAPPS = 
TESTOBJS = 
#TESTOBJS_XXX = 

# Dynamic Libraries to be made.
DYNALIBS = 
DLOBJS = 
#DLOBJS_XXX = 

# Libraries required by the user and/or testing applications on all
# systems--ignored when building libraries
# (list .a files only, by path relative to SUITEROOT)
DEPLIBS = 

# App specific libraries.  appname must be the same as that the user
# or testing application that needs the libraries
#DEPLIBS_appname = 
DEPLIBS_rwfglob = rw/librw/librw.a util/utils/libutils.a

# System-dependent libraries required by this application
# (as above, list .a files only by path relative to suiteroot)
DEPLIBS_LINUX = 
DEPLIBS_SUNOS = 
DEPLIBS_OSF1 = 
DEPLIBS_AIX = 

# Additional system include directories and libraries go here
# these are C compiler flags, so precede each with -I, -L or -l
INCLUDE_GLOBAL = 
INCLUDE_LINUX = 
INCLUDE_SUNOS = 
INCLUDE_OSF1 = 
INCLUDE_AIX = 
LIBDIR_GLOBAL = 
LIBDIR_LINUX = 
LIBDIR_SUNOS = 
LIBDIR_OSF1 = 
LIBDIR_AIX = 
STDLIBS_GLOBAL = 
STDLIBS_LINUX = 
STDLIBS_SUNOS = 
STDLIBS_OSF1 = 
STDLIBS_AIX = 

# Library or application specific C compiler flags; precede each with
# -D, -I, -L, or -l.  BEWARE: Objects used in multiple libraries
# and/or applications will NOT be rebuilt with the different flags.
#CFLAGS_XXX = 

# Additional library or linker flags to use when issuing the link
# command to create a dynamic library or shared object
DYNALIBS_FLAGS = 

# List applications that use dlopen() here.  Additional linking flags
# will be added to support run time loading of shared objects.
USES_DLOPEN = 

# include common build makefile rules
ifndef SUITEROOT
$(error please set SUITEROOT and try again)
endif
include $(SUITEROOT)/scripts/build/build.mk

####################################################################
# Directory-specific make rules 
####################################################################
