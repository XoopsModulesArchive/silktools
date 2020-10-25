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
**  1.28
**  2004/03/18 21:18:37
** thomasm
*/



/*
  rwread.c

  library to open and read various raw file types.
  also handle dumping to copyInputFD when required.

  When dumping to copyInputFD, for all except rwfilter, write a generic header
  and then write each record in rwRec format (i.e., already processed).
  For rwfilter, write the filterheader (not just the generic  header) and
  write the rwFilterRec record (i.e., pre-processed).
  
  Suresh Konda
*/
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"
#include "filter.h"

/* Get sensor id definitions and other file-related information */
#include "silk_site.h"

#include "bits.h"

RCSIDENT("rwread.c,v 1.28 2004/03/18 21:18:37 thomasm Exp");


/* static variables */

/* set to 0 after the first file is opened in rwOpenFile() */
static uint32_t fileCount = 0;

/* private functions */
static void rwPrintHeaderV0(rwIOStructPtr rwIOSPtr, FILE *fp);

/* exported functions */
void sendHeader(rwIOStructPtr rwIOSPtr);
int librwSkipHeaderPadding(rwIOStructPtr rwIOSPtr);

/* imported functions */
rwIOStructPtr rwOpenRouted(rwIOStructPtr rwIOSPtr);
uint32_t rwReadRoutedRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipRoutedRec_V1(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenNotRouted(rwIOStructPtr rwIOSPtr);
uint32_t rwReadNotRoutedRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipNotRoutedRec_V1(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenSplit(rwIOStructPtr rwIOSPtr);
uint32_t rwReadSplitRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipSplitRec_V1(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenFilter(rwIOStructPtr rwIOSPtr);
uint32_t rwReadFilterRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipFilterRec_V1(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenGeneric(rwIOStructPtr rwIOSPtr);
uint32_t rwReadGenericRec(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipGenericRec(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenAcl(rwIOStructPtr rwIOSPtr);
uint32_t rwReadAclRec_V0(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipAclRec_V0(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenWeb(rwIOStructPtr rwIOSPtr);
uint32_t rwReadWebRec_V0(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipWebRec_V0(rwIOStructPtr rwIOSPtr, int skipCount);


/*
** getSIDFromFName:
**	Get the sensor-name portion of the file name and convert to an
**	id between 0..254.  255 is returned if the sensor name cannot be
**	parsed from the file name.
** Inputs:
**	char *fPath; int stdinFlag (if 1 do not print error message)
**
*/
#if (SENSOR_COUNT == 0)
#define getSIDFromFName(fPath,stdinFlag) ((uint8_t) SENSOR_INVALID_SID)
#else
static uint8_t getSIDFromFName(const char *fPath, int stdinFlag) {
  /*
  ** as a stop gap measure, we have to stash the sensor id into the
  ** reader struct and get the value from the file name. When we move
  ** to the next version where the sid is embedded into the record itself
  ** we have to change this here and in each of the rwReadXXXX functions.
  ** The general filename format is
  **     <prefix>-<sensorid>_<YYYYMMDD.HH>[.gz]
  **
  ** For version 0, sensorid was X<nn>
  ** For version 1, sensorid is XXXXX, a 5 character mnemonic.
  **
  ** These are stashed in sensorInfo
  */
  unsigned int i = 0;
  char *cp, *ep;
  const size_t max_sid_len = 5;
  unsigned int sid_len;
  char name[max_sid_len + 1];

  cp = baseName(fPath);

  /* skip past <prefix>- */
  cp = strchr(cp, '-');
  if (!cp) {
    goto ERROR;
  }
  cp++;			/* past - */

  if (!isupper((int)*cp)) {
    goto ERROR;
  }

  /* get end of sensor id */
  ep = strchr(cp, '_');
  if (!ep) {
    goto ERROR;
  }

  sid_len = (ep - cp);
  if (sid_len == 0 || sid_len > max_sid_len) {
    goto ERROR;
  }

  /* copy sensor to name */
  strncpy(name, cp, sid_len);
  name[sid_len] = '\0';
  /* do the compare */
  for (i  = 0; i < numSensors; i++ ) {
    if (strcmp(sensorInfo[i].sensorName, name) == 0 ) {
      return i;
    }
  }

 ERROR:
  if (!stdinFlag) {
    if (0 == i) {
      fprintf(stderr, "Warning: could not find Sensor ID. Setting to %u\n",
              SENSOR_INVALID_SID);
    } else {
      fprintf(stderr, "Warning: could not match sensor name %s to a sensor ID."
              " Setting to %u\n", name, SENSOR_INVALID_SID);
    }
  }
  return SENSOR_INVALID_SID;
}
#endif  /* #else clause of #if SENSOR_COUNT == 0 */

/*
** _errorReturn:
**	an internal utility function to cleanup the malloc'd
**	stuff from a rwRdr and return a NULL.
** Input: rwIOStructPtr
** Output: (rwIOStructPtr) NULL
*/
rwIOStructPtr _errorReturn(rwIOStructPtr rwIOSPtr) {
  rwCloseFile(rwIOSPtr);
  fileCount--;
  return (rwIOStructPtr) NULL;
}

/*
** rwCloseFile:
**	close everthing associated with this read struct
** Input: rwIOStructPtr
** Output: None
** 
*/
void rwCloseFile (rwIOStructPtr rwIOSPtr) {
  if (rwIOSPtr->FD) {
    if (rwIOSPtr->FD != stdin && rwIOSPtr->closeFn) {
      rwIOSPtr->closeFn(rwIOSPtr->FD);
    }
    rwIOSPtr->FD = (FILE *)NULL;
  }
  if (rwIOSPtr->hdr) {
    free(rwIOSPtr->hdr);
    rwIOSPtr->hdr = NULL;
  }
  if (rwIOSPtr->fPath) {
    free(rwIOSPtr->fPath);
    rwIOSPtr->fPath = NULL;
  }
  if (rwIOSPtr->origData) {
    free(rwIOSPtr->origData);
    rwIOSPtr->origData = NULL;
  }
  free(rwIOSPtr);
  return;
}

  
/*
** rwOpenFile:
** 	Open the given file and based on the header information
**	setup everything we need to read through the file always
**	returning rwRec.
** Input: char *fPath
**	  FILE * copyInputFD: file descriptor to copy input into
** Output: rwIOStructPtr
**	a pointer to a new struct to contain all the information
**	needed to traverse the file.
**	NULL on error.
** Special Case:
**	If fPath is "stdin", then read binary input stream from stdin.
*/

rwIOStructPtr rwOpenFile (char *fPath, FILE *copyInputFD) {
  rwIOStructPtr rwIOSPtr;
  genericHeader *gHdrPtr;
  FILE *inF;
  int isPipe;
  
  fileCount++;
  rwIOSPtr = (rwIOStructPtr) malloc(sizeof(rwIOStruct));
  memset(rwIOSPtr, 0, sizeof(rwIOStruct));

  if (strcmp(fPath, "stdin") == 0) {
    if (FILEIsATty(stdin)) {
      fprintf(stderr, "stdin is connected to a terminal. Abort\n");
      return _errorReturn(rwIOSPtr);
    }
    rwIOSPtr->FD = stdin;
    /* leave closeFN alone as NULL */
  } else {
    if (openFile(fPath, 0 /* read */, &inF, &isPipe)) {
      return _errorReturn(rwIOSPtr);
    }
    rwIOSPtr->closeFn = isPipe ? pclose : fclose;
    rwIOSPtr->FD = inF;
  }
  if (copyInputFD) {
    if (FILEIsATty(copyInputFD)){
      fprintf(stderr, "copyInputFD is connected to a terminal. Abort\n");
      return _errorReturn(rwIOSPtr);
    }
  }
  rwIOSPtr->copyInputFD = copyInputFD;

  /* read the generic header */
  gHdrPtr = (genericHeader *) rwIOSPtr->hdr
    = (void *) malloc(sizeof(genericHeader));
  if (!fread(gHdrPtr, sizeof(genericHeader), 1, rwIOSPtr->FD)) {
    fprintf(stderr, "rwOpenFile: cannot read generic header from %s\n",
	    fPath);
    return _errorReturn(rwIOSPtr);
  }

  if (CHECKMAGIC(gHdrPtr)) {
    fprintf(stderr, "Invalid header in %s\n", fPath);
    return _errorReturn(rwIOSPtr);
  }

  MALLOCCOPY(rwIOSPtr->fPath, fPath);
  
#if	IS_LITTLE_ENDIAN
  rwIOSPtr->swapFlag = gHdrPtr->isBigEndian;
#else
  rwIOSPtr->swapFlag = ! gHdrPtr->isBigEndian;
#endif
  
  if (gHdrPtr->type != FT_RWFILTER && gHdrPtr->type != FT_RWGENERIC ) {
    rwIOSPtr->sID = getSIDFromFName(fPath, (rwIOSPtr->FD == stdin));
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
    fprintf(stderr, "Invalid file type for %s: 0x%X\n", fPath,
	    gHdrPtr->type);
    return _errorReturn(rwIOSPtr);
  }

  if (rwIOSPtr == NULL) {
    return NULL;;
  }

  /* One of the rwOpenXXX() functions may have modified the header,
   * and our gHdrPtr may be invalid */
  gHdrPtr = (genericHeader *) rwIOSPtr->hdr;

  /* Skip over header padding for version 2 records */
  if ((gHdrPtr->version == 2) && (rwIOSPtr->recLen > 0)) {
    if (librwSkipHeaderPadding(rwIOSPtr)) {
      fprintf(stderr, "rwOpenFile: cannot read header padding from %s\n",
              fPath);
      return _errorReturn(rwIOSPtr);
    }
  }

  return rwIOSPtr;
}


/*
**  librwSkipHeaderPadding(rwIOSPtr)
**      Assume that the rwIOSPtr has read the meaningful part of the
**      header, and read to the next integer multiple of the record
**      size.  Used to skip over any padding in the header for packed
**      files of Version 2 or greater.  The caller should make the
**      version check before calling this function.
**  Input:
**      rwIOStructPtr
**  Result:
**      0 on success, 1 if problem reading/skipping padding
*/
int librwSkipHeaderPadding(rwIOStructPtr rwIOSPtr) {
  long headerLen;
  long remainder;
  uint8_t padding[SK_MAX_RECORD_SIZE];

  if (FT_RWFILTER == ((genericHeader *)rwIOSPtr->hdr)->type) {
    /* Why don't RWFILTER files put their length into hdrLen? */
    headerLen = ((filterHeaderV1*)(rwIOSPtr->hdr))->totFilterLen;
  } else {
    headerLen = rwIOSPtr->hdrLen;
  }
  remainder = headerLen % rwIOSPtr->recLen;
  if (remainder > 0) {
    remainder = rwIOSPtr->recLen - remainder;
    if ( !fread(&padding,  remainder, 1, rwIOSPtr->FD)) {
      return 1;
    }
  }

  return 0;
}


/*
** sendHeader:
** 	Prepare and send a header to copyInputFD using ft generic and version 0
**	Set endian to whatever the current machine's endianness is
**	Set compression level to 0
**	There is no start time for generic records so only send the gHdr
** Input:
**	rwIOSPtr
** Output:
**	None
** Side Effects:
**	None
*/

void sendHeader(rwIOStructPtr rwIOSPtr) {
  genericHeader gHdr;
  rwFileHeaderV0Ptr rwFHdrPtr = rwIOSPtr->hdr;

  /* just for safety */
  if (! rwIOSPtr->copyInputFD) {
    return;
  }

  /* send header for the first file only */
  if (fileCount > 1) {
    return;
  }
    
  memcpy(&gHdr, rwFHdrPtr, sizeof(gHdr));
#if	IS_LITTLE_ENDIAN
  gHdr.isBigEndian = 0;
#else
  gHdr.isBigEndian = 1;
#endif
  gHdr.type = FT_RWGENERIC;
  /* SLK 10/11/2002. Shifted to version 1 */
  gHdr.version  = 1;
  gHdr.cLevel = 0;
  fwrite(&gHdr, sizeof(gHdr), 1, rwIOSPtr->copyInputFD);
  return;
}

void rwPrintHeader(rwIOStructPtr rwIOSPtr, FILE *fp) {
  register genericHeaderPtr gHdrPtr = (genericHeaderPtr) rwIOSPtr->hdr;
  if (gHdrPtr->version == 0) {
    rwPrintHeaderV0(rwIOSPtr, fp);
  } else {
    fprintf(stderr, "invalid version\n");
  }
  return;
}


static void rwPrintHeaderV0(rwIOStructPtr rwIOSPtr, FILE *fp) {
  register rwFileHeaderV0Ptr rwFHPtr = (rwFileHeaderV0Ptr) rwIOSPtr->hdr;

  fprintf(fp, "isBigEndian %s\n", rwFHPtr->gHdr.isBigEndian ? "Yes" : "No");
  fprintf(fp, "File Type %d\n", rwFHPtr->gHdr.type);
  fprintf(fp, "File Version %d\n", rwFHPtr->gHdr.version);
  fprintf(fp, "Compression Level %d\n", rwFHPtr->gHdr.cLevel);
  if (rwFHPtr->gHdr.type != FT_RWFILTER) {
    fprintf(fp, "Start Time %u %s\n", rwFHPtr->fileSTime,
	  timestamp(rwFHPtr->fileSTime));
  }
  if (rwFHPtr->gHdr.type == FT_RWFILTER) {
    filterPrintHeaderV1((filterHeaderV1Ptr)rwIOSPtr, fp);
  }
  return;
}

