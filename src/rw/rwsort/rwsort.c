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
** 1.12
** 2004/03/10 22:45:13
** thomasm
*/

/*
 * Application commentary goes here
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "fglob.h"
#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"

RCSIDENT("rwsort.c,v 1.12 2004/03/10 22:45:13 thomasm Exp");


/* local defines and typedefs */
#define	NUM_REQUIRED_ARGS	1
#define MAX_RECORDS             50000000
#define BUFFER_SIZE                (sizeof(rwRec) * MAX_RECORDS)
#define NUM_FIELDS              9

/* exported functions */
void appUsage(void);		/* never returns */

/* local functions */
static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns */
static void appOptionsUsage(void); 		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */

static int  appOptionsHandler(clientData cData, int index, char *optarg);
void rwqsort(char *a, size_t n, size_t es, int (*cmp)());

/* exported variables */

/* local variables */
struct option appOptions[] = {
  {"fields", REQUIRED_ARG, 0, 0},
  {"output-path", REQUIRED_ARG,	0, 1},
  {"input-pipe", REQUIRED_ARG, 0, 2},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
char *appHelp[] = {
  /* add help strings here for the applications options */
  "list in 1-7,9. 1=sIP; 2=dIP; 3=sPort; 4=dPort; 5=proto; 6=pkts; 7=bytes; 9=sTime",
  "destination for sorted output (stdout|pipe). Default stdout",
  "get input byte stream from pipe (stdin|pipe). Default no",
  (char *)NULL
};

static uint8_t g_fields[NUM_FIELDS]; /* Fields to sort and order */
static iochecksInfoStructPtr ioISP;
FILE *outF;
char *outFPath = "stdout";
int outIsPipe;
static void *recordBuffer = NULL; /* Region of memory for records */
static rwRec *currentRecord;	/* Record to write to */
static uint32_t recordCount;	/* Number of records */

/*
** dumpHeader:
** 	Prepare and dump a header to outF using ft generic and version 1
**	Set endian to whatever the current machine's endianness is
**	Set compression level to 0
**	There is no start time for generic records so only dump the gHdr
** Input:
**	rwIOSPtr
** Output:
**	None
** Side Effects:
**	None
*/
void dumpHeader(void) {
  genericHeader gHdr;

  PREPHEADER(&gHdr);
  gHdr.type = FT_RWGENERIC;
  gHdr.version  = 1;
  gHdr.cLevel = 0;
#if	IS_LITTLE_ENDIAN
  gHdr.isBigEndian = 0;
#else
  gHdr.isBigEndian = 1;
#endif
  fwrite(&gHdr, sizeof(gHdr), 1, outF);
  return;
}

/*
 * appUsage:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
void appUsage(void) {
  fprintf(stderr, "Use `%s --help for usage\n", skAppName());
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
  fprintf(stderr, "%s [app_opts] [fglob_opts] .... \n", skAppName());
  appOptionsUsage();
  fprintf(stderr, "--fields is required\n");
  fglobUsage();
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

  iochecksTeardown(ioISP);
  fglobTeardown();

  if (outF != stdout) {
    outIsPipe ? pclose(outF) : fclose(outF);
  }
  if (recordBuffer) {
    free(recordBuffer);
  }

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

  skAppRegister(argv[0]);
  outF = stdout;		/* default */

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if (argc - 1 < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);			/* never returns */
  }

  ioISP = iochecksSetup(0, 0, argc, argv);

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);			/* never returns */
  }

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }

  /* Make sure the user specified at least one field */
  if (g_fields[0] == 0) {
    fprintf(stderr, "key field not given\n");
    appUsage();		/* never returns */
  }

  /* Check that outF isn't a tty */
  if (FILEIsATty(outF)) {
    fprintf(stderr, "%s: output is a tty\n", skAppName());
    exit(1);
  }

  /*
  ** Input file checks:  if libfglob is NOT used, then check
  ** if files given on command line.  Do NOT allow both libfglob
  ** style specs and cmd line files. If cmd line files are NOT given,
  ** then default to stdin.
  */
  if (fglobValid()) {
    if (ioISP->firstFile < argc) {
      fprintf(stderr,
              "argument mismatch. Both fglob options & input files given\n");
      exit(1);
    }
  } else {
    if (ioISP->firstFile == argc) {
      if (FILEIsATty(stdin)) {
	appUsage();		/* never returns */
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

  /* Allocate the buffer */
  if ((recordBuffer = (void *)malloc(BUFFER_SIZE)) == (void *)NULL) {
    fprintf(stderr, "%s: unable to allocate enough memory\n", skAppName());
    exit(1);
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
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


/* Define our raw sorting functions */
#define SORT(t, a, b) {if ((t)(a) < (t)(b)) return -1; else if ((t)(a) > (t)(b)) return 1; return 0;}

static int sort1(rwRec *a, rwRec *b) {
  SORT(uint32_t, a->sIP.ipnum, b->sIP.ipnum);
}

static int sort2(rwRec *a, rwRec *b) {
  SORT(uint32_t, a->dIP.ipnum, b->dIP.ipnum);
}

static int sort3(rwRec *a, rwRec *b) {
  SORT(uint16_t, a->sPort, b->sPort);
}

static int sort4(rwRec *a, rwRec *b) {
  SORT(uint16_t, a->dPort, b->dPort);
}

static int sort5(rwRec *a, rwRec *b) {
  SORT(uint8_t, a->proto, b->proto);
}

static int sort6(rwRec *a, rwRec *b) {
  SORT(uint32_t, a->pkts, b->pkts);
}

static int sort7(rwRec *a, rwRec *b) {
  SORT(uint32_t, a->bytes, b->bytes);
}

static int sort9(rwRec *a, rwRec *b) {
  SORT(uint32_t, a->sTime, b->sTime);
}

typedef int (*cmpfun)();

/* Array mapping fields to sorting functions */
static cmpfun sorts[NUM_FIELDS] = {sort1, sort2, sort3, sort4, sort5,
				   sort6, sort7, NULL, sort9};
static cmpfun fields[NUM_FIELDS + 1]; /* Actual sorting order */

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
  unsigned int i, j;
  uint8_t count;
  uint8_t *parsedList;

  switch (index) {
  case 0:			/* --fields */
    if (!optarg) {
      return 1;
    }
    if (g_fields[0]) {
      fprintf(stderr, "--fields argument provided multiple times\n");
      return 1;
    }
    parsedList = skParseNumberList(optarg, 1, NUM_FIELDS, &count);
    if (!parsedList) {
      return 1;
    }
    for (i = 0; i < count; i++) {
      if (8 == parsedList[i]) {
        fprintf(stderr, "Invalid value: can't sort on 8.\n");
        free(parsedList);
        return 1;
      }
      /* check for duplicates later in list */
      for (j = i+1; j < count; j++) {
        if (parsedList[i] == parsedList[j]) {
          fprintf(stderr, "Invalid value: duplicate value %u\n",parsedList[i]);
          free(parsedList);
          return 1;
        }
      }
      g_fields[i] = parsedList[i];
    }
    free(parsedList);
    /* Set up the array of sorting functions */
    memset(fields, 0, sizeof(fields));
    for (i = 0; (i < NUM_FIELDS) && (g_fields[i] != 0); i++) {
      fields[i] = sorts[g_fields[i] - 1];
    }
    break;
  case 1:			/* --output-path */
    if (!optarg) {
      return 1;
    }
    if (strcmp("stdout", optarg) == 0) {
      outF = stdout;
      break;
    }
    MALLOCCOPY(outFPath, optarg);
    if ( openFile(outFPath, 1 /* write */, &outF, &outIsPipe) ) {
      return 1;
    }
    break;
  case 2:			/* --input-pipe */
    if (iochecksInputSource(ioISP, optarg)) {
      exit(1);
    }
    break;
  default:
    appUsage();			/* never returns */
    break;
  }
  return 0;			/* OK */
}

/*
 * addRecords:
 *     Reads the records into the record buffer
*/
int addRecords(char *inFName)
{
  static rwIOStructPtr rwIOSPtr;
  int noerror = 1;

  if ( ( rwIOSPtr = rwOpenFile(inFName, ioISP->inputCopyFD))
       == (rwIOStructPtr)NULL) {
    /* some error. the library would have dumped a msg */
    return 1;
  }

  while ((recordCount < MAX_RECORDS) && noerror)
  {
    if ((noerror = rwRead(rwIOSPtr, currentRecord))) {
      currentRecord++;
      recordCount++;
    }
  }
  if ((recordCount == MAX_RECORDS) && noerror)
  {
    fprintf(stderr, "Too many records (>%i).  Continuing with %i records.\n", MAX_RECORDS, MAX_RECORDS);
  }
  rwCloseFile(rwIOSPtr);
  return 0;
}

/*
 * recsort:
 *     Returns an ordering on the recs pointed to `a' and `b' using
 *     the sorting functions in the fields array.
*/
int recsort(rwRec *a, rwRec *b)
{
  register int i;
  for (i = 0; fields[i] != 0; i++)
  {
    register int v = (*fields[i])(a, b);
    if (v) {
      return v;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  int counter;
  char * curFName;
  rwRec *cr;
  uint32_t c;

  appSetup(argc, argv);			/* never returns */

  recordCount = 0;
  currentRecord = (rwRec *)recordBuffer;

  /* Read in records */
  if (fglobValid()) {
    while ((curFName = fglobNext())){
      if (addRecords(curFName)) {
	break;
      }
    }
  } else {
    for(counter = ioISP->firstFile; counter < ioISP->fileCount ; counter++) {
      if (addRecords(ioISP->fnArray[counter])) {
	break;
      }
    }
  }

  /* Sort */
  rwqsort(recordBuffer, recordCount, sizeof(rwRec), recsort);

  /* output */
  dumpHeader();

  cr = recordBuffer;
  for (c = 0; c < recordCount; c++) {
    fwrite(cr, sizeof(*cr) - sizeof(cr->padding), 1, outF);
    cr++;
  }

  appTeardown();
  return (0);
}


/*
  qsort()

  A la Bentley and McIlroy in Software - Practice and Experience,
  Vol. 23 (11) 1249-1265. Nov. 1993


*/

typedef long WORD;
#define	W		sizeof(WORD)	/* must be a power of 2 */
#define SWAPINIT(a, es)	swaptype = \
			(( (a-(char *)0) | es) % W) ? 2 : (es > W) ? 1 : 0
#define exch(a, b, t)	(t = a, a = b, b = t)
#define swap(a, b)	swaptype != 0 ? swapfunc(a, b, es, swaptype) : \
			(void )exch(*(WORD*)(a), *(WORD*)(b), t)
/* swap sequences of records */
#define vecswap(a, b, n)	if ( n > 0) swapfunc(a, b, n, swaptype)

#define	PVINIT(pv, pm)	if (swaptype != 0) pv = a, swap(pv, pm); \
			else pv = (char *)&v, v = *(WORD*)pm

#define min(a, b)	a <= b ? a : b

static char *med3(char *a, char *b, char *c, int (*cmp)())
{
  return(   cmp(a, b) < 0 ?
	    (cmp(b, c) < 0 ? b : cmp(a, c) < 0 ? c : a)
	    : (cmp(b, c) > 0 ? b : cmp(a, c) > 0 ? c : a)
	    );
}



static void swapfunc(char *a, char *b, size_t n, int swaptype)
{
  if( swaptype <= 1)
  {
    WORD t;
    for( ; n > 0; a += W, b += W, n -= W)
      exch(*(WORD*)a, *(WORD*)b, t);
  }
  else
  {
    char t;
    for( ; n > 0; a += 1, b += 1, n -= 1)
      exch(*a, *b, t);
  }
}


void rwqsort(char *a, size_t n, size_t es, int (*cmp)())
{
  char *pa, *pb, *pc, *pd, *pl, *pm, *pn, *pv;
  int r, swaptype;
  WORD t, v;
  size_t s;

  SWAPINIT(a, es);
  if(n < 7)
  {
    /* use insertion sort on smallest arrays */
    for (pm = a + es; pm < a + n*es; pm += es)
      for(pl = pm; pl > a && cmp(pl-es, pl) > 0; pl -= es)
	swap(pl, pl-es);
    return;
  }

  pm = a + (n/2)*es;		/* small arrays middle element */
  if ( n > 7) 			/* SLK What about n == 7 ? */
  {
    pl = a;
    pn = a + (n - 1)*es;
    if( n > 40 )
    {
      /* big arays.  Pseudomedian of 9 */
      s = (n / 8) * es;
      pl = med3(pl, pl + s, pl + 2 * s, cmp);
      pm = med3(pm - s, pm, pm + s, cmp);
      pn = med3(pn - 2 * s, pn - s, pn, cmp);
    }
    pm = med3(pl, pm, pn, cmp);	/* mid-size, med of 3 */
  }
  PVINIT(pv, pm);		/* pv points to the partition value */
  pa = pb = a;
  pc = pd = a + (n - 1) * es;
  for(;;)
  {
    while(pb <= pc && (r = cmp(pb, pv)) <= 0)
    {
      if (r == 0)
      {
	swap(pa, pb);
	pa += es;
      }
      pb += es;
    }
    while (pc >= pb && (r = cmp(pc, pv)) >= 0)
    {
      if( r == 0)
      {
	swap(pc, pd);
	pd -= es;
      }
      pc -= es;
    }
    if( pb > pc)
      break;
    swap(pb, pc);
    pb += es;
    pc -= es;
  }
  pn = a + n * es;
  s = min(pa - a, pb - pa);
  vecswap(a, pb - s, s);
  s = min(pd - pc, pn -pd -(int32_t)es);
  vecswap(pb, pn - s, s);
  if (( s = pb - pa) > es)
    rwqsort(a, s/es, es, cmp);
  if (( s = pd - pc) > es)
    rwqsort(pn - s, s/es, es, cmp);

  return;
}
