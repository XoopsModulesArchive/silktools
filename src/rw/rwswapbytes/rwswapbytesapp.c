/*
**  Copyright (C) 2003-2004 by Carnegie Mellon University.
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
** 2004/03/10 22:45:14
** thomasm
*/

/*
 * rwswapbytes
 *
 * Read any rw file (rwpacked file, rwfilter output, etc) and output a
 * file in the specified byte order.
 *
 * This file contains the application setup code.
 *
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "utils.h"
#include "rwpack.h"
#include "iochecks.h"
#include "rwswapbytes.h"

RCSIDENT("rwswapbytesapp.c,v 1.5 2004/03/10 22:45:14 thomasm Exp");


/* local defines and typedefs */

/* 3 args: endianness, input, output */
#define	NUM_REQUIRED_ARGS	3


/* local functions */
static void appUsage(void);			/* never returns */
static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns on failure */
static void appOptionsUsage(void); 		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */
static int  appOptionsHandler(clientData cData, int index, char *optarg);


typedef enum _rwswapOptions {
  RWSW_BIG, RWSW_LITTLE, RWSW_NATIVE, RWSW_SWAP,
  RWSW_COPY
} rwswapOptions;


/* exported variables */
FILE *outF;


/* local variables */
static iochecksInfoStructPtr ioISP;
static int outIsPipe;
static rwswapOptions outEndian;
static char *inFName;

static struct option appOptions[] = {
  {"big-endian",       NO_ARG,       0, RWSW_BIG},
  {"little-endian",    NO_ARG,       0, RWSW_LITTLE},
  {"native-endian",    NO_ARG,       0, RWSW_NATIVE},
  {"swap-endian",      NO_ARG,       0, RWSW_SWAP},
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  /* add help strings here for the applications options */
  "write output in big-endian format",
  "write output in little-endian format",
  "write output in this machine's native format",
  "unconditionally swap the byte-order of the input",
  (char *)NULL
};


/*
 * appUsage:
 * 	print message on getting usage information and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
void appUsage(void)
{
  fprintf(stderr, "Use `%s --help' for usage\n", skAppName());
  exit(1);
}


/*
 * appUsageLong:
 * 	print usage information to stderr and exit with code 1.
 * 	passed to optionsSetup() to print usage when --help option
 * 	given.
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
static void appUsageLong(void)
{
  fprintf(stderr, "%s <endian-option> <input-file> <output-file>\n", skAppName());
  fprintf(stderr, "  Change the endian-ness of <input-file> as specified\n");
  fprintf(stderr, "  by <endian-option> and write result to <output-file>.\n");
  fprintf(stderr, "  You may use \"stdin\" for <input-file> and \"stdout\"\n");
  fprintf(stderr, "  for <output-file>.  Gzipped files are o.k.\n");
  appOptionsUsage();
  exit(1);
}


/*
 * appTeardown:
 *	teardown all used modules and all application stuff.
 * Arguments: None
 * Returns: None
 * Side Effects:
 * 	All modules are torn down. Then application teardown takes place.
 * 	Global variable teardownFlag is set.
 * NOTE: This must be idempotent using static teardownFlag.
*/
static void appTeardown(void)
{
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  if (outF && outF != stdout) {
    if (outIsPipe) {
      if (-1 == pclose(outF)) {
        fprintf(stderr, "%s: error closing output pipe\n", skAppName());
      }
    } else if (EOF == fclose(outF)) {
      fprintf(stderr, "%s: error closing output file: %s\n",
              skAppName(), strerror(errno));
    }
    outF = (FILE*)NULL;
  }

  optionsTeardown();

  return;
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
  int nextOptIndex;

  skAppRegister(argv[0]);

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if ((argc - 1) != NUM_REQUIRED_ARGS) {
    fprintf(stderr, "%s: expecting %d arguments\n", skAppName(),NUM_REQUIRED_ARGS);
    appUsageLong();		/* never returns */
  }

  /* remove one from argc since final arg is output path */
  ioISP = iochecksSetup(1, 0, (argc-1), argv);
  if (!ioISP) {
    fprintf(stderr, "%s: unable to setup iochecks module\n", skAppName());
    exit(1);
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);			/* never returns */
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);			/* never returns */
  }

  /* parse options */
  if ((nextOptIndex = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
  }

  /* Check that we have input */
  if (nextOptIndex >= argc) {
    fprintf(stderr, "%s: Expecting input file name\n", skAppName());
    appUsage();		/* never returns */
  }
  inFName = argv[nextOptIndex];

  ++nextOptIndex;
  if (nextOptIndex >= argc) {
    fprintf(stderr, "%s: Expecting output file name\n", skAppName());
    appUsage();		/* never returns */
  }
  if (iochecksPassDestinations(ioISP, argv[nextOptIndex], 0)) {
    appUsage();		/* never returns */
  }
  outF = ioISP->passFD[0];
  outIsPipe = ioISP->passIsPipe[0];

  ++nextOptIndex;
  if (argc != nextOptIndex) {
    fprintf(stderr, "%s: Got extra options\n", skAppName());
    appUsage();		/* never returns */
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    appTeardown();
    exit(1);		/* never returns */
  }

  return;			/* OK */
}


/*
 * appOptionsUsage:
 * 	print options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
 * NOTE:
 *	do NOT add non-option usage here (i.e., required and other optional args)
*/
static void appOptionsUsage(void)
{
  register int i;

  fprintf(stderr, "\nendian-option:\n");
  for (i = 0; i < appOptionCount; i++ ) {
    fprintf(stderr, "--%s: %s\n", appOptions[i].name, appHelp[i]);
  }

  return;
}


/*
 * appOptionsSetup:
 * 	setup to parse application options.
 * Arguments: None.
 * Returns:
 *	0 OK; 1 else.
*/
static int appOptionsSetup(void)
{
  /* register the apps  options handler */
  if (optionsRegister(appOptions, (optHandler) appOptionsHandler,
                      (clientData) 0)) {
    fprintf(stderr, "%s: unable to register application options\n",
            skAppName());
    return 1;			/* error */
  }
  return 0;			/* OK */
}


/*
 * appOptionsHandler:
 * 	called for each option the app has registered.
 * Arguments:
 *	clientData cData: ignored here.
 *	int index: index into appOptions of the specific option
 *	char *optarg: the argument; 0 if no argument was required/given.
 * Returns:
 *	0 if OK. 1 else.
 * Side Effects:
 *	Relevant options are set.
*/
static int appOptionsHandler(clientData UNUSED(cData), int index,
                             char UNUSED(*optarg))
{

  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }

  switch (index) {
#if !IS_LITTLE_ENDIAN
  case RWSW_NATIVE:
#endif
  case RWSW_BIG:
    if ((0 != outEndian) && (RWSW_BIG != outEndian)) {
      fprintf(stderr, "%s: conflicting endian options given\n", skAppName());
      return 1;
    }
    outEndian = RWSW_BIG;
    break;

#if IS_LITTLE_ENDIAN
  case RWSW_NATIVE:
#endif
  case RWSW_LITTLE:
    if ((0 != outEndian) && (RWSW_LITTLE != outEndian)) {
      fprintf(stderr, "%s: conflicting endian options given\n", skAppName());
      return 1;
    }
    outEndian = RWSW_LITTLE;
    break;

  case RWSW_SWAP:
    if ((0 != outEndian) && (RWSW_SWAP != outEndian)) {
      fprintf(stderr, "%s: conflicting endian options given\n", skAppName());
      return 1;
    }
    outEndian = RWSW_SWAP;
    break;

  default:
    appUsage();			/* never returns */
    break;
  }

  return 0;			/* OK */
}


int main(int argc, char **argv)
{
  int32_t fileType;
  rwIOStructPtr rwIOSPtr;

  appSetup(argc, argv);

  rwIOSPtr = rwOpenFile(inFName, NULL);
  if (!rwIOSPtr) {
    return 1;
  }
  fileType = rwGetFileType(rwIOSPtr);

  if (rwGetIsBigEndian(rwIOSPtr)) {
    if (RWSW_BIG == outEndian) {
      /* Just copy the bytes */
      outEndian = RWSW_COPY;
    } else if (RWSW_SWAP == outEndian) {
      outEndian = RWSW_LITTLE;
    }
  } else {
    if (RWSW_LITTLE == outEndian) {
      /* Just copy the bytes */
      outEndian = RWSW_COPY;
    } else if (RWSW_SWAP == outEndian) {
      outEndian = RWSW_BIG;
    }
  }

  if (RWSW_COPY == outEndian) {
    fileCopy(rwIOSPtr);
  } else {
    switch(fileType) {
    case FT_RWROUTED:
      swapRouted(rwIOSPtr);
      break;
    
    case FT_RWNOTROUTED:
      swapNotRouted(rwIOSPtr);
      break;
    
    case FT_RWSPLIT:
      swapSplit(rwIOSPtr);
      break;
      
    case FT_RWACL:
      swapAcl(rwIOSPtr);
      break;

    case FT_RWFILTER:
      swapFilter(rwIOSPtr);
      break;
    
    case FT_RWWWW:
      swapWeb(rwIOSPtr);
      break;

    case FT_RWGENERIC:
      swapGeneric(rwIOSPtr);
      break;

    default:
      fprintf(stderr, "%s: Invalid file type for %s: 0x%X\n",
              skAppName(), inFName, fileType);
      break;
    }
  }

  /* done */
  appTeardown();
  return 0;
}
