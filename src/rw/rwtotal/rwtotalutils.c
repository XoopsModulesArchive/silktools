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

**  1.14
**  2004/03/15 18:29:20
** collinsm
*/

/*
** rwcututils.c
**	utility routines in support of rwcut.
** Suresh Konda
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"
#include "rwtotal.h"

RCSIDENT("rwtotalutils.c,v 1.14 2004/03/15 18:29:20 collinsm Exp");


#define NUM_REQUIRED_ARGS 0

/* imported variables */
rwRec rwrec;
rwIOStructPtr rwIOSPtr;

/* exprted functions */
void appSetup(int argc, char **argv);
void appTeardown(void);

/* local functions */
static void appUsageLong(void);
static void appOptionsUsage(void);
static int  appOptionsSetup(void);
static void appOptionsTeardown(void);
static int  appOptionsHandler(clientData cData, int index, char *optarg);


/* local variables */
static struct option appOptions[] = {
  {"sip-first-8", NO_ARG, 0, RWTO_SIP_8},
  {"sip-first-16", NO_ARG, 0, RWTO_SIP_16},
  {"sip-first-24", NO_ARG, 0, RWTO_SIP_24},
  {"sip-last-8", NO_ARG, 0, RWTO_SIP_L8},
  {"sip-last-16", NO_ARG, 0,  RWTO_SIP_L16},
  {"dip-first-8", NO_ARG, 0, RWTO_DIP_8},
  {"dip-first-16", NO_ARG, 0, RWTO_DIP_16},
  {"dip-first-24", NO_ARG, 0, RWTO_DIP_24},
  {"dip-last-8", NO_ARG, 0, RWTO_DIP_L8},
  {"dip-last-16", NO_ARG, 0, RWTO_DIP_L16},
  {"icmp-code", NO_ARG, 0, RWTO_ICMPCODE},
  {"duration", NO_ARG, 0, RWTO_DURATION},
  {"sport", NO_ARG, 0, RWTO_SPORT},
  {"dport", NO_ARG, 0, RWTO_DPORT},
  {"proto", NO_ARG, 0, RWTO_PROTO},
  {"packets", NO_ARG, 0, RWTO_PACKETS},
  {"bytes", NO_ARG, 0, RWTO_BYTES},
  {"delimiter", REQUIRED_ARG, 0, RWTO_DELIM},
  {"skip-zeroes", NO_ARG, 0, RWTO_SKIPZ},
  {"no-titles", NO_ARG, 0,RWTO_NOTITLES},
  {"print-filenames", NO_ARG, 0,RWTO_PRINTFILENAMES},
  {"copy-input", REQUIRED_ARG, 0,RWTO_COPYINPUT},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  "Key on the first 8 bits of the source IP address",
  "Key on the first 16 bits of the source IP address",
  "Key on the first 24 bits of the source IP address",
  "Key on the last 8 bits of the source IP address",
  "Key on the last 16 bits of the source IP address",
  "Key on the first 8 bits of the destination IP address",
  "Key on the first 16 bits of the destination IP address",
  "Key on the first 24 bits of the destination IP address",
  "Key on the last 8 bits of the destination IP address",
  "Key on the last 16 bits of the  destination  IP address",
  "Key on icmp type and code (DOES NOT check to see if the record is ICMP)",
  "Key on duration",
  "Key on the source port",
  "Key on the destination port",
  "Key on the protocol",
  "Key on the number of packets",
  "Key on the number of bytes",
  "Delimiter, defaults to '|'",
  "Skip zero entries",
  "Remove column titles",
  "Print filenames",
  "Copy all input records to given file",
  (char *)NULL
};

/* exported variables */
iochecksInfoStructPtr ioISP;

/*
 * appUsage:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
static void appUsage(void) {
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
  fprintf(stderr, "%s [total_opts]", skAppName());
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
 * 	Static variable teardownFlag is set.
 * NOTE: This must be idempotent using static teardownFlag.
*/
void appTeardown(void) {
  static uint8_t teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;
  appOptionsTeardown();

  if (rwIOSPtr) {
    rwCloseFile(rwIOSPtr);
  }
  rwIOSPtr = (rwIOStructPtr)NULL;

  return;
}

/*
 * appSetup
 *	do all the setup for this application include setting up required
 *	 modules etc.
 * Arguments:
 *	argc, argv
 * Returns: None.
 * Side Effects:
 *	exits with code 1 if anything does not work.
*/
void appSetup(int argc, char **argv) {

  skAppRegister(argv[0]);
  countMode = RWTO_NONE;
  skipFlag = 0;
  /* first do some sanity checks */
  /* check that defined FIELD_COUNT is correct */
  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  /* set defaults */
  ioISP = iochecksSetup(0, 0, argc, argv);

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);			/* never returns */
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);
  }

  if (argc < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
  }

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
  }

  if (ioISP->firstFile == argc) {
    if (FILEIsATty(stdin)) {
      appUsage();
    }
    if(iochecksInputSource(ioISP, "stdin")) {
      exit(1);
    }
  }

  if(iochecksInputs(ioISP, 0)) {
    appUsage();
  }
  if(countMode == RWTO_NONE) {
    fprintf(stderr, "No count mode specified, please choose one of the available modes\n");
    exit(1);
  }
  (void)umask((mode_t) 0002);

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
	    skAppName());
    appTeardown();
    exit(1);
  }

  return;			/* OK */
}

/*
 * appOptionsUsage:
 * 	print help for options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
 * NOTE:
 *  do NOT add non-option usage here (i.e., required and other optional args)
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
 * appOptionsTeardown:
 * 	teardown application options.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None
*/
static void appOptionsTeardown(void) {
  return;
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


static int  appOptionsHandler(clientData UNUSED(cData), int index, char *optarg) {

  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }
  switch (index) {
  case RWTO_DIP_8:
    destFlag = 1;
  case RWTO_SIP_8:
    countMode = RWTO_COPT_ADD8;
    totalRecs = 256;
    break;
  case RWTO_DIP_16:
    destFlag = 1;
  case RWTO_SIP_16:
    countMode = RWTO_COPT_ADD16;
    totalRecs = 65536;
    break;
  case RWTO_DIP_24:
    destFlag = 1;
  case RWTO_SIP_24:
    countMode = RWTO_COPT_ADD24;
    totalRecs = 65536 * 256;
    break;
  case RWTO_DIP_L8:
    destFlag = 1;
  case RWTO_SIP_L8:
    countMode = RWTO_COPT_LADD8;
    totalRecs = 256;
    break;
  case RWTO_DIP_L16:
    destFlag = 1;
  case RWTO_SIP_L16:
    countMode = RWTO_COPT_LADD16;
    totalRecs = 65536;
    break;
  case RWTO_DPORT:
    destFlag = 1;
  case RWTO_SPORT:
    countMode = RWTO_COPT_PORT;
    totalRecs = 65536;
    break;
  case RWTO_PROTO:
    countMode = RWTO_COPT_PROTO;
    totalRecs = 256;
    break;
  case RWTO_DURATION:
    countMode = RWTO_COPT_DURATION;
    totalRecs = 4096;
    break;
  case RWTO_ICMPCODE:
    countMode = RWTO_COPT_ICMP;
    totalRecs = 65536;
    break;
  case RWTO_BYTES:
    countMode = RWTO_COPT_BYTES;
    totalRecs = 65536 * 256;
    break;
  case RWTO_PACKETS:
    countMode = RWTO_COPT_PACKETS;
    totalRecs = 65536 * 256; 
    break;
  case RWTO_DELIM:
    delimiter = optarg[0];
    break;
  case RWTO_SKIPZ:
    skipFlag = 1;
    break;
  case RWTO_NOTITLES:
    noTitlesFlag = 1;
    break;
  case RWTO_PRINTFILENAMES:
    printFileNamesFlag = 1;
    break;
  case RWTO_COPYINPUT:
    if (!optarg) {
      fprintf(stderr, "Required arg to copy-input missing\n");
      return 1;
    }
    if (iochecksAllDestinations(ioISP, optarg)) {
      exit(1);
    }
    break;
  default:
    fprintf(stderr, "Invalid option\n");
    exit(1);
  } /* switch */
  return 0;			/* OK */
}


