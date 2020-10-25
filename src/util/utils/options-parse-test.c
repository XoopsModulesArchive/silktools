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
** 1.4
** 2004/03/10 21:20:15
** thomasm
*/

/*
**  Test the options parsing code.  Moved from options.c to this
**  external file.
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

RCSIDENT("$Id");


static struct  option filter_options[] =
{
  {"stime",	REQUIRED_ARG,	0, 0},
  {"etime",	REQUIRED_ARG,	0, 1},
  {"duration",	REQUIRED_ARG,	0, 2},
  {"sport",	REQUIRED_ARG,	0, 3},
  {"dport",	REQUIRED_ARG,	0, 4},
  {"protocol",	REQUIRED_ARG,	0, 5},
  {"bytes",	REQUIRED_ARG,	0, 6},
  {"pkts",	REQUIRED_ARG,	0, 7},
  {"flows",	REQUIRED_ARG,	0, 8},
  {"saddress",	REQUIRED_ARG,	0, 9},
  {"daddress",	REQUIRED_ARG,	0, 10},
  {"bytes_per_packet",	REQUIRED_ARG,	0, 13},
  {"pkts_per_flow",	REQUIRED_ARG,	0, 14},
  {"bytes_per_flow",	REQUIRED_ARG,	0, 15},
  {"not-saddress",	REQUIRED_ARG,	0, 16},
  {"not-daddress",	REQUIRED_ARG,	0, 17},
  {0, 0, 0, 0}
};

static struct option fglob_options[] =
{
  {"start-date", REQUIRED_ARG, 0, 1},
  {"end-date", REQUIRED_ARG, 0, 2},
  {"tcpdump", NO_ARG, 0, 3},
  {"glob", REQUIRED_ARG, 0, 4},
  {0, 0, 0, 0}
};

char *pName;
static void filterUsage(char *UNUSED(pName)) {
  unsigned int i;

  fprintf(stdout, "Filter Options:\n");
  for (i = 0; i < (sizeof(filter_options)/sizeof(struct option)) - 1; i++ ) {
    fprintf(stdout, "--%s %s\n", filter_options[i].name,
	    filter_options[i].has_arg ? filter_options[i].has_arg == 1
	    ? "Required Arg" : "Optional Arg" : "No Arg");
  }
  return;
}

static void fglobUsage(char *UNUSED(pName)) {
  unsigned int i;

  fprintf(stdout, "Fglob Options:\n");
  for (i = 0; i < (sizeof(fglob_options)/sizeof(struct option)) - 1; i++ ) {
    fprintf(stdout, "--%s %s\n", fglob_options[i].name,
	    fglob_options[i].has_arg ? fglob_options[i].has_arg == 1
	    ? "Required Arg" : "Optional Arg" : "No Arg");
  }
  return;
}

/* optHandlers */
static int filterHandler(clientData UNUSED(cData), int index, char *optarg) {
  static int optionCount = (sizeof(filter_options)/sizeof(struct option)) - 1;

  if (index < 0 || index > optionCount) {
    fprintf(stderr, "filterHandler: invalid index %d\n", index);
    return 1;			/* error */
  }

  if (index == 18) {
    filterUsage(pName);
    return 0;
  }
  fprintf(stderr, "filterHandler: %s %s %s\n",
	  filter_options[index].name,
	  filter_options[index].has_arg == 0 ? "No Arg" :
	  (filter_options[index].has_arg == 1 ? "Required Arg" : "Optional Arg"),
	  optarg ? optarg : "NULL");
  return 0;			/* OK */
}

static int fglobHandler(clientData UNUSED(cData), int index, char *optarg) {
  static int optionCount = (sizeof(fglob_options)/sizeof(struct option)) - 1;

  if (index < 0 || index > optionCount) {
    fprintf(stderr, "fglobHandler: invalid index %d\n", index);
    return 1;			/* error */
  }

  if (index == 5) {
    fglobUsage(pName);
    return 0;
  }

  fprintf(stderr, "fglobHandler: %s %s %s\n",
	  fglob_options[index-1].name,
	  fglob_options[index-1].has_arg == 0 ? "No Arg" :
	  (fglob_options[index-1].has_arg == 1 ? "Required Arg" : "Optional Arg"),
	  optarg ? optarg : "NULL");
  return 0;			/* OK */
}


int main(int argc, char **argv) {
  int nextArgIndex;

  pName = argv[0];
  if (argc < 2) {
    filterUsage(pName);
    fglobUsage(pName);
    exit(1);
  }

  if (optionsRegister(&filter_options[0], filterHandler, (clientData) 0) ) {
    fprintf(stderr, "Unable to register filter options\n");
    filterUsage(pName);
    exit(1);
  }
  if (optionsRegister(fglob_options, fglobHandler, (clientData) 0)) {
    fprintf(stderr, "Unable to register fglob options\n");
    fglobUsage(pName);
    exit(1);
  }

  nextArgIndex = optionsParse(argc, argv);
  if (nextArgIndex < argc) {
    fprintf(stdout, "Remaining command line arguments: ");
    while (nextArgIndex < argc) {
      fprintf(stdout, "[%s] ", argv[nextArgIndex]);
      nextArgIndex++;
    }
    fprintf(stdout, "\n");
  }
  exit(0);
}

