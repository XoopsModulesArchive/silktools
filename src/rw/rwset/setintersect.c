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
** 1.12
** 2004/03/10 23:07:18
** thomasm
*/

/*
 *
 * setintersect.c
 *
 * This is an application that takes multiple set files and intersects them,
 * generating a set of addresses that are in all sets.  Sets are
 * specified as 'removes' or adds, where a 'remove' file is one that
 * should have all of its addresses present removed, and an add file
 * will keep addresses that are in it.
 *
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
#include <errno.h>

#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"

#include "iptree.h"
#include "rwset.h"

RCSIDENT("setintersect.c,v 1.12 2004/03/10 23:07:18 thomasm Exp");


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
static uint8_t printIPSFlag = 0;
static int integerIPSFlag = 0; /* whether to print ips as uint32_t */
static uint8_t addFiles, removeFiles;
static char *addFileNames[4];
static char *removeFileNames[4];
static char *targetFileName;
/* local variables */
struct option appOptions[] = {
  {"print-ips", NO_ARG, 0, SIO_PRINT_IPS},
  {"integer-ips", NO_ARG, 0, SIO_INTEGER_IPS},
  {"add-set", REQUIRED_ARG, 0, SIO_ADD_FILE},
  {"remove-set", REQUIRED_ARG, 0, SIO_REMOVE_FILE},
  {"set-file", REQUIRED_ARG, 0, SIO_TGTFILE},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
char *appHelp[] = {
  /* add help strings here for the applications options */
  "Print IP's as dotted quad to stdout",
  "Print IP's as 32 bit integers to stdout",
  "Specify a set of IPs to use",
  "Specify a set of IPs to remove",
  "File to write resulting set to",
  (char *)NULL
};

static iochecksInfoStructPtr ioISP;


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
  fprintf(stderr, "%s [app_opts] .... \n", skAppName());
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
static void appTeardown(void) {
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  iochecksTeardown(ioISP);

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
  targetFileName = NULL;
  /* check that we have the same number of options entries and help*/
  addFiles = 0;
  removeFiles = 0;
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

  /* check input files, pipes, etc */


  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n", skAppName());
    appTeardown();
    exit(1);
  }
  if(!targetFileName &&
     !printIPSFlag) {
    fprintf(stderr, "setintersect: Specify an output file name or use the ips flag\n");
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
  case SIO_INTEGER_IPS:
    integerIPSFlag = 1;
    /* FALL THRU to set print-ips as well */
  case SIO_PRINT_IPS:
    printIPSFlag = 1;
    break;
  case SIO_TGTFILE:
    MALLOCCOPY(targetFileName, optarg);
    break;
  case SIO_ADD_FILE:
    if(addFiles == 4) {
      fprintf(stderr, "File limit exceeded\n");
      return -1;
    };
    MALLOCCOPY(addFileNames[addFiles], optarg);
    addFiles++;
    break;
  case SIO_REMOVE_FILE:
    if(removeFiles == 4) {
      fprintf(stderr, "File limit exceeded\n");
      return -1;
    };
    MALLOCCOPY(removeFileNames[removeFiles], optarg);
    removeFiles++;
    break;
  }
  return 0;			/* OK */
}


int main(int argc, char **argv) {
  int i, j, k;
  char * curFName;
  FILE *dataFile;
  macroTree newTree;
  macroTree tmpTree;

  curFName = NULL;
  appSetup(argc, argv);			/* never returns */

  /* Read in initial --add file */
  initMacroTree(&newTree);
  if(!addFiles) {
    fprintf(stderr, "Please specify at least one add file before\n");
    exit(0);
  }
  dataFile = fopen(addFileNames[0], "rb");
  if (NULL == dataFile) {
    fprintf(stderr, "%s: Cannot open file '%s': %s\n",
            skAppName(), addFileNames[0], strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (0 != readMacroTree(dataFile, &newTree)) {
    fprintf(stderr, "%s: Error reading tree from file '%s'\n",
            skAppName(), addFileNames[0]);
    return 1;
  }
  fclose(dataFile);

  /* Process remaining --add files */
  for(i = 1; i < addFiles; i++) {
    dataFile = fopen(addFileNames[i], "rb");
    if (NULL == dataFile) {
      fprintf(stderr, "%s: Cannot open file '%s': %s\n",
              skAppName(), addFileNames[i], strerror(errno));
      exit(EXIT_FAILURE);
    }
    freeMacroTree(&tmpTree);
    if (0 != readMacroTree(dataFile, &tmpTree)) {
      fprintf(stderr, "%s: Error reading tree from file '%s'\n",
              skAppName(), addFileNames[i]);
      return 1;
    }
    fclose(dataFile);
    for(j = 0; j < 65535; j++) {
      /*
       * we cut out every empty j.
       */
      if(tmpTree.macroNodes[j] == NULL) {
	if(newTree.macroNodes[j] != NULL) {
	  free(newTree.macroNodes[j]);
	  newTree.macroNodes[j] = NULL;
	}
      } else {
	if(newTree.macroNodes[j] == NULL) {
	  /*
	   * We can skip onwards because there's nothing in the
	   * tree implicitly.
	   */
	  continue;
	} else {
	  for(k = 0; k < 2048; k++) {
	    newTree.macroNodes[j]->addressBlock[k] &=
	      tmpTree.macroNodes[j]->addressBlock[k];
	  }
	}
      }
    }
  }

  /* Process --remove files */
  for(i = 0; i < removeFiles; i++) {
    dataFile = fopen(removeFileNames[i], "rb");
    if (NULL == dataFile) {
      fprintf(stderr, "%s: Cannot open file '%s': %s\n",
              skAppName(), removeFileNames[i], strerror(errno));
      exit(EXIT_FAILURE);
    }
    freeMacroTree(&tmpTree);
    if (0 != readMacroTree(dataFile, &tmpTree)) {
      fprintf(stderr, "%s: Error reading tree from file '%s'\n",
              skAppName(), removeFileNames[i]);
      return 1;
    }
    fclose(dataFile);
    for(j = 0; j < 65535; j++) {
      /*
       * we cut out every empty j.
       */
      if(tmpTree.macroNodes[j] != NULL) {
      	if(newTree.macroNodes[j] == NULL) {
	  /*
	   * We can skip onwards because there's nothing in the
	   * tree implicitly.
	   */
	  continue;
	} else {
	  for(k = 0; k < 2048; k++) {
	    newTree.macroNodes[j]->addressBlock[k] &=
	      ~tmpTree.macroNodes[j]->addressBlock[k];
	  }
	}
      }
    }
  }

  /* Do output */
  if(printIPSFlag) {
    printMacroIPs(stdout, &newTree, integerIPSFlag);
  }
  if(targetFileName != NULL) {
    dataFile = fopen(targetFileName, "wb");
    if (NULL == dataFile) {
      fprintf(stderr, "%s: Cannot open output file '%s': %s\n",
              skAppName(), targetFileName, strerror(errno));
      exit(EXIT_FAILURE);
    }
    writeMacroTree(dataFile, &newTree);
    if (EOF == fclose(dataFile)) {
      fprintf(stderr, "%s: Unable to close output file '%s': %s\n",
              skAppName(), targetFileName, strerror(errno));
    }
  }
  /* output */
  appTeardown();
  return (0);
}



