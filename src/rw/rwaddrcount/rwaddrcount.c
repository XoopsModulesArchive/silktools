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
 * addrcount.c
 * this is the last major rwset tool I want to write, it takes in two streams
 * of data, an rwset and an rwfilter stream.  From this data, it then generates
 * a result - one of three outputs:
 *            totals (default) - outputs to screen a table containing the
 *                               ip address, bytes, packets, flows
 *            print-ips        - outputs to screen the ip addresses
 *            set-file         - outputs to screen the set data.
 *
 * So the reason for the second two is because I'm including three thresholds
 * here - bytes, packets & records. 
 *
 * 12/2 notes.  Often, the best is the enemy of the good, I am implementing
 * a simple version of this application for now with the long-term plan being
 * that I am going to write a better, faster, set-friendly version later. 
 */

#include "silk.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "fglob.h"
#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"

#include "rwaddrcount.h"

RCSIDENT("rwaddrcount.c,v 1.5 2004/03/10 22:11:58 thomasm Exp");


#define NUM_REQUIRED_ARGS 1
#define BIG_PRIME 50865917 

/* local defines and typedefs */
/* I'm #define the hash and comparison functions to avoid stack overhead */
#define HASHFUNC(value) ((value ^ (value >> 7) ^ (value << 23)) % BIG_PRIME)
/* IPHASH:
 * rwrec: rwRecord to use. 
 * useSrc: use Source IP address
 */
#define IPHASH(rwrec, useDst) (useDst ? HASHFUNC(rwrec.dIP.ipnum) : HASHFUNC(rwrec.sIP.ipnum))
#define CMPFNC(rwrec, crPtr, useDst) (useDst ? rwrec.dIP.ipnum == crPtr->value : \
                                      rwrec.sIP.ipnum == crPtr->value)                       

/* exported functions */
static void appUsage(void);		/* never returns */
static void appUsageLong(void);
/* local functions */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns */
static void appOptionsUsage(void); 		/* application options usage */
static int  appOptionsSetup();			/* applications setup */

static int  appOptionsHandler(clientData cData, int index, char *optarg);

/* exported variables */
char *appName;
uint8_t printMode = 0;
uint8_t useDestFlag = 0;
uint32_t byteMin, recMin, packetMin, byteMax, recMax, packetMax;
countRecord *countArray[BIG_PRIME];
uint32_t totalHashRecords, outputRecCount;
char* outputSetFN;

/* local variables */

struct option appOptions[] = {
  {"print-stat", NO_ARG, 0, RWACOPT_NOREPORT},
  {"print-recs", NO_ARG, 0, RWACOPT_PRINTRECS},
  {"print-ips", NO_ARG, 0, RWACOPT_PRINTIPS},
  {"use-dest", NO_ARG, 0, RWACOPT_USEDEST},
  {"byte-min", REQUIRED_ARG, 0, RWACOPT_BYTEMIN},
  {"packet-min", REQUIRED_ARG, 0, RWACOPT_PKTMIN},
  {"rec-min", REQUIRED_ARG, 0, RWACOPT_RECMIN},
  {"byte-max", REQUIRED_ARG, 0, RWACOPT_BYTEMAX},
  {"packet-max", REQUIRED_ARG, 0, RWACOPT_PKTMAX},
  {"rec-max", REQUIRED_ARG, 0, RWACOPT_RECMAX},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
char *appHelp[] = {
  /* add help strings here for the applications options */
  "print statistics rather than a record table",
  "print summary records of traffic", 
  "Print ip's to screen (will not print records)",
  "use destination address (default is to use source IP)",
  "Min number of bytes in an ip record before printing",
  "Min number of packets in an ip record before printing",
  "Min number of records in an ip record before printing",
  "Max number of bytes in an ip record before printing",
  "Max number of packets in an ip record before printing",
  "Max number of records in an ip record before printing",
  (char *)NULL
};

static iochecksInfoStructPtr ioISP;
static rwRec currentRecord;	/* Record we're currently reading*/
/*
 * appUsage:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
 */
static void appUsage(void) {
  fprintf(stderr, "Use `%s --help' for usage\n", appName);
  exit(1);
}


/*
 * appUsageLong:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
 */
void appUsageLong() {
  fprintf(stderr, "%s [app_opts] \n", appName);
  fprintf(stderr, "Specify either --print-stat, --print-recs or --print-ips\n");
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
static void appTeardown() {
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  iochecksTeardown(ioISP);
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

  appName = argv[0];
  /*
   * Variable initialization.
   */
  useDestFlag = 0;
  printMode = RWAC_PMODE_NONE;
  byteMin = 0;
  recMin = 0;
  packetMin = 0;

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if (argc - 1 < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", appName);
    exit(1);			/* never returns */
  }

  ioISP = iochecksSetup(0, 0, argc, argv);

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", appName);
    exit(1);			/* never returns */
  }

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }
  if (ioISP->firstFile == argc) {
    if (FILEIsATty(stdin)) {
      appUsage();		/* never returns */
    }
    if (iochecksInputSource(ioISP, "stdin")) {
      exit(1);
    }
  }
  /* check input files, pipes, etc */
  if (iochecksInputs(ioISP, 0)) {
    appUsage();
  }

  if(printMode == RWAC_PMODE_NONE) { 
    appUsage();
    exit(1);
  };

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n", appName);
    appTeardown();
    exit(1);
  }

  (void) umask((mode_t) 0002);
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
 *	optional args)
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
static int  appOptionsSetup() {
  /* register the apps  options handler */
  if (optionsRegister(appOptions, (optHandler) appOptionsHandler,
		      (clientData) 0)) {
    fprintf(stderr, "%s: unable to register application options\n",
            appName);
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
static int  appOptionsHandler(clientData UNUSED(cData),int index,char *optarg){
  switch(index) { 
  case RWACOPT_USEDEST:
    useDestFlag = 1;
    break;
  case RWACOPT_BYTEMIN:
    byteMin = strtoul(optarg, NULL, 0);
    break;
  case RWACOPT_PKTMIN:
    packetMin = strtoul(optarg, NULL, 0);
    break;
  case RWACOPT_RECMIN:
    recMin = strtoul(optarg, NULL, 0);
    break;
  case RWACOPT_BYTEMAX:
    byteMax = strtoul(optarg, NULL, 0); 
    break;
  case RWACOPT_PKTMAX:
    packetMax = strtoul(optarg, NULL, 0);
    break;
  case RWACOPT_RECMAX:
    recMax = strtoul(optarg, NULL, 0);
    break;
  case RWACOPT_NOREPORT:
    printMode = RWAC_PMODE_STAT;
    break;
  case RWACOPT_PRINTIPS:
    printMode = RWAC_PMODE_IPS;
    break;
  case RWACOPT_PRINTRECS:
    printMode = RWAC_PMODE_RECS;
    break;
  };
  return 0;			/* OK */
}

/*
 * int addRecord(rwRec *Location, rwRec *currentRecord)
 *
 * Adds the contents of a record to the values stored in currentRecord.  
 *
 * This function is actually designed for new records or records that 
 * already exist.  The "not existence" flag is the epoch start time which, if set 
 * to zero is assumed to be a recently calloc'd record. 
 */
int addRecord(countRecord *tgt, rwRec *newRecord) {
  tgt->bytes += newRecord->bytes;
  tgt->packets += newRecord->pkts;
  tgt->flows ++; 
  if(!tgt->start) { 
    tgt->value = useDestFlag ? newRecord->dIP.ipnum : newRecord->sIP.ipnum;
    tgt->start = newRecord->sTime;
    tgt->end = newRecord->sTime + newRecord->elapsed; 
    totalHashRecords++;
  } else {
    tgt->start = newRecord->sTime <= tgt->start ? newRecord->sTime : tgt->start;
    tgt->end = (newRecord->sTime + newRecord->elapsed) > tgt->end ? 
      (newRecord->sTime + newRecord->elapsed) : tgt->end; 
  };
  return 0;
}

/*
 *
 */
int dumpRecords() { 
  uint32_t i, totalRecs;
  countRecord *currentRec;
  fprintf(stdout, "%15s|%10s|%10s|%10s|%20s|%20s\n", "IP Address", "Bytes", "Packets", "Records", "Start Time", "End Time");
  for(i = 0, totalRecs = 0; i < BIG_PRIME; i++) { 
    currentRec = countArray[i]; 
    while(currentRec != NULL) { 
      if(currentRec->bytes >= byteMin &&
	 currentRec->packets >= packetMin &&
	 currentRec->flows >= recMin && 
	 (!byteMax || currentRec->bytes <= byteMax) && 
	 (!packetMax || currentRec->packets <= packetMax) &&
	 (!recMax || currentRec->flows <= recMax)) { 
	fprintf(stdout, "%15s|", num2dot(currentRec->value));
	fprintf(stdout, "%10d|%10d|%10d|%20s", currentRec->bytes, 
		currentRec->packets,
		currentRec->flows, timestamp(currentRec->start));
	fprintf(stdout, "|%20s\n", timestamp(currentRec->end));
	totalRecs++;
      };
      if(totalRecs == outputRecCount) return 0;
      if(currentRec->nextRecord == countArray[i]) break;
      currentRec = currentRec->nextRecord;
    }
  }
  return 0;
}

/*
 *
 */
int dumpIPs() { 
  uint32_t i, totalRecs;
  countRecord *currentRec;
  fprintf(stdout, "%15s\n", "IP Address");
  for(i = totalRecs =  0; i < BIG_PRIME; i++) { 
    currentRec = countArray[i]; 
    while(currentRec != NULL) { 
      if(currentRec->bytes >= byteMin &&
	 currentRec->packets >= packetMin &&
	 currentRec->flows >= recMin && 
	 (!byteMax || currentRec->bytes <= byteMax) && 
	 (!packetMax || currentRec->packets <= packetMax) &&
	 (!recMax || currentRec->flows <= recMax)) { 
	fprintf(stdout, "%15s\n", num2dot(currentRec->value));
	totalRecs++;
      };
      if(totalRecs == outputRecCount) return 0;
      if(currentRec->nextRecord == countArray[i]) break;
      currentRec = currentRec->nextRecord;
    }
  }
  return 0;
}

int dumpStats() { 
  uint32_t i;
  uint64_t bytes, records, packets;
  countRecord *currentRec;
  bytes = records = packets = 0;
  for(i = 0; i < BIG_PRIME; i++) { 
    currentRec = countArray[i]; 
    while(currentRec != NULL) { 
      if(currentRec->bytes >= byteMin &&
	 currentRec->packets >= packetMin &&
	 currentRec->flows >= recMin && 
	 (!byteMax || currentRec->bytes <= byteMax) && 
	 (!packetMax || currentRec->packets <= packetMax) &&
	 (!recMax || currentRec->flows <= recMax)) { 
	bytes += currentRec->bytes;
	records +=  currentRec->flows; 
	packets += currentRec->packets; 
      };
      if(currentRec->nextRecord == countArray[i]) break;
      currentRec = currentRec->nextRecord;
    }
  }
  fprintf(stderr, "%u unique IP's read\t%llu qualifying records\t%llu packets\t%llu bytes\n", 
	  totalHashRecords, records, packets, bytes);
  return 0;
}

int main(int argc, char **argv) {
  int counter;
  char * curFName;
  uint32_t hashIndex, recCount;
  countRecord *hashRecord;
  countRecord *rootRecord;
  rwIOStructPtr rwIOSPtr;
  curFName = NULL;
  counter = hashIndex = recCount = 0;
  appSetup(argc, argv);			/* never returns */
  /* Read in records */
  for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
    if((rwIOSPtr = rwOpenFile(ioISP->fnArray[counter], ioISP->inputCopyFD)) ==
       (rwIOStructPtr) NULL) {
      exit(1);
    }
    while(rwRead(rwIOSPtr, &currentRecord)) {
      recCount++;
      hashIndex = IPHASH(currentRecord, useDestFlag);
      /*
       * standard hash foo - check to see if we've got a value.  If not, 
       * create and stuff.  If so, check to see if they match - if so, 
       * stuff.  If not, move down until you do - if you find nothing, 
       * stuff. 
       */
      if(countArray[hashIndex] == NULL) { 	  
	if(((countArray[hashIndex] = (countRecord *) calloc(1, sizeof(countRecord))))
	   == NULL) { 
	  fprintf(stderr, "Error allocating memory for record, terminating\n");
	  exit(-1);
	}
	/* 
	 * No error on callocing, continue
	 */
	if(addRecord(countArray[hashIndex], &currentRecord)) { 
	  fprintf(stderr, "Error creating record, terminating\n");
	  exit(-1);
	}
	/*
	 * Alright, I'm modifying the record set so that it's circular,
	 * the explicit rationale for this is that a loop (rather than
	 * an array) will allow me to effectively rotate the "contact 
	 * point of the loop" so that the last used is always 
	 * at the countArray[hashIndex] position. 
	 */
	countArray[hashIndex]->nextRecord = countArray[hashIndex];
      } else { 
	hashRecord = countArray[hashIndex];
	rootRecord = hashRecord;
	while(!CMPFNC(currentRecord, hashRecord, useDestFlag) && 
	      (hashRecord->nextRecord != rootRecord)) { 
	  hashRecord = hashRecord->nextRecord;
	}; 
	/*
	 * Alright, we've either hit the end of the linked list or we've 
	 * found the value.  We check and handle the end of list case first. 
	 */
	if(!CMPFNC(currentRecord, hashRecord, useDestFlag)) { 
	  if((hashRecord->nextRecord = (countRecord *) calloc(1, sizeof(countRecord))) == NULL) { 

	    fprintf(stderr, "Error allocating memory for record, terminating\n");
	    exit(-1);
	  } 
	  hashRecord = hashRecord->nextRecord; 
	  hashRecord->nextRecord = rootRecord; /* Restore the loop */
	  /* We've now implicitly created an empty
	   * record and dropped it at currentRecord.
	   */
	};
	countArray[hashIndex] = hashRecord;
	if(addRecord(countArray[hashIndex], &currentRecord)) { 
	  fprintf(stderr, "Error creating record, terminating\n");
	  exit(-1);
	}
      }
    }
    rwCloseFile(rwIOSPtr);
  }
  if(FILEIsATty(stdout)) {
    outputRecCount = MAX_STD_RECS;
  } else {
    outputRecCount = totalHashRecords;
  };
  switch(printMode) { 
  case RWAC_PMODE_STAT:
    dumpStats();
    break;
  case RWAC_PMODE_RECS: 
    dumpRecords();
    break;
  case RWAC_PMODE_IPS:
    dumpIPs();
    break;
  }; 
  /* output */
  appTeardown();
  return (0);
}

