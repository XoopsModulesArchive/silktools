/*
** Copyright (C) 2003-2004 by Carnegie Mellon University.
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
**  Quick application to print the IPs in a set file
**
*/


/* Includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "utils.h"
#include "iptree.h"

RCSIDENT("readset.c,v 1.12 2004/03/10 23:07:18 thomasm Exp");


/* local defines and typedefs */
#define	NUM_REQUIRED_ARGS	1

/* exported functions */
void appUsage(void);				/* never returns */

/* local functions */
/* main routine to process a file */
static int readsetProcessFile(const char *fileName);

static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns */
static void appOptionsUsage(void);		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */
static int  appOptionsHandler(clientData cData, int index, char *optarg);

/* exported variables */

typedef enum _appOptionsEnum {
  READSET_COUNT, READSET_PRINT, READSET_INTEGER_IP, READSET_STATISTICS
} appOptionsEnum;


/* local variables */
static int nextOptIndex;   /* first non-handled option */
static struct {
  int print_ips;   /* whether to print IPs: default yes */
  int count_ips;   /* whether to count IPs: default no */
  int integer_ips; /* whether to print IPs as integers; default no */
  int statistics;  /* whether to print statistics; default no */
} optFlags = {
  1, 0, 0, 0
};

static struct option appOptions[] = {
  {"count-ips",        NO_ARG, 0, READSET_COUNT},
  {"print-ips",        NO_ARG, 0, READSET_PRINT},
  {"integer-ips",      NO_ARG, 0, READSET_INTEGER_IP},
  {"print-statistics", NO_ARG, 0, READSET_STATISTICS},
  {0,0,0,0} /* sentinel entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  /* add help strings here for the applications options */
  "print the number of IPs; disables default printing of IPs",
  "force printing of IPs when count option is given",
  "print IPs as integers; implies --print-ips",
  "print statistics (min-ip, max-ip, etc) about the set",
  (char *)NULL /* sentinel entry */
};


/*
 * appUsage:
 * 	print short usage information to stderr and exit with code 1
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
 * 	print complete usage information to stderr and exit with code.
 * 	passed to optionsSetup() to print usage when --help option
 * 	given.
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
 */
static void appUsageLong(void) {
  fprintf(stdout, "%s [app-option] <set-file>\n", skAppName());
  fprintf(stdout, "  By default, prints the IPs in the specified set-file\n");
  fprintf(stdout, "  Use the options to optionally/additionally print the\n");
  fprintf(stdout, "  number of IPs in the file\n");
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
static void appSetup(int argc, char **argv) {

  skAppRegister(argv[0]);

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if ((argc - 1) < NUM_REQUIRED_ARGS) {
    fprintf(stderr, "%s: expecting %d arguments\n",
            skAppName(), NUM_REQUIRED_ARGS);
    appUsage();		/* never returns */
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
  if ( (nextOptIndex = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
    return;
  }

  if (nextOptIndex >= argc) {
    fprintf(stderr, "%s: Expecting input set-file name\n", skAppName());
    appUsage();
    return;
  }

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
 * 	print options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
 * NOTE:
 *	do NOT add non-option usage here (i.e., required and other optional args)
 */
static void appOptionsUsage(void) {
  int i;
  fprintf(stdout, "\napp options\n");
  for (i = 0; i < appOptionCount; i++ ) {
    fprintf(stdout, "--%s %s. %s\n", appOptions[i].name,
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
static int appOptionsHandler(clientData UNUSED(cData), int index,
                             char UNUSED(*optarg))
{
  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }

  switch (index) {
  case READSET_STATISTICS:
    ++optFlags.statistics;
    /* FALL THRU since characterists implies count */
  case READSET_COUNT:
    /* increment count and decrement print, but only once */
    if (!optFlags.count_ips) {
      ++optFlags.count_ips;
      --optFlags.print_ips;
    }
    break;
  case READSET_INTEGER_IP:
    ++optFlags.integer_ips;
    /* FALL THRU since integer-ip implies print */
  case READSET_PRINT:
    /* increment print_ips to counter effect of count_ips */
    ++optFlags.print_ips;
    break;
  default:
    appUsage();			/* never returns */
  }

  return 0;			/* OK */
}


/*
 *  readsetProcessFile(fileName);
 *
 *    Open the set-file given in fileName, read the macroTree from it;
 *    then print its IPs and/or print a count of its IPs
 *
 *  INPUT: fileName - name of binary set file to read
 *  RETURNS: 0 on sucess, 1 otherwise
 *  SIDE EFFECTS: prints to stdout
 */
static int readsetProcessFile(const char *fileName)
{
  uint32_t count;
  macroTree m;
  FILE *tgtFile;

  /* zero the macroTree */
  memset(&m, 0, sizeof(macroTree));

  /* Read macroTree from file */
  tgtFile = fopen(fileName, "rb");
  if(NULL == tgtFile) {
    fprintf(stderr, "%s: error opening set file '%s': %s\n",
            skAppName(), fileName, strerror(errno));
    return 1;
  }
  if (0 != readMacroTree(tgtFile, &m)) {
    fprintf(stderr, "%s: error reading tree from '%s'\n",
            skAppName(), fileName);
    return 1;
  }
  fclose(tgtFile);

  if (optFlags.count_ips) {
    count = countMacroIPs(&m);
    printf("%s: %u IPs\n", fileName, count);
    if (count && optFlags.statistics) {
      skIptreePrintStatistics(stdout, &m, 0);
    }
  }
  if (optFlags.print_ips) {
    printMacroIPs(stdout, &m, optFlags.integer_ips);
  }
  return 0;
}


int main(int argc, char **argv) {
  int i;

  appSetup(argc, argv);			/* never returns on error */

  for (i = nextOptIndex; i < argc; ++i) {
    (void)readsetProcessFile(argv[i]);
  }

  /* done */
  appTeardown();

  return 0;
}
