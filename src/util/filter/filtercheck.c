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
** 1.13
** 2004/03/10 22:23:52
** thomasm
*/

/*
 *
 *  filter.c
 *
 *  Michael Collins
 *
 *  routines to read and use filtering rules for extracting subsets of our data
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "filter.h"
#include "rwpack.h"

RCSIDENT("filtercheck.c,v 1.13 2004/03/10 22:23:52 thomasm Exp");


#define checkMark(i, f) ( (f[i/32] & (1 << (i % 32))) ? 1 : 0 )
#define checkRange(v, t) (((v >= t.min) && (v <= t.max)) ? 1 : 0 )
#define isolateOctet(a, i) ((a & (0xFF << i * 8)) >> (i * 8))
/*
 * checkAddress(uint32_t tgtAddress, addressRange *maskRange)
 *
 * SUMMARY:
 *
 * checkAddress verifies whether a particular address falls within an
 * address range.
 *
 * PARAMETERS:
 *	Address to check and the maskRange to use
 *
 *
 * RETURNS:
 *	Returns 1 if a successful match. O otherwise.
 *
 *
 * SIDE EFFECTS:
 *	None.
 *
 *
 * NOTES:
 *
 * Endianism.
*/
#if	1
int checkAddress(uint32_t tgtAddress, addressRange *maskRange) {
  ipUnion address;
  address.ipnum = tgtAddress;

  /*
   * We short circuit on the octets.
   */
  if (!checkMark(address.O.o1, maskRange->octets[0])) {
    return 0;
  }
  if (!checkMark(address.O.o2, maskRange->octets[1])) {
    return 0;
  }
  if (!checkMark(address.O.o3, maskRange->octets[2])) {
    return 0;
  }
  if (!checkMark(address.O.o4, maskRange->octets[3])) {
    return 0;
  }

  return 1;			/* in range */
}
#else
/*
 * The following has been tested as code and fails to operate as predicted; it
 * appears to be passing all addresses throuhg.  I have disabled it for the
 * duration.
*/
#define checkAddress(a,r) !checkMark(((ipUnion)(a)).O.o1, ((addressRange *) (r))->octets[0]) ?  0 : \
                           !checkMark(((ipUnion)(a)).O.o2, ((addressRange *) (r))->octets[1]) ?  0 : \
                           !checkMark(((ipUnion)(a)).O.o3, ((addressRange *) (r))->octets[2]) ?  0 : \
                           !checkMark(((ipUnion)(a)).O.o4, ((addressRange *) (r)) ->octets[3]) ?  0 : 1
#endif



/*
 * checkRwRules
 *
 * SUMMARY:
 *
 *
 *
 * PARAMETERS:
 *
 *
 *
 * RETURNS:
 *
 *
 *
 * SIDE EFFECTS:
 *
 *
 *
 * NOTES:
 *
 *
*/
int checkRWRules(void *tmpRec) {
  extern filterRules *fltrRulesPtr;
  rwRec *tgtRec;
  register int i, j;

  tgtRec = (rwRec  *) tmpRec;
  for (j = 0; j < fltrRulesPtr->maxRulesUsed; j++) {
    switch(fltrRulesPtr->ruleSet[j]) {

    case STIME:
      if (checkRange(tgtRec->sTime, fltrRulesPtr->sTime) == 0) {
	return 1;
      }
      break;

    case ENDTIME:
      if (checkRange(tgtRec->sTime+tgtRec->elapsed, fltrRulesPtr->eTime)== 0) {
	return 2;
      }
      break;

    case DURATION:
      if (checkRange(tgtRec->elapsed, fltrRulesPtr->duration) == 0) {
	return 3;
      }
      break;

    case SPORT:
      if (checkMark(tgtRec->sPort, fltrRulesPtr->srcPorts) == 0) {
	return 4;
      }
      break;

    case DPORT:
      if (checkMark(tgtRec->dPort, fltrRulesPtr->dstPorts) == 0) {
	return 5;
      }
      break;

    case PROTOCOL:
      if (checkMark(tgtRec->proto, fltrRulesPtr->protocols) == 0) {
	return 7;
      }
      break;

    case BYTES:
      if (!checkRange(tgtRec->bytes, fltrRulesPtr->bytes)) {
	return 7;
      }
      break;

    case PKTS:
      if (checkRange(tgtRec->pkts, fltrRulesPtr->packets) == 0) {
	return 8;
      }
      break;

    case BYTES_PER_PKT:
      if (!checkRange((((uint64_t)(tgtRec->bytes)) * PRECISION)
		      / ((uint64_t)(tgtRec->pkts)),
		      fltrRulesPtr->bytesPerPacket)) {
	return 10;
      }
      break;

    case NOT_SADDRESS:
    case SADDRESS:
      for(i=0; i < fltrRulesPtr->totalAddresses[SRC]; i++) {
	/* MC: comment this code. I cannot grok it */
	if(checkAddress(tgtRec->sIP.ipnum, &(fltrRulesPtr->IP[SRC][i]))
	   ^ !fltrRulesPtr->negateIP[SRC]) {
	  return 1;
	}
      }
      break;

    case NOT_DADDRESS:
    case DADDRESS:
      for(i=0; i < fltrRulesPtr->totalAddresses[DST]; i++) {
	if(checkAddress(tgtRec->dIP.ipnum, &(fltrRulesPtr->IP[DST][i]))
           ^ !fltrRulesPtr->negateIP[DST]) {
          return 13;
	}
      }
      break;

      /*
       * TCP check.  Passes if there's an intersection between the raised flags
       * and the  filter flags.
       */
    case TCP_FLAGS:
      if(! (fltrRulesPtr->tcpFlags & tgtRec->flags) ) {
	return 17;
      }
      break;

    case APORT:
       if (checkMark(tgtRec->sPort, fltrRulesPtr->anyPorts))
	break;			/* want it */
      if (checkMark(tgtRec->dPort, fltrRulesPtr->anyPorts))
	break;			/* want it */
      return 22;		/* dont want it */
      break;			/* want it */

      /*
       * This ensures that we fail o the first bad flag regardless...
       *
       */
    case OPT_SYN:
    case OPT_ACK:
    case OPT_FIN:
    case OPT_PSH:
    case OPT_URG:
    case OPT_RST:
      /*
       * This is !(marked xor flags) and (careMask) == marked
       */
      if((0x3F & (~(fltrRulesPtr->flagMark ^ tgtRec->flags) | ~fltrRulesPtr->flagCare)) != 0x3F) {
	return OPT_SYN;
      }
      break;
#if	ADD_SENSOR_RULE
      /*
       * sensor ID check.
       */
    case SENSORS:
      if (!checkMark(tgtRec->sID, fltrRulesPtr->sensors)) {
	return 18;
      }
      break;
#endif
    case INPUT_INTERFACES:
      if (!checkMark(tgtRec->input, fltrRulesPtr->inputInterfaces)) {
	return 19;
      }
      break;

    case OUTPUT_INTERFACES:
      if (!checkMark(tgtRec->output, fltrRulesPtr->outputInterfaces)) {
	return 20;
      }
      break;

    case NEXT_HOP_ID:
      if (!checkAddress(tgtRec->nhIP.ipnum, &(fltrRulesPtr->nextHop))) {
	return 21;
      }
      break;

    } /* switch */
  } /* outer for */

  return 0;			/* WANTED! */
}

/*
**  loadFilterRulesV0:
**  	read the filter rules actually used in a compressed format from
** 	the given file in binary. Version 0 only.
**  Return:
**  	0 OK; 1 else.
**  NOTE that the caller should free the returned pointer.
*/

filterHeaderV0Ptr loadFilterRulesV0(FILE *fp, int UNUSED(swapFlag)) {
  uint8_t filterCount;
  filterRulesPtr frPtr;
  filterHeaderV0Ptr fltrHPtr;
  filterRulesPtr (*array)[1];
  int i;

  if ( fread(&filterCount, sizeof(filterCount), 1, fp) != 1) {
    fprintf(stderr, "Unable to get filterCount from file\n");
    return (filterHeaderV0Ptr) NULL;
  }

  fltrHPtr = (filterHeaderV0Ptr) malloc( sizeof(filterHeaderV0));
  fltrHPtr->filterCount = filterCount;
  array = fltrHPtr->frArray = (filterRulesPtr (*)[1])
    malloc(filterCount * sizeof(filterRulesPtr));
  for (i = 0; i < filterCount; i++) {
    frPtr = (*array)[i];
    if ( fread(frPtr, sizeof(filterRules), 1, fp) != 1) {
      fprintf(stderr, "Unable to get filterRules # %d from file\n", i);
      return (filterHeaderV0Ptr) NULL;
    }
  }
  return (fltrHPtr);
}


/*
**  loadFilterRulesV1:
**  	read the filter rules actually the given file in V1 binary file
**  Return:
**  	0 OK; 1 else.
**  NOTE that the caller should free the returned pointer.
*/

filterHeaderV1Ptr loadFilterRulesV1(FILE *fp, int swapFlag) {
  uint32_t filterCount;
  filterHeaderV1Ptr fltrHPtr;
  filterInfoV1Ptr fIPtr;
  uint32_t i;

  if ( fread(&filterCount, sizeof(filterCount), 1, fp) != 1) {
    fprintf(stderr, "Unable to get filterCount from file\n");
    return (filterHeaderV1Ptr) NULL;
  }
  if(swapFlag) {
    filterCount = BSWAP32(filterCount);
  }
  fltrHPtr = (filterHeaderV1Ptr) malloc( sizeof(filterHeaderV1));
  fltrHPtr->filterCount = filterCount;
  for (i = 0; i < filterCount; i++) {
    fIPtr = fltrHPtr->fiArray[i];
    if ( fread(&fIPtr->byteCount, sizeof(fIPtr->byteCount), 1, fp) != 1) {
      fprintf(stderr, "Unable to read fIPtr->byteCount\n");
      return (filterHeaderV1Ptr)NULL;
    }
    if(swapFlag) {
      fIPtr->byteCount = BSWAP16(fIPtr->byteCount);
    }
    if (fread(fIPtr->info, fIPtr->byteCount, 1, fp) != 1) {
      fprintf(stderr, "Unable to get filterRules # %d from file\n", i);
      return (filterHeaderV1Ptr) NULL;
    }
  }
  return (fltrHPtr);
}

