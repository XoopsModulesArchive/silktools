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
** 1.24
** 2004/03/10 23:00:36
** thomasm
*/


/*
**  rwfilter.c
**
**  6/3/2001
**
**  Suresh L. Konda
**
**  Allows for selective extraction of records and fields from a rw
**  packed file.  This version, unlike rwcut, creates a binary file with
**  the filtered records.  A new file type is used.  The header does not
**  contain valid recCount and rejectCount values.  The other fields are
**  taken from the original input file.
**
**  A second header is also created which records the filter rules used
**  for each pass.  Thus this is a variable length header
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "rwpack.h"
#include "fglob.h"
#include "utils.h"
#include "filter.h"
#include "iochecks.h"
#include "dynlib.h"

RCSIDENT("rwfilter.c,v 1.24 2004/03/10 23:00:36 thomasm Exp");


#define DISALLOW_DIFFERENT_FILETYPES 0
#define DEBUG 0

/* globals */
iochecksInfoStructPtr ioISP;
rwIOStructPtr rwIOSPtr;
int dryRunFlag = 0;
int printFNameFlag = 0;
int printStatsFlag = 0;

dynlibInfoStructPtr dlISP;

#define FILTER_MAX_DYNLIBS 8

int filterDynlibCount;
dynlibInfoStructPtr filterDynlibList[FILTER_MAX_DYNLIBS];
const char *filterDynlibNames[] = {
  "libipfilter.so",
  NULL /* sentinel */
};

struct option appOptions[] = {
  {"pass-destination",	REQUIRED_ARG,	0, 0},
  {"fail-destination", REQUIRED_ARG, 0, 1},
  {"all-destination", REQUIRED_ARG, 0, 2},
  {"input-pipe", REQUIRED_ARG, 0, 3},
  {"dynamic-library", REQUIRED_ARG, 0, 4},
  {"dry-run", NO_ARG, 0, 5},
  {"print-filenames", NO_ARG, 0, 6},
  {"print-statistics", NO_ARG, 0, 7},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
char *appHelp[] = {
  "destination for records which pass the filter (fPath|stdout) No default.",
  "destination for records which fail the filter (fPath|stdout) No default.",
  "destination for all (input) records (stdout|pipe). Default no.",
  "get input byte stream from pipe (stdin|pipe). Default no",
  "use given dynamic library. Default no",
  "do a dry run by parsing parameters but do not execute",
  "print filename while processing",
  "print statistics after processing",
  (char *)NULL
};


extern void appSetup(int argc, char **argv);/* never returns */
extern void appTeardown(void);

uint32_t rwWriteFilterRec(void);
int dumpHeader(int argc, char **argv);

#if DEBUG
void writeRRec(rwRec *rwrec);
void writeFRec(rwFilterRec *frec);
#endif

/* purely local variables */
static int swapFlag;
static int fileType;		/* record of the first file's type */
/* static int fileVersion; */	/* record of the first file's version */
/* static filterRules cutRules; */
static rwRec rwrec;
/* static char *inFPath; */	/* current input file path */
/* static int prepRec; */	/* flag to prepare rec for output  */
static uint64_t rCount = 0;	/* recs */
static uint64_t pCount = 0;	/* passed */
static uint32_t fileCount = 0;	/* files */

/* these variables and macros for packind and dumping records */
static uint32_t ow[2], pkts, pktsFlag;
#define PREPREC \
if (rwrec.pkts >= MAX_PKTS)\
{pkts = rwrec.pkts/ PKTS_DIVISOR;  pktsFlag = 1;}\
else { pktsFlag = 0; pkts = rwrec.pkts;}\
ow[0] = (pkts << 12) | (rwrec.elapsed << 1) | pktsFlag;\
ow[1] = ((rwrec.bytes/rwrec.pkts) << 18)\
| ( ((BPP_PRECN * (rwrec.bytes % rwrec.pkts)) /  rwrec.pkts)  << 12 ) | ( rwrec.sID);
#define	WRITEREC(fd)  fwrite(&rwrec, 24, 1, fd); fwrite(ow, 8, 1, fd);


/* function pointers to handle checking and or processing */
#define MAX_CHECKERS 10
int  (*checker[MAX_CHECKERS])(rwRecPtr);
static int checkerCount = 0;
static int filterFile(char *inFName, int oType);

/* for dumping headers */
int pargc;
char **pargv;
int main(int argc, char ** argv) {
  int j;
  int counter;
  int oType;			/* takes values 1-8 */
  char * curFName;

  appSetup(argc, argv);

  pargc = argc;
  pargv = argv;

  oType =  ioISP->failCount * 3 + ioISP->passCount;
  fileType = -1;/* force checking the first file  */
  checkerCount = 0;

  if (dynlibCheckLoaded(dlISP)) {
    /* dlISP is always exclusive for legacy reasons */
    checker[checkerCount] = dynlibGetRWProcessor(dlISP);
    ++checkerCount;
  }
  else {
    checker[checkerCount] = (int (*)(rwRecPtr))checkRWRules;
    ++checkerCount;

    for (j = 0; j < filterDynlibCount; ++j) {
      if (dynlibCheckActive(filterDynlibList[j])) {
        checker[checkerCount] = dynlibGetRWProcessor(filterDynlibList[j]);
        ++checkerCount;
      }
    }
  }

  /* get files to process from the globber or the command line */
  if (fglobValid()) {
    while ((curFName = fglobNext())){
      if (filterFile(curFName, oType)) {
	break;
      }
    }
  } else {
    for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
      if (filterFile(ioISP->fnArray[counter], oType)) {
	break;
      }
    }
  }
  /* Done.
  ** dump statistics if this is NOT a dynamic lib processing
  ** situation and the user asked for them. NOTE: dump these
  ** to stderr so that we can use stdout for pipes etc.
  */
  if (printStatsFlag && (dynlibGetStatus(dlISP) != DYNLIB_WILLPROCESS)) {
    fprintf(stderr, "Files %5u.  Read %10llu.  Pass %10llu. Fail  %10llu.\n",
	    fileCount, rCount, pCount, (rCount - pCount));
  }

  appTeardown();
  exit(0);
  return(0);			/* make gcc happy on linux */
}

/*
** filterFile:
**	This is the actuall filtering of a file.  Also handles
**	dumping out the header if required.
*/

static int filterFile(char *inFName, int oType) {
  int i;
  int fail = 1;

  fileCount++;
  if (dryRunFlag) {
    fprintf(stdout, "%s\n", inFName);
    return 0;
  }

  if (printFNameFlag) {
    fprintf(stderr, "%s\n", inFName);
  }

  if ( ( rwIOSPtr = rwOpenFile(inFName, ioISP->inputCopyFD))
       == (rwIOStructPtr)NULL) {
    /* some error. the library would have dumped a msg */
    return 1;
  }

#if	IS_LITTLE_ENDIAN
  swapFlag = rwGetIsBigEndian(rwIOSPtr);
#else
  swapFlag = ! rwGetIsBigEndian(rwIOSPtr);
#endif

  /* check that the files are all of the same type and version  */
  if (fileType < 0) {
    /* Processing first input file. Record the file type */
    fileType = rwGetFileType(rwIOSPtr);
    /* and dump header into output file(s) */
    if (dumpHeader(pargc, pargv)) {
      /* abort */
      appTeardown();
      exit(EXIT_FAILURE);
    }
  } else {
    /* if the file type is FT_RWFILTER, only 1 file is allowed */
    if (fileType == FT_RWFILTER && ioISP->fileCount > 1) {
      fprintf(stderr, "Only 1 input file of type filter allowed. Abort.\n");
      return 1;
    }

#if DISALLOW_DIFFERENT_FILETYPES
    if (rwGetFileType(rwIOSPtr) != fileType) {
      fprintf(stderr, "File types and/or versions vary.  Abort\n");
      return 1;
    }
#endif
  }
  if (DYNLIB_WILLPROCESS == dynlibGetStatus(dlISP)) {
    /* dynamic lib handles all processing */
    while (rwRead(rwIOSPtr,&rwrec)) {
      rCount++;
      pCount += (*(checker[0]))(&rwrec);
    }
  } else {
    switch (oType) {
    case 0:
      /* write nothing. get stats only */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (! fail) {
	  /* pass */
	  pCount++;
	}
      }
      break;

    case 1:
      /* write pass only; 1 file */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (! fail) {
	  /* pass */
	  pCount++;
	  PREPREC;
	  WRITEREC(ioISP->passFD[0]);
	}
      }
      break;

    case 2:
      /* write pass only; 2 files */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (! fail) {
	  /* pass */
	  pCount++;
	  PREPREC;
	  WRITEREC(ioISP->passFD[0]);
	  WRITEREC(ioISP->passFD[1]);
	}
      }
      break;

    case 3:
      /* write fail only; 1 file */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (fail) {
	  /* fail */
	  PREPREC;
	  WRITEREC(ioISP->failFD[0]);
	} else {
	  pCount++;
	}
      }
      break;

    case 4:
      /* write pass 1 and fail 1 */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
	PREPREC;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (fail) {
	  /* fail */
	  WRITEREC(ioISP->failFD[0]);
	} else {
	  pCount++;
	  WRITEREC(ioISP->passFD[0]);
	}
      }
      break;

    case 5:
      /* write pass 1/2 and fail 1 */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
	PREPREC;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (fail) {
	  /* fail */
	  WRITEREC(ioISP->failFD[0]);
	} else {
	  pCount++;
	  WRITEREC(ioISP->passFD[0]);
	  WRITEREC(ioISP->passFD[1]);
	}
      }
      break;

    case 6:
      /* write fail 1/2 */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (fail) {
	  /* fail */
	  PREPREC;
	  WRITEREC(ioISP->failFD[0]);
	  WRITEREC(ioISP->failFD[1]);
	} else {
	  pCount++;
	}
      }
      break;

    case 7:
      /* write pass 1 and fail 1/2 */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
	PREPREC;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (fail) {
	  /* fail */
	  WRITEREC(ioISP->failFD[0]);
	  WRITEREC(ioISP->failFD[1]);
	} else {
	  pCount++;
	  WRITEREC(ioISP->passFD[0]);
	}
      }
      break;

    case 8:
      /* write pass 1/2 and fail 1/2 */
      while (rwRead(rwIOSPtr,&rwrec)) {
	rCount++;
	PREPREC;
        /* run all checker()'s until end or one fails */
        for (i=0; i < checkerCount && !(fail = (*(checker[i]))(&rwrec)); ++i)
          ;
        if (fail) {
	  /* fail */
	  WRITEREC(ioISP->failFD[0]);
	  WRITEREC(ioISP->failFD[1]);
	} else {
	  pCount++;
	  WRITEREC(ioISP->passFD[0]);
	  WRITEREC(ioISP->passFD[1]);
	}
      }
      break;
    } /* switch */
  }
  rwCloseFile(rwIOSPtr);
  rwIOSPtr = (rwIOStructPtr)NULL;
  return 0;
}

/*
** dumpHeader
**	dump the header into all relevant  output files.
** 	If the input file is already a filter, add the new filter set
** 	to it and then dump it.
** 	If not, create a new one, add the filter set to it and then dump
** Input: None
** Output: 0 if OK.  1 else
*/

int dumpHeader(int argc, char **argv) {
  filterHeaderV1Ptr FHPtr;

  if (fileType == FT_RWFILTER) {
    /* already a filter file; copy the header, set the version number
     * to 2, and correctly set the endianness */
    FHPtr = (filterHeaderV1Ptr) calloc(1, sizeof(filterHeaderV1));
    memcpy(FHPtr, rwIOSPtr->hdr, sizeof(filterHeaderV1));
    FHPtr->gHdr.version = 2;
#if	IS_LITTLE_ENDIAN
    FHPtr->gHdr.isBigEndian = 0;
#else
    FHPtr->gHdr.isBigEndian = 1;
#endif
  } else {
    /* a new filter file.  Construct the header and add to it*/
    FHPtr = (filterHeaderV1Ptr) calloc(1, sizeof(filterHeaderV1));
    PREPHEADER(&FHPtr->gHdr);
    FHPtr->gHdr.cLevel = 0;
    FHPtr->gHdr.type = FT_RWFILTER;
    FHPtr->gHdr.version = 2;
#if	IS_LITTLE_ENDIAN
    FHPtr->gHdr.isBigEndian = 0;
#else
    FHPtr->gHdr.isBigEndian = 1;
#endif
  }
  /* add new info to the header */
  filterAddFInfoToHeaderV1(FHPtr, argc, argv);

  /*
  ** dump generic header here and use filterWriteHeaderV1() to dump the
  ** other stuff
  */
  if(ioISP->passFD[0]) {
    fwrite(&(FHPtr->gHdr), sizeof(genericHeader), 1, ioISP->passFD[0]);
    if (filterWriteHeaderV1(FHPtr, ioISP->passFD[0])) {
      fprintf(stderr, "Unable to write new filter header.  Abort\n");
      exit(EXIT_FAILURE);
    }
  }
  if(ioISP->passFD[1]) {
    fwrite(&FHPtr->gHdr, sizeof(genericHeader), 1, ioISP->passFD[1]);
    if (filterWriteHeaderV1(FHPtr, ioISP->passFD[1])) {
      fprintf(stderr, "Unable to write new filter header.  Abort\n");
      exit(EXIT_FAILURE);
    }
  }
  if(ioISP->failFD[0]) {
    fwrite(&FHPtr->gHdr, sizeof(genericHeader), 1, ioISP->failFD[0]);
    if (filterWriteHeaderV1(FHPtr, ioISP->failFD[0])) {
      fprintf(stderr, "Unable to write new filter header.  Abort\n");
      exit(EXIT_FAILURE);
    }
  }
  if(ioISP->failFD[1]) {
    fwrite(&FHPtr->gHdr, sizeof(genericHeader), 1, ioISP->failFD[1]);
    if (filterWriteHeaderV1(FHPtr, ioISP->failFD[1])) {
      fprintf(stderr, "Unable to write new filter header.  Abort\n");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}


#if DEBUG
void writeRRec(rwRec *rwrec) {
  fprintf(stderr, "%3u.%3u.%3u.%3u|%3u.%3u.%3u.%3u|",
	  rwrec->sIP.O.o1, rwrec->sIP.O.o2, rwrec->sIP.O.o3, rwrec->sIP.O.o4,
	  rwrec->dIP.O.o1, rwrec->dIP.O.o2, rwrec->dIP.O.o3, rwrec->dIP.O.o4);
  fprintf(stderr, "%5u|%5u|%3u|%8u|%8u|", rwrec->sPort, rwrec->dPort,
	  rwrec->proto, rwrec->pkts, rwrec->bytes);
  fprintf(stderr, "%10u|%10u", rwrec->sTime, (rwrec->sTime +rwrec->elapsed));
  if(rwrec->proto == 6) {
    fprintf(stderr, "|%3u",rwrec->flags);
  } else {
    fprintf(stderr, "|   ");
  }
  fprintf(stderr, "|%3u|%3u|%3u.%3u.%3u.%3u|S%02u", rwrec->input, rwrec->output,
	  rwrec->nhIP.O.o1, rwrec->nhIP.O.o2, rwrec->nhIP.O.o3, rwrec->nhIP.O.o4,
	  rwrec->sID);
  fprintf(stderr, "\n");
  return;
}
void writeFRec(rwFilterRec *frec) {
  fprintf(stderr, "%3u.%3u.%3u.%3u|%3u.%3u.%3u.%3u|",
	  frec->sIP.O.o1, frec->sIP.O.o2, frec->sIP.O.o3, frec->sIP.O.o4,
	  frec->dIP.O.o1, frec->dIP.O.o2, frec->dIP.O.o3, frec->dIP.O.o4);
  fprintf(stderr, "%5u|%5u|%3u|%8u|%8u|", frec->sPort, frec->dPort,
	  frec->proto, frec->pkts, frec->bPPkt);
  fprintf(stderr, "%10u|%10u", frec->sTime, (frec->sTime +frec->elapsed));
  if(frec->proto == 6) {
    fprintf(stderr, "|%3u",frec->flags);
  } else {
    fprintf(stderr, "|   ");
  }
  fprintf(stderr, "|%3u|%3u|%3u.%3u.%3u.%3u|S%02u", frec->input, frec->output,
	  frec->nhIP.O.o1, frec->nhIP.O.o2, frec->nhIP.O.o3, frec->nhIP.O.o4,
	  frec->sID);
  fprintf(stderr, "\n");
  return;
}
#endif

