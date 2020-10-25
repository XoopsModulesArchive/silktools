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
** 1.14
** 2004/03/05 18:07:14
** thomasm
*/


/*
** rwuniq.c
**
** Implementation of the rwuniq suite application. Takes a
** specification of the field to count over on the command line, and
** writes an human-readable ascii representation to standard out.
**
** Will eventually support at least two binary formats (serialized
** hash data and a more compact record-oriented format with no "empty"
** entries).
**
**
** mthomas.2004.03.03: it looks like rwuniq was designed to uniq on
** multiple fields, but right now it only supports a single field, so
** I'm changing the code (as of v1.13) from using tuple_counters to
** using ip_counters.
*/


#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#include "utils.h"
#include "fglob.h"
#include "rwpack.h"
#include "iochecks.h"

/* #define IP_ONLY */
/* #define GIVE_META_INFO */

/* Application-specific includes */
#include "hashlib.h"
#include "hashwrap.h"

RCSIDENT("rwuniq.c,v 1.14 2004/03/05 18:07:14 thomasm Exp");


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
static void writeColTitles(void);
static void dumpAscii(FILE *fd);
static void dumpRecordAscii(uint32_t key, uint32_t count);

/* Functions that implement tuple counting */
static void countFile(char *curFName);

/* exported functions */
void appUsage(void);            /* never returns */

/* Application-specific data */
static uint32_t g_ip_counter_size = 10000;
static IpCounter *g_ip_counter_ptr;
static int32_t g_fields = -1;
static char g_delimiter='|';
static uint32_t g_threshold =  1;
static uint32_t g_output_count = 0;

static iochecksInfoStructPtr ioISP;
static FILE *outF;
static char *outFPath = "stdout";
static int outIsPipe;
static uint8_t abortFlag = 0;

static struct {
  uint8_t binary_output;      /* Default to ascii */
  uint8_t integer_ips;
  uint8_t print_filenames;
  uint8_t tabular_output;
  uint8_t print_titles;
  uint8_t epoch_time;
  uint8_t integer_sensors;
} uniq_flags = {0, 0, 0, 1, 1, 0, 0};


/* The length of the fieldIdent_t enum and fieldTitles array. */
#define FIELD_COUNT 16

/* Create an ID for each field/column that we can possibly print.  The
 * key will be any one of these except the final one, which is the
 * count of records we saw that match the key. */
typedef enum {
  SIP_FIELD,
  DIP_FIELD,
  SPORT_FIELD,
  DPORT_FIELD,
  PROTO_FIELD,
  PACKETS_FIELD,
  BYTES_FIELD,
  FLAGS_FIELD,
  STIME_FIELD,
  ELAPSED_FIELD,
  ETIME_FIELD,
  SENSOR_FIELD,
  INPUT_FIELD,
  OUTPUT_FIELD,
  NHIP_FIELD,
  NUM_RECS_FIELD
} fieldIdent_t;

/* A name for each field */
static const char *fieldTitles[FIELD_COUNT+1] = {
  "sIP",                        /* 0 */
  "dIP",                        /* 1 */
  "sPort",                      /* 2 */
  "dPort",                      /* 3 */
  "proto",                      /* 4 */
  "packets",                    /* 5 */
  "bytes",                      /* 6 */
  "flags",                      /* 7 */
  "sTime",                      /* 8 */
  "dur",                        /* 9 */
  "eTime",                      /* 10 */
  "sensor",                     /* 11 */
  "in",                         /* 12 */
  "out",                        /* 13 */
  "nhIP",                       /* 14 */
  "count",                      /* 15 */
  (char *)NULL
};

/* -1 to remove the sentinal */
static const uint8_t fieldCount = (sizeof(fieldTitles)/sizeof(char *)  - 1);

/* column width for each field */
static uint8_t fieldWidths[FIELD_COUNT];


#define NUM_REQUIRED_ARGS       1

enum {
  UNIQ_OPT_FIELD,
  UNIQ_OPT_THRESHOLD,
  UNIQ_OPT_INTEGER_IPS,
  UNIQ_OPT_OUTPUT_PATH,
  UNIQ_OPT_PRINT_FILENAMES,
  UNIQ_OPT_DELIMITED,
  UNIQ_OPT_NO_TITLES,
  UNIQ_OPT_COPY_INPUT,
  UNIQ_OPT_EPOCH_TIME,
  UNIQ_OPT_INTEGER_SENSORS,
#ifdef ENABLE_BINARY
  UNIQ_OPT_ENABLE_BINARY,
#endif
} appOptionEnum;

static struct option appOptions[] = {
  {"field", REQUIRED_ARG, 0, UNIQ_OPT_FIELD},
  {"threshold", REQUIRED_ARG, 0, UNIQ_OPT_THRESHOLD},
  {"integer-ips", NO_ARG, 0, UNIQ_OPT_INTEGER_IPS},
  {"output-path", REQUIRED_ARG, 0, UNIQ_OPT_OUTPUT_PATH},
  {"print-filenames", NO_ARG, 0, UNIQ_OPT_PRINT_FILENAMES},
  {"delimited", OPTIONAL_ARG, 0, UNIQ_OPT_DELIMITED},
  {"no-titles", NO_ARG, 0, UNIQ_OPT_NO_TITLES},
  {"copy-input", REQUIRED_ARG, 0, UNIQ_OPT_COPY_INPUT},
  {"epoch-time", NO_ARG, 0, UNIQ_OPT_EPOCH_TIME},
  {"integer-sensors", NO_ARG, 0, UNIQ_OPT_INTEGER_SENSORS},
#ifdef ENABLE_BINARY
  {"binary", NO_ARG, 0, UNIQ_OPT_ENABLE_BINARY},
#endif
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  /* add help strings here for the applications options */
  "", /* built dynamically */
  "minimum count value required output record. Def. 1",
  "print ip numbers as integers. Def. dotted decimal.",
  "send output to given file path. Def. stdout",
  "print input filenames. Def. No",
  "delimit output w/ optional delimiter (def. '|'). Def. tabular output",
  "do not print titles",
  "copy all input to given pipe or file",
  "print times in unix epoch seconds",
  "print sensor as an integer",
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
  fprintf(stderr, "NOTE: --field is required\n");
  fglobUsage();
  exit(1);
}


/*
** setWidths:
**      set the field widths for printing if we are priting a table
**      and not delimited records.
** Input: None
** Output: None
** Side Effects:
**      fieldWidths filled in
*/
static void setWidths(void) {
  unsigned int i;

  if (uniq_flags.tabular_output == 0) {
    return;
  }

  for (i = 0; i < fieldCount; i++) {
    switch (i) {
    case SIP_FIELD:
    case DIP_FIELD:
    case NHIP_FIELD:
      /* ip numbers */
      if (uniq_flags.integer_ips) {
        fieldWidths[i] = 10;
      } else {
        fieldWidths[i] = 15;
      }
      break;

    case SPORT_FIELD:
    case DPORT_FIELD:
      /* sport and dport */
      fieldWidths[i] = 5;
      break;

    case PROTO_FIELD:
      /* proto */
      fieldWidths[i] = 5;
      break;

    case PACKETS_FIELD:
    case BYTES_FIELD:
    case FLAGS_FIELD:
      /* packets, bytes, and flags */
      fieldWidths[i] = 10;
      break;

    case STIME_FIELD:
    case ETIME_FIELD:
      /* sTime and end time*/
      if (uniq_flags.epoch_time) {
        fieldWidths[i] = 10;
      } else {
        fieldWidths[i] = 19;
      }
      break;

    case ELAPSED_FIELD:
      /* elapsed/duration  */
      fieldWidths[i] = 5;
      break;

    case SENSOR_FIELD:
      /* sensor */
      fieldWidths[i] = 6;
      break;

    case INPUT_FIELD:
    case OUTPUT_FIELD:
      /* input,output */
      fieldWidths[i] = 3;
      break;

    case NUM_RECS_FIELD:
      /* count */
      fieldWidths[i] = 10;
      break;

    default:
      fprintf(stderr, "Invalid field index %u\n", i);
      exit(1);
      break;
    } /* switch */
  }
  return;
}


static void writeColTitles(void) {
  if (uniq_flags.print_titles == 0) {
    return;
  }

  if (uniq_flags.tabular_output) {
    fprintf(outF, "%*s%c%*s\n",
            fieldWidths[g_fields], fieldTitles[g_fields], g_delimiter,
            fieldWidths[NUM_RECS_FIELD], fieldTitles[NUM_RECS_FIELD]);
  } else {
    fprintf(outF, "%s%c%s\n",
            fieldTitles[g_fields], g_delimiter,
            fieldTitles[NUM_RECS_FIELD]);
  }

  return;
}


/*
** buildHelp:
** dynamically create the field help
*/
static void buildHelp(void) {
  char *cp;
  unsigned int i;
  int len, cLen;
  char buffer[64];

  cp = (char *)malloc(1024);
  if (!cp) {
    fprintf(stderr, "Out of memory!  cp=malloc()\n");
    return;
  }
  strcpy(cp, "Field to use as key: ");
  cLen = 20 + strlen(cp);
  for (i = 0; i < NUM_RECS_FIELD; ++i) {
    len = strlen(cp);
    sprintf(buffer, "%s = %d, ", fieldTitles[i], i+1);

    if ( (cLen + strlen(buffer)) >= 70) {
      len--;                    /* eat the trailing space */
      sprintf(cp+len, "\n  ");
      len += 3;
      cLen = 0;
    }
    sprintf(cp + len, "%s", buffer);
    cLen += strlen(buffer);
  }
  len = strlen(cp);
  cp[len - 2] = '\0';  /* eat the trailing ", " */
  appHelp[0] = cp;
}


static void dumpRecordAscii(uint32_t key, uint32_t count) {
  char *cp;
  char buf[64];

  cp = buf;
  switch (g_fields) {
  case SIP_FIELD:
  case DIP_FIELD:
  case NHIP_FIELD:
    /* sIP, dIP, nhIP */
    if (uniq_flags.integer_ips) {
      sprintf(cp, "%u", key);
    } else {
      cp = num2dot(key);
    }
    break;

  case FLAGS_FIELD:
    /* tcp_flags */
    cp = tcpflags_string(key);
    break;

  case STIME_FIELD:
  case ETIME_FIELD:
    /* sTime, eTime */
    if (uniq_flags.epoch_time) {
      sprintf(cp, "%u", key);
    } else {
      cp = timestamp(key);
    }
    break;

  case SENSOR_FIELD:
    /* sensor ID */
    if ( !uniq_flags.integer_sensors ) {
      cp = (char*)sensorIdToName(key);
    } else if (SENSOR_INVALID_SID == key) {
      cp = "-1";
    } else {
      sprintf(cp, "%u", key);
    }
    break;

  default:
    /* anything else is just a number */
    sprintf(cp , "%u", key);
    break;
  }

  if (uniq_flags.tabular_output) {
    fprintf(outF, "%*s%c%*u\n",
            fieldWidths[g_fields], cp, g_delimiter,
            fieldWidths[NUM_RECS_FIELD], count);
  } else {
    fprintf(outF, "%s%c%u\n", cp, g_delimiter, count);
  }
}


/* Dump data in columnar ascii form */
void dumpAscii(FILE * UNUSED(fd)) {
  HASH_ITER it;
  uint32_t key;
  uint32_t count;
#ifdef GIVE_META_INFO
  uint32_t tCount = 0;          /* table entries */
#endif

  setWidths();
#ifdef GIVE_META_INFO
  /* A little bit of header information */
  fprintf(outF, "Accumulated %u records.  ",
          ipctr_count_entries(g_ip_counter_ptr));
  fprintf(outF, "Threshold = %u\n", g_threshold);
#endif

  /* Write out the headings */
  writeColTitles();
  /* Write out the records */
  it = ipctr_create_iterator(g_ip_counter_ptr);
  while (ipctr_iterate(g_ip_counter_ptr,&it,&key,&count)!= ERR_NOMOREENTRIES) {

#ifdef GIVE_META_INFO
    tCount++;
#endif
    if (count >= g_threshold) {
      dumpRecordAscii(key, count);
      g_output_count++;
    }
  }

#ifdef GIVE_META_INFO
  if (g_output_count == 0) {
    fprintf(outF, "No records with frequency > than cutoff.\n");
  } else {
    fprintf(outF, "%u of %u records > %u\n", g_output_count,
            tCount, g_threshold);
  }
#endif
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
  hashlib_serialize_table((HashTable*) g_ip_counter_ptr,
                          outF,
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

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  fglobTeardown();
  iochecksTeardown(ioISP);
  if (outF != stdout) {
    if (outIsPipe) {
      pclose(outF);
    } else {
      if (EOF == fclose(outF)) {
        fprintf(stderr, "%s: Error closing output file '%s': %s\n",
                skAppName(), outFPath, strerror(errno));
      }
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
  int i;
  long val;
  char *sp;

  switch (index) {
  case UNIQ_OPT_FIELD:
    if (!optarg) {
      return 1;
    }
    if (g_fields >= 0) {
      fprintf(stderr, "%s: field option given multiple times\n", skAppName());
      return 1;
    }
    sp = optarg;
    while(*sp) {
      if (!isdigit((int)*sp)) {
        fprintf(stderr,"Bad field: %s \n", optarg);
        return 1;
      }
      sp++;
    }

    i = (int) strtoul(optarg, NULL, 10);
    /* Use FIELD_COUNT-1 since use cannot specify the count field */
    if ( i < 1 || i > (FIELD_COUNT - 1) ) {
      fprintf(stderr, "Invalid field value %s\n", optarg);
      return 1;
    }
    g_fields = i - 1;
    break;

  case UNIQ_OPT_THRESHOLD:
    if (!optarg) {
      return 1;
    }
    val = strtol(optarg, NULL, 10);
    if (val < 0 || val == LONG_MIN || val == LONG_MAX) {
      fprintf(stderr, "Illegal threshold.\n");
      return 1;
    }
    g_threshold = (uint32_t) val;
    break;

  case UNIQ_OPT_INTEGER_IPS:
    uniq_flags.integer_ips = 1;
    break;

  case UNIQ_OPT_OUTPUT_PATH:
    if (!optarg) {
      fprintf(stderr, "%s: missing output path name\n", skAppName());
      return 1;
    }
    outFPath = strdup(optarg);
    if (!outFPath) {
      fprintf(stderr, "%s: strdup(): out of memory!\n", skAppName());
      exit(1);
    }
    if ( openFile(outFPath, 1 /* write */, &outF, &outIsPipe) ) {
      return 1;
    }
    break;

  case UNIQ_OPT_PRINT_FILENAMES:
    uniq_flags.print_filenames = 1;
    break;

  case UNIQ_OPT_DELIMITED:
    if (optarg) {
      g_delimiter = optarg[0];
    }
    uniq_flags.tabular_output = 0;
    break;

  case UNIQ_OPT_NO_TITLES:
    uniq_flags.print_titles = 0;
    break;

  case UNIQ_OPT_COPY_INPUT:
    if (!optarg) {
      fprintf(stderr, "Required arg to copy-input missing\n");
      return 1;
    }
    if (iochecksAllDestinations(ioISP, optarg)) {
      exit(1);
    }
    break;

  case UNIQ_OPT_EPOCH_TIME:
    uniq_flags.epoch_time = 1;
    break;

  case UNIQ_OPT_INTEGER_SENSORS:
    uniq_flags.integer_sensors = 1;
    break;

#ifdef ENABLE_BINARY
  case UNIQ_OPT_ENABLE_BINARY:
    fprintf(stderr, "Enabling binary output!\n");
    uniq_flags.binary_output = 1;
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
  int i;

  buildHelp(); /* dynamically create the field help */

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

  skAppRegister(argv[0]);
  outF = stdout;                /* default */

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
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

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }

  /* Make sure the user specified at least one field */
  if (g_fields < 0) {
    fprintf(stderr, "key field not given\n");
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
  /* if binary is selected, check that outF is not a tty */
  if (uniq_flags.binary_output) {
    if (FILEisATty(outF)) {
      fprintf(stderr, "Will not dump binary output to tty\n");
      exit(1);
    }
  }
#endif

  /* final check. See if stdout is being used for both --copy-input
  ** and as the destination for the uniq data
  */
  if (ioISP->stdoutUsed && outF == stdout) {
    fprintf(stderr, "stdout used for both --copy-input and ascii output\n");
    exit(1);
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    abortFlag = 1;              /* abort */
    appTeardown();
    exit(1);
  }

  return;                       /* OK */
}


/* Process a file */
static void countFile(char *curFName) {
  rwRec rwrec;
  rwIOStructPtr rwIOSPtr;

  if (! (rwIOSPtr = rwOpenFile(curFName, ioISP->inputCopyFD))) {
    exit(1);
  }
  if (uniq_flags.print_filenames) {
    fprintf(stderr, "%s\n", curFName);
  }

#define READ_INC(key) \
    while(rwRead(rwIOSPtr,&rwrec)) {ipctr_inc(g_ip_counter_ptr, (key));}

  switch (g_fields) {
  case SIP_FIELD:
    READ_INC(rwrec.sIP.ipnum);
    break;
  case DIP_FIELD:
    READ_INC(rwrec.dIP.ipnum);
    break;
  case SPORT_FIELD:
    READ_INC(rwrec.sPort);
    break;
  case DPORT_FIELD:
    READ_INC(rwrec.dPort);
    break;
  case PROTO_FIELD:
    READ_INC(rwrec.proto);
    break;
  case PACKETS_FIELD:
    READ_INC(rwrec.pkts);
    break;
  case BYTES_FIELD:
    READ_INC(rwrec.bytes);
    break;
  case FLAGS_FIELD:
    READ_INC(rwrec.flags);
    break;
  case STIME_FIELD:
    READ_INC(rwrec.sTime);
    break;
  case ELAPSED_FIELD:
    READ_INC(rwrec.elapsed);
    break;
  case ETIME_FIELD:
    /* eTime is computed */
    READ_INC(rwrec.sTime + rwrec.elapsed);
    break;
  case SENSOR_FIELD:
    READ_INC(rwrec.sID);
    break;
  case INPUT_FIELD:
    READ_INC(rwrec.input);
    break;
  case OUTPUT_FIELD:
    READ_INC(rwrec.output);
    break;
  case NHIP_FIELD:
    READ_INC(rwrec.nhIP.ipnum);
    break;
  default:
    fprintf(stderr, "%s: bad index\n", skAppName());
    exit(1);
    break;
  } /* switch */

  rwCloseFile(rwIOSPtr);
  return;
}


int main(int argc, char **argv) {
  char *curFName;
  int ctr;

  /* Global setup */
  appSetup(argc, argv);

  /* Create a ctr using the fields specified */
  g_ip_counter_ptr = ipctr_create_counter(g_ip_counter_size);
  if (g_ip_counter_ptr == NULL) {
    exit(1);
  }

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
  if (uniq_flags.binary_output) {
    dumpBinary(outF);
  } else {
    dumpAscii(outF);
  }
#else
  dumpAscii(outF);
#endif

  /* Done, do cleanup */
  appTeardown();                /* AOK. Dump information */
  exit(0);
}
