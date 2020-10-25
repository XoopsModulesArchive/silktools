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
 *
 * count application testing routines.
*/
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>

#include "rwcount.h"

int main() {
  /*
   * first round of main testing - let's see if the basic
   * count io functions work.
   */
  countFile newCountFile;
  fprintf(stderr, "Creating generic count file\n");
  newCountFile.bins = (double *) malloc(sizeof(double) * ROW_BINS * 2);
  newCountFile.fileHeader = (countFileHeader *)
    malloc(sizeof(countFileHeader));
  newCountFile.fileHeader->binSize = 30;
  newCountFile.fileHeader->totalBins = 1;
  newCountFile.fileHeader->initOffset = 1045533300;
  newCountFile.fileHeader->gHdr.type = FT_RWCOUNT;
  newCountFile.fileHeader->gHdr.version = RWCO_VERSION;
  newCountFile.fileHeader->gHdr.isBigEndian = IS_BIG_ENDIAN;
  fprintf(stderr, "Writing values to file (0.0, 1.0, 2.0)\n");
  newCountFile.bins[0] = 0.0;
  newCountFile.bins[1] = 1.0;
  newCountFile.bins[2] = 2.0;
  writeCountFile("tCount", &newCountFile,1);
  /*
   * Resetting bins
   */
  fprintf(stderr, "Successfully wrote data...\nNow resetting bins\n");
  fprintf(stderr, "Bins were %3.2f\t%3.2f\t%3.2f\n", newCountFile.bins[0],
	  newCountFile.bins[1], newCountFile.bins[2]);
  newCountFile.bins[0] = newCountFile.bins[1] = newCountFile.bins[2] = 0.0;
  fprintf(stderr, "Now values are %3.2f\t%3.2f\t%3.2f\n", newCountFile.bins[0],
	  newCountFile.bins[1], newCountFile.bins[2]);
  fprintf(stderr, "Reading\n");
  readCountFile("tCount", &newCountFile);
  fprintf(stderr, "Now values are %3.2f\t%3.2f\t%3.2f\n", newCountFile.bins[0],
	  newCountFile.bins[1], newCountFile.bins[2]);

  return 0;
}
