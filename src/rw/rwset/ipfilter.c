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
**  1.8
**  2004/03/10 23:07:18
** thomasm
*/

/*
** ipfilter.c
**
** This is a dynamic library which allows us to incorporate ip sets into
** rwfilter.
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "dynlib.h"
#include "rwpack.h"
#include "iptree.h"

RCSIDENT("ipfilter.c,v 1.8 2004/03/10 23:07:18 thomasm Exp");


/* exported functions--called by dynlib */
int setup(dynlibInfoStructPtr dlISP, dynlibSymbolId appType);
void teardown(dynlibSymbolId appType);
void optionsUsage(dynlibSymbolId appType);
int filter(rwRecPtr rwrec);
int cut(char *out, size_t len_out, char delimiter, rwRecPtr rwrec);

/* globals */
#define IPFO_SIPSET 0
#define IPFO_NOTSIPSET 2
#define IPFO_DIPSET 1
#define IPFO_NOTDIPSET 3
#define IPFO_FNLEN 256

/* private function prototypes */
static int libOptionsSetup(dynlibInfoStructPtr dlISP, dynlibSymbolId aType);
static int optionsHandler(clientData cData, int index, char *optarg);


/* Plugin's name */
static const char *pluginName = "ccfilter";
/* the options and help strings.  these vary depending on the type of
 * app that is using the shared library, and they get set to one of
 * the arrays below.
*/
static struct option *libOptions = NULL;
static char **optionsHelp = NULL;
static int libOptionsCount = -1;

/*
 * I'm allocating four pointers here, but I expect to use only two
 * at any time because of the amount of RAM involved.
*/

/* AJK. On darwin there are dynamic lib issues unless I declare these
 * static. Not sure why. Verify this is safe. Should be. */
static macroTree *sourceSet;
static macroTree *destSet;
static macroTree *sourceNot;
static macroTree *destNot;

/*
 *
 * Add options for sourceCC/destCC or whatever you want to call them
*/

/* Options and Help for filtering */
static struct option optionsFilter[] =
{
  {"sipset", REQUIRED_ARG, 0, IPFO_SIPSET},
  {"dipset", REQUIRED_ARG, 0, IPFO_DIPSET},
  {"not-sipset", REQUIRED_ARG, 0, IPFO_NOTSIPSET},
  {"not-dipset", REQUIRED_ARG, 0, IPFO_NOTDIPSET},
  {0, 0, 0, 0}
};
static char *optionsHelpFilter[] = {
  "Source IP Address Set; pass sourceIP addresses which are in this set",
  "Destination IP Address Set; pass destination IP addresses which are in this set",
  "Non-IP Address Set, pass source ip addresses which are NOT in this set",
  "Non-IP Address Set, pass destination ip addresses which are NOT in this set",
  (char*)NULL
};

/*
 * ip sets do not get cuts
*/
static struct option optionsCut[] =
{
  /* include your options for rwcut here */
  {0, 0, 0, 0}
};
static char *optionsHelpCut[] = {
  (char*)NULL
};


/*
 *  setup
 *      Called by dynlib interface code to setup this plugin.  This
 *      routine should set up options handling, if required.
 *  Arguments:
 *      --pointer to the dynamic library interface structure
 *      --Application type that is using this plugin.
 *  Returns:
 *      DYNLIB_FAILED - if processing fails
 *      DYNLIB_WONTPROCESS - if application should do normal output
 *      DYNLIB_WILLPROCESS - if this plugin takes over output
 *  Side Effects:
*/
int setup(dynlibInfoStructPtr dlISP, dynlibSymbolId appType) {
  /*
   * We're just going to memset the buffer filenames
   */
  sourceSet = NULL;
  destSet = NULL;
  sourceNot = NULL;
  destNot = NULL;
  if (libOptionsSetup(dlISP, appType)) {
    return DYNLIB_FAILED;
  };
  return DYNLIB_WONTPROCESS;
}


/*
 *  teardown
 *      Called by dynlib interface code to tear down this plugin.
 *  Arguments:
 *      --Application type that is using this plugin
 *  Returns:
 *      None.
 *  Side Effects:
 *      None.
*/
void teardown(dynlibSymbolId UNUSED(appType)) {
  return;
}


/*
 *  optionsUsage
 *      Called by dynlib interface code to allow this plugin to print
 *      the options it accepts.
 *  Arguments:
 *      --Application type that is using this plugin
 *  Returns:
 *      None.
 *  Side Effects:
 *      None.
*/
void optionsUsage(dynlibSymbolId UNUSED(appType)) {
  register int i;

  for (i = 0; i < libOptionsCount; ++i) {
    fprintf(stderr, "--%s %s. %s\n", libOptions[i].name,
            libOptions[i].has_arg ? libOptions[i].has_arg == 1
            ? "Req. Arg" : "Opt. Arg" : "No Arg", optionsHelp[i]);
  }

  return;
}


/*
 *  libOptionsSetup
 *      Sets up the options handler for this plugin
 *  Arguments:
 *      --pointer to the dynamic library interface structure
 *      --Application type that is using this plugin.
 *  Returns:
 *      0 if o.k.; nonzero otherwise.
 *  Side Effects:
 *      Options are registered.
*/
static int libOptionsSetup(dynlibInfoStructPtr dlISP, dynlibSymbolId appType) {
  int help_size = 0;

  if (-1 != libOptionsCount) {
    /* we've been here before */
    return 0;
  }
  if (DYNLIB_SHAR_FILTER == appType) {
    libOptions = optionsFilter;
    optionsHelp = optionsHelpFilter;
  } else if (DYNLIB_CUT == appType) {
    libOptions = optionsCut;
    optionsHelp = optionsHelpCut;
  } else {
    libOptionsCount = 0;
    return 0;
  }

  for (libOptionsCount=0; libOptions[libOptionsCount].name; ++libOptionsCount)
    ; /* NO BODY */
  for (help_size = 0; optionsHelp[help_size]; ++help_size)
    ; /* NO BODY */

  /* check that we have the same number of options entries and help*/
  if (help_size != libOptionsCount) {
    fprintf(stderr, "%s: mismatch in option (%d) and help (%d) counts\n",
            pluginName, libOptionsCount, help_size);
    return 1;
  }

  if (optionsRegister(libOptions, &optionsHandler, (clientData)dlISP)) {
    fprintf(stderr, "unable to register options\n");
    return 1;
  }

  return 0;
}


/*
 *  optionsHandler
 *      Called by options parser to handle one user option
 *  Arguments:
 *	clientData cData: the dynamic lib interface struct pointer
 *	int index: index into appOptions of the specific option
 *	char *optarg: the argument; 0 if no argument was required/given.
 * Returns:
 *	0 if OK. 1 else.
 * Side Effects:
 *	Relevant options are set.
*/
static int optionsHandler(clientData cData, int index, char *optarg) {
  FILE *tgtFile;
  macroTree *m = NULL;

  /* All options deal with set files, so process the file first */
  if (!optarg || 0 == strlen(optarg)) {
    fprintf(stdout, "Expected name of set file\n");
    goto ERROR;
  }
  m = (macroTree*)malloc(sizeof(macroTree));
  if (!m) {
    fprintf(stderr, "Cannot malloc macroTree.  Out of memory!\n");
    goto ERROR;
  }
  if (initMacroTree(m)) {
    fprintf(stderr, "Cannot init macroTree\n");
    goto ERROR;
  }
  tgtFile = fopen(optarg, "rb");
  if (!tgtFile) {
    fprintf(stderr, "Cannot open set file '%s': %s\n",
            optarg, strerror(errno));
    goto ERROR;
  }
  if (readMacroTree(tgtFile, m)) {
    fprintf(stderr, "Error reading set from file '%s'\n", optarg);
    goto ERROR;
  }
  fclose(tgtFile);
  
  switch (index) {
  case IPFO_SIPSET:
    if (sourceSet || sourceNot) {
      fprintf(stderr, "Source set already initialized\n");
      goto ERROR;
    }
    sourceSet = m;
    break;
  case IPFO_NOTSIPSET:
    if (sourceSet || sourceNot) {
      fprintf(stderr, "Source set already initialized\n");
      goto ERROR;
    }
    sourceNot = m;
    break;
  case IPFO_DIPSET:
    if (destSet || destNot) {
      fprintf(stderr, "Dest set already initialized\n");
      goto ERROR;
    }
    destSet = m;
    break;
  case IPFO_NOTDIPSET:
    if (destSet || destNot) {
      fprintf(stderr, "Dest set already initialized\n");
      goto ERROR;
    }
    destNot = m;
    break;
  } /* switch */

  /* check if we've done this before */
  if (dynlibCheckActive((dynlibInfoStructPtr)cData)) {
    return 0;
  }

  /* do one time expensive initialization, e.g., parse a config file,
     connect to a database, etc */
  dynlibMakeActive((dynlibInfoStructPtr)cData);
  return 0;

  /* above code jumps here on error */
 ERROR:
  if (m) {
    freeMacroTree(m);
    free(m);
  }
  return 1;
}


/*
 *  filter
 *      Called by dynlib interface code to filter records
 *  Arguments:
 *      a RW record
 *  Returns:
 *      0 to accept the record, 1 to reject the record
 *  Side Effects:
 *      None
 * Note:
 * A bug I should have caught earlier on; this function originally 
 * only did the first applicable check for an IP, so if 
 * you specified multiple trees it would only use the first one.
 *
*/
int filter(rwRecPtr rwrec)
{
  if(sourceSet) {
    if(!macroTreeCheckAddress(rwrec->sIP.ipnum, sourceSet)) {
      return 1;
    }
  };
  if(destSet) {
    if(!macroTreeCheckAddress(rwrec->dIP.ipnum, destSet)) {
      return 1;
    }
  };
  if(sourceNot) {
    if(macroTreeCheckAddress(rwrec->sIP.ipnum, sourceNot)) {
      return 1;
    } 
  };
  if(destNot) {
    if(macroTreeCheckAddress(rwrec->dIP.ipnum, destNot)) {
      return 1;
    } 
  };
  return 0;
}


/*
 *  cut
 *      Called by dynlib interface code to output records
 *  Arguments:
 *      a string buffer to hold the output
 *      the length of the buffer
 *      character to use as delimiter if routine prints multiple columns
 *      a RW record
 *  Returns:
 *      number of characters written to buffer
 *  Side Effects:
 *      None
*/
#if 0 /* UNIMPLEMENTED */
int cut(char *out, size_t len_out, char delimiter, rwRecPtr rwrec)
{
  int len = 0;

  /* generate your output */

  return len;
}
#endif /* UNIMPLEMENTED */
