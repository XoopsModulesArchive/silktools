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
** 1.13
** 2004/03/10 23:07:18
** thomasm
*/

/*
 * Rwset.c
 *
 * Michael Collins
 * May 6th
 *
 * rwset is an application which takes filter data and generates a
 * tree (rwset) of ip addresses which come out of a filter file.  This
 * tree can then be used to generate aggregate properties; rwsets will
 * be used for filtering large sets of ip addresses, aggregate properties
 * per element can be counted, &c.
 */

/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "fglob.h"
#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"

#include "iptree.h"
#include "rwset.h"

RCSIDENT("rwset.c,v 1.13 2004/03/10 23:07:18 thomasm Exp");


#define NUM_REQUIRED_ARGS 0
/* local defines and typedefs */

/* exported functions */
void appUsage(void);		/* never returns */

/* local functions */
static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns */
static void appOptionsUsage(void); 		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */

static int  appOptionsHandler(clientData cData, int index, char *optarg);

/* exported variables */
static int integerIPSFlag = 0;
static uint8_t printIPSFlag = 0;
static uint8_t useDestFlag = 0;
static char *setFileName;
/* local variables */
struct option appOptions[] = {
  {"print-ips", NO_ARG, 0, RWSO_PRINT_IPS},
  {"integer-ips", NO_ARG, 0, RWSO_INTEGER_IPS},
  {"saddress", NO_ARG, 0, RWSO_SOURCE_ADDR},
  {"daddress", NO_ARG, 0, RWSO_DEST_ADDR},
  {"set-file", REQUIRED_ARG, 0, RWSO_TGTFILE},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
char *appHelp[] = {
  /* add help strings here for the applications options */
  "Print out IP addresses as dotted quad",
  "Print out IP addresses as 32bit integers",
  "Use source address as key",
  "Use dest address as key",
  "Write Macro Tree to this file",
  (char *)NULL
};

static iochecksInfoStructPtr ioISP;
static rwRec currentRecord;	/* Record to write to */

/*
 * appUsage:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
 */
void appUsage(void) {
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
static void appUsageLong(void) {
  fprintf(stderr, "%s [app_opts] [fglob_opts] .... \n", skAppName());
  appOptionsUsage();
  fglobUsage();
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
static void appTeardown(void) {
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  iochecksTeardown(ioISP);
  fglobTeardown();

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
static void appSetup(int argc, char **argv) {

  skAppRegister(argv[0]);

  setFileName = NULL;
  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if (argc - 1 < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);			/* never returns */
  }

  ioISP = iochecksSetup(0, 0, argc, argv);

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);			/* never returns */
  }

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }


  /*
  ** Input file checks:  if libfglob is NOT used, then check
  ** if files given on command line.  Do NOT allow both libfglob
  ** style specs and cmd line files. If cmd line files are NOT given,
  ** then default to stdin.
  */
  if (fglobValid()) {
    if (ioISP->firstFile < argc) {
      fprintf(stderr, "argument mismatch. Both fglob options & input files given\n");
      exit(1);
    }
  } else {
    if (ioISP->firstFile == argc) {
      if (FILEIsATty(stdin)) {
	appUsage();		/* never returns */
      }
      if (iochecksInputSource(ioISP, "stdin")) {
	exit(1);
      }
    }
  }

  /* check input files, pipes, etc */
  if (iochecksInputs(ioISP, fglobValid())) {
    appUsage();
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n", skAppName());
    appTeardown();
    exit(1);
  }

  if(!setFileName &&
     !printIPSFlag) {
    fprintf(stderr, "rwset: Specify an output file name or use the ips flag\n");
    appUsage();
    exit(1);
  }
  (void) umask((mode_t) 0002);
  return;			/* OK */
}

/*
 * appOptionsUsage:
 * 	print options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
 * NOTE:
 *	do NOT add non-option usage here (i.e., required and other
 *	optional args)
 */
static void appOptionsUsage(void) {
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
 * appOptionsSetup:
 * 	setup to parse application options.
 * Arguments: None.
 * Returns:
 *	0 OK; 1 else.
 */
static int  appOptionsSetup(void) {
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
static int  appOptionsHandler(clientData UNUSED(cData),int index,char *optarg){
  switch (index) {
  case RWSO_INTEGER_IPS:
    integerIPSFlag = 1;
    /* FALL THROUGH to set print flag as well */
  case RWSO_PRINT_IPS:
    printIPSFlag = 1;
    break;
  case RWSO_DEST_ADDR:
    useDestFlag = 1;
    break;
  case RWSO_TGTFILE:
    MALLOCCOPY(setFileName, optarg);
    return 0;
  }
  return 0;			/* OK */
}

int main(int argc, char **argv) {
  int counter;
  char * curFName;
  FILE *dataFile;
  macroTree newTree;
  macroNode *tgtBlock;
  rwIOStructPtr rwIOSPtr;
  curFName = NULL;
  appSetup(argc, argv);			/* never returns */
  /* Read in records */
  initMacroTree(&newTree);
  if(useDestFlag) {
    /*
     * OF COURSE I'm going to completely unroll the if
     */
    if (fglobValid()) {
      while ((curFName = fglobNext())){
	if((rwIOSPtr = rwOpenFile(curFName, ioISP->inputCopyFD)) ==
	   (rwIOStructPtr) NULL) {
	  exit(1);
	}
	while(rwRead(rwIOSPtr, &currentRecord)) {
	  tgtBlock = newTree.macroNodes[(currentRecord.dIP.ipnum >> 16)];
	  if(!tgtBlock) {
	    tgtBlock = (macroNode *) malloc(sizeof(macroNode));
	    memset(tgtBlock->addressBlock, '\0', 8192);
	    tgtBlock->parentAddr = (currentRecord.dIP.ipnum >> 16);
	    newTree.macroNodes[(currentRecord.dIP.ipnum >> 16)] = tgtBlock;
	  }
	  macroTreeAddAddress(currentRecord.dIP.ipnum,
			      (&newTree));
	}
	rwCloseFile(rwIOSPtr);
      }
    } else {
      for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
	if((rwIOSPtr = rwOpenFile(ioISP->fnArray[counter], ioISP->inputCopyFD)) ==
	   (rwIOStructPtr) NULL) {
	  exit(1);
	}
	while(rwRead(rwIOSPtr, &currentRecord)) {
	  tgtBlock = newTree.macroNodes[(currentRecord.dIP.ipnum >> 16)];
	  if(!tgtBlock) {
	    tgtBlock = (macroNode *) malloc(sizeof(macroNode));
	    memset(tgtBlock->addressBlock, '\0', 8192);
	    tgtBlock->parentAddr = (currentRecord.dIP.ipnum >> 16);
	    newTree.macroNodes[(currentRecord.dIP.ipnum >> 16)] = tgtBlock;
	  }
	  macroTreeAddAddress(currentRecord.dIP.ipnum,
			      (&newTree));
	}
	rwCloseFile(rwIOSPtr);
      }
    }
  } else {
    if (fglobValid()) {
      while ((curFName = fglobNext())){
	if((rwIOSPtr = rwOpenFile(curFName, ioISP->inputCopyFD)) ==
	   (rwIOStructPtr) NULL) {
	  exit(1);
	}
	while(rwRead(rwIOSPtr, &currentRecord)) {
	  tgtBlock = newTree.macroNodes[(currentRecord.sIP.ipnum >> 16)];
	  if(!tgtBlock) {
	    tgtBlock = (macroNode *) malloc(sizeof(macroNode));
	    memset(tgtBlock->addressBlock, '\0', 8192);
	    tgtBlock->parentAddr = (currentRecord.sIP.ipnum >> 16);
	    newTree.macroNodes[(currentRecord.sIP.ipnum >> 16)] = tgtBlock;
	  }
	  macroTreeAddAddress(currentRecord.sIP.ipnum,
			      (&newTree));
	}
	rwCloseFile(rwIOSPtr);
      }
    } else {
      for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
	if((rwIOSPtr = rwOpenFile(ioISP->fnArray[counter], ioISP->inputCopyFD)) ==
	   (rwIOStructPtr) NULL) {
	  exit(1);
	}
	while(rwRead(rwIOSPtr, &currentRecord)) {
	  tgtBlock = newTree.macroNodes[(currentRecord.sIP.ipnum >> 16)];
	  if(!tgtBlock) {
	    tgtBlock = (macroNode *) malloc(sizeof(macroNode));
	    memset(tgtBlock->addressBlock, '\0', 8192);
	    tgtBlock->parentAddr = (currentRecord.sIP.ipnum >> 16);
	    newTree.macroNodes[(currentRecord.sIP.ipnum >> 16)] = tgtBlock;
	  }
	  macroTreeAddAddress(currentRecord.sIP.ipnum,
			      (&newTree));
	}
	rwCloseFile(rwIOSPtr);
      }
    }
  }
  if(printIPSFlag) {
    fprintf(stderr, "Printing ips\n");
    printMacroIPs(stdout, &newTree, integerIPSFlag);
  }
  if(setFileName != NULL) {
    dataFile = fopen(setFileName, "wb");
    if (!dataFile) {
      fprintf(stderr, "%s: Cannot create set file '%s'\n",
              skAppName(), setFileName);
      exit(1);
    }
    writeMacroTree(dataFile, &newTree);
    fclose(dataFile);
  }
  /* output */
  appTeardown();
  return (0);
}

