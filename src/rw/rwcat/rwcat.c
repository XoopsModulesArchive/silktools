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
** 1.11
** 2004/03/10 22:11:59
** thomasm
*/


/*
**  rwcat.c
**
**  12/13/2002
**
**  Suresh L. Konda
**
**  Stream out a bunch of input files to  stdout or given file Path
**
**  Input files can be on the command line or via fglob but not both.
**  Options: output-file-path; print file names flag;
**
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
#include "fglob.h"

RCSIDENT("rwcat.c,v 1.11 2004/03/10 22:11:59 thomasm Exp");


/* globals */
#define NUM_REQUIRED_ARGS 3     /* at least two input files */

rwIOStructPtr rwIOSPtr;
void appUsage(void);
void dumpHeader(void);

#if DEBUG
void writeRRec(rwRec *rwrec);
void writeFRec(rwFilterRec *frec);
#endif

/* purely local variables */
rwRec rwrec;
char *inFPath;          /* current input file path */
int firstFile;
int flagPrintFN = 0;
FILE *outF;
char *outFPath = "stdout";
int outIsPipe;
static uint32_t outC = 0;

/* local variables */
static struct option appOptions[] = {
  {"output-path",       REQUIRED_ARG,   0, 0},
  {"print-filenames", NO_ARG, 0, 1},
  {0,0,0,0}                     /* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  "full path name of the output file",
  "print input filenames while processing",
  (char *)NULL
};

static void appUsageLong(void);
static int catFile(char *inFName);
void  appSetup(int, char**);
void appTeardown(void);
static void appOptionsUsage(void);
/*
** dumpHeader:
**      Prepare and dump a header to outF using ft generic and version 1
**      Set endian to whatever the current machine's endianness is
**      Set compression level to 0
**      There is no start time for generic records so only dump the gHdr
** Input:
**      rwIOSPtr
** Output:
**      None
** Side Effects:
**      None
*/

void dumpHeader(void) {
  genericHeader gHdr;

  PREPHEADER(&gHdr);
  gHdr.type = FT_RWGENERIC;
  gHdr.version  = 1;
  gHdr.cLevel = 0;
  fwrite(&gHdr, sizeof(gHdr), 1, outF);
  return;
}


/*
** appUsage:
** print usage information to stderr and exit with code 1
** Arguments: None
** Returns: None
** Side Effects: exits application.
*/
void appUsage(void) {
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
  fprintf(stderr, "Usage %s [rwcut options] <fglob-options | inputFiles...>\n",
          skAppName());
  appOptionsUsage();
  fglobUsage();
  exit(1);
}


/*
 * appOptionsUsage:
 *      print help for options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
 * NOTE:
 *  do NOT add non-option usage here (i.e., required and other optional args)
*/

static void appOptionsUsage(void) {
  register int i;
  fprintf(stderr, "\napp options\n");
  for (i = 0; i < appOptionCount; i++ ) {
    fprintf(stderr, "--%s %s. %s\n", appOptions[i].name,
            appOptions[i].has_arg ? appOptions[i].has_arg == 1
            ? "Req. Arg" : "Opt. Arg" : "No Arg", appHelp[i]);
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
  case 0:
    /* output file path */
    MALLOCCOPY(outFPath, optarg);
    if ( openFile(outFPath, 1 /* write */, &outF, &outIsPipe) ) {
      return 1;
    }
    break;

  case 1:
    /* print input file names */
    flagPrintFN = 1;
    break;

  default:
    fprintf(stderr, "Invalid option %d\n", index);
    return 1;
    break;

  } /* switch */
  return 0;                     /* OK */
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
** appSetup
**do all the setup for this application include setting up required
** modules etc.
** Arguments:
**argc, argv
** Returns: None.
** Side Effects:
**exits with code 1 if anything does not work.
*/
void appSetup(int argc, char **argv) {

  skAppRegister(argv[0]);

  if(argc < NUM_REQUIRED_ARGS) {
    appUsageLong();
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);/* never returns */
  }

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);
  }

  outF = stdout;

  /* parse options */
  if ( (firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();         /* never returns */
  }

  /* if output is to stdout, see that it is not a tty */
  if (outF == stdout && FILEIsATty(stdout)) {
    fprintf(stderr, "stdout is connected to a terminal. Abort\n");
    appUsage();                 /* never returns */
  }

  if (fglobValid()) {
    /*
     * fglob used. if files also explicitly given, abort
     */
    if (firstFile < argc) {
      fprintf(stderr, "argument mismatch. Both fglob options & input files given\n");
      exit(1);
    }
  }
  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    appTeardown();
    exit(1);
  }

  (void) umask((mode_t) 0002);
  return;/* OK */
}


/*
** appTeardown:
**      Teardown all used modules and all application stuff.
** Arguments: None
** Returns: None
** Side Effects:
** All modules are torn down. Then application teardown takes place.
** Static variable teardownFlag is set.
** NOTE: This must be idempotent using static teardownFlag.
*/
void appTeardown(void) {
  static uint8_t teardownFlag = 0;

  if (teardownFlag) {
    return;
  }
  teardownFlag = 1;
  if (rwIOSPtr) {
    rwCloseFile(rwIOSPtr);
  }
  rwIOSPtr = (rwIOStructPtr )NULL;
  outIsPipe ? pclose(outF) : fclose(outF);

  return;
}

/* for dumping headers */
int main(int argc, char ** argv) {
  int counter;
  char * curFName;

  appSetup(argc, argv);

  dumpHeader();
  if (fglobValid()) {
    while ((curFName = fglobNext())){
      if (catFile(curFName)) {
        break;
      }
    }
  } else {
    for(counter = firstFile; counter < argc ; counter++) {
      if (catFile(argv[counter])) {
        break;
      }
    }
  }
  appTeardown();
  exit(0);
  return(0);                    /* make gcc happy on linux */
}

/*
** catFile:
**      Dump all records to  outF
*/

static int catFile(char *inFName) {
  uint32_t inC = 0;

  if ( ( rwIOSPtr = rwOpenFile(inFName, (FILE *)NULL))
       == (rwIOStructPtr)NULL) {
    /* some error. the library would have dumped a msg */
    return 1;
  }

  if (flagPrintFN) {
    fprintf(stderr, "%s\n", inFName);
  }
#if     1                       /* set to 0  for testing */
  while (rwRead(rwIOSPtr,&rwrec)) {
    inC++; outC++;
    fwrite(&rwrec, sizeof(rwrec) - sizeof(rwrec.padding), 1, outF);
  }
#endif
  rwCloseFile(rwIOSPtr);
  rwIOSPtr = (rwIOStructPtr)NULL;
  if (flagPrintFN) {
    fprintf(stderr, "Read %u Wrote %u\n", inC, outC);
  }
  return 0;
}
