#ifndef _CUT_H
#define _CUT_H
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
** 1.11
** 2004/02/18 15:07:22
** thomasm
*/

/*@unused@*/ static char rcsID_CUT_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "cut.h,v 1.11 2004/02/18 15:07:22 thomasm Exp";


/*
**  cut.h
**
**  Header file for the rwcut application.  See rwcut.c for a full
**  explanation.
**
*/

#include "silk.h"
#include "utils.h"
#include "rwpack.h"
#include "iochecks.h"
#include "fglob.h"
#include "dynlib.h"

/* With the shift to a "pure" filter, cut needs NO args */
#define	NUM_REQUIRED_ARGS	0

typedef struct flags {
  uint8_t swapFlag;
  uint8_t dottedip;
  uint8_t printTitles;
  uint8_t timestamp;
  uint8_t fromEOF;
  uint8_t rulesCheck;
  uint8_t printFileHeader;
  uint8_t printFileName;
  uint8_t delimited;
  uint8_t unmapPort;
  uint8_t icmpTandC;
  uint8_t integerSensor;
} flagsStruct;

typedef struct countersStruct {
  uint32_t firstRec;
  uint32_t lastRec;
  uint32_t numRecs;
} countersStruct;

/* function that plugins must supply */
typedef int (*cutf_t)(unsigned int, char *, size_t, rwRecPtr);

/* Internal interface to libdynlib */
typedef struct {
  dynlibInfoStructPtr dlisp; /* the libdynlib object */
  unsigned int offset;       /* number of field just before this dynlib */
  cutf_t fxn;                /* function to get output from dynlib */
} cutDynlib_t;


/* rwcututils.c */
extern uint8_t maxFieldsSelected;
extern uint8_t *fieldPositions;
extern uint8_t *fWidths;
extern const char *fieldTitles[];

/* rwcut.c */
extern rwRec rwrec;
extern rwIOStructPtr rwIOSPtr;
extern iochecksInfoStructPtr ioISP;
extern countersStruct fileCounters;
extern flagsStruct flags;
extern char delimiter;

/* Maximum number of dynamic libraries that rwcut can open */
#define CUT_MAX_DYNLIBS 8

/* List of dynamic libraries to attempt to open at startup */
extern const char *cutDynlibNames[];

/* Dynamic libraries we actually opened */
extern cutDynlib_t cutDynlibList[CUT_MAX_DYNLIBS];

/* Number of dynamic libraries actually opened */
extern int cutDynlibCount;


char *dumpPort(int sportFlag, uint16_t port, uint8_t proto);
void writeColTitles(void);
void dumprec(void);
void appTeardown(void);
void appSetup(int argc, char **argv);	/* never returns */
void setCounters(void);


cutDynlib_t* cutFieldNum2Dynlib(const unsigned int fieldNum);
/*
 *    Given a field number, return a pointer to a cutDynlib_t from the
 *    cutDynlibList[] that points to the dynamic library used to print
 *    the fieldNum field.  Return NULL if the fieldNum is not related
 *    to any dynamic library.
 */


#endif /* _CUT_H */
