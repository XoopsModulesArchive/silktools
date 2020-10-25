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
** 1.7
** 2004/03/10 21:38:57
** thomasm
*/

/*
** rwaclio.c
**
** Suresh L Konda
** 	routines to do io stuff with acl records. Acl off from
** 	an ever increasing rwread.c
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

RCSIDENT("rwacl.c,v 1.7 2004/03/10 21:38:57 thomasm Exp");



/* imported routines */
extern rwIOStructPtr _errorReturn(rwIOStructPtr rwIOSPtr);
extern void sendHeader(rwIOStructPtr rwIOSPtr);

rwIOStructPtr rwOpenAcl(rwIOStructPtr rwIOSPtr);
uint32_t rwReadAclRec_V0(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipAclRec_V0(rwIOStructPtr rwIOSPtr, int skipCount);


rwIOStructPtr rwOpenAcl(rwIOStructPtr rwIOSPtr) {
  genericHeader gHdr;		/* to stash the previously read values */
  rwFileHeaderV0Ptr rwFHdrPtr;

  /* copy the read in header into the temp gHdr */
  memcpy(&gHdr, rwIOSPtr->hdr, sizeof(genericHeader));
  free(rwIOSPtr->hdr);

  if (gHdr.version != 1 && gHdr.version != 2) {
    fprintf(stderr, "Invalid version\n");
    return _errorReturn(rwIOSPtr);
  }
  rwIOSPtr->rwReadFn = rwReadAclRec_V0;
  rwIOSPtr->rwSkipFn = rwSkipAclRec_V0;

  /* get space for this header and copy the generic header */
  rwIOSPtr->hdrLen = sizeof(rwFileHeaderV0);
  rwFHdrPtr = (rwFileHeaderV0Ptr) rwIOSPtr->hdr
    = (void *) malloc(rwIOSPtr->hdrLen);
  memcpy(rwIOSPtr->hdr, &gHdr,sizeof(genericHeader));

  /* read the rest of the header */
  if (!fread(&rwFHdrPtr->fileSTime, sizeof(rwFHdrPtr->fileSTime),
	     1, rwIOSPtr->FD) ) {
    fprintf(stderr, "Could not read rest of header on %s\n",
	    rwIOSPtr->fPath);
    return _errorReturn(rwIOSPtr);
  }

  if (rwIOSPtr->swapFlag) {
    rwFHdrPtr->fileSTime = BSWAP32(rwFHdrPtr->fileSTime);
  }

  /* 12 bytes on disk */
  rwIOSPtr->recLen = 12;
  if (rwIOSPtr->copyInputFD) {
    sendHeader(rwIOSPtr);
  }

  return rwIOSPtr;
}


/*
** rwReadAclRec_V0:
**	read one record from the file associated with the input
**	reader struct.
** Input: rwIOStructPtr: pointer to reader struct
**	  rwRecPtr     pointer to the struct to fill
**  ipUnion sIP; 0-3
**  ipUnion dIP; 4-7
**
**  uint16_t dPort;8-9
**  uint8_t proto; 10
**  uint8_t pFlag:1 + fFlag:1  + sTime:6;  11
**  12 bytes on disk.
**   
** Output: 1 if OK. 0 else.
**
*/

uint32_t rwReadAclRec_V0(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP) {
  uint8_t ar[12];
  register rwFileHeaderV0Ptr rwFHPtr = (rwFileHeaderV0Ptr)rwIOSPtr->hdr;

  if (rwIOSPtr->eofFlag) {
    return 0;
  }

  if ( fread((void*)ar, 1, rwIOSPtr->recLen, rwIOSPtr->FD) != rwIOSPtr->recLen) {
    /* EOF */
    rwIOSPtr->eofFlag  = 1;
    if (!feof(rwIOSPtr->FD)) {
      /* some error */
      fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
    }
    /* Do NOT  we close copyInputFD */
    return 0;
  }

  /* 0 fill since most fields are null */
  memset(rwRP, 0, sizeof(rwRec));
  /* sIP, dIP */
  memcpy(&rwRP->sIP, ar, 8);

  /* dPort + proto */
  memcpy(&rwRP->dPort, &ar[8], 3);
  /* pkts */
  rwRP->pkts = (ar[11] & 128) ? 2 : 1;
  /* flags */
  if ( rwRP->proto == 6  && ar[11] & 64) {
    rwRP->flags = 127;
  }

  rwRP->sTime = rwFHPtr->fileSTime + (60 * (ar[11] & 0x3F /* 00 111111 */));

  if (rwIOSPtr->swapFlag) {
    rwRP->sIP.ipnum = BSWAP32(rwRP->sIP.ipnum);
    rwRP->dIP.ipnum = BSWAP32(rwRP->dIP.ipnum);
    rwRP->dPort = BSWAP16(rwRP->dPort);
  }

  if (rwIOSPtr->copyInputFD) {
    fwrite(rwRP, 1, sizeof(rwRec)-sizeof(rwRP->padding),
	   rwIOSPtr->copyInputFD);
  }
  rwRP->sID = rwIOSPtr->sID;
  return 1;			/* OK */

}/* rwReadAclRec_V0 */


/*
** rwSkipAclRec_V0
**	skip past given number of records.
** Inputs:
**	rwIOStructPtr rwIOSPtr, int skipCount
** Output:
**	0 if ok. 1 else.
*/

uint32_t rwSkipAclRec_V0(rwIOStructPtr rwIOSPtr, int skipCount) {
  register int i;
  rwACLRec_V1 rwrec;

  for (i = 0; i < skipCount; i++) {
    if (! fread(&rwrec, sizeof(rwACLRec_V1), 1, rwIOSPtr->FD)) {
      return 1;			/* failed */
    }
  }
  return 0;
}
