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
** 1.7
** 2004/03/10 22:23:52
** thomasm
*/

/*
**
**  num2doc.c
**
**  Suresh L Konda
**  7/31/2002
**	"filter" to convert numeric ip to dotted ip. The default field
**	delimiter is '|' in deference to our internal default.  The
**	default field is 1 (numbering starts at 1).  Changes can be
**	provided via options --ip-fields=<range> and
**	--delimiter=<char>.
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "utils.h"

RCSIDENT("num2dot.c,v 1.7 2004/03/10 22:23:52 thomasm Exp");


/* local defines and typedefs */
#define	NUM_REQUIRED_ARGS	0
#define MAX_FIELD_COUNT		15

/* exported functions */
void appUsage(void);		/* never returns */

/* local functions */
static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns */
static void appOptionsUsage(void); 		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */
static void appOptionsTeardown(void);		/* applications options teardown  */
static int  appOptionsHandler(clientData cData, int index, char *optarg);
static int parseIPFields(char *b);

/* exported variables */

/* local variables */
static struct option appOptions[] = {
  {"ip-fields", REQUIRED_ARG, 0, 0},
  {"delimiter", REQUIRED_ARG, 0, 1},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  "IP number fields (starting at 1). Default 1",
  "which delimiter to use. Default |",
  (char *)NULL
};

uint8_t ipFields[3];		/* at most sIP,dIP,nhIP */
uint8_t ipCount;			/* # of above */
char delimChar;			/* field delimiter */


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
  fprintf(stderr, "%s [app_opts].... \n", skAppName());
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
  appOptionsTeardown();
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
  int nextOptIndex;

  skAppRegister(argv[0]);

  /* first do some sanity checks */
  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if (argc < NUM_REQUIRED_ARGS) {
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

  /* set defaults */
  memset(ipFields, sizeof(ipFields), 0);
  ipFields[0] = 1;
  ipCount = 1;
  delimChar = '|';

  /* parse options */
  if ( (nextOptIndex = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
    return;
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: registering appTeardown() failed\n",
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
 *	do NOT add non-option usage here (i.e., required and other
 *      optional args)
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


static int appOptionsHandler(clientData UNUSED(cData), int index,char *optarg){

  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }

  switch (index) {
  case 0:
    /* ip fields */
    parseIPFields(optarg);		/* never returns on error */
    break;

  case 1:
    /* delimiter */
    delimChar = *optarg;
    break;

  default:
    appUsage();			/* never returns */
  }
  return 0;			/* OK */
}


/*
 * parseIPFields:
 *	given a list (i.e., comma or - delimited set of numbers), set the
 *	element in fields to 1 if the fields are wanted.
 * Input:
 *	char *
 * Return:
 *	0 if OK. 1 else;
 * Side Effects:
 *	fields[]
*/

#define BUMP_IPFIELDCOUNT(f)	{if (ipCount > 2) {fprintf(stderr, "Too many ip fields given\n");exit(1);} ipFields[ipCount++] = f;}
static int parseIPFields(char *b) {
  int n, m, i;
  char *sp, *ep;
  sp = b;

  memset(ipFields, sizeof(ipFields), 0);
  ipCount = 0;
  while(*sp) {
    n = (int)strtol(sp, &ep, 10);
    if (n < 0 || n > MAX_FIELD_COUNT) {
      fprintf(stderr, "%s: invalid ip field list %d\n", skAppName(), n+1);
      return 1;			/* error */
    }
    BUMP_IPFIELDCOUNT(n);

    switch (*ep) {
    case ',':
      sp = ep + 1;
      break;

    case '-':
      sp = ep + 1;
      m = (int) strtoul(sp, &ep, 10);
      if (m <= n || m > MAX_FIELD_COUNT) {
	fprintf(stderr, "%s: invalid ip field list %d\n", skAppName(), m);
	return 1;			/* error */
      }
      /* start of range already record; record only the remainder */
      for (i = n+1; i <= m; i++) {
	BUMP_IPFIELDCOUNT(i);
      }
      if (*ep == '\0') {
	return 0;		/* OK */
      }
      if (*ep == ',') {
	sp = ep + 1;
      } else {
	fprintf(stderr, "%s: invalid ip field list %d\n", skAppName(), n);
	return 1;			/* error */
      }
      break;

    case '\0':
      return 0;

    default:
      fprintf(stderr, "%s: invalid ip field list %s\n", skAppName(), b);
      return 1;			/* error */
    }
  }

  return 0;			/* OK */
}

int main(int argc, char **argv) {
  char line[2048];
  register char *cp, *ep;
  int fn;
  ipUnion ip;

  appSetup(argc, argv);			/* never returns */

  while (1) {
    if ( fgets(line, sizeof(line), stdin) == (char *)NULL) {
      break;
    }

    cp = line;
    fn = 0;
    if (*cp != delimChar) {
      cp--;	/* trick to deal with the lack of a leading delimiter */
    }
    do {
      cp++;
      ep = strchr(cp, delimChar);
      fn++;			/* next field # */
      if (ipFields[0] == fn ||ipFields[1] == fn ||ipFields[2] == fn) {
	ip.ipnum = (unsigned int) strtoul(cp, (char **)NULL, 10);
	fprintf(stdout, "%15s%c", num2dot(ip.ipnum),delimChar);
      } else {
	/* this is not a ip number field. dump it */
	if (!ep) {
	  /* to eol */
	  fprintf(stdout, "%s", cp);
	  if (cp[strlen(cp) -1] != '\n') {
	    fprintf(stdout, "\n");
	  }
	} else {
	  *ep = '\0';
	  fprintf(stdout, "%s%c", cp, delimChar);
	}
      }
      cp = ep;	 /* point to the start of the next field */
    } while(cp);
  } /* outer loop over lines  */

  appTeardown();
  exit(0);
}
