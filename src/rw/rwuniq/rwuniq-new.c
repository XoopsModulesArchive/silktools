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
** 1.7
** 2004/03/10 22:48:28
** thomasm
*/

/*
** rwuniq.c
**
** This is actually the next generation of rwuniq.c and will, eventually,
** replace the former.
**
** Takes a spec of the fields to count over on the command line, and
** writes an human-readable ascii representation to standard out or
** specified files.
**
** The output is controlled by three possible arguments:  min threshold,
** max threshold.  Only one of the three is possible.  The
** default is min threshold and is set to 1 which is, in effect, a dump
** of ALL values and reflects the default in the original rwuniq.
** An alternative way of specifying the thresholds is to user percentages:
**
**      a x-btm-pct dump records where the count is >= percent*totalCount
**      a x-top-pct dump records where the count is <= percent*totalCount
**
** Obviously, at least one field must be specified.
**
** Will eventually support at least two binary formats (serialized
** hash data and a more compact record-oriented format with no "empty"
** entries).
**
** This is an option rich application with multiple fields as options
** and the type of output required as options as well.
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "utils.h"
#include "fglob.h"
#include "rwpack.h"
#include "iochecks.h"

/* Application-specific includes */
#include "hashlib.h"
#include "hashwrap.h"

RCSIDENT("rwuniq-new.c,v 1.7 2004/03/10 22:48:28 thomasm Exp");


/* defines and typedefs */
/* #define GIVE_META_INFO */
#define FIELD_COUNT 8
#define BPP_ARRAY_LEN 0x3000

typedef uint32_t (*array_1_PtrType)[1];
typedef uint32_t (*array_2_PtrType)[1][2];


/* exported variables */

/* local functions */
static void appUsageLong(void);                 /* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);    /* never returns */
static void appOptionsUsage(void);
static int  appOptionsSetup(void);
static int  appOptionsHandler(clientData cData, int index, char *optarg);
#ifdef  ENABLE_BINARY
static void dumpBinary(void);
#endif
static void dumpAscii(void);
static void countFile(char *curFName); /* the actual work horse func */
void dumpTable(uint8_t field, IpCounter *ipTPtr);
void dumpArray(uint8_t field, array_1_PtrType arrayPtr);
/* exported functions */
void appUsage(void);            /* never returns */


/* Application-specific data */
array_1_PtrType bppAPtr, sportAPtr, dportAPtr, protoAPtr;
IpCounter *sipTPtr, *dipTPtr, *bytesTPtr, *pktsTPtr;
uint32_t arrayLengths[FIELD_COUNT] = {0,0,65536,65536,256,0,0,BPP_ARRAY_LEN};
/*
** set the default for any field to be a min thrshold of 1 and
** set the other output types to -1 to avoid them
*/

uint32_t topCounts[FIELD_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
uint32_t btmCounts[FIELD_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
uint32_t btmPcts[FIELD_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
uint32_t topPcts[FIELD_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};

char *outputPaths[FIELD_COUNT];
FILE * files[FIELD_COUNT];
uint8_t fields[FIELD_COUNT];
uint32_t numFields = 0;
char *outFPath = "stdout";
int outIsPipe[FIELD_COUNT];

/* column width for each field */
uint8_t fWidths[FIELD_COUNT];
uint8_t cWidth = 10;
uint8_t binaryFlag = 0; /* Default to ascii */
uint8_t integerIPFlag = 0;
uint8_t printFilenamesFlag = 0;
char delimiter='|';
uint8_t tabularOutputFlag = 1;
uint8_t printTitlesFlag = 1;
uint32_t rCount = 0;

iochecksInfoStructPtr ioISP;

const char *fieldTitles[] = {
  "sIP",                        /* 0 */
  "dIP",                        /* 1 */
  "sPort",                      /* 2 */
  "dPort",                      /* 3 */
  "proto",                      /* 4 */
  "bytes",                      /* 6 */
  "pkts",                       /* 5 */
  "BPP",                        /* 7 */
  (char *)NULL,
};
/* -1 to remove the sentinal */
uint8_t fieldCount = (sizeof(fieldTitles)/sizeof(char *)  - 1);
const char * cTitle = "Count";

#define NUM_REQUIRED_ARGS       1

/* local variables */
static uint8_t abortFlag = 0;

static struct option appOptions[] = {
  {"sip", NO_ARG, 0, 0},
  {"sip-btm-threshold", REQUIRED_ARG, 0, 1},
  {"sip-top-threshold", REQUIRED_ARG, 0, 2},
  {"sip-top-pct", REQUIRED_ARG, 0, 3},
  {"sip-btm-pct", REQUIRED_ARG, 0, 4},
  {"sip-output-path", REQUIRED_ARG, 0, 5},

  {"dip", NO_ARG, 0, 6},
  {"dip-btm-threshold", REQUIRED_ARG, 0, 7},
  {"dip-top-threshold", REQUIRED_ARG, 0, 8},
  {"dip-top-pct", REQUIRED_ARG, 0, 9},
  {"dip-btm-pct", REQUIRED_ARG, 0, 10},
  {"dip-output-path", REQUIRED_ARG, 0, 11},

  {"sport", NO_ARG, 0, 12},
  {"sport-btm-threshold", REQUIRED_ARG, 0, 13},
  {"sport-top-threshold", REQUIRED_ARG, 0, 14},
  {"sport-top-pct", REQUIRED_ARG, 0, 15},
  {"sport-btm-pct", REQUIRED_ARG, 0, 16},
  {"sport-output-path", REQUIRED_ARG, 0, 17},

  {"dport", NO_ARG, 0, 18},
  {"dport-btm-threshold", REQUIRED_ARG, 0, 19},
  {"dport-top-threshold", REQUIRED_ARG, 0, 20},
  {"dport-top-pct", REQUIRED_ARG, 0, 21},
  {"dport-btm-pct", REQUIRED_ARG, 0, 22},
  {"dport-output-path", REQUIRED_ARG, 0, 23},

  {"proto", NO_ARG, 0, 24},
  {"proto-btm-threshold", REQUIRED_ARG, 0, 25},
  {"proto-top-threshold", REQUIRED_ARG, 0, 26},
  {"proto-top-pct", REQUIRED_ARG, 0, 27},
  {"proto-btm-pct", REQUIRED_ARG, 0, 28},
  {"proto-output-path", REQUIRED_ARG, 0, 29},

  {"bytes", NO_ARG, 0, 30},
  {"bytes-btm-threshold", REQUIRED_ARG, 0, 31},
  {"bytes-top-threshold", REQUIRED_ARG, 0, 32},
  {"bytes-top-pct", REQUIRED_ARG, 0, 33},
  {"bytes-btm-pct", REQUIRED_ARG, 0, 34},
  {"bytes-output-path", REQUIRED_ARG, 0, 35},

  {"pkts", NO_ARG, 0, 36},
  {"pkts-btm-threshold", REQUIRED_ARG, 0, 37},
  {"pkts-top-threshold", REQUIRED_ARG, 0, 38},
  {"pkts-top-pct", REQUIRED_ARG, 0, 39},
  {"pkts-btm-pct", REQUIRED_ARG, 0, 40},
  {"pkts-output-path", REQUIRED_ARG, 0, 41},

  {"bpp", NO_ARG, 0, 42},
  {"bpp-btm-threshold", REQUIRED_ARG, 0, 43},
  {"bpp-top-threshold", REQUIRED_ARG, 0, 44},
  {"bpp-top-pct", REQUIRED_ARG, 0, 45},
  {"bpp-btm-pct", REQUIRED_ARG, 0, 46},
  {"bpp-output-path", REQUIRED_ARG, 0, 47},

  {"integer-ips", NO_ARG, 0, 48},
  {"print-filenames", NO_ARG, 0, 49},
  {"delimited", OPTIONAL_ARG, 0, 50},
  {"no-titles", NO_ARG, 0, 51},
#ifdef ENABLE_BINARY
  {"binary", NO_ARG, 0, 52},
#endif
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  "use sip as key",
  "print sip with count >= threshold",
  "print sip with count <= threshold",
  "print sip with count >= percentage of total",
  "print sip with count <= percentage of total",
  "print sip output to file",

  "use dip as key",
  "print dip with count >= threshold",
  "print dip with count <= threshold",
  "print dips with count >= percentage of total",
  "print dips with count <= percentage of total",
  "print dip output to file",

  "use sport as key",
  "print sport with count >= threshold",
  "print sport with count <= threshold",
  "print sports with count >= percentage of total",
  "print sports with count <= percentage of total",
  "print sport output to file",

  "use dport as key",
  "print dport with count >= threshold",
  "print dport with count <= threshold",
  "print dports with count >= percentage of total",
  "print dports with count <= percentage of total",
  "print dport output to file",

  "use proto as key",
  "print proto with count >= threshold",
  "print proto with count <= threshold",
  "print protos with count >= percentage of total",
  "print protos with count <= percentage of total",
  "print proto output to file",

  "use bytes as key",
  "print bytes with count >= threshold",
  "print bytes with count <= threshold",
  "print bytess with count >= percentage of total",
  "print bytess with count <= percentage of total",
  "print bytes output to file",

  "use pkts as key",
  "print pkts with count >= threshold",
  "print pkts with count <= threshold",
  "print pktss with count >= percentage of total",
  "print pktss with count <= percentage of total",
  "print pkts output to file",

  "use bpp as key",
  "print bpp with count >= threshold",
  "print bpp with count <= threshold",
  "print bpps with count >= percentage of total",
  "print bpps with count <= percentage of total",
  "print bpp output to file",

  "print ip numbers as integers. Def. dotted decimal.",
  "print input filenames. Def. No",
  "delimit output w/ optional delimiter (def. '|'). Def. tabular output",
  "do not print titles",
#ifdef ENABLE_BINARY
  "Output in binary (hash serialization) format. Default ascii",
#endif
  (char *)NULL
};


#define DUMP_MSGS


/*
 * appUsage:
 *      print usage information to stderr and exit with code 1
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
 *      print usage information to stderr and exit with code 1.
 *      passed to optionsSetup() to print usage when --help option
 *      given.
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
static void appUsageLong(void) {
  fprintf(stderr, "%s [app_opts] [fglob_opts] .... \n", skAppName());
  appOptionsUsage();
  fprintf(stderr, "NOTE: one of the field options is required\n");
  fprintf(stderr, "--sip, --dip, --sport, --dport, --proto, ");
  fprintf(stderr, "--bytes, --pkts, or --bpp\n");
  fglobUsage();
  exit(1);
}

/*
 * setWidths:
 *      set the field widths for printing if we are priting a table
 *      and not delimited records.
 * Input: None
 * Output: None
 * Side Effects:
 *      fWidths filled in
 */
static  void setWidths(void) {
  if (tabularOutputFlag == 0) {
    /* reset all output widths to 1 */
    memset(fWidths, 1, sizeof(fWidths));
    cWidth = 1;
    return;
  }
  if (integerIPFlag) {
    fWidths[0] = fWidths[1] = 10;
  } else {
    fWidths[0] = fWidths[1] = 15;
  }
  fWidths[2] =   fWidths[3] = 5; /* sport & dport */
  fWidths[4] = 3;       /* proto */
  fWidths[5] = fWidths[6] = 10; /* pkts & bytes */
  fWidths[7] = 5;               /* bpp */
  return;
}

/*
** dumpAscii
**      dump output data to desired location in ascii
*/

void dumpAscii(void) {
  register uint32_t i;

  for (i = 0; i < numFields; i++) {
    switch (fields[i]) {
    case 1:                     /* sip */
      dumpTable(fields[i], sipTPtr);
      break;

    case 2:                     /* dip */
      dumpTable(fields[i], dipTPtr);
      break;

    case 3:                     /* sport */
      dumpArray(fields[i], sportAPtr);
      break;

    case 4:                     /* dport */
      dumpArray(fields[i], dportAPtr);
      break;

    case 5:                     /* proto */
      dumpArray(fields[i], protoAPtr);
      break;

    case 6:                     /* bytes */
      dumpTable(fields[i], bytesTPtr);
      break;

    case 7:                     /* pkts */
      dumpTable(fields[i], pktsTPtr);
      break;

    case 8:                     /* bpp */
      dumpArray(fields[i], bppAPtr);
      break;
    }
  }
  return;
}

/*
** dumpTable
**      dump information for the given field and table
** Inputs: field number and table pointer
** Outputs: Appropriate data is dump to the opened file
** Side Effects: the table is freed after dumping
*/
#define DUMP(f,k,c) (f == 0 || f == 1) ? \
  integerIPFlag ? \
  fprintf(files[f], "%*u%c%*u\n", fWidths[f], k, delimiter, cWidth, c)\
  : fprintf(files[f], "%*s%c%*u\n", fWidths[f], num2dot(k),\
       delimiter, cWidth, c)\
  : fprintf(files[f], "%*u%c%*u\n", fWidths[f], k, delimiter, cWidth, c)

void dumpTable(uint8_t field, IpCounter *ipTPtr) {
  uint32_t key, count;
  register uint32_t cutOff;
  HASH_ITER iterator;

  field--;                      /* reduce to 0 origin */
  if (printTitlesFlag) {
    fprintf(files[field], "%*s%c%*s\n", fWidths[field], fieldTitles[field],
            delimiter, cWidth, cTitle);
  }
  iterator = ipctr_create_iterator(ipTPtr);
  if (topCounts[field]) {
    while (ipctr_iterate(ipTPtr, &iterator, &key, &count)
           != ERR_NOMOREENTRIES) {
      if (count >= topCounts[field]) {
        DUMP(field, key, count);
      }
    }
  } else   if (btmCounts[field]) {
    while (ipctr_iterate(ipTPtr, &iterator, &key, &count)
           != ERR_NOMOREENTRIES) {
      if (count <= btmCounts[field]) {
        DUMP(field, key, count);
      }
    }
  } else if (topPcts[field]) {
    cutOff = topPcts[field] * rCount / 100;
    while (ipctr_iterate(ipTPtr, &iterator, &key, &count)
             != ERR_NOMOREENTRIES) {
      if (count <= cutOff) {
        DUMP(field, key, count);
      }
    }
  } else if (btmPcts[field]) {
    cutOff = topPcts[field] * rCount / 100;
    while (ipctr_iterate(ipTPtr, &iterator, &key, &count)
             != ERR_NOMOREENTRIES) {
      if (count <= cutOff) {
        DUMP(field, key, count);
      }
    }
  }
  ipctr_free(ipTPtr);
  return;
}


/*
** dumpArray
**      dump information for the given field and array
** Inputs: field number and array pointer
** Outputs: Appropriate data is dump to the opened file
** Side Effects: the array is freed after dumping
*/

void dumpArray(uint8_t field, array_1_PtrType arrayPtr) {
  register uint32_t i;
  register uint32_t cutOff;

  field--;                      /* reduce to 0 origin */
  if (printTitlesFlag) {
    fprintf(files[field], "%*s%c%*s\n", fWidths[field], fieldTitles[field],
            delimiter,  cWidth, cTitle);
  }
  if (topCounts[field]) {
    for (i = 0; i < arrayLengths[field]; i++ ) {
      if ((*arrayPtr)[i] >= topCounts[field]) {
        DUMP(field, i, (*arrayPtr)[i]);
      }
    }
  } else if (btmCounts[field]) {
    for (i = 0; i < arrayLengths[field]; i++ ) {
      if ((*arrayPtr)[i] && (*arrayPtr)[i] <= btmCounts[field]) {
        DUMP(field, i, (*arrayPtr)[i]);
      }
    }
  } else if (topPcts[field]) {
    cutOff = topPcts[field] * rCount / 100;
    for (i = 0; i < arrayLengths[field]; i++ ) {
      if ((*arrayPtr)[i] && (*arrayPtr)[i] <= cutOff) {
        DUMP(field, i, (*arrayPtr)[i]);
      }
    }
  } else if (btmPcts[field]) {
    cutOff = topPcts[field] * rCount / 100;
    for (i = 0; i < arrayLengths[field]; i++ ) {
      if ((*arrayPtr)[i] && (*arrayPtr)[i] <= cutOff) {
        DUMP(field, i, (*arrayPtr)[i]);
      }
    }
  }
  free(arrayPtr);
  return;
}

#ifdef  ENABLE_BINARY
/* Serialize the hash table data in binary form */
void dumpBinary(void) {
  genericHeader header;

  /* Build the header */
  PREPHEADER(&header);
  header.type = FT_RWUNIQ_DATA;
  header.version = RWUNIQ_FILE_VERSION;

  /* Write the data, prefixed with the header */
  hashlib_serialize_table((HashTable*) tuple_counter_ptr,
                          stdout,
                          (uint8_t*) &header, sizeof(header));
}
#endif

/*
 * appTeardown:
 *      teardown all used modules and all application stuff.
 * Arguments: None.
 * Returns: None
 * Side Effects:
 *      All modules are torn down. Then application teardown takes place.
 *      However, global variable abortFlag controls whether to dump info.
 * NOTE: This must be idempotent using static teardownFlag.
*/
static void appTeardown(void) {
  static int teardownFlag = 0;
  register uint32_t i;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  fglobTeardown();
  iochecksTeardown(ioISP);
  for (i = 0; i < numFields; i++) {
    if (files[i] != stdout) {
      outIsPipe[i] ? pclose(files[i]) : fclose(files[i]);
    }
  }
  return;
}


/*
 * appOptionsHandler:
 *      called for each option the app has registered.
 * Arguments:
 *      clientData cData: ignored here.
 *      int index: index into appOptions of the specific option
 *      char *optarg: the argument; 0 if no argument was required/given.
 * Returns:
 *      0 if OK. 1 else.
 * Side Effects:
 *      Relevant options are set.
 */
static int appOptionsHandler(clientData UNUSED(cData), int index,char *optarg){
  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }
  switch (index) {
    case 0:        /* sip  */
    fields[numFields++] = 1;
    break;
    case 1:                     /* sip-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[0] = (int) strtoul(optarg, NULL, 10);
    break;
    case 2:                     /* sip-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[0] = (int) strtoul(optarg, NULL, 10);
    break;
    case 3:                     /* sip-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[0] = (int) strtoul(optarg, NULL, 10);
    break;
    case 4:                     /* sip-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[0] = (int) strtoul(optarg, NULL, 10);
    break;
    case 5:                     /* sip-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[0] = optarg;
    if (openFile(outputPaths[0], 1/*write*/, &files[0], &outIsPipe[0]) ) {
      return 1;
    }
    break;

    case 6:        /* dip  */
    fields[numFields++] = 2;
    break;
    case 7:                     /* dip-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[1] = (int) strtoul(optarg, NULL, 10);
    break;
    case 8:                     /* dip-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[1] = (int) strtoul(optarg, NULL, 10);
    break;
    case 9:                     /* dip-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[1] = (int) strtoul(optarg, NULL, 10);
    break;
    case 10:                    /* dip-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[1] = (int) strtoul(optarg, NULL, 10);
    break;
    case 11:                    /* dip-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[1] = optarg;
    if ( openFile(outputPaths[1], 1/*write*/, &files[1], &outIsPipe[1]) ) {
      return 1;
    }
    break;

    case 12:        /* sport  */
    fields[numFields++] = 3;
    break;
    case 13:                    /* sport-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[2] = (int) strtoul(optarg, NULL, 10);
    break;
    case 14:                    /* sport-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[2] = (int) strtoul(optarg, NULL, 10);
    break;
    case 15:                    /* sport-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[2] = (int) strtoul(optarg, NULL, 10);
    break;
    case 16:                    /* sport-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[2] = (int) strtoul(optarg, NULL, 10);
    break;
    case 17:                    /* sport-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[2] = optarg;
    if ( openFile(outputPaths[2], 1/*write*/, &files[2], &outIsPipe[2]) ) {
      return 1;
    }
    break;

    case 18:        /* dport  */
    fields[numFields++] = 4;
    break;
    case 19:                    /* dport-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[3] = (int) strtoul(optarg, NULL, 10);
    break;
    case 20:                    /* dport-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[3] = (int) strtoul(optarg, NULL, 10);
    break;
    case 21:                    /* dport-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[3] = (int) strtoul(optarg, NULL, 10);
    break;
    case 22:                    /* dport-btm-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[3] = (int) strtoul(optarg, NULL, 10);
    break;
    case 23:                    /* dport-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[3] = optarg;
    if ( openFile(outputPaths[3], 1/*write*/, &files[3], &outIsPipe[3]) ) {
      return 1;
    }
    break;

    case 24:        /* proto  */
    fields[numFields++] = 5;
    break;
    case 25:                    /* proto-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[4] = (int) strtoul(optarg, NULL, 10);
    break;
    case 26:                    /* proto-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[4] = (int) strtoul(optarg, NULL, 10);
    break;
    case 27:                    /* proto-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[4] = (int) strtoul(optarg, NULL, 10);
    break;
    case 28:                    /* proto-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[4] = (int) strtoul(optarg, NULL, 10);
    break;
    case 29:                    /* proto-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[4] = optarg;
    if ( openFile(outputPaths[4], 1/*write*/, &files[4], &outIsPipe[4]) ) {
      return 1;
    }
    break;

    case 30:        /* bytes  */
    fields[numFields++] = 6;
    break;
    case 31:                    /* bytes-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[5] = (int) strtoul(optarg, NULL, 10);
    break;
    case 32:                    /* bytes-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[5] = (int) strtoul(optarg, NULL, 10);
    break;
    case 33:                    /* bytes-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[5] = (int) strtoul(optarg, NULL, 10);
    break;
    case 34:                    /* bytes-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[5] = (int) strtoul(optarg, NULL, 10);
    break;
    case 35:                    /* bytes-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[5] = optarg;
    if ( openFile(outputPaths[5], 1/*write*/, &files[5], &outIsPipe[5]) ) {
      return 1;
    }
    break;

    case 36:        /* pkts  */
    fields[numFields++] = 7;
    break;
    case 37:                    /* pkts-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[6] = (int) strtoul(optarg, NULL, 10);
    break;
    case 38:                    /* pkts-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[6] = (int) strtoul(optarg, NULL, 10);
    break;
    case 39:                    /* pkts-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[6] = (int) strtoul(optarg, NULL, 10);
    break;
    case 40:                    /* pkts-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[6] = (int) strtoul(optarg, NULL, 10);
    break;
    case 41:                    /* pkts-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[6] = optarg;
    if ( openFile(outputPaths[6], 1/*write*/, &files[6], &outIsPipe[6]) ) {
      return 1;
    }
    break;

    case 42:        /* bpp  */
    fields[numFields++] = 8;
    break;
    case 43:                    /* bpp-btm-threshold */
    if (!optarg) {
      return 1;
    }
    topCounts[7] = (int) strtoul(optarg, NULL, 10);
    break;
    case 44:                    /* bpp-top-threshold */
    if (!optarg) {
      return 1;
    }
    btmCounts[7] = (int) strtoul(optarg, NULL, 10);
    break;
    case 45:                    /* bpp-top-pct */
    if (!optarg) {
      return 1;
    }
    topPcts[7] = (int) strtoul(optarg, NULL, 10);
    break;
    case 46:                    /* bpp-btm-pct */
    if (!optarg) {
      return 1;
    }
    btmPcts[7] = (int) strtoul(optarg, NULL, 10);
    break;
    case 47:                    /* bpp-output path */
    if (!optarg) {
      return 1;
    }
    outputPaths[7] = optarg;
    if ( openFile(outputPaths[7], 1/*write*/, &files[7], &outIsPipe[7]) ) {
      return 1;
    }
    break;


    case 48:                    /* integer-ips */
    integerIPFlag = 1;
    break;

    case 49:                    /* print filenames */
    printFilenamesFlag = 1;
    break;

    case 50:                    /* delimiter */
    if (optarg) {
      delimiter = optarg[0];
    }
    tabularOutputFlag = 0;
    break;

    case 51:
    printTitlesFlag = 0;
    break;

#ifdef ENABLE_BINARY
    case 52:
    fprintf(stderr, "Enabling binary output!\n");
    binaryFlag = 1;
    break;
#endif

    default:
    appUsage();                 /* never returns */
    return 0;
  }
  return 0;                     /* OK */
}


/*
 * appOptionsUsage:
 *      print options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
*/

static void appOptionsUsage(void) {
    register int i;
    fprintf(stderr, "\napp options:\n");
    for (i = 0; i < appOptionCount; i++ ) {
        fprintf(stderr, "--%s %s. %s\n", appOptions[i].name,
                appOptions[i].has_arg ? appOptions[i].has_arg == 1
                ? "Req. Arg" : "Opt. Arg" : "No Arg", appHelp[i]);
    }

    return;
}

/*
 * appOptionsSetup:
 *      setup to parse application options.
 * Arguments: None.
 * Returns:
 *      0 OK; 1 else.
 */
static int  appOptionsSetup(void) {
  /* register the apps  options handler */
  if (optionsRegister(appOptions, (optHandler) appOptionsHandler,
                (clientData) 0)) {
    fprintf(stderr, "%s: unable to register application options\n",
            skAppName());
    return 1;                   /* error */
  }
  return 0;                     /* OK */
}


/*
 * appSetup
 *      do all the setup for this application include setting up required
 *      modules etc.
 * Arguments:
 *      argc, argv
 * Returns: None.
 * Side Effects:
 *      exits with code 1 if anything does not work.
 */
static void appSetup(int argc, char **argv) {
  register uint32_t i;
  register int j;
  skAppRegister(argv[0]);

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option %lu and help count %d\n",
            ((unsigned long)(sizeof(appHelp)/sizeof(char *) - 1)),
            appOptionCount);
    exit(1);
  }
  if (fieldCount != FIELD_COUNT) {
    fprintf(stderr, "mismatch in fieldCount %u and FIELD_COUNT %u\n",
            fieldCount, FIELD_COUNT);
    exit(1);
  }

  if ((argc-1) < NUM_REQUIRED_ARGS) {
    appUsage();         /* never returns */
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);
  }

  /* allow for 0 input and 0 output pipes */
  ioISP = iochecksSetup(0, 0, argc, argv);

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);
  }

  /* set defaults */
  memset(fields, 0, sizeof(fields));/* no fields */

  for (i = 0; i < FIELD_COUNT; i++) {
    outputPaths[i] = outFPath; /* all to stdout */
    files[i] = stdout;          /* default */
  }

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }

  /* Make sure the user specified at least one field */
  if (numFields == 0) {
    fprintf(stderr, "No key field given\n");
    appUsage();         /* never returns */
  }

  /*
  ** Input file checks:  if libfglob is NOT used, then check
  ** if files given on command line.  Do NOT allow both libfglob
  ** style specs and cmd line files. If cmd line files are NOT given,
  ** then default to stdin.
  */
  if (fglobValid()) {
    if (ioISP->firstFile < argc) {
      fprintf(stderr, "Error: both fglob options & input files given\n");
      exit(1);
    }
  } else {
    if (ioISP->firstFile == argc) {
      if (FILEIsATty(stdin)) {
        appUsage();             /* never returns */
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

#ifdef ENABLE_BINARY
  /* if binary is selected, check that output files are tty */
  if (binaryFlag) {
    for (i = 0; i < FIELD_COUNT; i++) {
      if (files[i]) {
        fprintf(stderr, "Will not dump binary output to tty\n");
        exit(1);
      }
    }
  }
#endif

  /* now setup for counting */
  for (i = 0; i < numFields; i++) {
    /* set the default output type if the user has not give one */
    j = fields[i] - 1;
    if (topCounts[j] == 0 && btmCounts[j] == 0
        && topPcts[j] == 0 && btmPcts[j] == 0) {
      topCounts[j] = 1;
    }
    switch (fields[i]) {
    case 0:                     /* not wanted */
      break;
    case 1:                     /* sip */
      if (! (sipTPtr = ipctr_create_counter(0xFFFFF))) {
        fprintf(stderr, "Unable to create ipcounter\n");
        exit(1);
      }
      break;
    case 2:                     /* dip */
      if (! (dipTPtr = ipctr_create_counter(0xFFFFF))) {
        fprintf(stderr, "Unable to create ipcounter\n");
        exit(1);
      }
      break;

    case 3:                     /* sport */
      if ( !(sportAPtr = (array_1_PtrType) calloc(sizeof(uint32_t),
                                                  arrayLengths[2]))) {
        fprintf(stderr, "Unable to get space for array\n");
        exit(1);
      }
      break;

    case 4:                     /* dport */
      if ( !(dportAPtr = (array_1_PtrType) calloc(sizeof(uint32_t),
                                                  arrayLengths[3]))) {
        fprintf(stderr, "Unable to get space for array\n");
        exit(1);
      }
      break;

    case 5:                     /* proto */
      if ( !(protoAPtr = (array_1_PtrType) calloc(sizeof(uint32_t),
                                                  arrayLengths[4]))) {
        fprintf(stderr, "Unable to get space for array\n");
        exit(1);
      }
      break;

    case 6:                     /* bytes */
      if (! (bytesTPtr = ipctr_create_counter(0xFFFFF))) {
        fprintf(stderr, "Unable to create ipcounter\n");
        exit(1);
      }
      break;

    case 7:                     /* pkts */
      if (! (pktsTPtr = ipctr_create_counter(0xFFFFF))) {
        fprintf(stderr, "Unable to create ipcounter\n");
        exit(1);
      }
      break;

    case 8:                     /* bpp. Set aside 12000 spaces */
      if ( !(bppAPtr = (array_1_PtrType) calloc(sizeof(uint32_t),
                                                arrayLengths[7]))) {
        fprintf(stderr, "Unable to get space for array\n");
        exit(1);
      }
      break;
    }
  }

  setWidths();

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    abortFlag = 1;              /* abort */
    appTeardown();
    exit(1);
  }

  return;                       /* OK */
}

/*
** countFile:
**      Open the file; read records; and do the required counting; close file
** Inputs: FName
** Outputs: None
** Side Effects: tables and arrays incremented
*/
static void countFile(char *curFName) {
    rwRec rwrec;
    rwIOStructPtr rwIOSPtr;
    register uint32_t i, bpp;

    if (! (rwIOSPtr = rwOpenFile(curFName, ioISP->inputCopyFD))) {
      exit(1);
    }

    if (printFilenamesFlag) {
      fprintf(stderr, "%s\n", curFName);
    }
    while (rwRead(rwIOSPtr, &rwrec)) {
      rCount++;
      for (i = 0; i < numFields; i++) {
        switch (fields[i]) {
        case 1:                 /* sip */
          ipctr_inc(sipTPtr, rwrec.sIP.ipnum);
          break;

        case 2:                 /* dip */
          ipctr_inc(dipTPtr, rwrec.dIP.ipnum);
          break;

        case 3:                 /* sport */
          (*sportAPtr)[rwrec.sPort]++;
          break;

        case 4:                 /* dport */
          (*dportAPtr)[rwrec.dPort]++;
          break;

        case 5:                 /* proto */
          (*protoAPtr)[rwrec.proto]++;
          break;

        case 6:                 /* bytes */
          ipctr_inc(bytesTPtr, rwrec.bytes);
          break;

        case 7:                 /* pkts */
          ipctr_inc(pktsTPtr, rwrec.pkts);
          break;

        case 8:                 /* bpp */
          bpp = rwrec.bytes/rwrec.pkts;
          (*bppAPtr)[bpp]++;
          break;
        }
      }
    }
    rwCloseFile(rwIOSPtr);
    return;
}

int main(int argc, char **argv) {
    char *curFName;
    int ctr;

    /* Global setup */
    appSetup(argc, argv);

    /* Iterate over the files to process */
    if (fglobValid()) {
        while ((curFName = fglobNext())){
            countFile(curFName);
        }
    } else {
        for(ctr = ioISP->firstFile; ctr < ioISP->fileCount ; ctr++) {
            countFile(ioISP->fnArray[ctr]);
        }
    }

    /* Generate output */
#ifdef  ENABLE_BINARY
    if (binaryFlag) {
      dumpBinary();
    } else {
      dumpAscii();
    }
#else
      dumpAscii();
#endif

    /* Done, do cleanup */
    appTeardown();              /* AOK. Dump information */
    exit(0);

    return 0; /* quiet gcc */
}
