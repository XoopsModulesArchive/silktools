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
**
** 1.24
** 2004/03/10 23:00:36
** thomasm
*/


/*
**  rwfilterutils.c
**
**  utility routines for rwfilter.c
**
**  Suresh L. Konda
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
#include "filter.h"
#include "iochecks.h"
#include "dynlib.h"
#include "fglob.h"

RCSIDENT("rwfilterutils.c,v 1.24 2004/03/10 23:00:36 thomasm Exp");

#define FILTER_MAX_DYNLIBS 8

/* required externs */
extern struct option appOptions[];
extern int appOptionCount;
extern char *appHelp[];
extern rwIOStructPtr rwIOSPtr;
extern iochecksInfoStructPtr ioISP;
extern int dryRunFlag;
extern int printFNameFlag;
extern int printStatsFlag;
extern dynlibInfoStructPtr dlISP;
extern int filterDynlibCount;
extern const char *filterDynlibNames[];
extern dynlibInfoStructPtr filterDynlibList[FILTER_MAX_DYNLIBS];

static void appUsageLong(void);
void appTeardown(void);
void appOptionsUsage(void);
int  appOptionsSetup(void);
void appOptionsTeardown(void);
int  appOptionsHandler(clientData cData, int index, char *optarg);
static int filterAddDynlib(const char *dlpath, int warnOnError);

#define	NUM_REQUIRED_ARGS 2


/*
** appUsage:
** print usage information to stderr and exit with code 1
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
void appUsage(void) {
  fprintf(stderr, "Use `%s --help' for usage\n", skAppName());
  exit(EXIT_FAILURE);
}

/*
** appUsageLong:
** 	print usage information to stderr and exit with code 1.
** 	passed to optionsSetup() to print usage when --help option
** 	given.
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
static void appUsageLong(void) {
  int j;

  fprintf(stderr, "Usage %s [options] [inputFiles...]\n", skAppName());
  fglobUsage();
  filterUsage();

  for (j = 0; j < filterDynlibCount; ++j) {
    dynlibOptionsUsage(filterDynlibList[j]);
  }

  fprintf(stderr,
          "NOTE: to filter on sensors, use the --sensors fglob switch\n");
  appOptionsUsage();
  exit(EXIT_FAILURE);
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
  int j;
  int dynamicCheck = 0;

  skAppRegister(argv[0]);

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(EXIT_FAILURE);/* never returns */
  }

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(EXIT_FAILURE);
  }

  if (filterSetup()) {
    fprintf(stderr, "%s: unable to setup filter module\n", skAppName());
    exit(EXIT_FAILURE);
  }

  for (j = 0; filterDynlibNames[j]; ++j) {
    filterAddDynlib(filterDynlibNames[j], 0);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(EXIT_FAILURE);
  }

  if (argc < NUM_REQUIRED_ARGS) {
    appUsage();			/* never returns */
  }
  ioISP = iochecksSetup(2, 2, argc, argv);
  /* dlISP is specified on the command line; if specified it does all
   * filtering--to be consistent with legacy dynlibs.  Since we treat
   * dlISP specially, do not store it in the filterDynlibList[].
   */
  dlISP = dynlibSetup(DYNLIB_EXCL_FILTER);

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }

  if ( (filterGetRuleCount() < 1) && !dynlibCheckLoaded(dlISP) ) {
    /*
     * Check our subsidiary rulesets
     */
    for (j = 0; j < filterDynlibCount; ++j) {
      if (dynlibCheckActive(filterDynlibList[j])) {
        dynamicCheck = 1;
        break;
      }
    }

    /*
     * This is going to have to be formalized for future use.  What
     * I've been doing here is ensuring that each of the dynamic
     * loaded libraries, if present, provides us with another
     * opportunity to check and make sure that the parameters are
     * treated as first-order.  That is, if you check sipset, then it
     * should be treated as an acceptable parameter for go-ahead.
     */
    if(!dynamicCheck) {
      fprintf(stderr, "No filtering rules given\n");
      exit(EXIT_FAILURE);
    }
  }

  if (fglobValid()) {
    /*
     * fglob used. if files also explicitly given, abort
     */
    if (ioISP->firstFile < argc) {
      fprintf(stderr, "argument mismatch. Both --glob & input files given\n");
      exit(EXIT_FAILURE);
    }
  }

  /*
  ** check input  and output files, pipes, etc. If fglobValid() is true,
  ** then 0 explicit files is ok. Else it is not
  */
  if (iochecksInputs(ioISP, fglobValid())) {
    appUsage();
  }

  if (ioISP->passCount == 0
      && ioISP->failCount == 0
      && dynlibGetStatus(dlISP) != DYNLIB_WILLPROCESS
      && printStatsFlag == 0
      && ioISP->devnullUsed == 0
      && dryRunFlag == 0) {
      fprintf(stderr, "No output(s) specified. Abort\n");
      exit(EXIT_FAILURE);
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
	    skAppName());
    appTeardown();
    exit(EXIT_FAILURE);
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
	    ? "Req. Arg" : "Opt. Arg" : "No Arg", appHelp[i]);
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
    return 1;			/* error */
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
int  appOptionsHandler(clientData UNUSED(cData), int index, char *optarg) {

  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }
  switch (index) {
  case 0:
    /* pass */
    if (iochecksPassDestinations(ioISP, optarg, 0)) {
      exit(EXIT_FAILURE);
    }
    break;

  case 1:
    /* fail */
    if (iochecksFailDestinations(ioISP, optarg, 0)) {
      exit(EXIT_FAILURE);
    }
    break;

  case 2:
    /* all */
    if (iochecksAllDestinations(ioISP, optarg)) {
      exit(EXIT_FAILURE);
    }
    break;

  case 3:
    /*
    **  input-pipe. Delay check for multiple inputs sources till all
    **  options have been parsed
    */
    if (iochecksInputSource(ioISP, optarg)) {
      exit(EXIT_FAILURE);
    }
    break;

  case 4:
    /* dynamic-library */
    if(dynlibCheck(dlISP, optarg)) {
      fprintf(stderr, "%s: fatal error loading library '%s':\n  %s\n",
              skAppName(), optarg, dynlibLastError(dlISP));
      exit(EXIT_FAILURE);
    }
    break;

  case 5:
    dryRunFlag = 1;
    break;

  case 6:
    printFNameFlag = 1;
    break;

  case 7:
    printStatsFlag = 1;
    break;

  } /* switch */
  return 0;			/* OK */
}

/*
** appTeardown:
**	Teardown all used modules and all application stuff.
** Arguments: None
** Returns: None
** Side Effects:
** All modules are torn down. Then application teardown takes place.
** Static variable teardownFlag is set.
** NOTE: This must be idempotent using static teardownFlag.
*/
void appTeardown(void) {
  static uint8_t teardownFlag = 0;
  int j;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  iochecksTeardown(ioISP);
  dynlibTeardown(dlISP);

  for (j = 0; j < filterDynlibCount; ++j) {
    dynlibTeardown(filterDynlibList[j]);
    filterDynlibList[j] = NULL;
  }
  filterDynlibCount = 0;

  filterTeardown();

  appOptionsTeardown();

  if (rwIOSPtr) {
    rwCloseFile(rwIOSPtr);
  }
  rwIOSPtr = (rwIOStructPtr )NULL;
  return;
}


/*
** filterAddDynlib(dlpath, warnOnError);
**
**   Load the dynamic library at dlpath, store its information in the
**   filterDynlibList, and increment the filterDynlibCount.  Return 0
**   on success.  Return 1 if the library cannot be loaded or if too
**   many dynamic libraries have been loaded.
*/
static int filterAddDynlib(const char *dlpath, int warnOnError)
{
  dynlibInfoStructPtr dl;

  if (filterDynlibCount == FILTER_MAX_DYNLIBS) {
    fprintf(stderr, "%s: Too many dynlibs.  Only %u allowed\n",
            skAppName(), FILTER_MAX_DYNLIBS);
    return 1;
  }

  dl = dynlibSetup(DYNLIB_SHAR_FILTER);
  if (dynlibCheck(dl, dlpath)) {
    /* failed to find library, missing symbols, or setup failed */
    if (warnOnError) {
      fprintf(stderr, "%s: Error in dynamic library: %s\n",
              skAppName(), dynlibLastError(dl));
    }
    dynlibTeardown(dl);
    return 1;
  }

  filterDynlibList[filterDynlibCount] = dl;
  ++filterDynlibCount;

  return 0;
}
