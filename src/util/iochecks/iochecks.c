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
**
** 1.8
** 2004/03/10 22:31:33
** thomasm
*/

#include "silk.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "utils.h"
#include "iochecks.h"

RCSIDENT("iochecks.c,v 1.8 2004/03/10 22:31:33 thomasm Exp");


static   char *pseudoArgv[2] = {(char  *)NULL, (char *)NULL};

#define  ALLOW_STDERR	0

/*
** iochecksInit:
** 	malloc an iochecksInfo struct, initialize it, and return its ptr
*/
iochecksInfoStructPtr iochecksSetup(int maxPass, int maxFail, int argc,
				  char **argv) {
  iochecksInfoStructPtr ioISP;

  ioISP = (iochecksInfoStructPtr) malloc(sizeof(iochecksInfoStruct));
  memset(ioISP, 0, sizeof(iochecksInfoStruct));
  ioISP->maxPassDestinations = maxPass;
  ioISP->maxFailDestinations = maxFail;
  ioISP->argc = argc;
  ioISP->argv = argv;
  return ioISP;
}


/*
** iochecksPassDestinations:
** 	check that user options make sense. If they do, open the
**	files appropriately.
** Input:
**	iochecksInfoStructPtr ioISP;
**	char *fPath;
**	int ttyOK : it is OK for the file to be a tty
** Output:
**	1 if failed; 0 else
** Side Effects:
**
*/

int iochecksPassDestinations(iochecksInfoStructPtr ioISP, char *fPath, int ttyOK) {
  register int i = ioISP->passCount;	/* convenience variable */

  if (ioISP->passCount >= ioISP->maxPassDestinations) {
    fprintf(stderr, "Too many pass destinations.\n");
    return 1;
  }
  MALLOCCOPY(ioISP->passFPath[i], fPath);
  ioISP->passCount++;

  if (strcmp(ioISP->passFPath[i], "stdout") == 0 ) {
    if (!ttyOK && FILEIsATty(stdout)) {
      fprintf(stderr, "stdout is connected to a terminal. Abort\n");
      return 1;
    }
    if (ioISP->stdoutUsed) {
      fprintf(stderr, "stdout already allocated. Abort\n");
      return 1;
    }
    ioISP->stdoutUsed = 1;
    ioISP->passFD[i] = stdout;
    return 0;
  }
  if (strcmp(ioISP->passFPath[i], "stderr") == 0 ) {
#if	ALLOW_STDERR
    if (!ttyOK && FILEIsATty(stderr)) {
      fprintf(stderr, "stderr is connected to a terminal. Abort\n");
      return 1;
    }
    if (ioISP->stderrUsed) {
      fprintf(stderr, "stderr already allocated. Abort\n");
      return 1;
    }
    ioISP->stderrUsed = 1;
    ioISP->passFD[i] = stderr;
    return 0;
#else
    fprintf(stderr, "stderr not a valid output device. Abort\n");
    return 1;
#endif
  }

  if (strcmp(ioISP->passFPath[i], "/dev/null") == 0 ) {
    /* this is OK but do not open the file nor treat it as real file */
    free(ioISP->passFPath[i]);
    ioISP->passFPath[i] = NULL;
    ioISP->passCount--;
    ioISP->devnullUsed = 1;
    return 0;
  }

  if (fileExists(ioISP->passFPath[i])) {
    /*
      dont allow  existing non-null non-fifo files to be overwritten by
      accident
    */
    if (fileSize(ioISP->passFPath[i]) > 0 && !isFIFO(ioISP->passFPath[i])) {
      fprintf(stderr, "Output file [%s] exists.\n", ioISP->passFPath[i]);
      fprintf(stderr, "If you really want to overwrite this file, remove ");
      fprintf(stderr, "it manually and then re-run %s\n", skAppName());
      return 1;
    }
  }

  if (openFile(ioISP->passFPath[i], 1 /* write */, &ioISP->passFD[i],
	       &ioISP->passIsPipe[i])) {
    fprintf(stderr,"Unable to open output file %s\n", ioISP->passFPath[i]);
    return 1;
  }
  return 0;
}

/*
** iochecksFailDestinations:
** 	check that user options make sense. If they do, open the
**	files appropriately.
** Input:
**	iochecksInfoStructPtr ioISP;
**	char *fPath;
**	int ttyOK : it is OK for the file to be a tty
** Output:
**	0 if OK; 1 else.
** Side Effects:
**
*/


int iochecksFailDestinations(iochecksInfoStructPtr ioISP, char *fPath, int ttyOK) {
  register int i = ioISP->failCount;	/* convenience variable */

  if (ioISP->failCount >= ioISP->maxFailDestinations) {
    fprintf(stderr, "Too many fail destinations.\n");
    return 1;
  }
  MALLOCCOPY(ioISP->failFPath[i], fPath);
  ioISP->failCount++;

  if (strcmp(ioISP->failFPath[i], "stdout") == 0 ) {
    if (!ttyOK && FILEIsATty(stdout)) {
      fprintf(stderr, "stdout is connected to a terminal. Abort\n");
      return 1;
    }
    if (ioISP->stdoutUsed) {
      fprintf(stderr, "stdout already allocated. Abort\n");
      return 1;
    }
    ioISP->stdoutUsed = 1;
    ioISP->failFD[i] = stdout;
    return 0;
  }

  if (strcmp(ioISP->failFPath[i], "stderr") == 0 ) {
#if	ALLOW_STDERR
    if (!ttyOK && FILEIsATty(stderr)) {
      fprintf(stderr, "stderr is connected to a terminal. Abort\n");
      return 1;
    }
    if (ioISP->stderrUsed) {
      fprintf(stderr, "stderr already allocated. Abort\n");
      return 1;
    }
    ioISP->stderrUsed = 1;
    ioISP->failFD[i] = stderr;
    return 0;
#else
    fprintf(stderr, "stderr not a valid output device. Abort\n");
    return 1;
#endif
  }

  if (strcmp(ioISP->failFPath[i], "/dev/null") == 0 ) {
    /* this is OK but do not open the file nor treat it as real file */
    free(ioISP->failFPath[i]);
    ioISP->devnullUsed = 1;
    ioISP->failFPath[i] = NULL;
    ioISP->failCount--;
    return 0;
  }

  if (fileExists(ioISP->failFPath[i])) {
    /*
      dont allow  existing non-null non-fifo files to be overwritten by
      accident
    */
    if (fileSize(ioISP->failFPath[i]) > 0 && !isFIFO(ioISP->failFPath[i])) {
      fprintf(stderr, "Output file [%s] exists.\n", ioISP->failFPath[i]);
      fprintf(stderr, "If you really want to overwrite this file, remove ");
      fprintf(stderr, "it manually and then re-run %s\n", skAppName());
      return 1;
    }
  }

  if (openFile(ioISP->failFPath[i], 1 /* write */, &ioISP->failFD[i],
	       &ioISP->failIsPipe[i])) {
    fprintf(stderr,"Unable to open output file %s\n", ioISP->failFPath[i]);
    return 1;
  }
  return 0;
}


/*
** iochecksAllDestinations:
** 	check that user options make sense. If they do,
**	record the desired output descriptor in ioISP->inputCopyFD.
** Input:
**	iochecksInfoStructPtr ioISP;
**	char *fPath;
** Output:
**	0 if ok; 1 else;
*/

int iochecksAllDestinations(iochecksInfoStructPtr ioISP, char *fPath) {
  if (ioISP->inputCopyFD != (FILE *)NULL) {
    fprintf(stderr, "Too many destinations for all.\n");
    return 1;
  }
  MALLOCCOPY(ioISP->inputCopyFPath, fPath);

  if (strcmp(ioISP->inputCopyFPath, "stdout") == 0 ) {
    if (FILEIsATty(stdout)) {
      fprintf(stderr, "stdout is connected to a terminal. Abort\n");
      return 1;
    }
    if (ioISP->stdoutUsed) {
      fprintf(stderr, "stdout already allocated. Abort\n");
      return 1;
    }
    ioISP->stdoutUsed = 1;
    ioISP->inputCopyFD = stdout;
    return 0;
  }

  if (strcmp(ioISP->inputCopyFPath, "stderr") == 0 ) {
#if	ALLOW_STDERR
    if (FILEIsATty(stderr)) {
      fprintf(stderr, "stderr is connected to a terminal. Abort\n");
      return 1;
    }
    if (ioISP->stderrUsed) {
      fprintf(stderr, "stderr already allocated. Abort\n");
      return 1;
    }
    ioISP->stderrUsed = 1;
    ioISP->inputCopyFD = stderr;
    return 0;
#else
    fprintf(stderr, "stderr not a valid output device. Abort\n");
    return 1;
#endif
  }

  /* check if this is a fifo */
  if (fileExists(ioISP->inputCopyFPath) && ! isFIFO(ioISP->inputCopyFPath)) {
    fprintf(stderr, "non-fifo destination for input. Abort\n");
    return 1;
  }

  /* this is a FIFO. Open in write mode */
  if ( (ioISP->inputCopyFD = fopen(ioISP->inputCopyFPath, "w"))
       == (FILE *)NULL) {
    fprintf(stderr, "error opening fifo [%s]\n", strerror(errno));
    return 1;
  }
  return 0;
}


/*
**checkInputSources:
**	see if the input source option makes any sense; can only be
**	stdin or a named pipe (fifo)
** Input:
**	iochecksInfoStructPtr ioISP;
**	char *fPath
** Output:
**	0 if ok; 1 else.
** Side Effects:
**	None
*/
int iochecksInputSource(iochecksInfoStructPtr ioISP, char *fPath) {
  if (strcmp(fPath, "stdin") == 0) {
    if (FILEIsATty(stdin)) {
      fprintf(stderr, "stdin is connected to a terminal.\n");
      return 1;
    }
  } else {
    /* must be a fifo: isFIFO() checks for existance */
    if (!isFIFO(fPath)) {
      /* isn't */
      fprintf(stderr, "input-source %s doesn't exist or isn't a pipe\n",
	      fPath);
      return 1;
    }
  }
  /* delay opening: this is handled in openFile and relatives */
  ioISP->inputPipeFlag = 1;
  MALLOCCOPY(pseudoArgv[0], fPath);
  return 0;

}

/*
** iochecksInputs:
**	check that the various input options, filespecs etc make sense.
** Inputs:
**	iochecksInfoStructPtr ioISP.
**	zeroFilesOK: ok if no files given on command line.
** Output:
**	0 if OK. 1 else.
** Side Effects:
**	Sets variables: firstFile, fileCount, fnArray in ioISP
**
*/

int iochecksInputs(iochecksInfoStructPtr ioISP, int zeroFilesOK) {
  if (ioISP->inputPipeFlag) {
    /* can either specify input-pipe or give input files but not both */
    if( ioISP->firstFile < ioISP->argc ) {
      fprintf(stderr, "Conflicting input options: stdin and files\n");
      return 1;
    }
    ioISP->firstFile = 0;
    ioISP->fileCount = 1;
    ioISP->fnArray = &pseudoArgv[0];
    return 0;
  }

  /* files given explicitly. check that we have SOME to process */
  if( ioISP->firstFile >= ioISP->argc ) {
    if (zeroFilesOK) {
      ioISP->fileCount = 0;
      return 0;
    } else {
      fprintf(stderr, "No input files to process\n");
      return 1;
    }

  }
  /* some to process from command line */
  ioISP->fileCount = ioISP->argc - ioISP->firstFile;
  ioISP->fnArray = &ioISP->argv[ioISP->firstFile];
  ioISP->firstFile = 0;
  return 0;
}


void iochecksTeardown(iochecksInfoStructPtr UNUSED(ioISP)) {
  return;
}
