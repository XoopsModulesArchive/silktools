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

**  1.9
**  2004/03/10 21:38:58
** thomasm
*/

/*
** rwwwwio.c
**
** 	routines to do io stuff with web records.
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "utils.h"
#include "filter.h"
#include "rwpack.h"
#include "bits.h"

RCSIDENT("rwwwwio.c,v 1.9 2004/03/10 21:38:58 thomasm Exp");

/* imported routines */
extern rwIOStructPtr _errorReturn(rwIOStructPtr rwIOSPtr);
extern void sendHeader(rwIOStructPtr rwIOSPtr);

rwIOStructPtr rwOpenWeb(rwIOStructPtr rwIOSPtr);
uint32_t rwReadWebRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipWebRec_V1(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenWeb(rwIOStructPtr rwIOSPtr) {
  genericHeader gHdr;		/* to stash the previously read values */
  rwFileHeaderV0Ptr rwFHdrPtr;

  /* copy the read in header into the temp gHdr */
  memcpy(&gHdr, rwIOSPtr->hdr, sizeof(genericHeader));
  free(rwIOSPtr->hdr);

  if (gHdr.version == 0) {
    fprintf(stderr, "Invalid version\n");
    return _errorReturn(rwIOSPtr);
  } else   if (gHdr.version == 1 || gHdr.version == 2) {
    rwIOSPtr->rwReadFn = rwReadWebRec_V1;
  } else {
    fprintf(stderr, "Version mismatch\n");
    return _errorReturn(rwIOSPtr);
  }
  rwIOSPtr->rwSkipFn = rwSkipWebRec_V1;

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

  /* 20 bytes on disk and core */
  rwIOSPtr->recLen = 20;
  if (rwIOSPtr->copyInputFD) {
    sendHeader(rwIOSPtr);
  }

  return rwIOSPtr;
}

/*
** rwReadWebRec_V1:
**	read one record from the file associated with the input
**	reader struct.
** Input: rwIOStructPtr: pointer to reader struct
**	  rwRecPtr     pointer to the struct to fill
**  ipUnion sIP; 0-3
**  ipUnion dIP; 4-7
**
**  uint32_t pef 8-11 uint32_t pkts:20; uint32_t elapsed :11; uint32_t pktsFlag:1;
**  uint32_t sbb 12-15 uint32_t sTime:12;  uint32_t bPPkt:14;  uint32_t bPPFrac:6;
**
**  uint16_t port; 16-17
**  
**  uint8_t wf 18 uint32_t wsPort:1; uint32_t reserved:1; uint32_t flags:6;
**  uint8_t wp 19 uint32_t wPort:2; uint32_t padding:6; 
**
**  0 = 80 ; 1 = 443 ; 2 = 8080
**
**  20 bytes on disk and core
**   
** Output: 1 if OK. 0 else.
**
*/

uint32_t rwReadWebRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP) {
  uint8_t ar[20]; /* 20 bytes on disk and in core */
  register rwFileHeaderV0Ptr rwFHPtr = (rwFileHeaderV0Ptr)rwIOSPtr->hdr;
  uint32_t pkts, elapsed, pktsFlag, bPPkt, sTime, bPPFrac, iw[2];
  uint16_t port, wport;
  uint8_t ws, wp;
  register uint32_t i;

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
    /* should we close copyInputFD if we are copying to it? */
    /* fclose(rwIOSPtr->copyInputFD); */
    return 0;
  }

  /* sIP, dIP */
  memcpy(&rwRP->sIP, ar, 8);
  /* pef and sbb */
  memcpy(iw, &ar[8], 8);

  memcpy(&port, &ar[16], 2);

  rwRP->proto = 6;
  rwRP->flags = ar[18] & 0x3F;
  ws = ar[18] & 0x80;
  wp = (ar[19] >> 6) & 0x03;

  switch (wp) {
  case 0: wport = 80; break;
  case 1: wport = 443; break;
  case 2: wport = 8080; break;
  default:
    wport = 0;
  }
  
  if (rwIOSPtr->swapFlag) {
    rwRP->sIP.ipnum = BSWAP32(rwRP->sIP.ipnum);
    rwRP->dIP.ipnum = BSWAP32(rwRP->dIP.ipnum);
    port = BSWAP16(port);
    rwRP->dPort = BSWAP16(rwRP->dPort);
    iw[0] = BSWAP32(iw[0]);
    iw[1] = BSWAP32(iw[1]);
  }

  if (ws) {
    memcpy(&rwRP->sPort, &wport, 2);
    memcpy(&rwRP->dPort, &port, 2);
  } else {
    memcpy(&rwRP->dPort, &wport, 2);
    memcpy(&rwRP->sPort, &port, 2);
  }
    
  pkts = iw[0] >> 12;
  elapsed = (iw[0] >> 1) & MASKARRAY_11;
  pktsFlag = iw[0] & MASKARRAY_01;

  sTime = iw[1] >> 20;
  bPPkt = (iw[1] >> 6) & MASKARRAY_14;
  bPPFrac = iw[1] & MASKARRAY_06;

  if (pktsFlag) {
    rwRP->pkts = PKTS_DIVISOR * pkts;
  } else {
    rwRP->pkts = pkts;
  }
  i = bPPFrac * rwRP->pkts;
  rwRP->bytes = (bPPkt * rwRP->pkts)
    + i/BPP_PRECN + ((i % BPP_PRECN) >= BPP_PRECN_DIV_2 ? 1 : 0);

  rwRP->sTime = rwFHPtr->fileSTime + sTime;
  rwRP->elapsed = elapsed;
  rwRP->sID = rwIOSPtr->sID;
  rwRP->nhIP.ipnum = 0;
  rwRP->input = rwRP->output = 0;

  if (rwIOSPtr->copyInputFD) {
    fwrite(rwRP, 1, sizeof(rwRec)-sizeof(rwRP->padding),
	   rwIOSPtr->copyInputFD);
  }

  return 1;			/* OK */

}/* rwReadWebRec_V1 */

/*
** rwSkipWebRec_V1
**	skip past given number of records.
** Inputs:
**	rwIOStructPtr rwIOSPtr, int skipCount
** Output:
**	0 if ok. 1 else.
*/

uint32_t rwSkipWebRec_V1(rwIOStructPtr rwIOSPtr, int skipCount) {
  register int i;
  rwWebRec_V1 rwrec;

  for (i = 0; i < skipCount; i++) {
    if (! fread(&rwrec, sizeof(rwWebRec_V1), 1, rwIOSPtr->FD)) {
      return 1;			/* failed */
    }
  }
  return 0;
}
