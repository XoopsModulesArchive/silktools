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
** 1.16
** 2004/03/18 21:20:25
** thomasm
*/

/*
**  Reads PDUs from a router and writes the flows into packed files.
**
*/


/* Includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <limits.h>


#include "utils.h"
#include "log.h"
#include "rwpack.h"

RCSIDENT("rwflowpack.c,v 1.16 2004/03/18 21:20:25 thomasm Exp");

#undef DEBUGGING
/* #define DEBUGGING 1 */

#define WEBP(p) ((p) == 80 || (p) == 443 || (p) == 8080)
#define IS_WEB_PORT(p) WEBP(p)
#define IS_WEB(r) ((r).proto == 6 && (IS_WEB_PORT((r).sPort) || IS_WEB_PORT((r).dPort)))
#define WEBPORT(p) (((p) == 80) ? 0 : (((p) == 443) ? 1 : (((p) == 8080) ? 2 : 3)))
#define WEB_PORT(p) WEBPORT(p)

/* local defines and typedefs */
#define	NUM_REQUIRED_ARGS	2

typedef void (*cleanupHandler)(void *);

/* local data struct to hold information for the four periods
   PREVPREV, PREV, CUR, and NEXT
*/
typedef struct periodInfo {
  char ymdPath[16];
  char ymdhName[16];
  uint32_t sTime;
} periodInfo;

/* signal list */
struct siglist {
  int signal;
  char *name;
};

/* exported functions */
void appUsage(void);				/* never returns */

/* local functions */
static void appUsageLong(void);			/* never returns */
static void appTeardown(void);
static void appSetup(int argc, char **argv);	/* never returns */
static void appOptionsUsage(void);		/* application options usage */
static int  appOptionsSetup(void);		/* applications setup */
static void appOptionsTeardown(void);		/* applications options
						   teardown  */
static int  appOptionsHandler(clientData cData, int index, char *optarg);
static void stopProcessor(void);
static void closeFiles(void);

/* external functions */
extern int readerSetup(void);
extern void *processor(void *);

/* exported variables */

uint8_t reading = 0;
uint8_t shuttingDown = 0;

/* end of next hour. Reords cannot start or end after this  */
uint32_t nextMaxETime;

pthread_t process_thread;
pthread_t timing_thread;

uint8_t inputIndex;		/* input interface of incoming data */
uint8_t nullInterface = 0;	/* interface to which blocked traffic
				   is sent */

uint16_t port;			/* collection port */
int rcvrSock;			/* udp collection socket */

#define DECLARE_SITE_VARIABLES
#include "silk_site.h"

static streamInfo *pq[NUM_FLOW_TYPES][NUMPERIODS];

static pthread_mutex_t pqmutex = PTHREAD_MUTEX_INITIALIZER;


/* local variables */
static struct siglist flowcapSignals[] =
  {
    {SIGHUP, "SIGHUP"},
    {SIGINT, "SIGINT"},
    {SIGQUIT, "SIGQUIT"},
    {SIGUSR1, "SIGUSR1"},
    {SIGUSR1, "SIGUSR2"},
    {SIGTERM, "SIGTERM"},
#ifdef SIGPWR
    {SIGPWR, "SIGPWR"},
#endif

#ifndef DEBUGGING
    {SIGILL, "SIGILL"},
#ifdef SIGTRAP
    {SIGTRAP, "SIGTRAP"},
#endif
    {SIGABRT, "SIGABRT"},
#ifdef SIGIOT
    {SIGIOT, "SIGIOT"},
#endif
#ifdef SIGBUS
    {SIGBUS, "SIGBUS"},
#endif
    {SIGFPE, "SIGFPE"},
    {SIGSEGV, "SIGSEGV"},
#ifdef SIGURG
    {SIGURG, "SIGURG"},
#endif
#ifdef SIGXCPU
    {SIGXCPU, "SIGXCPU"},
#endif
#ifdef SIGXFSZ
    {SIGXFSZ, "SIGXFSZ"},
#endif
#ifdef SIGSYS
    {SIGSYS, "SIGSYS"},
#endif
#endif /* !DEBUGGING */

    {0, NULL}			/* end flag */
};

typedef enum _appOptionsEnum {
  FP_IN_INDEX, FP_NETFLOW_PORT, FP_NULLINT
} appOptionsEnum;

static struct option appOptions[] = {
  {"in-index", REQUIRED_ARG, 0, FP_IN_INDEX},
  {"netflow-port", REQUIRED_ARG, 0, FP_NETFLOW_PORT},
  {"null-interface", REQUIRED_ARG, 0, FP_NULLINT},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  "input interface number",
  "port on which to accept netflow packets",
  "interface to which blocked traffic is sent (default 0)",
  (char *)NULL
};


static char *rootDir;
static char *logDir;
static char *sID;


/*
 * appUsage:
 * 	print short usage information to stderr and exit with code 1
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
 * 	print complete usage information to stderr and exit with code.
 * 	passed to optionsSetup() to print usage when --help option
 * 	given.
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
static void appUsageLong(void) {
  fprintf(stderr, "%s [options] <outputRoot> <logDir> ", skAppName());
  fprintf(stderr, "<sensorID>\n");
  fprintf(stderr, "Required Args\n");
  fprintf(stderr, "\tpath to dir from which to create hierarchy\n");
  fprintf(stderr, "\tfully qualified path to log file\n");
  fprintf(stderr, "\tsensor ID\n");

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
static void appTeardown(void) {
  static int teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;

  logMsg("Shutting Down");
  shuttingDown = 1;
  stopProcessor();
  closeFiles();
  logTeardown();
  appOptionsTeardown();

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
  int nextOptIndex;

  skAppRegister(argv[0]);

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if (argc < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
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
  if ( (nextOptIndex = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
    return;
  }

  /* Check number of remaining arguments */
  if (argc - nextOptIndex != 3) {
    appUsage();
    return;
  }

  /* root */
  rootDir = argv[nextOptIndex++];

  /* log directory */ 
  logDir = argv[nextOptIndex++];

  /* sensor id */
  sID = argv[nextOptIndex++];
  if (SENSOR_INVALID_SID == sensorNameToId(sID)) {
    fprintf(stderr, "%s: warning: unknown sensor name '%s'\n",
            skAppName(), sID);
  }

  /* set the mask so that the mode is 0664 */
  (void) umask((mode_t) 0002);

  /* Open log file */
  /* Rotate logs 1 hour after midnight.  No mail available */
  if (logSetup("packerlogs", logDir, 1, 0)) {
    fprintf(stderr, "%s: unable to initialize log file\n", skAppName());
    exit(1);
  }

  

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
	    skAppName());
    appTeardown();
    exit(1);
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
 * appOptionsTeardown:
 * 	teardown application options.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None
*/
static void appOptionsTeardown(void) {
  return;
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
  long i;
  char *end;

  switch (index) {

  case FP_IN_INDEX:
    i = strtol(optarg, &end, 10);
    if ((*end != '\0') || i < 0 || i >= MAX_INDICES) {
      logMsg("Invalid index number %s\n", optarg);
      exit(1);
    }
    inputIndex = i;
    break;

  case FP_NETFLOW_PORT:
    i = strtoul(optarg, &end, 10);
    if (*end != '\0') {
      logMsg("Invalid port number %s\n", optarg);
      exit(1);
    }
    port = i;
    break;

  case FP_NULLINT:
    i = strtoul(optarg, &end, 10);
    if ((*end != '\0') || i < 0 || i >= MAX_INDICES) {
      fprintf(stderr, "Invalid interface number %s\n", optarg);
      exit(1);
    }
    nullInterface = i;
    break;

  default:
    appUsage();			/* never returns */
  }
  return 0;			/* OK */
}

/*
** signalHandler:
**	Trap all signals and shutdown when told to.
*/
static void signalHandler(int sigNum) {
  struct siglist *s;
  sigset_t mask_set;	/* used to set a signal masking set. */
  sigset_t old_set;	/* used to store the old mask set.   */

  /* mask any further signals while we're inside the handler. */
  sigfillset(&mask_set);
  sigprocmask(SIG_SETMASK, &mask_set, &old_set);

  /* This is the body of our signal handler */
  for (s = flowcapSignals; s->signal != sigNum; s++);
  logMsg("Shut down by %s signal", s->name);
  printf("Signalled to death\n");
  appTeardown();
}

/*
** installSignalHandler:
**	Trap all signals we can here with our own handler.
**	Exception: SIGPIPE.  Set this to SIGIGN.
** Inputs: None
** Outputs: 0 if OK. 1 else.
*/
static int installSignalHandler(void) {
  struct siglist *s;

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    fprintf(stderr, "%s: Couldn't ignore SIGPIPE\n", skAppName());
    return 1;
  }
  for (s = flowcapSignals; s->signal; s++) {
    if (signal(s->signal, signalHandler) == SIG_ERR) {
      fprintf(stderr, "%s: Couldn't handle %s\n", skAppName(), s->name);
      return 1;
    }
  }

  return 0;
}


static void* flushFiles(void *UNUSED(dummy)) {
  sigset_t sigs;
  struct timespec ts, nts;
  uint32_t i, j;

  sigfillset(&sigs);
  pthread_sigmask(SIG_SETMASK, &sigs, NULL); /* No signals */
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  while (reading) {			/* Infinite loop */
    ts.tv_nsec = 0;
    ts.tv_sec = 300;		/* 5 minutes */

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (nanosleep(&ts, &nts)) {
      ts = nts;
    }
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    pthread_cleanup_push((cleanupHandler)pthread_mutex_unlock,
			 (void *)&pqmutex);
    pthread_mutex_lock(&pqmutex);

    logMsg("Five minute flush.");

    /* Bump down other hours */
    for (i = 0 ; i < numFileTypes; i++) {
      for (j = 0; j < NUMPERIODS; j++) {
	fflush(outFInfo[i].sInfo[j].outF);
      }
    }
    pthread_cleanup_pop(1);
  }

  return NULL;			/* Never gets here */
}

static void closeFiles() {
  uint32_t i, j;
  
  pthread_cleanup_push((cleanupHandler)pthread_mutex_unlock, (void *)&pqmutex);
  pthread_mutex_lock(&pqmutex);

  logMsg("Five minute flush.");

  /* Bump down other hours */
  for (i = 0 ; i < numFileTypes; i++) {
    for (j = 0; j < NUMPERIODS; j++) {
      fflush(outFInfo[i].sInfo[j].outF);
      outFInfo[i].sInfo[j].outF = NULL;
      free(outFInfo[i].sInfo[j].fName);
      outFInfo[i].sInfo[j].fName = NULL;
    }
  }
  pthread_cleanup_pop(1);
}

int processRecord(v5Record *v5RPtr) {
  int type;
  uint8_t u8;
  uint32_t holder;
  uint32_t epoch;
  uint32_t pkts;		/* 10-12 */
  uint32_t bytes;		/* 13-16 */
  uint32_t sTime;		/* 17-20 */
  uint16_t dur;			/* 21-22 */
  uint32_t sbb;
  uint32_t pef;
  int i;
  streamInfo *file = NULL;
  char fName[PATH_MAX];
  rwFileHeaderV0 hdr;
  struct tm *tmS;
  char *cp;
  int flag;
  const uint8_t packed_file_version = 2;
  int pad_length;
  static char padding[SK_MAX_RECORD_SIZE];

  pthread_mutex_lock(&pqmutex);

  sTime = v5RPtr->First;
  epoch = sTime - sTime % 3600;

  /* Deal with blocked flows (ACL, etc.) */
  if (v5RPtr->output == nullInterface) {
    type = -1;
    goto end;
  }

  if (v5RPtr->input == inputIndex) {
    /* Flow is coming in from the isp */
    if (IS_WEB(*v5RPtr)) {
      type = RW_IN_WEB;
    } else {
      type = RW_IN;
    }
  } else {
    /* Flow is coming out to isp */
    if (IS_WEB(*v5RPtr)) {
      type = RW_OUT_WEB;
    } else {
      type = RW_OUT;
    }
  }

  /* Look in out LRU cache for this time period */
  for (i = 0; i < NUMPERIODS; i++)
  {
    if (pq[type][i]->sTime == epoch)
    {
      file = pq[type][i];
      if (i) {
	memmove(&pq[type][1], &pq[type][0], i * sizeof(streamInfo *));
	pq[type][0] = file;
      }
      break;
    }
  }

  /* If file is not in cache, bring it in. */
  if (file == NULL) {
    file = pq[type][NUMPERIODS - 1];
    memmove(&pq[type][1], &pq[type][0], 
	    (NUMPERIODS - 1) * sizeof(streamInfo *));
    pq[type][0] = file;
    if (file->fName) {
      free(file->fName);
    }
    if (file->outF) {
      fclose(file->outF);
    }
    file->sTime = epoch;
    tmS = gmtime((time_t *)&file->sTime);
    snprintf(fName, sizeof(fName),
	     "%s/%s/%04d/%02d/%02d/%s-%s_%04d%02d%02d.%02d",
	     rootDir, outFInfo[type].dirPrefix, 
	     tmS->tm_year + 1900, tmS->tm_mon + 1, tmS->tm_mday,
	     outFInfo[type].filePrefix, sID, 
	     tmS->tm_year + 1900, tmS->tm_mon + 1, tmS->tm_mday, tmS->tm_hour);
    /* make the directory */
    cp = strrchr(fName, '/');
    *cp = 0;
    if (mkDirPath(fName)) {
      logMsg("Unable to create directory %s\n", fName);
      type = -1;
      goto end;
    }
    *cp = '/';
    
    if (fileExists(fName)) {
      /* already exists. Check and load fileSTime */
      if ((file->outF = fopen(fName, "r+")) == (FILE *)NULL) {
	logMsg("Unable to read existing file %s [%s]", fName,
	       strerror(errno));
	type = -1;
	goto end;		/* error */
      }
      if (getWLockFILE_Wait(file->outF) ) {
	logMsg("Cannot get write lock on %s", fName);
	type = -1;
	goto end;
      }

      fread(&hdr, sizeof(hdr), 1, file->outF);

#if	IS_LITTLE_ENDIAN
      if (hdr.gHdr.isBigEndian) {
	logMsg("Error: endianess mismatch");
	type = -1;
	goto end;
      }
#else
      if (!hdr.gHdr.isBigEndian) {
	logMsg("Error: endianess mismatch");
	type = -1;
	goto end;
      }
#endif

      if (file->sTime != hdr.fileSTime) {
	logMsg("Error: computed and stored sTimes vary %u vs %u",
	       file->sTime, hdr.fileSTime);
	type = -1;
	goto end;
      }

      if (hdr.gHdr.type != outFInfo[type].packType ||
	  hdr.gHdr.version != packed_file_version) {
	logMsg("Error: type and version mismatch\n");
	type = -1;
	goto end;
      }

      fseek(file->outF, 0, SEEK_END);

    } else {
      /* create it */
      PREPHEADER(&hdr.gHdr);

      /*extra header info*/
      hdr.gHdr.cLevel=0;
#if IS_LITTLE_ENDIAN
      hdr.gHdr.isBigEndian=0;
#else
      hdr.gHdr.isBigEndian=1;
#endif
      hdr.gHdr.type=outFInfo[type].packType;
      hdr.gHdr.version=packed_file_version;

      hdr.fileSTime = file->sTime;

      if ((file->outF = fopen(fName, "w")) == (FILE *)NULL) {
	logMsg("Unable to create file %s [%s]", fName,
	       strerror(errno));
	type = -1;
	goto end;

      }

      if (getWLockFILE_Wait(file->outF) ) {
	logMsg("Cannot get write lock on %s", fName);
	type = -1;
	goto end;
      }

      fwrite(&hdr, sizeof(hdr), 1, file->outF);

      /* Pad the header to a multiple of the record size for packing
       * versions >= 2 */
      if (packed_file_version >= 2) {
        switch (outFInfo[type].packType) {
        case FT_RWSPLIT:
          pad_length = sizeof(rwSplitRec_V1) - sizeof(hdr);
          break;
        case FT_RWROUTED:
          pad_length = sizeof(rwRtdRec_V1) - sizeof(hdr);
          break;
        case FT_RWNOTROUTED:
          pad_length = sizeof(rwNrRec_V1) - sizeof(hdr);
          break;
        case FT_RWWWW:
          pad_length = sizeof(rwWebRec_V1) - sizeof(hdr);
          break;
        default:
          logMsg("Unknown file type %d\n", outFInfo[type].packType);
          type = -1;
          goto end;
        }
        fwrite(padding, pad_length, 1, file->outF);
      }
    }

    file->fName = strdup(fName);
  }

  pkts = v5RPtr->dPkts;
  bytes = v5RPtr->dOctets;
  holder = v5RPtr->Last - v5RPtr->First;
  if (holder > 0xFFFF) {
    dur = 0xFFFF;
  } else {
    dur = (uint16_t)holder;
  }

  if (dur >= MAX_ELAPSED_TIME) {
    logMsg("Duration is too large: %d  Setting to maximum.", dur);
    dur = MAX_ELAPSED_TIME;
  }

  sTime -= epoch;

  if (sTime >= MAX_START_TIME) {
    logMsg("Start time doesn't fit in epoch.");
    type = -1;
    goto end;
  }
  
  sbb = (sTime << 20) | ((bytes / pkts) << 6) |
	((BPP_PRECN * (bytes % pkts)) / pkts);

  pef = 0;
  if (pkts >= MAX_PKTS) {
    pkts /= PKTS_DIVISOR;
    if (pkts >= MAX_PKTS) {
      logMsg("Double overflow in pkts: ");
      type = -1;
      goto end;
    }
    pef = 1;
  }
  pef |= (pkts << 12) | (dur << 1);

  switch (outFInfo[type].packType) {
  case FT_RWSPLIT: /* rwSplitRec_V1 */
    /* sIp */
    fwrite(&v5RPtr->sIp, 4, 1, file->outF);
    /* dIp */
    fwrite(&v5RPtr->dIp, 4, 1, file->outF);
    /* sPort */
    fwrite(&v5RPtr->sPort, 2, 1, file->outF);
    /* dPort */
    fwrite(&v5RPtr->dPort, 2, 1, file->outF);
    /* pef */
    fwrite(&pef, 4, 1, file->outF);
    /* sbb */
    fwrite(&sbb, 4, 1, file->outF);
    /* proto */
    fwrite(&v5RPtr->proto, 1, 1, file->outF);
    /* flags */
    fwrite(&v5RPtr->flags, 1, 1, file->outF);
    break;    
  case FT_RWROUTED: /* rwRtdRec_V1 */
    /* sIp */
    fwrite(&v5RPtr->sIp, 4, 1, file->outF);
    /* dIp */
    fwrite(&v5RPtr->dIp, 4, 1, file->outF);
    /* nhIP */
    v5RPtr->nexthop.ipnum = ntohl(v5RPtr->nexthop.ipnum);
    fwrite(&v5RPtr->nexthop, 4, 1, file->outF);
    /* sPort */
    fwrite(&v5RPtr->sPort, 2, 1, file->outF);
    /* dPort */
    fwrite(&v5RPtr->dPort, 2, 1, file->outF);
    /* pef */
    fwrite(&pef, 4, 1, file->outF);
    /* sbb */
    fwrite(&sbb, 4, 1, file->outF);
    /* proto */
    fwrite(&v5RPtr->proto, 1, 1, file->outF);
    /* flags */
    fwrite(&v5RPtr->flags, 1, 1, file->outF);
    /* input */
    u8 = (uint8_t)v5RPtr->input;
    fwrite(&u8, 1, 1, file->outF);
    /* output */
    u8 = (uint8_t)v5RPtr->output;
    fwrite(&u8, 1, 1, file->outF);
    break;
  case FT_RWNOTROUTED: /* rwNrRec_V1 */
    /* sIp */
    fwrite(&v5RPtr->sIp, 4, 1, file->outF);
    /* dIp */
    fwrite(&v5RPtr->dIp, 4, 1, file->outF);
    /* sPort */
    fwrite(&v5RPtr->sPort, 2, 1, file->outF);
    /* dPort */
    fwrite(&v5RPtr->dPort, 2, 1, file->outF);
    /* pef */
    fwrite(&pef, 4, 1, file->outF);
    /* sbb */
    fwrite(&sbb, 4, 1, file->outF);
    /* proto */
    fwrite(&v5RPtr->proto, 1, 1, file->outF);
    /* flags */
    fwrite(&v5RPtr->flags, 1, 1, file->outF);
    /* input */
    u8 = (uint8_t)v5RPtr->input;
    fwrite(&u8, 1, 1, file->outF);
    break;
  case FT_RWWWW: /* rwWebRec_V1 */
    /* sIp */
    fwrite(&v5RPtr->sIp, 4, 1, file->outF);
    /* dIp */
    fwrite(&v5RPtr->dIp, 4, 1, file->outF);
    /* pef */
    fwrite(&pef, 4, 1, file->outF);
    /* sbb */
    fwrite(&sbb, 4, 1, file->outF);
    /* port */
    flag = WEBP(v5RPtr->sPort);
    fwrite(flag ? &v5RPtr->dPort : &v5RPtr->sPort, 2, 1, file->outF);
    /* wrf */
    u8 = (flag ? 0x80 : 0) | (v5RPtr->flags & 0x3f);
    fwrite(&u8, 1, 1, file->outF);
    /* wPort */
    u8 = WEBPORT(flag ? v5RPtr->sPort : v5RPtr->dPort) << 6;
    fwrite(&u8, 1, 1, file->outF);
    break;
  default:
    logMsg("Unknown file type %d\n", outFInfo[type].packType);
    type = -1;
    goto end;
  }
 end:
  pthread_mutex_unlock(&pqmutex);
  return type;
}

/*
 * function: bigsockbuf
 *
 * There is no portable way to determine the max send and receive buffers
 * that can be set for a socket, so guess then decrement that guess by
 * 2K until the call succeeds.  If n > 1MB then the decrement by .5MB
 * instead.
 *
 * returns size or -1 for error
*/
static int bigsockbuf(int fd, int dir, int size)
{
  int n, tries;

  /* initial size */
  n = size;
  tries = 0;

  while (n > 4096) {

    if (setsockopt(fd, SOL_SOCKET, dir, (char*)&n, sizeof (n)) < 0) {

      /* anything other than no buffers available is fatal */
      if (errno != ENOBUFS) {
        logMsg("setsockopt(size=%d) failed", n);
        return -1;
      }

      /* try a smaller value */

      if (n > 1024*1024) /* most systems not > 256K bytes w/o tweaking */
        n -= 1024*1024;
      else
        n -= 2048;

      ++tries;

    } else {

      logMsg("setsockopt(size=%d) succeeded", n);
      return n;

    }

  } /* while */

  /* no increase in buffer size */
  return 0;

} /* bigsockbuf */

static int startProcessor(void) { 
  uint32_t i, j;
  struct sockaddr_in addr;
  pthread_attr_t attr;
  int size;

  for (i = 0; i < numFileTypes; i++) {
    for (j = 0; j < NUMPERIODS; j++) {
      pq[i][j] = &outFInfo[i].sInfo[j];
    }
  }

  /* Set up socket to listen */
  if ((rcvrSock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  {
    logMsg("Could not allocate socket for listening");
    return 1;
  }

  /* We need big socket buffers.  Let's try 8MB */
  size = bigsockbuf(rcvrSock, SO_RCVBUF, 1024 * 1024 * 8);
  logMsg("Socket buffer set to %d bytes.", size);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  /* Bind socket to port */
  if (bind(rcvrSock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    logMsg("Could not bind socket to port %u", port);
    return 1;
  }

  if (readerSetup()) {
    return 1;
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  reading = 1;

  if (pthread_create(&timing_thread, NULL, flushFiles, NULL)) {
    return 1;
  }

  if (pthread_create(&process_thread, NULL, processor, NULL)) {
    reading = 0;
    return 1;
  }

  return 0;
}

static void stopProcessor(void) {
  if (reading) {
    logMsg("Stopping processor");
    reading = 0;
    sleep(2); 			/* Give the thread a chance to 
				   quit on its own */
    pthread_cancel(timing_thread);
    pthread_cancel(process_thread); /* Cancel it manually */
    logMsg("Waiting for record handler.");
    pthread_join(process_thread, NULL); /* join */
    close(rcvrSock);
  }
}

/* Make it a daemon */
static void daemonize(void)
{
  pid_t pid;
  char pids[13];
  int fd;
  int len;
  char pidfile[PATH_MAX];

  if ((pid = fork()) == -1) {
    logMsg("Couldn't fork for daemon.");
    exit(1);
  } else if (pid != 0) {
    exit(0);
  }

  setsid();
  chdir("/");
  umask(0);

  if (atexit(appTeardown) < 0) {
    logMsg("unable to register appTeardown() with atexit()");
    appTeardown();
    exit(1);
  }

  pid = getpid();

  logMsg("new rwflowpack: PID=%d", pid);

  len = snprintf(pids, sizeof(pids), "%d", pid);
  if (len <= 0) {
    logMsg("Couldn't generate pid.");
    exit(1);
  }

  snprintf(pidfile, sizeof(pidfile), "%s/rwflowpack.pid", logDir);

  if ((fd = open(pidfile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
    logMsg("Couldn't write pid file.");
    exit(1);
  }

  if (write(fd, pids, len) != len) {
    logMsg("Error writing pid file.");
    unlink(pidfile);
    exit(1);
  }

  close(fd);
}


int main(int argc, char **argv) {
  appSetup(argc, argv);			/* never returns on error */

  /* install the signal hanlers */
  if (installSignalHandler()) {
    logMsg("Failed to start signal handler.  Exiting.");
    exit(1);
  }

#ifndef DEBUGGING
  /* Run as a daemon */
  daemonize();
#endif

  if (startProcessor()) {
    logMsg("Failed to start processor.  Exiting.");
    exit(1);
  }

  /* We now run forever, excepting signals */
  while (!shuttingDown) {
    pause();
  }

  /* done */
  appTeardown();

  return 0;
}
