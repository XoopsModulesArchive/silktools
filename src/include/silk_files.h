/*
** Copyright (C) 2001-2004 by Carnegie Mellon University.
**
** @OPENSOURCE_HEADER_START@
** 
** Use of the SILK system and related source code is subject to the terms 
** of the following licenses:
** 
** GNU Public License (GPL) Rights pursuant to Version 2, June 1991
** Government Purpose License Rights (GPLR) pursuant to DFARS 252.225-7013
** 
** NO WARRANTY
** 
** ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER 
** PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY 
** PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN 
** "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY 
** KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT 
** LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE, 
** MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE 
** OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT, 
** SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY 
** TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF 
** WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES. 
** LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF 
** CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON 
** CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE 
** DELIVERABLES UNDER THIS LICENSE.
** 
** Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie 
** Mellon University, its trustees, officers, employees, and agents from 
** all claims or demands made against them (and any related losses, 
** expenses, or attorney's fees) arising out of, or relating to Licensee's 
** and/or its sub licensees' negligent use or willful misuse of or 
** negligent conduct or willful misconduct regarding the Software, 
** facilities, or other rights or assistance granted by Carnegie Mellon 
** University under this License, including, but not limited to, any 
** claims of product liability, personal injury, death, damage to 
** property, or violation of any laws or regulations.
** 
** Carnegie Mellon University Software Engineering Institute authored 
** documents are sponsored by the U.S. Department of Defense under 
** Contract F19628-00-C-0003. Carnegie Mellon University retains 
** copyrights in all material produced under this contract. The U.S. 
** Government retains a non-exclusive, royalty-free license to publish or 
** reproduce these documents, or allow others to do so, for U.S. 
** Government purposes only pursuant to the copyright license under the 
** contract clause at 252.227.7013.
** 
** @OPENSOURCE_HEADER_END@
*/

/*
** 1.8
** 2004/01/30 19:26:18
** thomasm
*/

#ifndef _SILK_H
#error "Never include silk_files.h directly, include silk.h instead."
#endif

/*@unused@*/ static char rcsID_SILK_FILES_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "silk_files.h,v 1.8 2004/01/30 19:26:18 thomasm Exp";

/* in order to be byte order independent, define 4 bytes size magic values */


#define MAGIC1  0xDE
#define MAGIC2  0xAD
#define MAGIC3  0xBE
#define MAGIC4  0xEF
#define BIGEND_MAGIC 0xDEADBEEF
#define LITTLEEND_MAGIC 0xEFBEADDE


/* this struct is invariant over all file types we create */
typedef struct {
  uint8_t magic1;       /* fixed byte order 4byte magic number */
  uint8_t magic2;
  uint8_t magic3;
  uint8_t magic4;
  uint8_t isBigEndian;  /* endian order on hw creating file 1/0 big/little */
  uint8_t type;         /* file type */
  uint8_t version;      /* version of above */
  uint8_t cLevel;       /* compression level */
} genericHeader;
typedef genericHeader * genericHeaderPtr;

/* macros for dealing with the header */
#define PREPHEADER(h) {  (h)->magic1 = MAGIC1;  (h)->magic2 = MAGIC2;  (h)->magic3 = MAGIC3;  (h)->magic4 = MAGIC4; (h)->isBigEndian = IS_BIG_ENDIAN;(h)->cLevel = 0;}

#define CHECKMAGIC(h) (  (h)->magic1 != MAGIC1 ||  (h)->magic2 != MAGIC2 ||  (h)->magic3 != MAGIC3 ||  (h)->magic4 != MAGIC4 )

/* define various file types here that we create */

#define FT_TCPDUMP      0x00
#define FT_GATEWAY      0x01
#define FT_ADDRESSES    0x02
#define FT_PORTMAP      0x03
#define FT_SERVICEMAP   0x04
#define FT_NIDSMAP      0x05
#define FT_GWCOUNTS     0x06
#define FT_TDCOUNTS     0x07
#define FT_GWFILTER     0x08
#define FT_TDFILTER     0x09
#define FT_GWSCAN1      0x0A
#define FT_TDSCAN1      0x0B
#define FT_GWSCAN2      0x0C
#define FT_TDSCAN2      0x0D
#define FT_GWADDRESS    0x0E
#define FT_GWSCANMATIRX 0x0F
#define FT_RWROUTED     0X10    /* raw routed */
#define FT_RWNOTROUTED  0X11    /* raw not routed */
#define FT_RWSPLIT      0X12    /* raw split after r/nr */
#define FT_RWFILTER     0X13    /* raw split after r/nr */
#define FT_GWDNS        0X14
#define FT_GWWWW        0X15
#define FT_RWGENERIC    0x16
#define FT_GWGENERIC    0x17
#define FT_RWDAILY      0x18
#define FT_RWSCAN       0x19
#define FT_RWACL        0x1A
#define FT_RWCOUNT      0x1B
#define FT_FLOWCAP      0x1C
#define FT_MACROTREE    0x1D
#define FT_MICROTREE    0x1E
#define FT_RWWWW        0x1F
#define FT_SHUFFLE      0x20

#define GW_REC_TYPE     0
#define TD_REC_TYPE     1

/* This header is included by filetype.c and rwfileinfo.c after
 * declaring DECLARE_FILETYPE_VARIABLES. FIXME. Refactor the code so
 * there's function that an application can call to map a file type
 * code to a string. */
#ifdef DECLARE_FILETYPE_VARIABLES
char *fileTypes[] = {
  "FT_TCPDUMP",
  "FT_GATEWAY",
  "FT_ADDRESSES",
  "FT_PORTMAP",
  "FT_SERVICEMAP",
  "FT_NIDSMAP",
  "FT_GWCOUNTS",
  "FT_TDCOUNTS",
  "FT_GWFILTER",
  "FT_TDFILTER",
  "FT_GWSCAN1",
  "FT_TDSCAN1",
  "FT_GWSCAN2",
  "FT_TDSCAN2",
  "FT_GWADDRESS",
  "FT_GWSCANMATIRX",
  "FT_RWROUTED",
  "FT_RWNOTROUTED",
  "FT_RWSPLIT",
  "FT_RWFILTER",
  "FT_GWDNS",
  "FT_GWWWW",
  "FT_RWGENERIC",
  "FT_GWGENERIC",
  "FT_RWDAILY",
  "FT_RWSCAN",
  "FT_RWACL",
  "FT_RWCOUNT",
  "FT_FLOWCAP",
  "FT_MACROTREE",
  "FT_MICROTREE",
  "FT_RWWWW",
  "FT_SHUFFLE",
  "",
};

int ftypeCount = 33;
#endif


