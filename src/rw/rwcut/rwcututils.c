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
** 1.38
** 2004/02/18 15:07:23
** thomasm
*/

/*
** rwcututils.c
**      utility routines in support of rwcut.
** Suresh Konda
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cut.h"

RCSIDENT("rwcututils.c,v 1.38 2004/02/18 15:07:23 thomasm Exp");


#define DEFAULT_NUM_RECS 10

/* The last field printed by default.  This is from the user's
 * perspective, so fields are numbered from 1. */
#define RWCUT_LAST_DEFAULT_FIELD 12


/* local functions */
static void appUsageLong(void);
static void appOptionsUsage(void);
static void setWidths(void);
static int  appOptionsSetup(void);
static void appOptionsTeardown(void);
static int  appOptionsHandler(clientData cData, int index, char *optarg);
static void buildHelp(void);
static int  cutAddDynlib(const char *dlpath, int warnOnError);


/* local variables */
enum {
  CUT_OPT_FIELDS,
  CUT_OPT_INTEGER_IPS,
  CUT_OPT_NUM_RECS,
  CUT_OPT_START_REC_NUM,
  CUT_OPT_END_REC_NUM,
  CUT_OPT_NO_TITLES,
  CUT_OPT_DELIMITED,
  CUT_OPT_EPOCH_TIME,
  CUT_OPT_ICMP_TYPE_AND_CODE,
  CUT_OPT_DYNAMIC_LIBRARY,
  CUT_OPT_INTEGER_SENSORS
} appOptionEnum;

static struct option appOptions[] = {
  {"fields",    REQUIRED_ARG,   0, CUT_OPT_FIELDS},
  {"integer-ips", NO_ARG, 0, CUT_OPT_INTEGER_IPS},
  {"num-recs", REQUIRED_ARG, 0, CUT_OPT_NUM_RECS},
  {"start-rec-num", REQUIRED_ARG, 0, CUT_OPT_START_REC_NUM},
  {"end-rec-num", REQUIRED_ARG, 0, CUT_OPT_END_REC_NUM},
  {"no-titles", NO_ARG, 0, CUT_OPT_NO_TITLES},
  {"delimited", OPTIONAL_ARG, 0, CUT_OPT_DELIMITED},
  {"epoch-time", NO_ARG, 0, CUT_OPT_EPOCH_TIME},
  {"icmp-type-and-code", NO_ARG, 0, CUT_OPT_ICMP_TYPE_AND_CODE},
  {OPT_DYNAMIC_LIBRARY, REQUIRED_ARG, 0, CUT_OPT_DYNAMIC_LIBRARY},
  {"integer-sensors", NO_ARG, 0, CUT_OPT_INTEGER_SENSORS},
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  "", /* generated dynamically */
  "print ip # as an integer",
  "N number of recordes to print (Def. 10 if to stdout. all else.). 0 => all",
  "N starting record number (Def. 1)",
  "N ending record number (Def. 10)",
  "do not print column headers",
  "delimit output using delimiter (Def. no; delimiter |)",
  "print times in unix epoch seconds",
  "print icmp type and code",
  "use given dynamic library. Default no",
  "print sensor as an integer",
  (char *)NULL
};

static countersStruct globalCounters;

static const char *fieldNames[] = {
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
  (char *)NULL
};

const char *fieldTitles[] = {
  "sIP",                        /* 0 */
  "dIP",                        /* 1 */
  "sPort",                      /* 2 */
  "dPort",                      /* 3 */
  "pro",                        /* 4 */
  "packets",                    /* 5 */
  "bytes",                      /* 6 */
  "flags",                      /* 7 */
  "sTime",                      /* 8 */
  "dur",                        /* 9 */
  "eTime",                      /* 10 */
  "sr",                         /* 11 */
  "in",                         /* 12 */
  "out",                        /* 13 */
  "nhIP",                       /* 14 */
  (char *)NULL
};


/* -1 to remove the sentinal; range of field values is 0 =< n < fieldCount */
static uint8_t fieldCount = (sizeof(fieldNames)/sizeof(char *)  - 1);

/* number of fields the user has selected */
uint8_t maxFieldsSelected;

/* where in the row does a desired field come?  user enters this as a
 * 1-based array, but we map it back to a zero-based array */
uint8_t *fieldPositions = NULL;

/* size of the fieldPositions array.  Because of duplicates, may be
 * larger than fieldCount */
static uint8_t fieldPositionsSize;

/* column width for each field */
uint8_t *fWidths;


/*
** appUsage:
**      print usage information to stderr and exit with code 1
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
static void appUsage(void) {
  fprintf(stderr, "Use `%s --help' for usage\n", skAppName());
  exit(1);
}


/*
** appUsageLong:
**      print usage information to stderr and exit with code 1.
**      passed to optionsSetup() to print usage when --help option
**      given.
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
static void appUsageLong(void) {
  fprintf(stderr,"%s [rwcut_opts] [fglob_opts | fileglobspec]\n", skAppName());
  fprintf(stderr, "Default: read from stdin\n");
  appOptionsUsage();
  fglobUsage();
  exit(1);
}


/*
** appTeardown:
**      teardown all used modules and all application stuff.
** Arguments: None
** Returns: None
** Side Effects:
**      All modules are torn down. Then application teardown takes place.
**      Static variable teardownFlag is set.
** NOTE: This must be idempotent using static teardownFlag.
*/
void appTeardown(void) {
  static uint8_t teardownFlag = 0;
  int j;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;
  fglobTeardown();

  iochecksTeardown(ioISP);

  /* Dynamic library teardown */
  for (j = 0; j < cutDynlibCount; ++j) {
    dynlibTeardown(cutDynlibList[j].dlisp);
    cutDynlibList[j].dlisp = NULL;
  }
  cutDynlibCount = 0;

  appOptionsTeardown();

  if (rwIOSPtr) {
    rwCloseFile(rwIOSPtr);
  }
  rwIOSPtr = (rwIOStructPtr)NULL;

  return;
}


/*
** appSetup
**      do all the setup for this application include setting up required
**       modules etc.
** Arguments:
**      argc, argv
** Returns: None.
** Side Effects:
**      exits with code 1 if anything does not work.
*/
void appSetup(int argc, char **argv) {
  register int i;
  int j;
  int sportSelected, dportSelected;

  skAppRegister(argv[0]);

  /* try to load run-time dynamic libraries */
  for (j = 0; cutDynlibNames[j]; ++j) {
    cutAddDynlib(cutDynlibNames[j], 0);
  }

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  /* set defaults */

  globalCounters.numRecs = 0;
  globalCounters.firstRec = 1;
  globalCounters.lastRec = 0;

  flags.dottedip = 1;
  flags.printTitles = 1;
  flags.timestamp = 1;
  flags.fromEOF = 0;
  flags.rulesCheck = 0;
  flags.printFileHeader = 0;
  flags.printFileName = 0;
  flags.delimited = 0;
  flags.unmapPort = 1;
  flags.icmpTandC = 0;
  flags.integerSensor = 0;

  delimiter = '|';

  /* initialize fieldPositionsSize to 2*fieldCount */
  fieldPositionsSize = ((fieldCount < 127) ? 2*fieldCount : fieldCount);

  /* do not allow ANY output files other than stdout */
  ioISP = iochecksSetup(1, 0, argc, argv);

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);
  }

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);
  }

  if (argc < NUM_REQUIRED_ARGS) {
    appUsage();         /* never returns */
  }

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();         /* never returns */
  }

  if (!fieldPositions) {
    /* set fieldPositions to all fields in "normal" order */
    fieldPositions = calloc(fieldPositionsSize, sizeof(uint8_t));
    if (!fieldPositions) {
      fprintf(stderr, "out of memory! fieldPositions=calloc()\n");
      exit(1);
    }
    maxFieldsSelected = RWCUT_LAST_DEFAULT_FIELD;
    for (i = 0; i < maxFieldsSelected; i++) {
      fieldPositions[i] = i;
    }
  }

  if (fglobValid()) {
    /*
     * glob spec is not set via the --glob approach. See if the user
     * has given it an non-option argument
     */
    if (ioISP->firstFile < argc) {
      fprintf(stderr,
              "%s: argument mismatch:Both fglob options & input files given\n",
              skAppName());
      exit(1);
    }
  } else {
    /* fglob not used. Check for input files on the command line */
    if (ioISP->firstFile == argc) {
      /*
      ** none  on command line.  Register our own with
      ** iochecksPassDestinations() iff stdin is NOT a tty
      */
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

  (void)umask((mode_t) 0002);

  /*
  ** if icmpTandC is set, we must force sPort and dPort if they are not
  ** already set.
  */
  if (flags.icmpTandC) {
    sportSelected = dportSelected = 0;
    for (i = 0; i < maxFieldsSelected; i++) {
      if (fieldPositions[i] == 2 ) {
        sportSelected = 1;
      } else if (fieldPositions[i] == 3 ) {
        dportSelected = 1;
      }
    }
    if (!sportSelected) {
      fieldPositions[maxFieldsSelected++] = 2;
    }
    if (!dportSelected) {
      fieldPositions[maxFieldsSelected++] = 3;
    }
  }

  setWidths();

  /*
  ** re-set the default for num-recs depending on the output device:
  ** if tty then 10 else infinity. The globalCounter.numRecs value
  ** of 0 =>'s that the user did not give an explicit value.
  */
  if (globalCounters.numRecs == 0) {
    if (FILEIsATty(stdout)) {
      globalCounters.numRecs = DEFAULT_NUM_RECS;
    } else {
      globalCounters.numRecs = 0xFFFFFFFF;
    }
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    appTeardown();
    exit(1);
  }

  return;                       /* OK */
}


/*
** appOptionsUsage:
**      print help for options for this app to stderr.
** Arguments: None.
** Returns: None.
** Side Effects: None.
** NOTE:
**  do NOT add non-option usage here (i.e., required and other optional args)
*/
static void appOptionsUsage(void) {
  register int i;

  buildHelp();  /* dynamically create the field help */

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
**      setup to parse application options.
** Arguments: None.
** Returns:
**      0 OK; 1 else.
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
** appOptionsTeardown:
**      teardown application options.
** Arguments: None.
** Returns: None.
** Side Effects: None
*/
static void appOptionsTeardown(void) {
  return;
}


/*
** appOptionsHandler:
**      called for each option the app has registered.
** Arguments:
**      clientData cData: ignored here.
**      int index: index into appOptions of the specific option
**      char *optarg: the argument; 0 if no argument was required/given.
** Returns:
**      0 if OK. 1 else.
** Side Effects:
**      Relevant options are set.
*/
static int appOptionsHandler(clientData UNUSED(cData),int index, char *optarg){
  long nrecs;
  unsigned int i;
  cutDynlib_t *cutDL;

  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }

  switch (index) {
  case CUT_OPT_FIELDS:
    if (fieldPositions) {
      fprintf(stderr,"%s: --fields option given multiple times\n",skAppName());
      return 1;
    }
    fieldPositions = skParseNumberList(optarg,1,fieldCount,&maxFieldsSelected);
    if (!fieldPositions) {
      return 1;
    }
    for (i = 0; i < maxFieldsSelected; ++i) {
      /* map fieldPositions to 0-based array */
      fieldPositions[i]--;
      /* initialize dynamic library at this position if required */
      cutDL = cutFieldNum2Dynlib(fieldPositions[i]);
      if (cutDL) {
        if (dynlibInitialize(cutDL->dlisp)) {
          fprintf(stderr,"%s: field %u not available-cannot initialize %s\n",
                  skAppName(),1+fieldPositions[i],dynlibGetPath(cutDL->dlisp));
          return 1;
        }
      }
    }
    break;

  case CUT_OPT_INTEGER_IPS:
    /* integer ip */
    flags.dottedip = 0;
    break;

  case CUT_OPT_NUM_RECS:
    /* num-recs */
    if (checkArg(optarg, INTEGER_ONLY)) {
      fprintf(stderr, "Invalid argument %s\n", optarg);
      return 1;
    }
    nrecs = strtol(optarg, (char **)NULL, 10);
    if (nrecs < 0) {
      /* from end of file */
      globalCounters.numRecs = abs(nrecs);
#ifdef  FROM_EOF_POSSIBLE
      flags.fromEOF = 1;
#else
      fprintf(stderr, "Unable to work from EOF with rawdata\n");
      return 1;
#endif
    } else if (nrecs == 0) {
      /* all records */
      globalCounters.numRecs = 0xFFFFFFFF;
    } else {
      globalCounters.numRecs = (uint32_t) nrecs;
    }
    break;

  case CUT_OPT_START_REC_NUM:
    /* start-rec number */
    if (checkArg(optarg, INTEGER_ONLY)) {
      fprintf(stderr, "Invalid argument %s\n", optarg);
      return 1;
    }
    globalCounters.firstRec = (uint32_t) strtoul(optarg, (char **)NULL, 10);
    break;

  case CUT_OPT_END_REC_NUM:
    /* end rec */
    if (checkArg(optarg, INTEGER_ONLY)) {
      fprintf(stderr, "Invalid argument %s\n", optarg);
      return 1;
    }
    globalCounters.lastRec = (uint32_t) strtoul(optarg, (char **)NULL, 10);
    break;

  case CUT_OPT_NO_TITLES:
    /* no titles */
    flags.printTitles = 0;
    break;

  case CUT_OPT_DELIMITED:
    /* dump as delimited text */
    flags.delimited = 1;
    if (optarg) {
      delimiter = optarg[0];
    }
    break;

  case CUT_OPT_EPOCH_TIME:
    /* epoch time */
    flags.timestamp = 0;
    break;

  case CUT_OPT_ICMP_TYPE_AND_CODE:
    /* type and code */
    flags.icmpTandC = 1;
    break;

  case CUT_OPT_DYNAMIC_LIBRARY:
    /* dynamic library */
    if (cutAddDynlib(optarg, 1)) {
      return 1;
    }
    break;

  case CUT_OPT_INTEGER_SENSORS:
    flags.integerSensor = 1;
    break;

  default:
    fprintf(stderr, "Invalid option %d\n", index);
    return 1;
    break;

  } /* switch */
  return 0;                     /* OK */
}


/*
** setCounters:
**      set fileCounters.numRecs and fileCounters.firstRec based
**       on user options.
** Input:
**      None.
** Output:
**      None.
** Side Effects:
**      fileCounters.numRecs and fileCounters.firstRec,  are set
**      based globalCounter values.
*/
void setCounters(void) {
  register int i;

#ifdef  FROM_EOF_POSSIBLE
  /* check if from eof */
  if (flags.fromEOF) {
    fileCounters.firstRec = rwGetRecCount(rwIOSPtr)
      - rwGetRejectCount(rwIOSPtr) - globalCounters.numRecs;
    fileCounters.numRecs = globalCounters.numRecs;
    return;
  }
#endif

  /* figure out what to dump */
  fileCounters.numRecs = globalCounters.numRecs;
  fileCounters.firstRec = globalCounters.firstRec;

  if (fileCounters.firstRec) {
    fileCounters.firstRec--;
  }
  if (fileCounters.firstRec) {
    if (globalCounters.lastRec) {
      i = globalCounters.lastRec - fileCounters.firstRec;
      if ( i < 0) {
        /* bad request. adjust */
        fileCounters.numRecs = 0;
      } else {
        fileCounters.numRecs = i;
      }
    }
  } else if (globalCounters.lastRec) {
    /* start not given but end is. */
      i = globalCounters.lastRec - fileCounters.numRecs;
      if (i < 0) {
        /* more requested than available. Reduce fileCounters.numRecs */
        fileCounters.numRecs = globalCounters.lastRec;
        fileCounters.firstRec = 0;
      } else {
        fileCounters.firstRec = i;
      }
    }

  if (fileCounters.numRecs < 1) {
    fprintf(stderr, "requested < 1 records\n");
    exit(1);
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
  cutDynlib_t *cutDL;
  int len, cLen;
  char title[64];
  char buffer[64];

  cp = (char *)malloc(1024);
  if (!cp) {
    fprintf(stderr, "Out of memory!  cp=malloc()\n");
    return;
  }
  strcpy(cp, "Fields to print: ");
  cLen = 20 + strlen(cp);
  for (i = 0; i < fieldCount; i++) {
    len = strlen(cp);
    cutDL = cutFieldNum2Dynlib(i);
    if (NULL == cutDL) {
      sprintf(buffer, "%s = %d, ", fieldNames[i], i+1);
    } else {
      cutDL->fxn(i - cutDL->offset, title, sizeof(title), NULL);
      sprintf(buffer, "%s = %d, ", title, i + 1);
    }

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
  len -= 2;                     /* eat the trailing ", " */
  if ( len % 80 > 60) {
    sprintf(cp + len, ".\n  (Def 1-%d)", RWCUT_LAST_DEFAULT_FIELD);
  } else {
    sprintf(cp + len, ". Def 1-%d", RWCUT_LAST_DEFAULT_FIELD);
  }
  len = strlen(cp);
  cp = realloc(cp, len+1);
  if (!cp) {
    fprintf(stderr, "Out of memory!  cp=recalloc()\n");
    return;
  }
  appHelp[0] = cp;
}


/*
** cutAddDynlib(dlpath, warnOnError);
**
**   Load the dynamic library at dlpath, store its information in the
**   cutDynlibList, and increment the cutDynlibCount.  Return 0 on
**   success.  Return 1 if the library cannot be loaded or if too many
**   dynamic libraries have been loaded.
*/
static int cutAddDynlib(const char *dlpath, int warnOnError)
{
  dynlibInfoStructPtr dlisp;
  cutf_t cutf;

  if (cutDynlibCount == CUT_MAX_DYNLIBS) {
    fprintf(stderr, "%s: Too many dynlibs.  Only %u allowed\n",
            skAppName(), CUT_MAX_DYNLIBS);
    return 1;
  }

  dlisp = dynlibSetup(DYNLIB_CUT);
  if (dynlibCheck(dlisp, dlpath)) {
    /* failed to find library, missing symbols, or setup failed */
    if (warnOnError) {
      fprintf(stderr, "%s: Error in dynamic library: %s\n",
              skAppName(), dynlibLastError(dlisp));
    }
    dynlibTeardown(dlisp);
    return 1;
  }
  cutf = (cutf_t)dynlibGetRWProcessor(dlisp);
  /* dynlibCheck() should have made this check already */
  if (!cutf) {
    dynlibTeardown(dlisp);
    return 1;
  }

  cutDynlibList[cutDynlibCount].dlisp = dlisp;
  cutDynlibList[cutDynlibCount].fxn = cutf;
  /* offset is the field before the field this plugin manages, so
   * subtract 1 */
  cutDynlibList[cutDynlibCount].offset = fieldCount - 1;
  ++cutDynlibCount;

  fieldCount += cutf(0, NULL, 0, NULL); /* Number of fields */

  return 0;
}


/*
**  cutFieldNum2Dynlib(fieldNum);
**
**    Given a field number, return a pointer to a cutDynlib_t from the
**    cutDynlibList[] that points to the dynamic library used to print
**    the fieldNum field.  Return NULL if the fieldNum is not related
**    to any dynamic library.
*/
cutDynlib_t* cutFieldNum2Dynlib(const unsigned int fieldNum)
{
  int j;

  if (!cutDynlibCount || (fieldNum <= cutDynlibList[0].offset)) {
    return NULL;
  }
  for (j = 1; j < cutDynlibCount; ++j) {
    if (fieldNum <= cutDynlibList[j].offset) {
      break;
    }
  }
  return &(cutDynlibList[j-1]);
}


/*
** setWidths:
**      set the field widths for printing if we are priting a table
**      and not delimited records.
** Input: None
** Output: None
** Side Effects:
**      fWidths filled in
*/
static void setWidths(void) {
  unsigned int i;
  cutDynlib_t *cutDL;
  rwRec dummy;

  if (flags.delimited) {
    return;
  }

  fWidths = calloc(fieldCount, sizeof(uint8_t));
  if (!fWidths) {
    fprintf(stderr, "Out of memory!  fWidths=calloc()\n");
    exit(1);
  }

  for (i = 0; i < fieldCount; i++) {
    switch (i) {
    case 0:
    case 1:
    case 14:
      /* ip numbers */
      if (flags.dottedip) {
        fWidths[i] = 15;
      } else {
        fWidths[i] = 10;
      }
      break;

    case 2:
    case 3:
      /* sport and dport */
      fWidths[i] = 5;
      break;

    case 4:
      /* proto */
      fWidths[i] = 3;
      break;

    case 5:
    case 6:
    case 7:
      /* packets, bytes, and flags */
      fWidths[i] = 10;
      break;

    case 8:
    case 10:
      /* sTime and end time*/
      if (flags.timestamp) {
        fWidths[i] = 19;
      } else {
        fWidths[i] = 10;
      }
      break;

    case 9:
      /* elapsed/duration  */
      fWidths[i] = 5;
      break;

    case 11:
      /* sensor */
      if (flags.integerSensor) {
        fWidths[i] = 3; /* wide enough for 255 */
      } else {
        fWidths[i] = 5;
      }
      break;

    case 12:
    case 13:
      /* input,output */
      fWidths[i] = 3;
      break;

    default:
      cutDL = cutFieldNum2Dynlib(i);
      if (NULL == cutDL) {
        fprintf(stderr, "Invalid field index %u\n", i);
        exit(1);
      }
      fWidths[i] = -1 + cutDL->fxn(i - cutDL->offset, NULL, 0, &dummy);
      break;
    } /* switch */
  }
  return;
}
