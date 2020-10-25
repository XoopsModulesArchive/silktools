dnl Copyright (C) 2003 by Carnegie Mellon University.
dnl
dnl @OPENSOURCE_HEADER_START@
dnl 
dnl Use of the SILK system and related source code is subject to the terms 
dnl of the following licenses:
dnl 
dnl GNU Public License (GPL) Rights pursuant to Version 2, June 1991
dnl Government Purpose License Rights (GPLR) pursuant to DFARS 252.225-7013
dnl 
dnl NO WARRANTY
dnl 
dnl ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER 
dnl PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY 
dnl PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN 
dnl "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY 
dnl KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT 
dnl LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE, 
dnl MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE 
dnl OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT, 
dnl SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY 
dnl TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF 
dnl WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES. 
dnl LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF 
dnl CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON 
dnl CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE 
dnl DELIVERABLES UNDER THIS LICENSE.
dnl 
dnl Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie 
dnl Mellon University, its trustees, officers, employees, and agents from 
dnl all claims or demands made against them (and any related losses, 
dnl expenses, or attorney's fees) arising out of, or relating to Licensee's 
dnl and/or its sub licensees' negligent use or willful misuse of or 
dnl negligent conduct or willful misconduct regarding the Software, 
dnl facilities, or other rights or assistance granted by Carnegie Mellon 
dnl University under this License, including, but not limited to, any 
dnl claims of product liability, personal injury, death, damage to 
dnl property, or violation of any laws or regulations.
dnl 
dnl Carnegie Mellon University Software Engineering Institute authored 
dnl documents are sponsored by the U.S. Department of Defense under 
dnl Contract F19628-00-C-0003. Carnegie Mellon University retains 
dnl copyrights in all material produced under this contract. The U.S. 
dnl Government retains a non-exclusive, royalty-free license to publish or 
dnl reproduce these documents, or allow others to do so, for U.S. 
dnl Government purposes only pursuant to the copyright license under the 
dnl contract clause at 252.227.7013.
dnl 
dnl @OPENSOURCE_HEADER_END@

dnl 1.4
dnl 2004/03/10 20:09:40
dnl thomasm

AC_INIT([src/include/silk.h])
AC_CONFIG_HEADERS([src/include/silk_config.h])

AH_TOP([
#ifndef _SILK_H
#error "Do not include this file directly!.  include silk.h instead"
#endif

static char rcsID_SILK_CONFIG_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "configure.ac,v 1.4 2004/03/10 20:09:40 thomasm Exp";
])

AH_BOTTOM([
#ifndef WORDS_BIGENDIAN
#define IS_LITTLE_ENDIAN 1
#define IS_BIG_ENDIAN 0
#else
#define IS_LITTLE_ENDIAN 0
#define IS_BIG_ENDIAN 1
#endif])



AC_PROG_CPP

AC_PROG_EGREP

AC_CHECK_HEADER([sys/msg.h],[AC_DEFINE(SK_HAVE_SYS_MSG_H,[1],[Define to 1 if your system provides <sys/msg.h>])])

AC_MSG_CHECKING([if <errno.h> defines EPROTO])
AC_EGREP_CPP([eproto is EPROTO],[#include <errno.h>
eproto is EPROTO],[AC_MSG_RESULT([no])],[AC_MSG_RESULT([yes])
AC_DEFINE(SK_HAVE_EPROTO,[1],[Define to 1 if your system has error code EPROTO])])

AC_MSG_CHECKING([if <glob.h> defines GLOB_NOMATCH])
AC_EGREP_CPP([glob_nomatch is GLOB_NOMATCH],[#include <glob.h>
glob_nomatch is GLOB_NOMATCH],[AC_MSG_RESULT([no])],[AC_MSG_RESULT([yes])
AC_DEFINE(SK_HAVE_GLOB_NOMATCH,[1],[Define to 1 if your system's glob may return GLOB_NOMATCH])])

AC_MSG_CHECKING([if <glob.h> defines GLOB_ABORTED])
AC_EGREP_CPP([glob_aborted is GLOB_ABORTED],[#include <glob.h>
glob_aborted is GLOB_ABORTED],[AC_MSG_RESULT([no])],[AC_MSG_RESULT([yes])
AC_DEFINE(SK_HAVE_GLOB_ABORTED,[1],[Define to 1 if your system's glob may return GLOB_ABORTED])])

AC_CHECK_FUNC(getopt_long_only,[],[AC_DEFINE(SK_NEED_GETOPT,[1],[Define to 1 if your platform does not provide getopt_long_only])])

AC_CHECK_FUNC(timegm,[AC_DEFINE(SK_HAVE_TIMEGM,[1],[Define to 1 if your system provides timegm()])])

AC_C_BIGENDIAN

dnl would like to do this, but don't know what it may break
AC_SYS_LARGEFILE

AC_MSG_CHECKING([for SILK_DATA_ROOTDIR environment variable])
case x"$SILK_DATA_ROOTDIR" in
x)
    sk_datadir_prog=guessdatadir.sh
    if test -f $sk_datadir_prog ; then
      SILK_DATA_ROOTDIR=`/bin/sh $sk_datadir_prog 2>/dev/null`
      export SILK_DATA_ROOTDIR
    fi
    ;;
esac
case x"$SILK_DATA_ROOTDIR" in
x)
    AC_ERROR([You must provide the location of the packed data files.
Run configure as
  env SILK_DATA_ROOTDIR=<full-dir-path> configure <configure-options>])
    ;;
esac
AC_MSG_RESULT([$SILK_DATA_ROOTDIR])

AC_DEFINE_UNQUOTED(SILK_DATA_ROOTDIR,"$SILK_DATA_ROOTDIR",[Path to root directory of packed data files])


AC_OUTPUT
