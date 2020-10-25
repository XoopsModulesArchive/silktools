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
** 1.12
** 2004/03/18 15:22:39
** thomasm
*/

/*
** rwgenericio.c
**
** Suresh L Konda
**      routines to do io stuff with generic records. Split off from
**      an ever increasing rwread.c
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
#include "rwpack.h"
#include "bits.h"

RCSIDENT("rwgenericio.c,v 1.12 2004/03/18 15:22:39 thomasm Exp");


/* imported routines */
extern rwIOStructPtr _errorReturn(rwIOStructPtr rwIOSPtr);
extern void sendHeader(rwIOStructPtr rwIOSPtr);

rwIOStructPtr rwOpenGeneric(rwIOStructPtr rwIOSPtr);
uint32_t rwReadGenericRec(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipGenericRec(rwIOStructPtr rwIOSPtr, int skipCount);
uint32_t rwWriteGenericRec(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);


/*
** rwOpenGeneric:
**      open a generic file.  The header is the pure generic header
** NOTES: This is almost a noop since generic headers have no
**      byte swapping to do and nothing else to read etc.
*/
rwIOStructPtr rwOpenGeneric(rwIOStructPtr rwIOSPtr) {
  genericHeader * gHdrPtr;
  rwRec rwrec;

  gHdrPtr = (genericHeader *) rwIOSPtr->hdr;
  rwIOSPtr->hdrLen = sizeof(genericHeader);

  rwIOSPtr->rwReadFn = rwReadGenericRec;
  rwIOSPtr->rwSkipFn = rwSkipGenericRec;

  /* added version 1 on 10/11/2002 to avoid writing out the padding bytes*/
  switch (gHdrPtr->version) {
  case 0:
    rwIOSPtr->recLen = sizeof(rwrec);
    break;
  case 1:
    rwIOSPtr->recLen = sizeof(rwrec) - sizeof(rwrec.padding);
    break;
  default:
    fprintf(stderr, "Invalid generic version %d\n", gHdrPtr->version);
    return _errorReturn(rwIOSPtr);
  }

  if (rwIOSPtr->copyInputFD) {
    sendHeader(rwIOSPtr);
  }

  return rwIOSPtr;
}


/*
** rwReadGenericRec:
**      read one record from the file associated with the input
**      reader struct.
** Input: rwIOStructPtr: pointer to reader struct
**        rwRecPtr     pointer to the struct to fill
** Output: 1 if OK. 0 else.
**
*/
uint32_t rwReadGenericRec(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP) {
  if (rwIOSPtr->eofFlag) {
    return 0;
  }

  if (fread((void*)rwRP, 1, rwIOSPtr->recLen, rwIOSPtr->FD)!=rwIOSPtr->recLen){
    /* EOF */
    rwIOSPtr->eofFlag  = 1;
    if (!feof(rwIOSPtr->FD)) {
      /* some error */
      fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
    }
    /* should we close copyInputFD if we are copying to it? */
    /* fclose(rwIOSPtr->copyInputFD); */
    return 0;
  }

  if (rwIOSPtr->swapFlag) {
    rwRP->sIP.ipnum = BSWAP32(rwRP->sIP.ipnum);
    rwRP->dIP.ipnum = BSWAP32(rwRP->dIP.ipnum);
    rwRP->sPort = BSWAP16(rwRP->sPort);
    rwRP->dPort = BSWAP16(rwRP->dPort);
    rwRP->nhIP.ipnum = BSWAP32(rwRP->nhIP.ipnum);
    rwRP->sTime = BSWAP32(rwRP->sTime);
    rwRP->pkts = BSWAP32(rwRP->pkts);
    rwRP->bytes = BSWAP32(rwRP->bytes);
    rwRP->elapsed = BSWAP32(rwRP->elapsed);
  }

  /* Write to the all-dest file descriptor */
  if (rwIOSPtr->copyInputFD) {
    fwrite(rwRP, 1, rwIOSPtr->recLen, rwIOSPtr->copyInputFD);
  }

  return 1;                     /* OK */
} /*rwReadGenericRec */


/*
** rwSkipGenericRec
**      skip past given number of records.
** Inputs:
**      rwIOStructPtr rwIOSPtr, int skipCount
** Output:
**      0 if ok. 1 else.
*/
uint32_t rwSkipGenericRec(rwIOStructPtr rwIOSPtr, int skipCount) {
  int i;
  rwRec rwrec;
  for (i = 0; i < skipCount; i++) {
    if (! fread(&rwrec, rwIOSPtr->recLen, 1, rwIOSPtr->FD)) {
      return 1;                 /* failed */
    }
  }
  return 0;
}


/*
** rwWriteGenericRec:
**      read one record from the file associated with the input
**      reader struct.
** Input: rwIOStructPtr: pointer to reader struct
**        rwRecPtr     pointer to the struct to fill
** Output: 1 if OK. 0 else.
**
*/
uint32_t rwWriteGenericRec(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP) {
  if ( fwrite((void*)rwRP, 1, rwIOSPtr->recLen, rwIOSPtr->FD) != rwIOSPtr->recLen) {
    fprintf(stderr, "Write error on %s: [%s]\n", rwIOSPtr->fPath,
            strerror(errno));
    return 0;
  }
  return 1;                     /* OK */
} /*rwWriteGenericRec */
