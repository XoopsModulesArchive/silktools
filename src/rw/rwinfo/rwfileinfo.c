/*
** Copyright (C) 2003,2004 by Carnegie Mellon University.
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
** 2004/02/18 21:03:00
** thomasm
*/

/*
**  rwfileinfo
**
**  Prints information about an rw-file:
**    file type
**    file version
**    byte order
**    compression level
**    header size
**    record size
**    record count
**    file size
**    command line args used to generate file
**
*/


/* Includes */
/* FIXME. Fix this hack. See silk_files.h */
#define DECLARE_FILETYPE_VARIABLES
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>

#include "utils.h"
#include "filter.h"
#include "rwpack.h"

RCSIDENT("rwfileinfo.c,v 1.11 2004/02/18 21:03:00 thomasm Exp");


/* local defines and typedefs */
#define	NUM_REQUIRED_ARGS	1


/* This is used to store information about a files */
struct rwinfoFileInfo {
  int compression;          /* our compression level (not gzip) */
  int version;              /* file version */
  int type;                 /* file type, (RWSPLIT, etc) as integer */
  off_t fileSize;           /* (uncompressed) file size */
  size_t recordLength;      /* size of one record */
  size_t headerLength;      /* size required for all headers */
  off_t countRecords;       /* count of records */
  const char *byteOrder;    /* byte order */
  const char *path;         /* filesystem path to file */
  const char *typeString;   /* file type as string "RWSPLIT" */
  const char *commandLines; /* command lines */
};

/* A list of the properties that can be printed in the order that they
 * will be printed. */
enum rwinfoPropertyEnum {
  RWINFO_TYPE,
  RWINFO_VERSION,
  RWINFO_BYTE_ORDER,
  RWINFO_COMPRESSION,
  RWINFO_HEADER_LENGTH,
  RWINFO_RECORD_LENGTH,
  RWINFO_COUNT_RECORDS,
  RWINFO_FILE_SIZE,
  RWINFO_COMMAND_LINES,
  /* Last item is used to get a count of the above; it must be last */
  RWINFO_PROPERTY_COUNT
};

/* Whether each property will be printed, and the label to use */
struct rwinfoProperties {
  int userFields;                            /* use gave fields? */
  int8_t willPrint[RWINFO_PROPERTY_COUNT];   /* nonzero if printed */
  const char *label[RWINFO_PROPERTY_COUNT];  /* property's label */
};


/* imported functions from librw */
rwIOStructPtr rwOpenRouted(rwIOStructPtr rwIOSPtr);
rwIOStructPtr rwOpenNotRouted(rwIOStructPtr rwIOSPtr);
rwIOStructPtr rwOpenSplit(rwIOStructPtr rwIOSPtr);
rwIOStructPtr rwOpenFilter(rwIOStructPtr rwIOSPtr);
rwIOStructPtr rwOpenGeneric(rwIOStructPtr rwIOSPtr);
rwIOStructPtr rwOpenAcl(rwIOStructPtr rwIOSPtr);
rwIOStructPtr rwOpenWeb(rwIOStructPtr rwIOSPtr);

/* local functions */
static void appUsage(void);
static void appUsageLong(void);
static void appTeardown(void);
static void appSetup(int argc, char **argv);
static void appOptionsUsage(void);
static int  appOptionsSetup(void);
static void appOptionsTeardown(void);
static int  appOptionsHandler(clientData cData, int index, char *optarg);
static int  getFileInfo(struct rwinfoFileInfo *info);



static struct rwinfoProperties rwinfoProp = {
  0,
  {0, 0, 0, 0, 0, 0, 0, 0, 0},
  {
    "type",
    "version",
    "byte-order",
    "compression",
    "header-length",
    "record-length",
    "count-records",
    "file-size",
    "command-lines"
  }
};


/* Create constants for the option processor */
enum rwinfoOptionIds {
  RWINFO_OPT_FIELDS,
  /* Last item is used to get a count of the above; it must be last */
  RWINFO_OPTION_COUNT
};

static struct {
  /* a count of the options; from the enum above */
  int           count;
  /* index into argv of first non-option argument */
  int           nextIndex;
  /* the list of options; add one for sentinel */
  struct option options[1+RWINFO_OPTION_COUNT];
  /* their help strings; add one for sentinel */
  const char    *help[1+RWINFO_OPTION_COUNT];
} rwinfoOptions = {
  RWINFO_OPTION_COUNT,
  1,
  {
    {"fields", REQUIRED_ARG, 0, RWINFO_OPT_FIELDS},
    {0,0,0,0} /* sentinal entry */
  },
  {
    /* add help strings here for the applications options */
    "list of fields to print.  Default ALL",
    (char *)NULL /* sentinal entry */
  }
};


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
static void appUsageLong(void)
{
  int i;

  printf("%s [app_opts] <file1> [<file2> .. <fileN>]", skAppName());
  printf("  Prints info (number records, etc) about an rw-file\n");

  appOptionsUsage();

  printf("Field values:\n");
  for (i = 0; i < RWINFO_PROPERTY_COUNT; ++i) {
    printf("  %3d %s\n", 1+i, rwinfoProp.label[i]);
  }

  exit(0);
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

  appOptionsTeardown();
  optionsTeardown();

  /* local teardown segment */

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
  int i;

  skAppRegister(argv[0]);

  /* first do some sanity checks */

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
  if ( (rwinfoOptions.nextIndex = optionsParse(argc, argv)) < 0) {
    appUsage();		/* never returns */
    return;
  }

  if (rwinfoOptions.nextIndex >= argc) {
    fprintf(stderr, "%s: expecting file name\n", skAppName());
    appUsage();
    return;
  }

  /* if user selected no fields to print, print all */
  if (!rwinfoProp.userFields) {
    for (i = 0; i < RWINFO_PROPERTY_COUNT; ++i) {
      rwinfoProp.willPrint[i] = -1;
    }
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
static void appOptionsUsage(void)
{
  int i;
  printf("\napp options\n");
  for (i = 0; i < RWINFO_OPTION_COUNT; i++ ) {
    printf("--%s %s. %s\n", rwinfoOptions.options[i].name,
           rwinfoOptions.options[i].has_arg ? 
           rwinfoOptions.options[i].has_arg == 1
           ? "Required Arg" : "Optional Arg" : "No Arg", rwinfoOptions.help[i]);
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
  if (optionsRegister(rwinfoOptions.options, (optHandler) appOptionsHandler,
                      (clientData) &rwinfoProp)) {
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
static int appOptionsHandler(clientData cData, int index, char *optarg)
{
  uint8_t *numberList;
  uint8_t numberCount;
  uint8_t i;

  switch(index) {
  case RWINFO_OPT_FIELDS:
    numberList =skParseNumberList(optarg,1,RWINFO_PROPERTY_COUNT,&numberCount);
    if (!numberList) {
      return 1;
    }
    ((struct rwinfoProperties*)cData)->userFields = 1;
    for (i = 0; i < numberCount; ++i) {
      ((struct rwinfoProperties*)cData)->willPrint[numberList[i]-1] = 1;
    }
    free(numberList);
    break;

  default:
    fprintf(stderr, "%s: option index '%d' out of range\n", skAppName(),index);
    appUsage(); /* dies */
  }

  return 0;
}


/*
 * void getHeaderLength(rwIOStructPtr rwIOSPtr, struct rwinfoFileInfo *info);
 *
 * Determine the header length of the file and store this information
 * in the rwinfoFileInfo pointer.  If the file's has command line
 * information, obtain that as well.
 */
static void getHeaderLength(
  rwIOStructPtr rwIOSPtr,
  struct rwinfoFileInfo *info)
{
  div_t ratio;
  char *commandBuffer;
  char *bp;
  char *cp;
  uint32_t i;
  uint16_t bytecount;
  filterHeaderV1 *fhp;
  

  if ( 0 == rwIsCompressed(rwIOSPtr) ) {
    /* file is not compressed; rwOpenFile() leaves us at end of header */
    info->headerLength = ftell(rwGetFILE(rwIOSPtr));
    if (info->headerLength == (size_t)-1) {
      perror("tell failed");
    }
  } else {
    /* file is compressed.  get header length from rwIOSPtr */
    if (FT_RWFILTER == rwGetFileType(rwIOSPtr)) {
      info->headerLength = ((filterHeaderV1*)(rwIOSPtr->hdr))->totFilterLen;
    } else {
      info->headerLength = rwIOSPtr->hdrLen;
    }

    if ((2 == info->version) && (info->recordLength > 0)) {
      /* for version 2, the headerLength is always a multiple of the
       * recordLength */
      ratio = div(info->headerLength, info->recordLength);
      if (0 > ratio.rem) {
        /* headerLength is not exact multiple of recordLength */
        info->headerLength = info->recordLength * (1 + ratio.quot);
      }
    }
  }

  if (FT_RWFILTER == rwGetFileType(rwIOSPtr)) {
    /* we know the length of commands will not be larger than the
     * entire header, so malloc that much space */
    commandBuffer = calloc(info->headerLength, 1);
    bp = commandBuffer;
    /* local copy of the header */
    fhp = ((filterHeaderV1*)(rwIOSPtr->hdr));
    /* loop over each command */
    for (i = 0; i < fhp->filterCount; ++i) {
      /* get length and point cp at the first character in command */
      bytecount = fhp->fiArray[i]->byteCount;
      cp = &(fhp->fiArray[i]->info[0]);
      while(bytecount--) {
        /* each command "phrase" is separated by a \0; change it to a
         * space */
        if (*cp) {
          *bp = *cp;
        } else {
          *bp = ' ';
        }
        ++cp;
        ++bp;
      }
      /* finish this command and move forward to start the next
       * command */
      *bp = '\0';
      ++bp;
    }
    info->commandLines = commandBuffer;
  }
}


/*
 * void getNumberRecs(rwIOStructPtr rwIOSPtr, struct rwinfoFileInfo *info);
 *
 * Given rwIOSPtr to the opened file, read the file to determine the
 * file's size and the number of records in the file.  Add this
 * information to the rwinfoFileInfo pointer.
 */
static void getNumberRecs(rwIOStructPtr rwIOSPtr, struct rwinfoFileInfo *info)
{
  off_t rc = 0;	/* record count */
  off_t fSize;

  if (rwIsCompressed(rwIOSPtr)) {
    while (0 == rwSkip(rwIOSPtr, 1)) {
      ++rc;
    }
    info->fileSize = info->headerLength + (info->recordLength * rc);
  } else {
    info->fileSize = fileSize(rwGetFileName(rwIOSPtr));
    fSize = info->fileSize - (long) info->headerLength;
    if (info->recordLength > 0) {
      if ((fSize % info->recordLength) != 0) {
        fprintf(stderr,
                "Incomplete file. Size=%lld. HL=%lu. RL=%lu. %lld excess bytes\n",
                (long long)(info->headerLength+fSize),
                (unsigned long)info->headerLength,
                (unsigned long)info->recordLength,
                (long long)(fSize%(info->recordLength)));
      }
      rc = fSize / info->recordLength;
    }
  }
  info->countRecords = rc;

  return;
}


/*
 *  getFileInfo(struct rwinfoFileInfo info);
 *
 *  Print the file information for the file at path.  Return 0 on
 *  success, or 1 on failure.
 */
static int getFileInfo(struct rwinfoFileInfo *info) {
  /*
   * Basically we have to do a complete re-implementation of
   * rwOpenFile, except don't support copying the input, we don't do
   * the sensor check, and we support all known file types.
   */
  rwIOStructPtr rwIOSPtr;
  genericHeader *gHdrPtr;
  FILE *inF;
  int isPipe;
  long ftp;

  rwIOSPtr = (rwIOStructPtr) malloc(sizeof(rwIOStruct));
  memset(rwIOSPtr, 0, sizeof(rwIOStruct));

  if (strcmp(info->path, "stdin") == 0) {
    if (FILEIsATty(stdin)) {
      fprintf(stderr, "%s: stdin is connected to a terminal. Abort\n",
              skAppName());
      goto ERROR;
    }
    rwIOSPtr->FD = stdin;
    /* leave closeFN alone as NULL */
  } else {
    if (openFile(info->path, 0 /* read */, &inF, &isPipe)) {
      goto ERROR;
    }
    rwIOSPtr->closeFn = isPipe ? pclose : fclose;
    rwIOSPtr->FD = inF;
  }

  /* read the generic header */
  gHdrPtr = (genericHeader *) rwIOSPtr->hdr
    = (void *) malloc(sizeof(genericHeader));
  if (!fread(gHdrPtr, sizeof(genericHeader), 1, rwIOSPtr->FD)) {
    fprintf(stderr, "%s: cannot read generic header from %s\n",
	    skAppName(), info->path);
    goto ERROR;
  }

  if (CHECKMAGIC(gHdrPtr)) {
    fprintf(stderr, "%s: invalid header in %s\n", skAppName(), info->path);
    goto ERROR;
  }

  MALLOCCOPY(rwIOSPtr->fPath, info->path);
  
#if	IS_LITTLE_ENDIAN
  rwIOSPtr->swapFlag = gHdrPtr->isBigEndian;
#else
  rwIOSPtr->swapFlag = ! gHdrPtr->isBigEndian;
#endif


  /* get the information from the generic Header */
  info->type = gHdrPtr->type;
  info->compression = gHdrPtr->cLevel;
  info->version = gHdrPtr->version;
  if (gHdrPtr->isBigEndian) {
    info->byteOrder = "BigEndian";
  } else {
    info->byteOrder = "littleEndian";
  }
  if (info->type >= ftypeCount) {
    info->typeString = "UNKNOWN";
  } else {
    info->typeString = fileTypes[info->type];
  }


  switch(gHdrPtr->type) {
  case FT_RWROUTED:
    rwIOSPtr = rwOpenRouted(rwIOSPtr);
    break;
    
  case FT_RWNOTROUTED:
    rwIOSPtr = rwOpenNotRouted(rwIOSPtr);
    break;
    
  case FT_RWSPLIT:
    rwIOSPtr = rwOpenSplit(rwIOSPtr);
    break;
      
  case FT_RWFILTER:
    rwIOSPtr = rwOpenFilter(rwIOSPtr);
    break;
    
  case FT_RWGENERIC:
    rwIOSPtr = rwOpenGeneric(rwIOSPtr);
    break;
    
  case FT_RWACL:
    rwIOSPtr = rwOpenAcl(rwIOSPtr);
    break;

  case FT_RWWWW:
    rwIOSPtr = rwOpenWeb(rwIOSPtr);
    break;
    
  default:
    /* Can't get any more info */
    rwCloseFile(rwIOSPtr);
    return 0;
  }

  if (rwIOSPtr == NULL) {
    return 0;
  }

  /* One of the rwOpenXXX() functions may have modified the header,
   * and our gHdrPtr may be invalid */
  gHdrPtr = (genericHeader *) rwIOSPtr->hdr;

  /* Skip over header padding for version 2 records */
  if (gHdrPtr->version == 2) {
    ftp = ftell(rwIOSPtr->FD) % rwIOSPtr->recLen;
    fseek(rwIOSPtr->FD, ftp ? rwIOSPtr->recLen - ftp : 0, SEEK_CUR);
  }

  info->recordLength = rwIOSPtr->recLen;

  getHeaderLength(rwIOSPtr, info);
  getNumberRecs(rwIOSPtr, info);

  rwCloseFile(rwIOSPtr);

  return 0;

 ERROR:
  rwCloseFile(rwIOSPtr);
  return 1;
}


/*
 * printFileInfo(struct rwinfoFileInfo *info)
 *
 * Print information (as requested by the user in the rwinfoProp
 * structure) in 'info' to stdout.
 */
void printFileInfo(struct rwinfoFileInfo *info)
{
  const char *indentString = "  ";
  const int labelWidth = -16;
  const char *comm;
  uint32_t count;

  printf("%s:\n", info->path);
  if (rwinfoProp.willPrint[RWINFO_TYPE]) {
    printf("%s%*s%s\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_TYPE], info->typeString);
  }
  if (rwinfoProp.willPrint[RWINFO_VERSION]) {
    printf("%s%*s%d\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_VERSION], info->version);
  }
  if (rwinfoProp.willPrint[RWINFO_BYTE_ORDER]) {
    printf("%s%*s%s\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_BYTE_ORDER], info->byteOrder);
  }
  if (rwinfoProp.willPrint[RWINFO_COMPRESSION]) {
    printf("%s%*s%d\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_COMPRESSION], info->compression);
  }
  if (rwinfoProp.willPrint[RWINFO_HEADER_LENGTH]) {
    printf("%s%*s%u\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_HEADER_LENGTH], info->headerLength);
  }
  if (rwinfoProp.willPrint[RWINFO_RECORD_LENGTH]) {
    printf("%s%*s%u\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_RECORD_LENGTH], info->recordLength);
  }
  if (rwinfoProp.willPrint[RWINFO_COUNT_RECORDS]) {
    printf("%s%*s%lld\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_COUNT_RECORDS],
           (long long)info->countRecords);
  }
  if (rwinfoProp.willPrint[RWINFO_FILE_SIZE]) {
    printf("%s%*s%lld\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_FILE_SIZE], (long long)info->fileSize);
  }
  if (rwinfoProp.willPrint[RWINFO_COMMAND_LINES] && info->commandLines) {
    printf("%s%*s\n", indentString, labelWidth,
           rwinfoProp.label[RWINFO_COMMAND_LINES]);
    comm = info->commandLines;
    count = 0;
    while (*comm) {
      ++count;
      printf("%*d%s%s\n", (-1*labelWidth), count, indentString, comm);
      comm += 1+strlen(comm);
    }
  }
}


/*
 *  For each file, get the file's info then print it
 */
int main(int argc, char **argv)
{
  int i;
  struct rwinfoFileInfo info;

  appSetup(argc, argv);			/* never returns on error */

  for (i = rwinfoOptions.nextIndex; i < argc; ++i) {
    memset(&info, 0, sizeof(struct rwinfoFileInfo));
    info.path = argv[i];
    if (0 == getFileInfo(&info)) {
      printFileInfo(&info);
    } else {
      fprintf(stderr, "%s: perhaps '%s' is not a SiLK file?\n",
              skAppName(), argv[i]);
    }
    if (info.commandLines) {
      free((char*)info.commandLines);
    }
  }

  /* done */
  appTeardown();

  return 0;
}
