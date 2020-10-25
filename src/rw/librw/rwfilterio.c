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
**  1.10
**  2004/03/10 21:38:57
** thomasm
*/

/*
** rwsplitio.c
**
** Suresh L Konda
** 	routines to do io stuff with split records. Split off from
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

RCSIDENT("rwfilterio.c,v 1.10 2004/03/10 21:38:57 thomasm Exp");



/* imported routines */
extern rwIOStructPtr _errorReturn(rwIOStructPtr rwIOSPtr);
extern void sendHeader(rwIOStructPtr rwIOSPtr);

rwIOStructPtr rwOpenFilter(rwIOStructPtr rwIOSPtr);
uint32_t rwReadFilterRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipFilterRec_V1(rwIOStructPtr rwIOSPtr, int skipCount);
uint32_t rwReadFilterRec_V2(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP);
uint32_t rwSkipFilterRec_V2(rwIOStructPtr rwIOSPtr, int skipCount);

rwIOStructPtr rwOpenFilter(rwIOStructPtr rwIOSPtr) {
  genericHeader gHdr;
  filterHeaderV1Ptr FHPtr;
  int i;
  
  /* copy the read in header into the temp gHdr */
  memcpy(&gHdr, rwIOSPtr->hdr, sizeof(genericHeader));
  free(rwIOSPtr->hdr);

  /*
  ** because of the change in rwRec, we can no longer read version 0
  ** files without major hacking
  */
  /* FIXME. Need to revisit forward compatibility. */
  if (gHdr.version == 0) {
    fprintf(stderr, "Version %d: can only read version > 0 files\n",
	    gHdr.version);
    return _errorReturn(rwIOSPtr);
  }

  /* get space for this header and copy the generic header */
  rwIOSPtr->hdrLen = sizeof(filterHeaderV1);
  FHPtr = (filterHeaderV1Ptr) rwIOSPtr->hdr = calloc(1, rwIOSPtr->hdrLen);
  memcpy(FHPtr, &gHdr, sizeof(genericHeader));

  /* change endianness and copy generic header to copyInputFD if required*/
  if(rwIOSPtr->copyInputFD) {
    i = gHdr.isBigEndian;
#if	IS_LITTLE_ENDIAN
    gHdr.isBigEndian = 0;
#else
    gHdr.isBigEndian = 1;
#endif
    fwrite(&gHdr, sizeof(gHdr), 1, rwIOSPtr->copyInputFD);
    gHdr.isBigEndian = i;
  }

  if (filterReadHeaderV1(FHPtr, rwIOSPtr->FD, rwIOSPtr->swapFlag)) {
    return _errorReturn(rwIOSPtr);
  }
  if (rwIOSPtr->copyInputFD) {
    filterWriteHeaderV1(FHPtr, rwIOSPtr->copyInputFD);
  }

  if (gHdr.version == 1) {
    rwIOSPtr->rwReadFn = rwReadFilterRec_V1;
    rwIOSPtr->rwSkipFn = rwSkipFilterRec_V1;
  } else {
    rwIOSPtr->rwReadFn = rwReadFilterRec_V2;
    rwIOSPtr->rwSkipFn = rwSkipFilterRec_V2;
  }

  /* get space for the original data */
  rwIOSPtr->recLen = sizeof(rwFilterRec_V1);
  rwIOSPtr->origData = (uint8_t *) malloc(rwIOSPtr->recLen);
  return rwIOSPtr;
}



/* 
** rwReadFilterRec_V1: 
**      read one record from the file associated with the input 
**      reader struct. 
** Input: rwIOStructPtr: pointer to reader struct 
**        rwRecPtr     pointer to the struct to fill 
**  ipUnion sIP; 0-3
**  ipUnion dIP; 4-7
**
**  uint16_t sPort; 8-9
**  uint16_t dPort;10-11
**
**  uint8_t proto; 12
**  uint8_t flags; 13
**  uint8_t input; 14
**  uint8_t output; 15
**
**  ipUnion nhIP; 16-19
**  uint32_t sTime; 20-23
**
**  uint32_t pkts    :20; 24-27
**  uint32_t elapsed :11;
**  uint32_t pktsFlag: 1;
**
**  uint32_t bPPkt   :14; 28-31
**  uint32_t bPPFrac : 6;
**  uint32_t sID     : 6;
**  uint32_t pad2    : 6;
** Output: 1 if OK. 0 else. 
** 
*/ 
 
uint32_t rwReadFilterRec_V1(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP) { 
  register uint32_t pkts, pktsFlag, bPPkt, bPPFrac;
  uint32_t iw[2];
  register uint32_t i;

  if (rwIOSPtr->eofFlag) {
    return 0;
  }

  if ( fread((void*)(rwIOSPtr->origData), 1, rwIOSPtr->recLen, rwIOSPtr->FD)
       != rwIOSPtr->recLen) {
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

#if 0 /* Moved to after the byte swap */
  /* in this case, dump the pure input data */
  if (rwIOSPtr->copyInputFD) {
    fwrite((void*)(rwIOSPtr->origData), 1, rwIOSPtr->recLen,
	   rwIOSPtr->copyInputFD);
  }
#endif
  
  /* sip, dip, ports, proto, flags, input, output, nhip stime  */ 
  memcpy(rwRP, rwIOSPtr->origData, 24);
  memcpy(iw, &rwIOSPtr->origData[24], 8);

  if (rwIOSPtr->swapFlag) {
    rwRP->sIP.ipnum = BSWAP32(rwRP->sIP.ipnum);
    rwRP->dIP.ipnum = BSWAP32(rwRP->dIP.ipnum);
    rwRP->nhIP.ipnum = BSWAP32(rwRP->nhIP.ipnum);
    rwRP->sPort = BSWAP16(rwRP->sPort);
    rwRP->dPort = BSWAP16(rwRP->dPort);
    rwRP->sTime = BSWAP32(rwRP->sTime);
    iw[0] = BSWAP32(iw[0]);
    iw[1] = BSWAP32(iw[1]);
  }

  /* Write to the all-dest file descriptor */
  if (rwIOSPtr->copyInputFD) {
    if (rwIOSPtr->swapFlag) {
      /* Dump the swapped input data */
      fwrite((void*)rwRP, 1, 24, rwIOSPtr->copyInputFD);
      fwrite((void*)iw,   1,  8, rwIOSPtr->copyInputFD);
    } else {
      fwrite((void*)(rwIOSPtr->origData), 1, rwIOSPtr->recLen,
             rwIOSPtr->copyInputFD);
    }
  }
  

  pkts = iw[0] >> 12;
  rwRP->elapsed = (iw[0] >> 1) & MASKARRAY_11;
  pktsFlag = iw[0] & MASKARRAY_01;

  if (pktsFlag) {
    rwRP->pkts = PKTS_DIVISOR * pkts;
  } else {
    rwRP->pkts = pkts;
  }
  bPPkt = (iw[1] >> 18);
  bPPFrac = (iw[1] >> 12)  & MASKARRAY_06;
  i = bPPFrac * rwRP->pkts;
  rwRP->bytes = (bPPkt * rwRP->pkts)
    + i/BPP_PRECN + ((i % BPP_PRECN) >= BPP_PRECN_DIV_2 ? 1 : 0);

  rwRP->sID = (iw[1] >> 6)  & MASKARRAY_06;

  return 1;			/* OK */
} /*rwReadFilterRec_V1 */ 


/*
** rwSkipFilterRec_V1
**	skip past given number of records.
** Inputs:
**	rwIOStructPtr rwIOSPtr, int skipCount
** Output:
**	0 if ok. 1 else.
*/

uint32_t rwSkipFilterRec_V1(rwIOStructPtr rwIOSPtr, int skipCount) {
  register int i;
  rwFilterRec_V1 rwrec;

  for (i = 0; i < skipCount; i++) {
    if (! fread(&rwrec, sizeof(rwFilterRec_V1), 1, rwIOSPtr->FD)) {
      return 1;			/* failed */
    }
  }
  return 0;
}

/*
** NOTE: This is identitical to the V1 version except that the sensor
** id is stored in the lower 8 bits of the word with four bits of
** padding. FIXME. A little reuse in the new version of librw?
**
** rwReadFilterRec_V2: 
**      read one record from the file associated with the input 
**      reader struct.
** Input: rwIOStructPtr: pointer to reader struct 
**        rwRecPtr     pointer to the struct to fill 
**  ipUnion sIP; 0-3
**  ipUnion dIP; 4-7
**
**  uint16_t sPort; 8-9
**  uint16_t dPort;10-11
**
**  uint8_t proto; 12
**  uint8_t flags; 13
**  uint8_t input; 14
**  uint8_t output; 15
**
**  ipUnion nhIP; 16-19
**  uint32_t sTime; 20-23
**
**  uint32_t pkts    :20; 24-27
**  uint32_t elapsed :11;
**  uint32_t pktsFlag: 1;
**
**  uint32_t bPPkt   :14; 28-31
**  uint32_t bPPFrac : 6;
**  uint32_t pad2    : 4;
**  uint32_t sID     : 8;
** Output: 1 if OK. 0 else. 
** 
*/ 
 
uint32_t rwReadFilterRec_V2(rwIOStructPtr rwIOSPtr, rwRecPtr rwRP) { 
  register uint32_t pkts, pktsFlag, bPPkt, bPPFrac;
  uint32_t iw[2];
  register uint32_t i;

  if (rwIOSPtr->eofFlag) {
    return 0;
  }

  if ( fread((void*)(rwIOSPtr->origData), 1, rwIOSPtr->recLen, rwIOSPtr->FD)
       != rwIOSPtr->recLen) {
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

#if 0 /* Moved to after the byte swap */
  /* in this case, dump the pure input data */
  if (rwIOSPtr->copyInputFD) {
    fwrite((void*)(rwIOSPtr->origData), 1, rwIOSPtr->recLen,
	   rwIOSPtr->copyInputFD);
  }
#endif
  
  /* sip, dip, ports, proto, flags, input, output, nhip stime  */ 
  memcpy(rwRP, rwIOSPtr->origData, 24);
  memcpy(iw, &rwIOSPtr->origData[24], 8);

  if (rwIOSPtr->swapFlag) {
    rwRP->sIP.ipnum = BSWAP32(rwRP->sIP.ipnum);
    rwRP->dIP.ipnum = BSWAP32(rwRP->dIP.ipnum);
    rwRP->nhIP.ipnum = BSWAP32(rwRP->nhIP.ipnum);
    rwRP->sPort = BSWAP16(rwRP->sPort);
    rwRP->dPort = BSWAP16(rwRP->dPort);
    rwRP->sTime = BSWAP32(rwRP->sTime);
    iw[0] = BSWAP32(iw[0]);
    iw[1] = BSWAP32(iw[1]);
  }

  /* Write to the all-dest file descriptor */
  if (rwIOSPtr->copyInputFD) {
    if (rwIOSPtr->swapFlag) {
      /* Dump the swapped input data */
      fwrite((void*)rwRP, 1, 24, rwIOSPtr->copyInputFD);
      fwrite((void*)iw,   1,  8, rwIOSPtr->copyInputFD);
    } else {
      fwrite((void*)(rwIOSPtr->origData), 1, rwIOSPtr->recLen,
             rwIOSPtr->copyInputFD);
    }
  }
  

  pkts = iw[0] >> 12;
  rwRP->elapsed = (iw[0] >> 1) & MASKARRAY_11;
  pktsFlag = iw[0] & MASKARRAY_01;

  if (pktsFlag) {
    rwRP->pkts = PKTS_DIVISOR * pkts;
  } else {
    rwRP->pkts = pkts;
  }
  bPPkt = (iw[1] >> 18);
  bPPFrac = (iw[1] >> 12)  & MASKARRAY_06;
  i = bPPFrac * rwRP->pkts;
  rwRP->bytes = (bPPkt * rwRP->pkts)
    + i/BPP_PRECN + ((i % BPP_PRECN) >= BPP_PRECN_DIV_2 ? 1 : 0);

  rwRP->sID = (iw[1])  & MASKARRAY_08;

  return 1;			/* OK */
} /*rwReadFilterRec_V2 */ 


/*
** rwSkipFilterRec_V2
**	skip past given number of records.
** Inputs:
**	rwIOStructPtr rwIOSPtr, int skipCount
** Output:
**	0 if ok. 1 else.
*/

uint32_t rwSkipFilterRec_V2(rwIOStructPtr rwIOSPtr, int skipCount) {
  register int i;
  rwFilterRec_V1 rwrec;

  for (i = 0; i < skipCount; i++) {
    if (! fread(&rwrec, sizeof(rwFilterRec_V2), 1, rwIOSPtr->FD)) {
      return 1;			/* failed */
    }
  }
  return 0;
}
