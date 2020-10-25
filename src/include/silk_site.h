/*
**  Copyright (C) 2001,2002,2003 by Carnegie Mellon University.
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
** 1.4
** 2003/12/17 20:41:18
** thomasm
*/

#ifndef _SILK_SITE_H
#define _SILK_SITE_H

/* FIXME. Find a good home the structures declared in this file */

/*@unused@*/ static char rcsID_SILK_SITE_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "silk_site.h,v 1.4 2003/12/17 20:41:18 thomasm Exp";


/* Include packed file type structures */
#include "rwpack.h"


/* Find a home for these constants and structures definitions */
#define MAX_TYPES_PER_CLASS 12
#define FGLOB_MAX_DEFAULT_TYPES 4
#define MAX_CLASSES 2

/* For globbing: the root of the data tree */
#define _SILK_FGLOB_ROOTDIR SILK_DATA_ROOTDIR

#ifdef DECLARE_FGLOB_VARIABLES
char *silk_fglob_rootdir = _SILK_FGLOB_ROOTDIR;
#endif


typedef struct classInfoStruct {
  char *className;
  uint8_t numTypes;
  uint8_t numDefaults;
  uint8_t typeList[MAX_TYPES_PER_CLASS];
  uint8_t defaultList[FGLOB_MAX_DEFAULT_TYPES];
} classInfoStruct;

/* FIXME. Find a home for this */
typedef struct {
  char *sensorName;
  uint8_t numClasses;
  uint8_t classNumbers[MAX_CLASSES];
} sensorinfo_t;

/* A site-specific "implementation header", silk_site_*.h declares the
 * following statically populated deployment-specific global data
 * structures. */
extern fileTypeInfo outFInfo[];
extern uint32_t numFileTypes;
extern uint32_t numSensors;
extern sensorinfo_t sensorInfo[];
extern char *silk_fglob_rootdir;

/* For now, we just support the primary client */
#include "silk_site_generic.h"

#endif /* _SILK_SITE_H */
