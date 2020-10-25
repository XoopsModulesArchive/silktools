#######################################################-*- makefile -*-##
# TOP LEVEL MAKEFILE FOR SiLK ANALYSIS SUITE
# Makefile,v 1.10 2004/01/20 17:32:24 kompanea Exp
#########################################################################

# Default target: build everything
default :: all

clean ::

#########################################################################
#  VARIABLE DEFINITIONS
#########################################################################

# Set SUITEROOT if not set
ifndef SUITEROOT
export SUITEROOT = $(CURDIR)
endif


# Analysis Applications: to install into bin
TARGETS_APP = buildset readset rwaddrcount rwcount rwcut rwfilter rwset \
	 rwset-union rwsort rwstats rwtotal rwuniq rwuniq-new setintersect

# Support Utilities: to install into bin
TARGETS_UTIL = num2dot rwappend rwcat rwfileinfo rwfglob \
	 rwrandomizeip rwswapbytes mapsid

# Packing tools: to install into bin
TARGETS_PACK = rwflowpack rwrtd2split

# Additional items to install into bin
TARGETS_EXTRA = 

# Dynamic libraries: to install into lib
TARGETS_LIB = libipfilter.so librwslammer.so librwsynackfin.so

# Support files: to install into share
TARGETS_DATA =

# Libraries to build but are not installed
TARGETS_SUPPORT_LIB = libdynlib.a libfglob.a libfilter.a libhash.a \
	libheap.a libinterval.a libiochecks.a liblog.a \
	librw.a libutils.a

# Testing programs are not installed
TARGETS_TEST = \
	src/ds/heaplib/heaplib-test \
	src/util/filter/testFilter \
	src/rw/rwcount/counttest \
	src/rw/rwdynlibs/rwslammer-test \
	src/rw/rwdynlibs/rwsynackfin-test \
	src/util/utils/options-movedynlib-test \
	src/util/utils/options-parse-test


# Directories to install into for public use.  The makefile does not
# install to these directories, but it will compare the contents of
# the current subdirectories with the contents of these directories
SILK_INSTALL_ROOT = ~silk
SILK_INSTALL_BIN   = $(SILK_INSTALL_ROOT)/bin
SILK_INSTALL_LIB   = $(SILK_INSTALL_ROOT)/share/lib
SILK_INSTALL_SHARE = $(SILK_INSTALL_ROOT)/share


# Options for recursive make
MAKE_RECURSIVE_OPT = 
ifndef EMACS
MAKE_RECURSIVE_OPT += --no-print-directory
endif


#########################################################################
# See if this site adds additional targets
#########################################################################

-include site-extras.mk


#########################################################################
# Autoconf things
#########################################################################

src/include/silk_config.h: src/include/silk_config.h.in
	@echo 
	@echo '    You must run configure'
	@echo
	exit 1

src/include/silk_config.h.in:
	@echo 
	@echo '    You must run autoheader and autoconf.'
	@echo '    Make certain the environment variable M4 is set Gnu m4'
	@echo
	exit 1


#########################################################################
# Use above to make list of files to build
#########################################################################

BIN_TGT = $(foreach x,$(TARGETS_APP) $(TARGETS_UTIL) $(TARGETS_PACK) $(TARGETS_EXTRA),bin/$(x))

LIB_TGT = $(foreach x,$(TARGETS_LIB),lib/$(x))

SHARE_TGT = $(foreach x,$(TARGETS_DATA),share/$(x))

LOCAL_LIB_TGT = $(foreach x,$(TARGETS_SUPPORT_LIB),lib/$(x))

TGTS = $(BIN_TGT) $(LIB_TGT) $(SHARE_TGT)


#########################################################################
#  VIRTUAL TARGETS
#########################################################################

.PHONY: default install all clean uninstall check-silk \
	 check-silk-bin check-silk-lib check-silk-share \
	 internal-check-silk $(TGTS) $(LOCAL_LIB_TGT)

# default target: build everything
all :: src/include/silk_config.h $(LOCAL_LIB_TGT) $(TGTS) $(TARGETS_TEST)


#########################################################################
#  SILK INSTALL CHECKS
#########################################################################

#  These targets are used to compare the contents of bin, lib, and
#  share subdirectories of the current directory with the files
#  installed in ~silk

check-silk: check-silk-bin check-silk-lib check-silk-share

check-silk-bin: $(SILK_INSTALL_BIN)
	@$(MAKE) $(MAKE_RECURSIVE_OPT) internal-check-silk SILK_DIR=$^ \
	    SILK_TARGETS="$(foreach x,$(TARGETS_APP) $(TARGETS_UTIL),bin/$(x))"

check-silk-lib: $(SILK_INSTALL_LIB)
	@$(MAKE) $(MAKE_RECURSIVE_OPT) internal-check-silk SILK_DIR=$^ \
	    SILK_TARGETS="$(foreach x,$(TARGETS_LIB),lib/$(x))"

check-silk-share: $(SILK_INSTALL_SHARE)
	@$(MAKE) $(MAKE_RECURSIVE_OPT) internal-check-silk SILK_DIR=$^ \
	    SILK_TARGETS="$(foreach x,$(TARGETS_DATA),share/$(x))"

# Used for recursive make
internal-check-silk:
	@for source in $(SILK_TARGETS) /dev/null ; do \
	  if [ "$$source" != /dev/null -a -f "$$source" ]; then \
	    target=`dirname $(SILK_DIR)`/"$$source" ; \
	    if cmp -s "$$target" "$$source" ; then \
	      : ; \
	    else \
	      echo "**** $$source ****" ; \
	      if [ -f "$$target" ]; then \
	        ls -l "$$target" "$$source" ; \
	      else \
                echo "$$target does not exist" ; \
	      fi ; \
            fi ; \
	  fi ; \
	done


#########################################################################
#  CLEANING TARGETS
#########################################################################

uninstall:: clean

clean ::
	-rm -f bin/* lib/*
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/ds/hashlib -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/ds/heaplib -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/librw -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwaddrcount -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwappend -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcat -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcount -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcut -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwdynlibs -f Makefile-rwslammer clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwdynlibs -f Makefile-rwsynackfin clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwfilter -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwinfo -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwpack -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwrandomizeip -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwsort -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwstats -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwswapbytes -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwtotal -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwuniq -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/dynlib -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/fglob -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/filter -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/iochecks -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/log -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/num2dot -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/stats -f Makefile clean
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/utils -f Makefile clean


#########################################################################
#  FILE TARGETS
#########################################################################

bin/mapsid:
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/librw/ -f Makefile install-appnames-release APPNAMES='mapsid'

bin/buildset :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile install-appnames-release APPNAMES='buildset'

bin/num2dot :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/num2dot -f Makefile install-appnames-release APPNAMES='num2dot'

bin/readset :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile install-appnames-release APPNAMES='readset'

bin/rwaddrcount :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwaddrcount -f Makefile install-appnames-release APPNAMES='rwaddrcount'

bin/rwappend :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwappend -f Makefile install-appnames-release APPNAMES='rwappend'

bin/rwcat :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcat -f Makefile install-appnames-release APPNAMES='rwcat'

bin/rwcount :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcount -f Makefile install-appnames-release APPNAMES='rwcount'

bin/rwcut :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcut -f Makefile install-appnames-release APPNAMES='rwcut'

bin/rwfglob :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/fglob -f Makefile install-appnames-release APPNAMES='rwfglob'

bin/rwfileinfo :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwinfo -f Makefile install-appnames-release APPNAMES='rwfileinfo'

bin/rwfilter :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwfilter -f Makefile install-appnames-release APPNAMES='rwfilter'

bin/rwflowpack :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwpack -f Makefile install-appnames-release APPNAMES='rwflowpack'

bin/rwrandomizeip :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwrandomizeip -f Makefile install-appnames-release APPNAMES='rwrandomizeip'

bin/rwrtd2split :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/librw -f Makefile install-appnames-release APPNAMES='rwrtd2split'

bin/rwset :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile install-appnames-release APPNAMES='rwset'

bin/rwset-union :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile install-appnames-release APPNAMES='rwset-union'

bin/rwsort :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwsort -f Makefile install-appnames-release APPNAMES='rwsort'

bin/rwstats :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwstats -f Makefile install-appnames-release APPNAMES='rwstats'

bin/rwswapbytes :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwswapbytes -f Makefile install-appnames-release APPNAMES='rwswapbytes'

bin/rwtotal :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwtotal -f Makefile install-appnames-release APPNAMES='rwtotal'

bin/rwuniq :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwuniq -f Makefile install-appnames-release APPNAMES='rwuniq'

bin/rwuniq-new :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwuniq -f Makefile install-appnames-release APPNAMES='rwuniq-new'

bin/setintersect :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile install-appnames-release APPNAMES='setintersect'

lib/libdynlib.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/dynlib -f Makefile install-libraries-release LIBRARIES='dynlib'

lib/libfglob.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/fglob -f Makefile install-libraries-release LIBRARIES='fglob'

lib/libfilter.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/filter -f Makefile install-libraries-release LIBRARIES='filter'

lib/libhash.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/ds/hashlib -f Makefile install-libraries-release LIBRARIES='hash'

lib/libheap.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/ds/heaplib -f Makefile install-libraries-release LIBRARIES='heap'

lib/libinterval.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/stats -f Makefile install-libraries-release LIBRARIES='interval'

lib/libiochecks.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/iochecks -f Makefile install-libraries-release LIBRARIES='iochecks'

lib/libipfilter.so :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwset -f Makefile install-dynalibs-release DYNALIBS='ipfilter'

lib/liblog.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/log -f Makefile install-libraries-release LIBRARIES='log'

lib/librw.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/librw -f Makefile install-libraries-release LIBRARIES='rw'

lib/librwslammer.so :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwdynlibs -f Makefile-rwslammer install-dynalibs-release DYNALIBS='rwslammer'

lib/librwsynackfin.so :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwdynlibs -f Makefile-rwsynackfin install-dynalibs-release DYNALIBS='rwsynackfin'

lib/libutils.a :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/utils -f Makefile install-libraries-release LIBRARIES='utils'

src/ds/heaplib/heaplib-test :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/ds/heaplib -f Makefile testapps-release TESTAPPS='heaplib-test'

src/rw/rwcount/counttest :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwcount -f Makefile testapps-release TESTAPPS='counttest'

src/rw/rwdynlibs/rwslammer-test :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwdynlibs -f Makefile-rwslammer testapps-release TESTAPPS='rwslammer-test'

src/rw/rwdynlibs/rwsynackfin-test :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/rw/rwdynlibs -f Makefile-rwsynackfin testapps-release TESTAPPS='rwsynackfin-test'

src/util/filter/testFilter :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/filter -f Makefile testapps-release TESTAPPS='testFilter'

src/util/stats/descrip :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/stats -f Makefile testapps-release TESTAPPS='descrip'

src/util/utils/options-movedynlib-test :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/utils -f Makefile testapps-release TESTAPPS='options-movedynlib-test'

src/util/utils/options-parse-test :
	$(MAKE) $(MAKE_RECURSIVE_OPT) -C src/util/utils -f Makefile testapps-release TESTAPPS='options-parse-test'

