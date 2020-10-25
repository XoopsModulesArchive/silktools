/*
**  Copyright (C) 2003-2004 by Carnegie Mellon University.
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
** 1.3
** 2004/03/10 22:45:13
** thomasm
*/

/*
 * rwswapbytes
 *
 * Read any rw file (rwpacked file, rwfilter output, etc) and output a
 * file in the specified byte order.
 *
 * This file contains the file IO and swapping code.
 *
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "rwpack.h"
#include "rwswapbytes.h"
#include "filter.h"

RCSIDENT("rwswapbytes.c,v 1.3 2004/03/10 22:45:13 thomasm Exp");


/* buffer to hold records */
#define MAX_REC_LEN 4096
static uint8_t data[MAX_REC_LEN];
uint8_t * const dataPtr = &(data[0]);

/* swap the bytes */
#define SWAP_DATA32(d) *((uint32_t*)(d)) = BSWAP32(*(uint32_t*)(d))
#define SWAP_DATA16(d) *((uint16_t*)(d)) = BSWAP16(*(uint16_t*)(d))


/*
 * swapRwFileHeaderV0(rwIOSPtr)
 *
 *   Write the file header (rwFileHeaderV0) of the rw-file pointed at
 *   by "rwIOSPtr" to the "outF" FILE handle, changing the endian-ness
 *   of the header.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
static void swapRwFileHeaderV0(rwIOStructPtr rwIOSPtr)
{
  rwFileHeaderV0 hdr;

  if (MAX_REC_LEN < rwIOSPtr->recLen) {
    fprintf(stderr, "%s: recLen too big (%d > %d max)\n",
            skAppName(), rwIOSPtr->recLen, MAX_REC_LEN);
    exit(EXIT_FAILURE);
  }

  /* Make a copy of the header that we can modify */
  memcpy(&hdr, rwIOSPtr->hdr, sizeof(hdr));

  /* Change the byte order flag; swap the data now if we didn't on
   * read */
  hdr.gHdr.isBigEndian = !hdr.gHdr.isBigEndian;
  if (!rwIOSPtr->swapFlag) {
    hdr.fileSTime = BSWAP32(hdr.fileSTime);
  }

  /* Write header */
  fwrite(&hdr, sizeof(hdr), 1, outF);
}


/*
 * swapFilterHeaderV1(rwIOSPtr)
 *
 *   Write the file header (filterHeaderV1) of the rw-filter file
 *   pointed at by "rwIOSPtr" to the "outF" FILE handle, changing the
 *   endian-ness of the header.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
static void swapFilterHeaderV1(rwIOStructPtr rwIOSPtr)
{
  filterHeaderV1 hdr;
  uint32_t filterCount;
  uint32_t i;
  uint16_t byteCount;

  if (MAX_REC_LEN < rwIOSPtr->recLen) {
    fprintf(stderr, "%s: recLen too big (%d > %d max)\n",
            skAppName(), rwIOSPtr->recLen, MAX_REC_LEN);
    exit(EXIT_FAILURE);
  }

  /* Make a copy of the header that we can modify */
  memcpy(&hdr, rwIOSPtr->hdr, sizeof(hdr));

  /* Change the byte order flag; write generic header */
  hdr.gHdr.isBigEndian = !hdr.gHdr.isBigEndian;
  fwrite(&(hdr.gHdr), sizeof(genericHeader), 1, outF);

  /* write the number of filters; swap the data now if we didn't do it
   * on read */
  filterCount = hdr.filterCount;
  if (!rwIOSPtr->swapFlag) {
    hdr.filterCount = BSWAP32(filterCount);
  }
  fwrite(&(hdr.filterCount), sizeof(hdr.filterCount), 1, outF);

  /* write each filter, byte swapping the length if required */
  for (i = 0; i < filterCount; ++i) {
    byteCount = hdr.fiArray[i]->byteCount;
    if (!rwIOSPtr->swapFlag) {
      hdr.fiArray[i]->byteCount = BSWAP16(byteCount);
    }
    fwrite(hdr.fiArray[i], byteCount + sizeof(byteCount), 1, outF);
  }

  return;
}


/*
 * swapRouted(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWROUTED file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapRouted(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapRwFileHeaderV0(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA32(dataPtr +  8);   /* nhIP */
    SWAP_DATA16(dataPtr + 12);   /* sPort */
    SWAP_DATA16(dataPtr + 14);   /* dPort */
    SWAP_DATA32(dataPtr + 16);   /* pef */
    SWAP_DATA32(dataPtr + 20);   /* sbb */
    fwrite(dataPtr, 1, recLen, outF);
  }
}


/*
 * swapNotRouted(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWNOTROUTED file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapNotRouted(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapRwFileHeaderV0(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA16(dataPtr +  8);   /* sPort */
    SWAP_DATA16(dataPtr + 10);   /* dPort */
    SWAP_DATA32(dataPtr + 12);   /* pef */
    SWAP_DATA32(dataPtr + 16);   /* sbb */
    fwrite(dataPtr, 1, recLen, outF);
  }

  return;
}


/*
 * swapSplit(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWSPLIT file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapSplit(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapRwFileHeaderV0(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA16(dataPtr +  8);   /* sPort */
    SWAP_DATA16(dataPtr + 10);   /* dPort */
    SWAP_DATA32(dataPtr + 12);   /* pef */
    SWAP_DATA32(dataPtr + 16);   /* sbb */
    fwrite(dataPtr, 1, recLen, outF);
  }

  return;
}


/*
 * swapAcl(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWACL file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapAcl(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapRwFileHeaderV0(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA32(dataPtr +  8);   /* dPort */
    fwrite(dataPtr, 1, recLen, outF);
  }

  return;
}


/*
 * swapFilter(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWFILTER file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapFilter(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapFilterHeaderV1(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA16(dataPtr +  8);   /* sPort */
    SWAP_DATA16(dataPtr + 10);   /* dPort */
    /* Four single bytes: proto, flags, input, output */
    SWAP_DATA32(dataPtr + 16);   /* nhIP */
    SWAP_DATA32(dataPtr + 20);   /* sTime */
    SWAP_DATA32(dataPtr + 24);   /* pef */
    SWAP_DATA32(dataPtr + 28);   /* sbb */
    fwrite(dataPtr, 1, recLen, outF);
  }

  return;
}


/*
 * swapWeb(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWWWW file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapWeb(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapRwFileHeaderV0(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA32(dataPtr +  8);   /* pef */
    SWAP_DATA32(dataPtr + 12);   /* sbb */
    SWAP_DATA16(dataPtr + 16);   /* port */
    fwrite(dataPtr, 1, recLen, outF);
  }

  return;
}


/*
 * swapGeneric(rwIOSPtr)
 *
 *   Change the endian-ness of the FT_RWGENERIC file pointed at by
 *   "rwIOSPtr" and write the output to the "outF" FILE handle.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void swapGeneric(rwIOStructPtr rwIOSPtr)
{
  const size_t recLen = rwIOSPtr->recLen;

  swapRwFileHeaderV0(rwIOSPtr);

  /* Handle the body */
  while (1) {
    if (fread((void*)data, 1, recLen, rwIOSPtr->FD) != recLen) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "Short read on %s\n", rwIOSPtr->fPath);
      }
      break;
    }

    SWAP_DATA32(dataPtr +  0);   /* sIP */
    SWAP_DATA32(dataPtr +  4);   /* dIP */
    SWAP_DATA16(dataPtr +  8);   /* sPort */
    SWAP_DATA16(dataPtr + 10);   /* dPort */
    /* Four single bytes: proto, flags, input, output */
    SWAP_DATA32(dataPtr + 16);   /* nhIP */
    SWAP_DATA32(dataPtr + 20);   /* sTime */
    SWAP_DATA32(dataPtr + 24);   /* pkts */
    SWAP_DATA32(dataPtr + 28);   /* bytes */
    SWAP_DATA32(dataPtr + 32);   /* elapsed */
    fwrite(dataPtr, 1, recLen, outF);
  }

  return;
}


/* **********  Following are called when no byte swapping occurs ********* */


/*
 * copyRwFileHeaderV0(rwIOSPtr)
 *
 *   Write the file header (rwFileHeaderV0) of the rw-file pointed at
 *   by "rwIOSPtr" to the "outF" FILE handle; make the header an exact
 *   copy of the header read from disk/stdin
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
static void copyRwFileHeaderV0(rwIOStructPtr rwIOSPtr)
{
  rwFileHeaderV0 hdr;

  /* Make a copy of the header that we can modify */
  memcpy(&hdr, rwIOSPtr->hdr, sizeof(hdr));

  /* If we swapped the data in the header on read, then we need to
   * swap it again */
  if (rwIOSPtr->swapFlag) {
    hdr.fileSTime = BSWAP32(hdr.fileSTime);
  }

  /* Write header */
  fwrite(&hdr, sizeof(hdr), 1, outF);
}


/*
 * copyFilterHeaderV1(rwIOSPtr)
 *
 *   Write the file header (filterHeaderV1) of the rw-fitler file
 *   pointed at by "rwIOSPtr" to the "outF" FILE handle; make the
 *   header an exact copy of the header read from disk/stdin
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
static void copyFilterHeaderV1(rwIOStructPtr rwIOSPtr)
{
  filterHeaderV1 hdr;
  uint32_t filterCount;
  uint32_t i;
  uint16_t byteCount;

  /* Make a copy of the header that we can modify */
  memcpy(&hdr, rwIOSPtr->hdr, sizeof(hdr));

  /* Write generic header */
  fwrite(&(hdr.gHdr), sizeof(genericHeader), 1, outF);

  /* write the number of filters; if we swapped on read, swap again */
  filterCount = hdr.filterCount;
  if (rwIOSPtr->swapFlag) {
    hdr.filterCount = BSWAP32(filterCount);
  }
  fwrite(&(hdr.filterCount), sizeof(hdr.filterCount), 1, outF);

  /* write each filter, byte swapping the length if required */
  for (i = 0; i < filterCount; ++i) {
    byteCount = hdr.fiArray[i]->byteCount;
    if (rwIOSPtr->swapFlag) {
      hdr.fiArray[i]->byteCount = BSWAP16(byteCount);
    }
    fwrite(hdr.fiArray[i], byteCount + sizeof(byteCount), 1, outF);
  }

  return;
}


/*
 * fileCopy(rwIOSPtr)
 *
 *   Write the data in the input file pointed at by "rwIOSPtr" to the
 *   "outF" FILE handle; make the output an exact copy of the data
 *   read from disk/stdin.
 *
 *   ARGUMENTS: rwIOSPtr: pointer holding info about input file
 *   RETURNS:   none
 *   SIDE EFFECTS: write to outF
*/
void fileCopy(rwIOStructPtr rwIOSPtr)
{
  int32_t fileType;
  size_t bytesRead;

  fileType = rwGetFileType(rwIOSPtr);

  /* Handle the header */
  switch(fileType) {
  case FT_RWROUTED:
  case FT_RWNOTROUTED:
  case FT_RWSPLIT:
  case FT_RWACL:
  case FT_RWWWW:
  case FT_RWGENERIC:
    copyRwFileHeaderV0(rwIOSPtr);
    break;

  case FT_RWFILTER:
    copyFilterHeaderV1(rwIOSPtr);
    break;

  default:
    fprintf(stderr, "%s: Invalid file type for %s: 0x%X\n",
            skAppName(), rwIOSPtr->fPath, fileType);
    break;
  }

  /* Handle the body */
  while (1) {
    bytesRead = fread((void*)data, 1, MAX_REC_LEN, rwIOSPtr->FD);
    if (!bytesRead) {
      /* EOF or error */
      rwIOSPtr->eofFlag  = 1;
      if (!feof(rwIOSPtr->FD)) {
        /* some error */
        fprintf(stderr, "%s: short read on %s\n", skAppName(), rwIOSPtr->fPath);
      }
      break;
    }
    fwrite(dataPtr, 1, bytesRead, outF);
  }

  return;
}
