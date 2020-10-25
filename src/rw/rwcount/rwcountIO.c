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
** 2004/03/10 22:11:59
** thomasm
*/

/*
 * rwcountIO.c
 *
 * Input/Output routines for countfiles.
*/

#include "silk.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rwcount.h"



/*
 * int _readCountFileHeader(FILE *tgtFile, countFileHeader *tgtHeader)
 *
 * DESCRIPTION:
 * reads a single count file header from tgtFile.
 *
 * PARAMETERS:
 * tgtFile: FILE * to read the data from.
 * tgtHeader: file header to write the data to.
 *
*/
static int _readCountFileHeader(FILE *tgtFile, countFileHeader *tgtHeader) {
  /*
   * read generic header
   */
  if ((fread((void *) tgtHeader, sizeof(countFileHeader), 1,
		       tgtFile)) != 1) {
    return RWCO_ERR_FILE;
  }
  /* Just do a header and version check */

  if (!((tgtHeader->gHdr.type == FT_RWCOUNT) &&
	(tgtHeader->gHdr.version == RWCO_VERSION) &&
	(tgtHeader->gHdr.isBigEndian == IS_BIG_ENDIAN) &&
	(!CHECKMAGIC(&(tgtHeader->gHdr))))) {
    return RWCO_ERR_FILE;
  }
  /*
   * All's well, we terminate and quit
   */
  return 0;
}
/*
 *
 * FILE _openCountFile(char *fName)
 *
 * DESCRIPTION
 *
 * opens up the countFile associated with fName; if no such
 * file exists, it exists.
 *
 * PARAMETERS
 *
 * char *fName: name of the countfile.
 *
 * NOTES:
 *
 * Files can only be opened for reading and writing, and
 * writing is an implicit create.
*/
static FILE* _openCountFile(char *fName, char mode) {
  FILE *result;
  char filemode[3];
  switch(mode) {
  case 'r':
  case 'w':
    filemode[0] = mode;
    break;
  default:
    return (FILE *) NULL;
  }
  filemode[1] = 'b';
  filemode[2] = 0;
  if((result = fopen(fName, filemode)) == NULL) {
    return (FILE *) NULL;
  }
  return result;
}

/*
 * int readCountFile(char *fName, countFile *cData)
 *
 * DESCRIPTION
 * atomic count file read routine. Opens and snarfs the file,
 * terminates.
 *
 * PARAMETERS
 * char *fName: target file's date.
 * countfile cData: countfile pointer.
 *
 * RETURNS
 * 0 on success
 * error code otherwise
 *
 * SIDE EFFECTS
 *
 * malloc on cData.
*/
int readCountFile(char *fName, countFile *cData) {
  FILE *tgtFile;
  cData->fileHeader = (countFileHeader *) malloc(sizeof(countFileHeader));
  if((tgtFile = _openCountFile(fName, 'r')) == (FILE *) NULL) {
    fprintf(stderr, "countIO Error opening %s\n", fName);
    return RWCO_ERR_GENERAL;
  } else {
    if(_readCountFileHeader(tgtFile, cData->fileHeader)) {
      fprintf(stderr, "count IO Error\n");
      return RWCO_ERR_GENERAL;
    }
  }
  /*
   * At this stage, we've successfully read the header, so now
   * we malloc, then read.
   */
  cData->bins = (double *) malloc(sizeof(double) * ROW_BINS *
				  cData->fileHeader->totalBins);
  if(fread((void *) cData->bins, sizeof(double),
	   ROW_BINS * cData->fileHeader->totalBins, tgtFile) !=
     cData->fileHeader->totalBins * ROW_BINS) {
    fprintf(stderr, "Error reading data from %s\n", fName);
    fclose(tgtFile);
    return RWCO_ERR_FILE;
  };
  fclose(tgtFile);
  return 0;
}
/*
 * int _writeCountFileHeader(FILE *tgtFile, countFile *cData)
 *
 * DESCRIPTION
 * static countfile header writing routine; implicitly preps
 * header as well.
 *
 * PARAMETERS:
 * FILE *tgtFile - file to write to.
 * countFile *cData - countFile
 *
 * NOTES:
 * This routine implicilty preps the genericHeader in
 * cData. This shouldn't be an issue, but it should be noted.
*/
int _writeCountFileHeader(FILE *tgtFile, countFileHeader *cHdr) {
  /*
   * prep the generic header
   */
  cHdr->gHdr.type = FT_RWCOUNT;
  cHdr->gHdr.version = RWCO_VERSION;
  cHdr->gHdr.isBigEndian = IS_BIG_ENDIAN;
  PREPHEADER(&(cHdr->gHdr));
  if(fwrite((void *) cHdr, sizeof(countFileHeader), 1,
	    tgtFile) != 1) {
    fprintf(stderr, "Size mismatch while writing header\n");
    return -1;
  }
  return 0;
}

/*
 * int writeCountFile(char *fName, countFile *cData, int overwrite)
 *
 * DESCRIPTION
 *
 * atomic writer routine.  Writes a count file to disk.
 *
 * PARAMETERS
 *
 * char *fName: name of the file
 * countFile *cData: the actual countfile to write
 * int overwrite: if 1, will write over an existing file,
 *                if 0, will produce an error if fName points
 *                to an existing file.
 *
*/

int writeCountFile(char *fName, countFile *cData, int overwrite) {
  FILE *tgtFile;
  if(!overwrite) {
    if((tgtFile = fopen(fName, "rb")) != NULL) {
      fprintf(stderr, "rwcountIO error: file %s exists\n", fName);
      fclose(tgtFile);
      return RWCO_ERR_FILE;
    }
  }else {
    if((tgtFile = fopen(fName, "wb")) == NULL) {
	fprintf(stderr, "rwcountIO error opening file %s for writing:%s",
		fName, strerror(errno));
	return RWCO_ERR_FILE;
    }
  }
  /*
   * We have passed the basic tests.  Now write the header
   */
  if(_writeCountFileHeader(tgtFile, cData->fileHeader)) {
    fprintf(stderr, "Error writing header, exiting\n");
    return RWCO_ERR_FILE;
  };
  /*
   * Whee, we've successfully exited, now we have to write and exit.
   */
  if((fwrite((void *) cData->bins, sizeof(double),
	     ROW_BINS * cData->fileHeader->totalBins, tgtFile)) !=
     (ROW_BINS * cData->fileHeader->totalBins)) {
    fprintf(stderr, "Error writing data, exiting\n");
    return RWCO_ERR_FILE;
  };
  fclose(tgtFile);
  return 0;
}
