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
** 1.14
** 2004/02/18 15:07:23
** thomasm
*/

/*
** rwcut.c
**
**      cut fields/records from the given input file(s) using field
**      specifications from here, record filter specifications from
**      module libfilter, and file selection specifications from
**      libutils (fglob module).
**
** 1/15/2002
**
** Suresh L. Konda
**
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cut.h"

RCSIDENT("rwcut.c,v 1.14 2004/02/18 15:07:23 thomasm Exp");


/* exported variables */
rwRec rwrec;
rwIOStructPtr rwIOSPtr;
iochecksInfoStructPtr ioISP;
countersStruct fileCounters;
flagsStruct flags;
char delimiter;


/* List of dynamic libraries to attempt to open at startup */
const char *cutDynlibNames[] = {
  NULL /* sentinel */
};

/* Dynamic libraries we actually opened */
cutDynlib_t cutDynlibList[CUT_MAX_DYNLIBS];

/* Number of dynamic libraries actually opened */
int cutDynlibCount;



/* local functions */
static void cutFile(char *inFName);

/* local variables */

int main(int argc, char **argv) {
  int counter;
  char * curFName;

  appSetup(argc, argv);                 /* never returns */

  /* get files to process from the globber or the command line */
  if (fglobValid()) {
    while ((curFName = fglobNext())){
      cutFile(curFName);
    }
  } else {
    for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
      cutFile(ioISP->fnArray[counter]);
    }
  }

  /* done */
  appTeardown();
  exit(0);
  return 0;                     /* keep gcc happy */
}


static void cutFile(char *inFName) {
#if     0                       /* for debugging only */
  fprintf(stdout, "Processing %s\n", inFName);
  return;
#endif
  if ( (rwIOSPtr = rwOpenFile(inFName, ioISP->inputCopyFD))
       == (rwIOStructPtr)NULL) {
    fprintf(stderr, "%s: unable to open %s\n", skAppName(), inFName);
    exit(1);
  }

  /*
    we have to swap the bytes of the ip number for inet_ntoa() if
    a. this is little endian and the writer was little endian or
    b. this is big endian and the writer was  little endian.
  */

#if     IS_LITTLE_ENDIAN
  flags.swapFlag = rwGetIsBigEndian(rwIOSPtr) == 0;
#else
  flags.swapFlag = rwGetIsBigEndian(rwIOSPtr) != 1;
#endif

  if (flags.printFileName) {
    fprintf(stderr, "%s\n", baseName(inFName));
  }

  /* print the file header information if requested */
  if (flags.printFileHeader) {
    rwPrintHeader(rwIOSPtr, stderr);
  }
  setCounters();

  if (fileCounters.numRecs > 0) {
    writeColTitles();
    if (fileCounters.firstRec) {
      if (rwSkip(rwIOSPtr, fileCounters.firstRec)) {
        appTeardown();
        exit(1);
      }
    }
    while (fileCounters.numRecs--  && rwRead(rwIOSPtr, &rwrec)) {
      dumprec();
    }
  }
  rwCloseFile(rwIOSPtr);
  rwIOSPtr = (rwIOStructPtr)NULL;
}

