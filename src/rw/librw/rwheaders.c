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
** 1.6
** 2004/03/10 21:38:57
** thomasm
*/

/*
**  rwheaders.c
**
**  Routines for reading/writing rwfilter header files
**  (moved from util/filter/filterio.c)
**
**/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "filter.h"
#include "rwpack.h"

RCSIDENT("rwheaders.c,v 1.6 2004/03/10 21:38:57 thomasm Exp");


/*
** filterWriteHeaderV1:
**	Dump the filter header (NOT the generic header)  into the given outF.
**	NOTE: We do NOT dump the total byte count field which is declared
**	in the header but only used in the reader.
** Inputs:
**	filterHeaderV1Ptr, outF
** Output:
**	0 if OK.  1 else
*/
int filterWriteHeaderV1(filterHeaderV1Ptr FHPtr, FILE *outF) {
  uint32_t i;
  uint16_t byteCount;
  uint32_t bytesWritten;

  /* Keep track of bytes written so we can pad out to a multiple of
   * the record length.  So far, the generic header has been
   * written. */
  bytesWritten = sizeof(genericHeader);

  /* Write the number of filters first */
  if (!fwrite(&FHPtr->filterCount, sizeof(FHPtr->filterCount), 1, outF)) {
    return 1;
  }
  bytesWritten += sizeof(FHPtr->filterCount);

  for (i = 0; i < FHPtr->filterCount; i++) {
    byteCount = FHPtr->fiArray[i]->byteCount;
    fwrite(FHPtr->fiArray[i], byteCount + sizeof(byteCount), 1, outF);
    bytesWritten += byteCount + sizeof(byteCount);
  }

  /* If required, add padding so that header length is multiple of the
   * record length. */
  if (FHPtr->gHdr.version == 2) {
    /* Ugh.  I really wish I had an rwIOStructPtr here so I would know
     * the length of the thing I was writing.  Having all writing in
     * librw in the redesign will fix this I hope.... */
    size_t remainder = bytesWritten % sizeof(rwFilterRec_V2);
    if (0 != remainder) {
      /* must pad */
      uint8_t padding[sizeof(rwFilterRec_V2)];
      memset(&padding, '\0', sizeof(padding));
      fwrite(&padding, (sizeof(rwFilterRec_V2) - remainder), 1, outF);
    }
  }

  return 0;
} /*filterWriteHeaderV1*/


/*
** filterAddFInfoToHeaderV1:
** 	add a new filter info header to the given header.
** Inputs:
**	filterHeaderV1Ptr, argc, argv
** Outputs:
**	0 if OK. 1 else
*/
int filterAddFInfoToHeaderV1(filterHeaderV1Ptr FHPtr, int argc, char **argv) {
  int i;
  filterInfoV1Ptr fInfoPtr;
  char *cp;
  int len;

  len = 0;
  for(i = 1; i < argc; i++) {
    len += strlen(argv[i]) + 1; /* allow for the null byte as well */
  }
  /* malloc the required space byteCount + info */
  fInfoPtr = (filterInfoV1Ptr) malloc( sizeof(fInfoPtr->byteCount) + len);
  fInfoPtr->byteCount = len;

  cp = &fInfoPtr->info[0];
  for(i = 1; i < argc; i++) {
    /* copy all including trailing null */
    len = strlen(argv[i]) + 1;
    memcpy(cp, argv[i], len);
    cp += len;
  }
  /* stash the filter info into the header */
  FHPtr->fiArray[FHPtr->filterCount] = fInfoPtr;
  FHPtr->filterCount++;
  return 0;
} /* filteraddFInfoToHeaderV1 */


/*
** filterPrintHeaderV1
**	Print the filter in a decent format.
** Inputs:
**	filterHeaderV1Ptr
**	FILE *
** Output: none
*/
void filterPrintHeaderV1(filterHeaderV1Ptr fHPtr, FILE *outF) {
  uint32_t i;
  int bLen;
  char *cp;
  int sawNL = 0;

  fprintf(outF, "File Type = %s; Version = %d\n",
	  fHPtr->gHdr.type == FT_GWFILTER ? "Gateway Filter" :
	  fHPtr->gHdr.type == FT_RWFILTER ? "Raw Filter" :
	  fHPtr->gHdr.type == FT_TDFILTER ? "TcpDump Filter" : "Unknown",
	  fHPtr->gHdr.version);
  fprintf(outF, "isBigEndian = %s; cLevel = 0\n",
	  fHPtr->gHdr.isBigEndian ? "Yes" : "no");
  fprintf(outF, "# of Filters Applied %u\n", fHPtr->filterCount);
  for (i = 0; i < fHPtr->filterCount; i++) {
    fprintf(outF, "Fltr %d:\t", i+1);
    sawNL = 0;
    bLen = fHPtr->fiArray[i]->byteCount;
    cp = &fHPtr->fiArray[i]->info[0];
    while(bLen--) {
      if (sawNL) {
	sawNL = 0;
	fprintf(outF, "\t");
      }
      if (*cp) {
	fprintf(outF, "%c", *cp);
      } else {
	fprintf(outF, "\n");
	sawNL = 1;
      }
      cp++;
    }
  }
  if (!sawNL) {
    fprintf(outF, "\n");
  }
  return;
} /*filterPrintHeaderV1*/


/*
** filterReadHeaderV1
**	Read the filter header from the given fp.
** Inputs:
**	filterHeaderV1Ptr
**	FILE *
**	swapFlag
** Output:
**	0 if OK; 1 else.
*/
int filterReadHeaderV1(filterHeaderV1Ptr FHPtr, FILE *inF, int swapFlag) {
  uint16_t byteCount;
  uint32_t i;

  /* read the # of filter information structs we have */
  if (!fread(&FHPtr->filterCount, sizeof(FHPtr->filterCount), 1, inF)) {
    fprintf(stderr, "Can't read filterCount value from file\n");
    return 1;
  }

  /* and load each one into newly malloc'd space */
  if (swapFlag) {
     FHPtr->filterCount = BSWAP32(FHPtr->filterCount);
  }

  FHPtr->totFilterLen = sizeof(genericHeader) + sizeof(FHPtr->filterCount);
  for (i = 0; i < FHPtr->filterCount; i++) {
    /* the info length */
    if (!fread(&byteCount, sizeof(byteCount), 1, inF)) {
      fprintf(stderr,"Can't read byteCount value for filter %d from file\n",i);
      return 1;
    }
    if (swapFlag) {
      byteCount = BSWAP16(byteCount);
    }

    /* get space for the struct in the array */
    FHPtr->fiArray[i] = (filterInfoV1Ptr)
      malloc(byteCount + sizeof(byteCount));
    FHPtr->fiArray[i]->byteCount = byteCount;
    FHPtr->totFilterLen += byteCount + sizeof(byteCount);
    /* read in the real information */
    if (byteCount) {
      if (!fread(&FHPtr->fiArray[i]->info, byteCount, 1, inF)) {
	fprintf(stderr,
          "Can't read %d value bytes for filter %d from file\n",
	  byteCount, i);
	return 1;
      }
    }
  }
  return 0;
} /*filterPrintHeaderV1*/
