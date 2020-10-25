#ifndef _SILK_H
#define _SILK_H
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
** 1.5
** 2004/03/11 15:01:18
** thomasm
*/

/*@unused@*/ static char rcsID_SILK_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "silk.h,v 1.5 2004/03/11 15:01:18 thomasm Exp";

/* Our first pass at autoconfiscation */
#include "silk_config.h"

/* We use Posix integer types everywhere */
#include <inttypes.h>

/* Defines generic file header and supported file types */
#include "silk_files.h"

/* Name of environment variable pointing to the root of install */
#define ENV_SILK_PATH "SILK_PATH"

/* First look for plugins in this subdir of $SILK_PATH; if that fails,
 * use platform's default (LD_LIBRARY_PATH or similar). */
#define SILK_SUBDIR_PLUGINS "share/lib"

/* Subdirectory of $SILK_PATH for support files */
#define SILK_SUBDIR_SUPPORT "share"

/* Define an endianness-aware ip address structure. */
typedef union {
#if IS_LITTLE_ENDIAN
  struct {
    uint8_t o4;
    uint8_t o3;
    uint8_t o2;
    uint8_t o1;
  } O;
#else
  struct {
    uint8_t o1;
    uint8_t o2;
    uint8_t o3;
    uint8_t o4;
  } O;
#endif    
  uint32_t ipnum;
} ipUnion;

/* Bit-swapping macros for changing endianness */
#define BSWAP16(a) ((( (a) & 0xFF00) >> 8) | (( (a) & 0x00FF) << 8))
#define BSWAP32(a) ((((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)&0xff0000)>>8) | (((a)>>24)&0xff))

#ifdef __GNUC__
#define RCSIDENT(id) /*@unused@*/ static char *_rcsID __attribute__ ((__unused__)) = (id)
#define UNUSED(var) /*@unused@*/ var __attribute__((__unused__))
#else
#define RCSIDENT(id) /*@unused@*/ static char *_rcsID = (id)
#define UNUSED(var) /*@unused@*/ var
#endif

#endif /* _SILK_H */
