/*
**  Copyright (C) 2001-2004 by Carnegie Mellon University.
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
** 2004/03/10 21:38:58
** thomasm
*/

/*
 * rwrtd2split
 *
 *  Read an input file in RWROUTED format and write the records to a
 *  new file in RWSPLIT format.
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "rwpack.h"
#include "utils.h"
#include "filter.h"

RCSIDENT("rwrtd2split.c,v 1.11 2004/03/10 21:38:58 thomasm Exp");



/* local defines and typedefs */
#define	NUM_REQUIRED_ARGS	2


/* local functions */
static void appUsage(void);		/* never returns */
static void appSetup(int argc, char **argv);

/* exported variables */

/* local variables */
static char *inFPath;
static char *outFPath;
static FILE *outFD;
rwIOStructPtr rwIOSPtr;


/*
 * appUsage:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
static void appUsage(void) {
  fprintf(stderr, "%s <inFile> <outFile> \n", skAppName());
  fprintf(stderr, "  Write the <outFile> in RWSPLIT format given the\n");
  fprintf(stderr, "  <inFile> in RWROUTED format.\n");
  exit(EXIT_FAILURE);
}


/*
 * appSetup
 *	do all the setup for this application include setting up required 
 *	modules etc.
 * Arguments:
 *	argc, argv
 * Returns: None.
 * Side Effects:
 *	exits with code 1 if anything does not work.
*/
static void appSetup(int argc, char **argv)
{
  skAppRegister(argv[0]);

  if ((argc - 1) < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
  }

  /* Open input file; check its type */
  inFPath = strdup(argv[1]);
  if (!inFPath) {
    fprintf(stderr, "%s: Out of memory (strdup)!\n", skAppName());
    exit(EXIT_FAILURE);
  }
  if (!fileExists(inFPath)) {
    fprintf(stderr, "%s: Non-existent input file %s\n", skAppName(), inFPath);
    exit(EXIT_FAILURE);
  }
  if ( (rwIOSPtr = rwOpenFile(inFPath, NULL)) == NULL) {
    exit(EXIT_FAILURE);
  }
  if (rwGetFileType(rwIOSPtr) != FT_RWROUTED) {
    fprintf(stderr, "%s: Input file %s not in RWROUTED format\n",
            skAppName(), inFPath);
    exit(EXIT_FAILURE);
  }

  /* Output output file */
  outFPath = strdup(argv[2]);
  if (!outFPath) {
    fprintf(stderr, "%s: Out of memory (strdup)!\n", skAppName());
    exit(EXIT_FAILURE);
  }
  if (fileExists(outFPath)) {
    fprintf(stderr, "%s: Output file %s exists.\n", skAppName(), outFPath);
    exit(EXIT_FAILURE);
  }
  if ( (outFD = fopen(outFPath, "w")) == NULL) {
    fprintf(stderr, "%s: Unable to open output file %s.\n", skAppName(), outFPath);
    exit(EXIT_FAILURE);
  }

  return;			/* OK */
}

int main(int argc, char **argv) {
  rwFileHeaderV0 rwFH;
  const size_t routedRecLen = 28;
  const size_t splitRecLen = 22;
  uint8_t ibuf[routedRecLen];
  uint8_t obuf[splitRecLen];
  int64_t rCount = 0;
  int64_t fSize = 0;

  /* Setup app: open input and output files; will exit(1) on error */
  appSetup(argc, argv);

  /* Copy the header; modify the file type */
  memcpy(&rwFH, rwGetHeader(rwIOSPtr), sizeof(rwFH));
  rwFH.gHdr.type = FT_RWSPLIT;

  /* We want the output to have the same byte order as the input, so
   * if we swapped the stime on read, swap it again for write. */
  if (rwIOSPtr->swapFlag) {
    rwFH.fileSTime = BSWAP32(rwFH.fileSTime);
  }

  /* write header */
  fwrite(&rwFH, sizeof(rwFH), 1, outFD);
  
  /* Process body */
  while (1) {
    if ( ! fread(&ibuf, routedRecLen, 1, rwIOSPtr->FD) ) {
      break;
    }
    rCount++;
    memcpy(&obuf[0], &ibuf, 8);	/* sip dip */
    memcpy(&obuf[8], &ibuf[12], 14); /* sPort, dPort, pef, sbb, proto, flags */
    fwrite(&obuf, splitRecLen, 1, outFD);
  }
  rwCloseFile(rwIOSPtr);
  fclose(outFD);

  fSize = rwIOSPtr->hdrLen + splitRecLen * rCount;
  if (fileSize(outFPath) != fSize) {
    fprintf(stderr, "ERROR: output filesize mismatch. Calc. %lld vs real %lld\n",
	    fileSize(outFPath), fSize);
    exit(EXIT_FAILURE);
  }

  /* done */
  return 0;
}



  
    
