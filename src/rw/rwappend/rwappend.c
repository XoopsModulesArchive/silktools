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
** 1.10
** 2004/03/10 22:11:59
** thomasm
*/


/*
**
**  rwappend.c
**
**  Suresh L Konda
**  8/10/2002
**      Append f2..fn to f1 as long as the headers match exactly.
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rwpack.h"
#include "utils.h"

RCSIDENT("rwappend.c,v 1.10 2004/03/10 22:11:59 thomasm Exp");


/* local defines and typedefs */
#define NUM_REQUIRED_ARGS       2
#define BUF_SIZE 8192

/* exported functions */
void appUsage(void);            /* never returns */

/* local functions */
static void appTeardown(void);
static void appSetup(int argc, char **argv);    /* never returns */

/* exported variables */

/* local variables */
static  int inoutFD, inFD;
static  rwFileHeaderV0 inoutHdr, inHdr;
static int exitVal = 0;

/*
 * appUsage:
 *      print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
void appUsage(void) {
  fprintf(stderr, "%s <input-fPath1> <input-fPath2> [<input-fPath3>...]\n",
          skAppName());
  exit(1);
}

/*
 * appTeardown:
 *      teardown all used modules and all application stuff.
 * Arguments: None
 * Returns: None
 * Side Effects:
 *      All modules are torn down. Then application teardown takes place.
 *      Global variable teardownFlag is set.
 * NOTE: This must be idempotent using static teardownFlag.
*/
static void appTeardown(void) {
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;
  if (inoutFD > -1)   close(inoutFD);
  if (inFD > -1)   close(inFD);
  inoutFD = -1;
  inFD = -1;
  return;
}

/*
 * appSetup
 *      do all the setup for this application include setting up required
 *      modules etc.
 * Arguments:
 *      argc, argv
 * Returns: None.
 * Side Effects:
 *      exits with code 1 if anything does not work.
*/
static void appSetup(int argc, char **argv) {

  skAppRegister(argv[0]);

  /* first do some sanity checks */
  if (argc <= NUM_REQUIRED_ARGS) {
    appUsage();         /* never returns */
  }
  inoutFD = inFD = -1;

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: registering appTeardown() failed\n",
            skAppName());
    appTeardown();
    exit(1);
  }

  return;                       /* OK */
}

int main(int argc, char **argv) {
  char *inoutPath, *inPath;
  ssize_t rLen, wLen;           /* read/write lengths */
  unsigned char *buffer;
  unsigned int cumRead, cumWrite;
  int curFileIndex;
  int thisExitVal;

  appSetup(argc, argv);                 /* never returns */

  inoutPath = argv[1];
  if (strstr(inoutPath, ".gz")) {
    fprintf(stderr, "File 1 %s cannot be a compressed file\n", inoutPath);
    exit(1);
  }
  if (! fileExists(inoutPath) ) {
   fprintf(stderr, "File 1 %s does not exist\n", inoutPath);
   exit(1);
  }
  if ( (inoutFD = open(inoutPath, O_RDWR)) < 0) {
    fprintf(stdout, "unable to open %s: [%s]\n", inoutPath, strerror(errno));
    thisExitVal = 1;
    exit(1);
  }


  if (read(inoutFD, &inoutHdr, sizeof(inoutHdr)) < (ssize_t)sizeof(inoutHdr)) {
    fprintf(stderr, "File %s too small\n", inoutPath);
    exit(1);
  }

  /* ok - skip to eof on inoutFD */
  if ( lseek(inoutFD, 0, SEEK_END) < 0) {
    fprintf(stderr, "Unable to skip to EOF on %s\n", inoutPath);
    exit(1);
  }
  buffer = (unsigned char *)malloc(BUF_SIZE);
  cumWrite = 0;

  for (curFileIndex = 2; curFileIndex < argc; curFileIndex++ ) {
    thisExitVal = 0;
    inPath = argv[curFileIndex];

    if (strstr(inPath, ".gz")) {
      fprintf(stderr, "File 2 %s cannot be a compressed file\n", inPath);
      thisExitVal = 1;
      continue;
    }

    if (strcmp(inoutPath, inPath) == 0) {
      fprintf(stderr, "File 1 and File 2 identical\n");
      thisExitVal = 1;
      continue;
    }

    if (! fileExists(inPath) ) {
      fprintf(stderr, "File 2 %s does not exist\n", inPath);
      thisExitVal = 1;
      continue;
    }

    inFD = open(inPath, O_RDONLY);
    if ( (inFD = open(inPath, O_RDONLY)) < 0) {
      fprintf(stdout, "unable to open %s: [%s]\n", inPath, strerror(errno));
      thisExitVal = 1;
      continue;
    }

    if (read(inFD, &inHdr, sizeof(inHdr)) < (ssize_t)sizeof(inHdr)) {
      fprintf(stderr, "File %s too small\n", inPath);
      thisExitVal = 1;
      continue;
    }

    if (memcmp(&inoutHdr, &inHdr, sizeof(inHdr))) {
      fprintf(stderr, "Headers do not match. Abort\n");
      thisExitVal = 1;
      continue;
    }

    cumRead = sizeof(inHdr);
    while (1) {
      rLen = read(inFD, buffer, BUF_SIZE);
      if (rLen < 0) {
        /* io error */
        fprintf(stderr, "IO error [%s] after reading %d bytes\n",
                strerror(errno), cumRead);
      } else if (rLen == 0 ) {
        break;                  /* EOF */
      }
      cumRead += rLen;
      wLen = write(inoutFD, buffer, rLen);
      if (wLen != rLen) {
        fprintf(stderr, "write/read mismatch: %u vs. %u after %u: %s\n",
                (uint32_t)wLen, (uint32_t)rLen, (uint32_t)cumWrite,
                strerror(errno));
        break;
      }
      cumWrite += wLen;
    } /* inner read loop */
    exitVal += thisExitVal;
    close(inFD);
    inFD = -1;
  } /* outer for loop */

  /* done with all files */
  fprintf(stdout, "Appended %d bytes to %s\n", cumWrite, inoutPath);
  appTeardown();
  exit(exitVal);
}
