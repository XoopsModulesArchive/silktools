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
**  rwcountutils.c
**
**  utility routines for rwcount.c
**
**  Michael P. Collins
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"
#include "rwcount.h"
#include "fglob.h"

RCSIDENT("rwcountutils.c,v 1.10 2004/03/10 22:11:59 thomasm Exp");


/* required externs */

/*
 * External application state crap
*/
extern struct option appOptions[];
extern int appOptionCount;
extern char *appHelp[];
/*
 * I/O stuff
*/

extern rwIOStructPtr rwIOSPtr;
extern iochecksInfoStructPtr ioISP;

/*
 * Status flags and other state
*/
extern uint64_t recsCounted, recsRead;

#define NUM_REQUIRED_ARGS 0

static void appUsageLong(void);
void appTeardown(void);
void appOptionsUsage(void);
int  appOptionsSetup(void);
void appOptionsTeardown(void);
int  appOptionsHandler(clientData cData, int index, char *optarg);


/*
** appUsage:
** print usage information to stderr and exit with code 1
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
void appUsage(void) {
  fprintf(stderr, "Use `%s --help' for usage\n", skAppName());
  exit(1);
}


/*
** appUsageLong:
**      print usage information to stderr and exit with code 1.
**      passed to optionsSetup() to print usage when --help option
**      given.
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
static void appUsageLong(void) {
  fprintf(stderr, "Usage %s [options] [inputFiles...]\n", skAppName());
  fglobUsage();
  appOptionsUsage();
  exit(1);
}


/*
** appSetup
**do all the setup for this application include setting up required
** modules etc.
** Arguments:
**argc, argv
** Returns: None.
** Side Effects:
**exits with code 1 if anything does not work.
*/
void appSetup(int argc, char **argv) {
  skAppRegister(argv[0]);
  recsCounted = recsRead = 0;

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);/* never returns */
  }

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);
  }

  if (argc < NUM_REQUIRED_ARGS) {
    appUsage();                 /* never returns */
  }

  ioISP = iochecksSetup(0, 0, argc, argv);
  /*
  ** check input  and output files, pipes, etc. If fglobValid() is true,
  ** then 0 explicit files is ok. Else it is not
  */
  if ((ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    appTeardown();
    exit(1);
  }
  if(fglobValid()) {
    if (ioISP->firstFile < argc) {
      fprintf(stderr, "Error: both fglob options & input files given\n");
      exit(1);
    }
  }else {
    if (ioISP->firstFile == argc) {
      if (FILEIsATty(stdin)) {
        appUsage();             /* never returns */
      }
      if (iochecksInputSource(ioISP, "stdin")) {
        exit(1);
      }
    }
  }
  if(iochecksInputs(ioISP, fglobValid())) {
    appUsage();
  }
  (void) umask((mode_t) 0002);
  return;/* OK */
}

/*
** appOptionsUsage:
** print help for options for this app to stderr.
** Arguments: None.
** Returns: None.
** Side Effects: None.
** NOTE:
**  do NOT add non-option usage here (i.e., required and other optional args)
*/

void appOptionsUsage(void) {
  register int i;

  fprintf(stderr, "\napp options\n");
  for (i = 0; i < appOptionCount; i++ ) {
    fprintf(stderr, "--%s %s. %s\n", appOptions[i].name,
            appOptions[i].has_arg ? appOptions[i].has_arg == 1
            ? "Required Arg" : "Optional Arg" : "No Arg", appHelp[i]);
  }
  return;
}


/*
** appOptionsSetup:
** setup to parse application options.
** Arguments: None.
** Returns:
**0 OK; 1 else.
*/
int  appOptionsSetup(void) {
  /* register the apps  options handler */
  if (optionsRegister(appOptions, (optHandler) appOptionsHandler,
                (clientData) 0)) {
    fprintf(stderr, "%s: unable to register application options\n",
            skAppName());
    return 1;                   /* error */
  }
  return 0;/* OK */
}


/*
** appOptionsTeardown:
** teardown application options.
** Arguments: None.
** Returns: None.
** Side Effects: None
*/
void appOptionsTeardown(void) {
  return;
}

/*
** appOptionsHandler:
** called for each option the app has registered.
** Arguments:
**clientData cData: ignored here.
**int index: index into appOptions of the specific option
**char *optarg: the argument; 0 if no argument was required/given.
** Returns:
**0 if OK. 1 else.
** Side Effects:
**Relevant options are set.
*/
int  appOptionsHandler(UNUSED(clientData cData), int index, char *optarg) {
  uint32_t tmpArg;
  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }
  switch(index) {
  case RWCO_BIN_LOAD:
    tmpArg = strtoul(optarg, NULL, 10);
    /*
     * sanity check on bin loading.  I'd do it directly, but I
     * should be fascist.  I'd use <>, but I'm trying to avoid an
     * assumption of linearity in the flags
     */
    switch(tmpArg) {
    case LOAD_MEAN:
    case LOAD_START:
    case LOAD_END:
    case LOAD_MIDDLE:
      binLoadMode = tmpArg;
      break;
    default:
      fprintf(stderr, "unrecognized loading option, choose 0,1,2 or 3\n");
      exit(1);
    };
    break;
  case RWCO_BIN_SIZE:
    defaultBinSize = strtoul(optarg, NULL, 10);
    break;
  case RWCO_SKIPZ:
    skipZeroFlag = 1;
    break;
  case RWCO_PRINTFNAME:
    printFNameFlag = 1;
    break;
  case RWCO_DELIM:
    delimiter = optarg[0];
    break;
  case RWCO_NOTITLE:
    printTitleFlag = 0;
    break;
  case RWCO_EPOCHTIME:
    binStampFlag = EPOCH;
    break;
  case RWCO_BINTIME:
    binStampFlag = INDEX;
    break;
  case RWCO_FIRSTEPOCH:
    zeroFlag = strtoul(optarg, NULL, 10);
    break;
  }
  return 0;                     /* OK */
}

/*
** appTeardown:
**      Teardown all used modules and all application stuff.
** Arguments: None
** Returns: None
** Side Effects:
** All modules are torn down. Then application teardown takes place.
** Static variable teardownFlag is set.
** NOTE: This must be idempotent using static teardownFlag.
*/
void appTeardown(void) {
  static uint8_t teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  iochecksTeardown(ioISP);
  appOptionsTeardown();

  if (rwIOSPtr) {
    rwCloseFile(rwIOSPtr);
  }
  rwIOSPtr = (rwIOStructPtr )NULL;
  return;
}

