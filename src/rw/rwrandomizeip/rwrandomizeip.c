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
** 1.6
** 2004/03/10 22:48:28
** thomasm
*/

/*
 * rwrandomizeip
 *
 * Read any rw file (rwpacked file, rwfilter output, etc) and output a
 * file with the source IPs and destination IPs randomly put into
 * 10.0.0.0/8, 172.16.0.0/12, or 192.168.0.0/16 space.
 *
 * No effort is made to consistantly map a real address to an
 * obfuscated address, i.e., 128.2.214.225 may be mapped to
 * 10.10.10.10 one time and 10.20.20.20 the next.  MOST TRAFFIC
 * STUDIES ON THE RESULTING FILES WILL BE MEANINGLESS.
 *
 * Though the IPs are gone, the port numbers remain.  These randomized
 * files could provide some information to a malicious party, e.g.,
 * letting them know that a particular service is in use.
 *
 * TODO:
 *
 * --It would be nice if the user could optionally provide the cidr
 * block into which source and/or destination IPs should be placed.
 *
 * --Randomize the ports.
 *
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include "utils.h"
#include "rwpack.h"
#include "iochecks.h"
#include "filter.h"

RCSIDENT("rwrandomizeip.c,v 1.6 2004/03/10 22:48:28 thomasm Exp");


/* local defines and typedefs */

/* buffer to hold records */
#define MAX_REC_LEN 4096
static uint8_t data[MAX_REC_LEN];
uint8_t * const dataPtr = &(data[0]);

/* 2 args: input, output */
#define	NUM_REQUIRED_ARGS	2

#define MAXOCTETS 4
#define MAXADDRS 256
uint8_t addrSwapTable[MAXOCTETS][MAXADDRS];

/* local functions */
static void appUsage(void);			/* never returns */
static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns on failure */
static void appOptionsUsage(void); 		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */
static int  appOptionsHandler(clientData cData, int index, char *optarg);
static void fileCopy(rwIOStructPtr);
static void copyRwFileHeaderV0(rwIOStructPtr);
static void copyFilterHeaderV1(rwIOStructPtr);
static void randomizeIP(uint8_t swapFlag, uint32_t *ip);
static void shuffleIPTables();
static void consistentlyRandomizeIP(uint32_t *ip); 
static int loadShuffleFile(char *shuffleFileName);
static int saveShuffleFile(char *shuffleFileName);
/* exported variables */
FILE *outF;


/* local variables */
static iochecksInfoStructPtr ioISP;
static int outIsPipe;
static char *inFName;
static uint8_t consistentFlag = 0;

static struct option appOptions[] = {
  {"consistent", NO_ARG, 0, 0},
  {"load-table", REQUIRED_ARG, 0, 1},
  {"save-table", REQUIRED_ARG, 0, 2},
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  /* add help strings here for the applications options */
  "Consistently randomize IP addresses", 
  "Load up a randomization table from a previous run",
  "Save this run's randomization table for future use",
  (char *)NULL
};


/*
 * appUsage:
 * 	print message on getting usage information and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
void appUsage(void)
{
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
static void appUsageLong(void)
{
  fprintf(stderr, "%s <input-file> <output-file>\n", skAppName());
  fprintf(stderr, "  Substitute a non-routable IP address for the source\n");
  fprintf(stderr, "  and destination IP addresses of <input-file> and\n");
  fprintf(stderr, "  write result to <output-file>.\n");
  fprintf(stderr, "  You may use \"stdin\" for <input-file> and \"stdout\"\n");
  fprintf(stderr, "  for <output-file>.  Gzipped files are o.k.\n");
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
static void appTeardown(void)
{
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  if (outF && outF != stdout) {
    if (outIsPipe) {
      if (-1 == pclose(outF)) {
        fprintf(stderr, "%s: error closing output pipe\n", skAppName());
      }
    } else if (EOF == fclose(outF)) {
      fprintf(stderr, "%s: error closing output file: %s\n",
              skAppName(), strerror(errno));
    };
    outF = (FILE*)NULL;
  }

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
static void appSetup(int argc, char **argv)
{
  int nextOptIndex;

  skAppRegister(argv[0]);

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if ((argc - 1) < NUM_REQUIRED_ARGS) {
    fprintf(stderr, "%s: expecting %d arguments\n", skAppName(),NUM_REQUIRED_ARGS);
    appUsageLong();		/* never returns */
  }

  /* remove one from argc since final arg is output path */
  ioISP = iochecksSetup(1, 0, (argc-1), argv);
  if (!ioISP) {
    fprintf(stderr, "%s: unable to setup iochecks module\n", skAppName());
    exit(1);
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
  if ((nextOptIndex = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
  }

  /* Check that we have input */
  if (nextOptIndex >= argc) {
    fprintf(stderr, "%s: Expecting input file name\n", skAppName());
    appUsage();		/* never returns */
  }
  inFName = argv[nextOptIndex];

  ++nextOptIndex;
  if (nextOptIndex >= argc) {
    fprintf(stderr, "%s: Expecting output file name\n", skAppName());
    appUsage();		/* never returns */
  }
  if (iochecksPassDestinations(ioISP, argv[nextOptIndex], 0)) {
    appUsage();		/* never returns */
  }
  outF = ioISP->passFD[0];
  outIsPipe = ioISP->passIsPipe[0];

  ++nextOptIndex;
  if (argc != nextOptIndex) {
    fprintf(stderr, "%s: Got extra options\n", skAppName());
    appUsage();		/* never returns */
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    appTeardown();
    exit(1);		/* never returns */
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
static void appOptionsUsage(void)
{
  return;
}


/*
 * appOptionsSetup:
 * 	setup to parse application options.
 * Arguments: None.
 * Returns:
 *	0 OK; 1 else.
*/
static int appOptionsSetup(void)
{
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
  switch(index) { 
  case 0:  /* consistent ip's */
    if(consistentFlag) {
      fprintf(stderr, "Choose --load, --consistent, OR --save.\n");
      exit(-1);
    };
    shuffleIPTables();
    consistentFlag = 1;
    break;
  case 1:  /* load a table */
    if(consistentFlag) { 
      fprintf(stderr, "Choose --load, --consistent, OR --save.\n");
      exit(-1);
    };
    consistentFlag = 1;
    loadShuffleFile(optarg);
    break;
  case 2:  /* write a table */
    if(consistentFlag) { 
      fprintf(stderr, "Choose --load, --consistent, OR --save.\n");
      exit(-1);
    };
    consistentFlag = 1;
    shuffleIPTables();
    saveShuffleFile(optarg);
    break;
  default:
    appUsage();
  };
  return 0;			/* OK */
}

/*
 * void shuffleIPTables(void) 
 * 
 * shuffleIPTables generates a set of consistent mappings between IP addresses.
 * Each table maps a corresponding octet value.  To actually generate the maps
 * we start with a 0-255 table and then swap each value once.
 */
static void shuffleIPTables() { 
  int i,j, swapIndex, stash;
  /*
   * Initialize the value.
   */
  srandom((uint32_t) time(NULL));
  /*
   * First loop is pure initialization.  Set everything so
   * addrSwapTable[i][j] = j.
   */
  for(i = 0; i < MAXOCTETS; i++) { 
    for(j = 0; j < MAXADDRS; j++) { 
      addrSwapTable[i][j] = j;
    };
  };
  for(i = 0; i < MAXOCTETS; i++) { 
    for(j = 0; j < MAXADDRS; j++) { 
      swapIndex = random() % MAXADDRS;
      stash = addrSwapTable[i][swapIndex];
      addrSwapTable[i][swapIndex] = addrSwapTable[i][j];
      addrSwapTable[i][j] = stash;
    };
  };
};

/*
 * void consistentlyRandomizeIP(uint32_t *ip)
 *
 * This writes a new ip address to the location specified by IP, 
 * however it does so using the consistent mapping built using
 * shuffleIPTables.
 * Parameters:

 * RETURNS: None
 * SE: Writes to *ip
 */
void consistentlyRandomizeIP(uint32_t *ip) { 
  int tgtByte,i,mask;
  for(i = 0 ; i < 4 ; i++) { 
    /*
     * isolate the targeted byte. 
     */
    mask = (0xFF << (i * 8));
    tgtByte = (*ip & mask) >> (i * 8);
    tgtByte = addrSwapTable[i][tgtByte];
    *ip = (*ip & ~mask) | (tgtByte << (i * 8));
  };
};

/*
 * int saveShuffleFile(char *shuffleFileName) 
 *
 * Writes the ambient shuffle file data (the contents of the 
 *
 * NOTES: 
 * This routine is supposed to be called immediately after shuffling and
 * before any filtering has been done.
 */
static int saveShuffleFile(char *shuffleFileName) { 
  genericHeader gHdr; /* 
		       * we don't have anything particularly 
		       * special here, so we can just read the generic
		       * header. 
		       */
  int i;
  FILE *shuffleFile;
  /*
   * First step - check to see if the file exists and if it does
   * complain and terminate.
   */
  if(fileExists(shuffleFileName)) { 
    fprintf(stderr, "Shuffle file %s already exists, delete it to continue\n",
	    shuffleFileName);
    exit(-1);
  };
  if((shuffleFile = fopen(shuffleFileName, "wb")) == NULL) { 
    fprintf(stderr, "Error opening file %s: %s.  Terminating.\n", 
	    shuffleFileName, strerror(errno));
    exit(-1);
  };
  PREPHEADER(&gHdr);
  gHdr.type = FT_SHUFFLE; 
  fwrite((void *) &gHdr, sizeof(genericHeader), 1, shuffleFile);
  /*
   * We're always writing in the form 0,1,2,3
   * 
   */
  for(i = 0 ; i < MAXOCTETS; i++) { 
    if(!fwrite((void *) addrSwapTable[i], 1, sizeof(uint8_t) * MAXADDRS, 
	   shuffleFile)) { 
      fprintf(stderr, "Error writing to file %s: %s terminating\n", 
	      shuffleFileName, strerror(errno)); 
      exit(-1); 
    };
  };
  if(fclose(shuffleFile)) { 
    fprintf(stderr, "Error closing file %s: %s; terminating\n",
	    shuffleFileName, strerror(errno));
  };
  return 0; /* in defiance of all expectations, everything works */ 
};

/*
 * int loadShuffleFile(char *shuffleFileName) 
 * Loads a shuffle file off of disk by reading the octet streams, given
 * the endian issue, this function will swap direction of the 
 * file was read in the opposite format. 
 */
static int loadShuffleFile(char *shuffleFileName) { 
  genericHeader gHdr;
  int i, reverse;
  FILE *shuffleFile;
  if(!fileExists(shuffleFileName)) { 
    fprintf(stderr, "No file named %s, check the pathname and try again.\n", 
	    shuffleFileName); 
    exit(-1);
  };
  if((shuffleFile = fopen(shuffleFileName, "rb")) == NULL) { 
    fprintf(stderr, "Error opening file %s: %s.  Terminating\n", 
	    shuffleFileName, strerror(errno)); 
    exit(-1);
  };
  if((fread((void *) &gHdr, sizeof(genericHeader), 1, shuffleFile)) == 0) { 
    fprintf(stderr, "Error reading header from file %s: %s.  Terminating\n",
	    shuffleFileName, strerror(errno));
    exit(-1);
  };
  if(gHdr.type != FT_SHUFFLE) { 
    fprintf(stderr, 
	    "Error reading file %s: not a shuffle file.  Terminating\n",
	    shuffleFileName);
    exit(-1);
  };
#if IS_LITTLE_ENDIAN
  reverse = gHdr.isBigEndian;
#else
  reverse = !gHdr.isBigEndian;
#endif
  if(reverse) { 
    for(i = 4; i < 0; i++) { 
      if(!fread((void *) addrSwapTable[i], 
		sizeof(uint8_t), MAXADDRS, shuffleFile)) {
	fprintf(stderr, "Error reading file %s: %s.  Terminating\n", 
		shuffleFileName, strerror(errno)); 
      };
    } 
  } else { 
    for(i = 0; i < 4; i++) { 
      if(!fread((void *) addrSwapTable[i], 
		sizeof(uint8_t), MAXADDRS, shuffleFile)) {
	fprintf(stderr, "Error reading file %s: %s.  Terminating\n", 
		shuffleFileName, strerror(errno)); 
      };
    };
  }
  if(fclose(shuffleFile)) { 
    fprintf(stderr, "Error closing file %s: %s\n", shuffleFileName,
	    strerror(errno)); 
  };
  return 0; 
};

/*
 * randomizeIP(swapFlag, ip)
 *
 *   Write a new IP address into the location pointed to be ip.  Write
 *   the IP in network byte order if swapFlag is 0; or in little
 *   endian order if swapFlag is 1.
 *
 *   ARGUMENTS:
 *     swapFlag: 0 to use network byte order; 1 to use little endian
 *     ip: pointer to location where the new IP should be written
 *   RETURNS: none
 *   SIDE EFFECTS: none
*/
void randomizeIP(uint8_t swapFlag, uint32_t* ip)
{
  int x, y;

  /* on Solaris, we always get an even value for y since RAND_MAX is
   * 32767, or half of 65536. */
  x = (int) ((256.0 + 16.0 + 1.0)*rand()/(RAND_MAX+1.0));
  y = (int) (65536.0*rand()/(RAND_MAX+1.0));

  if (x < 256) {
    if (!swapFlag) {
      *ip = 10<<24 | x<<16 | y;
    } else {
      *ip = 10 | x<<8 | y<<16;
    }
  }
  else if (x == (256 + 16)) {
    if (!swapFlag) {
      *ip = 192<<24 | 168<<16 | y;
    } else {
      *ip = 192 | 168<<8 | y<<16;
    }
    return;
  }
  else {
    if (!swapFlag) {
      *ip = 172<<24 | (x - 256 + 16)<<16 | y;
    } else {
      *ip = 172 | (x - 256 + 16)<<8 | y<<16;
    }
  }
}


/*
 * copyRwFileHeaderV0(rwIOSPtr)
 *
 *   Write the file header (rwFileHeaderV0) of the rw-file pointed at
 *   by "rwIOSPtr" to the "outF" FILE handle; make the header an exact
 *   copy of the header read from disk/stdin
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
static void copyRwFileHeaderV0(rwIOStructPtr rwIOSPtr)
{
  rwFileHeaderV0 hdr;

  /* Make a copy of the header that we can modify */
  memcpy(&hdr, rwIOSPtr->hdr, sizeof(hdr));

  /* If we swapped the data in the header on read, then we need to
   * swap it again */
  if (rwIOSPtr->swapFlag) {
    hdr.fileSTime = BSWAP32(hdr.fileSTime);
  }

  /* Write header */
  fwrite(&hdr, sizeof(hdr), 1, outF);
}


/*
 * copyFilterHeaderV1(rwIOSPtr)
 *
 *   Write the file header (filterHeaderV1) of the rw-fitler file
 *   pointed at by "rwIOSPtr" to the "outF" FILE handle; make the
 *   header an exact copy of the header read from disk/stdin
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
static void copyFilterHeaderV1(rwIOStructPtr rwIOSPtr)
{
  filterHeaderV1 hdr;
  uint32_t filterCount;
  uint32_t i;
  uint16_t byteCount;

  /* Make a copy of the header that we can modify */
  memcpy(&hdr, rwIOSPtr->hdr, sizeof(hdr));

  /* Write generic header */
  fwrite(&(hdr.gHdr), sizeof(genericHeader), 1, outF);

  /* write the number of filters; if we swapped on read, swap again */
  filterCount = hdr.filterCount;
  if (rwIOSPtr->swapFlag) {
    hdr.filterCount = BSWAP32(filterCount);
  }
  fwrite(&(hdr.filterCount), sizeof(hdr.filterCount), 1, outF);

  /* write each filter, byte swapping the length if required */
  for (i = 0; i < filterCount; ++i) {
    byteCount = hdr.fiArray[i]->byteCount;
    if (rwIOSPtr->swapFlag) {
      hdr.fiArray[i]->byteCount = BSWAP16(byteCount);
    }
    fwrite(hdr.fiArray[i], byteCount + sizeof(byteCount), 1, outF);
  }

  return;
}


/*
 * fileCopy(rwIOSPtr)
 *
 *   Write the data in the input file pointed at by "rwIOSPtr" to the
 *   "outF" FILE handle; make the output an exact copy of the data
 *   read from disk/stdin.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void fileCopy(rwIOStructPtr rwIOSPtr)
{
  const int32_t fileType = rwGetFileType(rwIOSPtr);
  const size_t recLen = rwIOSPtr->recLen;
  size_t bytesRead;

  /* Handle the header */
  switch(fileType) {
  case FT_RWROUTED:
  case FT_RWNOTROUTED:
  case FT_RWSPLIT:
  case FT_RWACL:
  case FT_RWWWW:
  case FT_RWGENERIC:
    copyRwFileHeaderV0(rwIOSPtr);
    break;

  case FT_RWFILTER:
    copyFilterHeaderV1(rwIOSPtr);
    break;

  default:
    fprintf(stderr, "%s: Invalid file type for %s: 0x%X\n",
            skAppName(), rwIOSPtr->fPath, fileType);
    break;
  }

  /* Handle the body */
  while (1) {
    bytesRead = fread((void*)data, 1, recLen, rwIOSPtr->FD);
    if (!bytesRead) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "%s: short read on %s\n", skAppName(), rwIOSPtr->fPath);
      }
      break;
    }

    if(consistentFlag) { 
      consistentlyRandomizeIP((uint32_t*)(dataPtr + 0));
      consistentlyRandomizeIP((uint32_t*)(dataPtr + 4));
    } else { 
      randomizeIP(rwIOSPtr->swapFlag, (uint32_t*)(dataPtr + 0));
      randomizeIP(rwIOSPtr->swapFlag, (uint32_t*)(dataPtr + 4));
    }
    fwrite(dataPtr, 1, bytesRead, outF);
  }

  return;
}


int main(int argc, char **argv)
{
  int32_t fileType;
  rwIOStructPtr rwIOSPtr;

  appSetup(argc, argv);

  rwIOSPtr = rwOpenFile(inFName, NULL);
  if (!rwIOSPtr) {
    return 1;
  }
  fileType = rwGetFileType(rwIOSPtr);

  fileCopy(rwIOSPtr);

  /* done */
  appTeardown();
  return 0;
}


