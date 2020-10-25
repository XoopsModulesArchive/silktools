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
** 1.14
** 2004/03/10 22:11:59
** thomasm
*/


/*
  rwcount.c
  This is a counting application; given the files provided by fglob, it
  generates counting results for the time period covered.
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
#include "utils.h"
#include "filter.h"
#include "fglob.h"
#include "iochecks.h"
#include "dynlib.h"
#include "rwcount.h"

RCSIDENT("rwcount.c,v 1.14 2004/03/10 22:11:59 thomasm Exp");
#define DEBUG 0


/* note that the variables are defined below */
#define getBin(t) ROW_BINS * ((t - countData.fileHeader->initOffset) / countData.fileHeader->binSize)

/* globals */
int (*addFunction) (rwRec *);
iochecksInfoStructPtr ioISP;
rwIOStructPtr rwIOSPtr;
uint64_t recsCounted, recsRead;
countFile countData;

struct option appOptions[] = {
  {"bin-size", REQUIRED_ARG, 0, RWCO_BIN_SIZE},   /* Size of the bins,
                                                     in seconds */
  {"load-scheme", REQUIRED_ARG, 0, RWCO_BIN_LOAD}, /* Bin splitting scheme */
  {"skip-zeroes", NO_ARG, 0, RWCO_SKIPZ},
  {"delimiter", REQUIRED_ARG, 0, RWCO_DELIM},
  {"print-filenames", NO_ARG, 0, RWCO_PRINTFNAME},
  {"no-titles", NO_ARG, 0, RWCO_NOTITLE},
  {"epoch-slots", NO_ARG, 0, RWCO_EPOCHTIME},
  {"bin-slots", NO_ARG, 0, RWCO_BINTIME},
  {"start-epoch", REQUIRED_ARG, 0, RWCO_FIRSTEPOCH},
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
char *appHelp[] = {
  "size of bins in seconds",
  "Defines bin loading, options are:\n\t0: split values evenly across the time the record covers\n\t1: fill first appropriate bin with values\n\t2: fill last appropriate bin\n\t3: fill centermost bin\n",
  "skip empty records",
  "Delimiter (defaults to '|')",
  "print filenames if globbing",
  "Drop titles",
  "Print slots using epoch time",
  "Print the internal bin index for the slots",
  "epoch time to start printing from",
  (char *)NULL
};


extern void appSetup(int argc, char **argv);/* never returns */
extern void appTeardown(void);

/* purely local variables */
int swapFlag;
int fileType;           /* record of the first file's type */
int fileVersion;        /* record of the first file's version */
rwRec rwrec;

char *inFPath;          /* current input file path */
int prepRec;            /* flag to prepare rec for output  */
uint64_t rCount = 0;    /* recs */
uint64_t pCount = 0;    /* passed */
uint32_t fileCount = 0; /* files */
double *bins = NULL;
int dryRunFlag = 0;
int printFNameFlag = 0;
int printStatsFlag = 0;
uint32_t zeroFlag = RWCO_INTUITIVE;
int binStampFlag = GMT;
uint32_t binLoadMode = LOAD_START;
uint32_t defaultBinSize = DEFAULT_BINSIZE;
uint32_t skipZeroFlag = 0;
int printTitleFlag = 1;
char delimiter = '|';
char tgtFileName[80];

static int countPackFile(char *inFName);

/*
 * int allocateBins(guessTime, range, length)
 * allocateBins allocates time bins based on an initial guess time and
 * the range of days its told to allocate for.
 *
 * parameters:
 * guessTime: the guess Time.  This is the pivot time used for the guessing
 * process.
 * range: the range (in days) to generate bins for.
 * length: the length of the bins, in seconds.
 *
 * notes:
 * DO NOT CALL THIS TWICE.
 *
 * initBins is the clean allocation function - it sets initial
 * environmental variables in addition to memory allocation. Calling
 * it twice will trigger an error.
 *
 * returns:
 * 0 on success
 *
 *
*/
int initBins(uint32_t expectedTime, uint16_t dayRange, uint32_t binSize) {
  uint32_t roundedDay, endOffset;
  if (! countData.fileHeader) {
    countData.fileHeader = (countFileHeader *) malloc(sizeof(countFileHeader));
  }
  /*
   * Step 1: determine the day when this time starts at by subtracting the
   * mod 86400
   */
  roundedDay = expectedTime - (expectedTime % DAY_SECS);
  countData.fileHeader->binSize = binSize;
  /*
   * Start setting offset, length, crap like that.
   */
  endOffset = roundedDay + (dayRange * DAY_SECS);
  countData.fileHeader->binSize = binSize;
  countData.fileHeader->initOffset = roundedDay - DAY_SECS;
  countData.fileHeader->totalBins = 1 + ((endOffset - countData.fileHeader->initOffset) / binSize);

  /*
   * Now do the allocation.
   */
  countData.bins = (double *) malloc(ROW_BINS * sizeof(double) * countData.fileHeader->totalBins);
  memset(countData.bins, 0.0, ROW_BINS * countData.fileHeader->totalBins * sizeof(double));
  /*
   * and we're done!
   */
  if (!bins) {
    return RWCO_ERR_MALLOC;
  } else {
    return 0;
  }
}

/*
 * int startAdd(rwRec *rwrec)
 *
 * DESCRIPTION
 *
 * front-loaded record addition.  In the front-loaded implementation,
 * the bytes and packets are added to the first bin relevant to the
 * record.
 *
 * PARAMETERS
 *
 * rwRec *rwrec: pointer to the targeted record.
 *
 *
 * RETURNS
 *
 * 0 on success.  Failure codes?
 *
*/
int startAdd(rwRec *rwrec) {
  static uint32_t tgtTime;
  tgtTime = getBin(rwrec->sTime);
  countData.bins[tgtTime + RECS] ++;
  countData.bins[tgtTime + BYTES] += rwrec->bytes;
  countData.bins[tgtTime + PACKETS] += rwrec->pkts;
  return 0;
}

/*
 * int endAdd(rwRec *rwrec)
 *
 * DESCRIPTION
 *
 * endAdd ends records to the end of a dataset.
 *
 * PARAMETERS
 *
 * rwRec *rwrec - the target record.
 *
*/
int endAdd(rwRec *rwrec) {
  uint32_t tgtTime;
  tgtTime = getBin(rwrec->sTime + rwrec->elapsed);
  countData.bins[tgtTime + RECS]++;
  countData.bins[tgtTime + BYTES] += rwrec->bytes;
  countData.bins[tgtTime + PACKETS] += rwrec->pkts;
  return 0;
}
/*
 * int midAdd(rwRec *rwrec)
 *
*/
int midAdd(rwRec *rwrec) {
  uint32_t tgtTime;
  tgtTime = getBin(rwrec->sTime + (rwrec->elapsed / 2));
  countData.bins[tgtTime + RECS]++;
  countData.bins[tgtTime + BYTES] += rwrec->bytes;
  countData.bins[tgtTime + PACKETS] += rwrec->pkts;
  return 0;
}
/*
 * int meanAdd(rwRec *rwrec)
 *
 * DESCRIPTION
 *
 * meanAdd adds the mean of the records to each count.
 *
 * PARAMETERS
 *
 * rwRec *rwrec - the target record.
 *
*/
int meanAdd(rwRec *rwrec) {
  uint32_t startBin, endBin, index, allBins;
  startBin = getBin(rwrec->sTime);
  endBin = getBin(rwrec->sTime + rwrec->elapsed);
  allBins = 1 + ((endBin - startBin) / ROW_BINS);
  for(index = startBin; index <= endBin; index += ROW_BINS) {
    countData.bins[index + RECS] += (1.0 / allBins);
    countData.bins[index + BYTES] += ((double) rwrec->bytes)/allBins;
    countData.bins[index + PACKETS] += ((double) rwrec->pkts) / allBins;
  }
  return 0;
}

/*
** countFile:
**      This is the actuall filtering of a file.  Also handles
**      dumping out the header if required.
*/

static int countPackFile(char *inFName) {
  fileCount++;
  if (dryRunFlag) {
    fprintf(stdout, "%s\n", inFName);
    return 0;
  }
  if ( ( rwIOSPtr = rwOpenFile(inFName, ioISP->inputCopyFD))
       == (rwIOStructPtr)NULL) {
    /* some error. the library would have dumped a msg */
    return 1;
  }
  /* check that the files are all of the same type and version  */
  if(!countData.fileHeader) {
    rwRead(rwIOSPtr, &rwrec);
    initBins(rwrec.sTime - (24 * 3600), 45, defaultBinSize);
    addFunction(&rwrec);

  }
  while (rwRead(rwIOSPtr,&rwrec)) {
    addFunction(&rwrec);
  }
  rwCloseFile(rwIOSPtr);
  rwIOSPtr = (rwIOStructPtr)NULL;
  return 0;
}

/*
 * int dumpText(FILE *tgtFile)
 *
 * DESCRIPTION
 * drops an ascii count file to screen.
 *
 * PARAMETERS
 * FILE *tgtFile - the output file for the
 *
 * RETURNS:
 * 0 on success; -1 on failure.
 *
*/
int dumpText(FILE *tgtFile) {
  uint32_t binIndex, binInc, i, binCount, printing;
  uint32_t startI, endI, currentTime;
  printing = 0;
  binIndex = 0;
  binInc = 1;

  /*
   * First, we use the skip zeroes flag.  Either we're using
   * 'intuitive mode' meaning, that we skip all leading an dtrailing
   * zeroes, or we're using 'force' mode.
   */
  if(countData.fileHeader == NULL) {
    fprintf(stderr, "Error printing count data, no data provided\n");
    return(1);
  };
  if(zeroFlag == RWCO_INTUITIVE) {
    for(i = 0; i < countData.fileHeader->totalBins; i++) {
      if(countData.bins[ROW_BINS * i] != 0.0) break;
    }
    startI = i;
    for(i = (countData.fileHeader->totalBins - 1); i >= startI; i--) {
      if(countData.bins[ROW_BINS * i] != 0.0) break;
    }
    endI = i;
  } else {
    /*
     * The first step with the zero flag is to get the index of our
     * epoch time value.
     */

    startI = (zeroFlag - countData.fileHeader->initOffset) / countData.fileHeader->binSize;
    fprintf(stderr, "initOffset: %u, zeroFlag: %u, startI: %u\n", countData.fileHeader->initOffset, zeroFlag, startI);
    for(i = (countData.fileHeader->totalBins - 1); i >= startI; i--) {
      if(countData.bins[ROW_BINS * i] != 0.0) break;
    }
    endI = i;
  }
  currentTime = countData.fileHeader->initOffset + (startI * countData.fileHeader->binSize);
  if(printTitleFlag) {
    fprintf(tgtFile, "%20s%c%18s%c%18s%c%18s\n",
            "Date", delimiter, "Records",
            delimiter, "Bytes",
            delimiter, "Packets");
  }
  switch(binStampFlag) {
  case INDEX:
    for(i = startI, binCount = ROW_BINS * startI;  i  <= endI;
        i += binInc, binCount += (ROW_BINS),
          currentTime += countData.fileHeader->binSize) {
      if((countData.bins[binCount] + countData.bins[binCount + 1] + countData.bins[binCount + 2]) || ! skipZeroFlag) {
        fprintf(tgtFile, "%20d%c%18.2f%c%18.2f%c%18.2f\n",
                i, delimiter,
                countData.bins[binCount], delimiter,
                countData.bins[binCount + 1], delimiter,
                countData.bins[binCount + 2]);
      }
    }
    break;
  case EPOCH:
    for(i = startI, binCount = ROW_BINS * startI;  i  <= endI;
        i += binInc, binCount += (ROW_BINS),
          currentTime += countData.fileHeader->binSize) {
      if((countData.bins[binCount] + countData.bins[binCount + 1] + countData.bins[binCount + 2]) || ! skipZeroFlag) {
        fprintf(tgtFile, "%20d%c%18.2f%c%18.2f%c%18.2f\n",
                currentTime, delimiter,
                countData.bins[binCount], delimiter,
                countData.bins[binCount + 1], delimiter,
                countData.bins[binCount + 2]);
      }
    }
    break;
  case GMT:
    for(i = startI, binCount = ROW_BINS * startI;  i  <= endI;
        i += binInc, binCount += (ROW_BINS),
          currentTime += countData.fileHeader->binSize) {
      if((countData.bins[binCount] + countData.bins[binCount + 1] + countData.bins[binCount + 2]) || ! skipZeroFlag) {
        fprintf(tgtFile, "%20s%c%18.2f%c%18.2f%c%18.2f\n",
                timestamp(currentTime), delimiter,
                countData.bins[binCount], delimiter,
                countData.bins[binCount + 1], delimiter,
                countData.bins[binCount + 2]);
      }
    }
    break;
  };
  fclose(tgtFile);
  return 0;
}

/*
 * int main(int argc, char **argv)
 *
 * DESCRIPTION
 *
 * main function, this is currently a bit too bulky since this app is
 * still more or less being felt out.
 *
 * PARAMETERS
 *
 *
 *
*/

int main(int argc, char ** argv) {
  int counter;
  int i;
  char * curFName;
  appSetup(argc, argv);
  switch(binLoadMode) {
  case LOAD_START:
    addFunction = startAdd;
    break;
  case LOAD_END:
    addFunction = endAdd;
    break;
  case LOAD_MIDDLE:
    addFunction = midAdd;
    break;
  case LOAD_MEAN:
    addFunction = meanAdd;
    break;
  }
  fileType = -1;/* force checking the first file  */
  /* get files to process from the globber or the command line */
  if (fglobValid()) {
    /*
     * Valid globbing means we can strip the required timing
     * information out of the glob,w hich is
     * what we'll do at this point.
     */
    /* We borrow i for a little while */
    i = 1+ ((fglobGetEpochEnd() - fglobGetEpochStart() )/ (DAY_SECS));
    initBins(fglobGetEpochStart(), i ,defaultBinSize);
    while ((curFName = fglobNext())){
      if(printFNameFlag) {
        fprintf(stderr, "%s\n", curFName);
      }
      if (countPackFile(curFName)) {
        break;
      }
    }
  } else {
    for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
      if (countPackFile(ioISP->fnArray[counter])) {
        break;
      }
    }
  }
  dumpText(stdout);
  appTeardown();
  return(0);                    /* make gcc happy on linux */
}
