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
** 1.11
** 2004/03/10 22:23:52
** thomasm
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"

RCSIDENT("filterio.c,v 1.11 2004/03/10 22:23:52 thomasm Exp");


/*
 * void dumpMarks(uint32_t *fields, uint16_t size) { 
 *
 * SUMMARY:
 * 
 * Prints out a list of marked values.  
 *
 * PARAMETERS:
 *
 * uint32_t fields: a pointer to the field listing. 
 * uint16_t size: the number of elements in the array; note that these
 *              are bits.
 * RETURNS:
 *
 * Nothing.
 *
 * SIDE EFFECTS:
 *
 * Output to stderr.
 * 
 * NOTES:
 *
 * This is a prettier version of the older dumpMarks code (which simply
 * did a line-by line dump).  I needed a more compact expression so that 
 * I could actually check marks without losing my sanity.
 */
void printMarks(FILE *fp, uint32_t *fields, uint32_t size) { 
  uint16_t i, lastMarked;
  uint8_t dashFlag;
  dashFlag = 0;
  lastMarked = 999;
  for(i=0;i<size;i++) { 
    if (fields[i / MARK_COMPRESS] & (1 << (i % MARK_COMPRESS))) { 
      if (lastMarked == 999) {
	fprintf(fp, "%d", i);
      } 
      else {
	if (lastMarked == i-1) {
	  dashFlag =1;
	} else { 
	  if(dashFlag) { 
	    fprintf(fp, "-%d, %d", lastMarked, i);
	  } else {
	    fprintf(fp, ", %d", i);
	  }
	  dashFlag = 0;
	}
      }
      lastMarked = i;
    }
  }
  if(dashFlag) { 
    fprintf(fp,  "-%d", lastMarked);
  }
}

/*
 * void printRange(FILE *fp, valueRange tgtRange)
 *
 * SUMMARY:
 *
 * Prints a valueRange object as a dash-delimited set of (min, max)
 *
 * PARAMETERS:
 *
 * FILE *fp:                             output Stream
 * valueRange tgtRange:                  range of values to print
 *
 * RETURNS:
 *
 * Nothing
 *
 * SIDE EFFECTS:
 *
 * Writes a string output containing the vlaue range.  This range is formatted
 * as min - max unless min == max, the it'll just be min.
 * 
 * NOTES:
 *
 *
 */
void printRange(FILE *fp, valueRange tgtRange) { 
  if(tgtRange.min == tgtRange.max) 
    fprintf(fp, "%llu", tgtRange.min);
  else
    fprintf(fp, "%llu-%llu", tgtRange.min, tgtRange.max);
}


/*
 *  filterPrintRuleSet:
 *  	write one filter rule set in a text format to the given fp
 *  Input:
 *  	file stream pointer, filterRulesPtr
 *  Return:
 *  	void.
 */
void filterPrintRuleSet(FILE *fp, filterRulesPtr fltrRPtr) {
  register int i;
  register int j;
  register int k;
  
  fprintf(fp, "Source IP Addresses:\n\tRule Count %u\tNegate %s\n", 
	  fltrRPtr->totalAddresses[SRC],
	  fltrRPtr->negateIP[SRC] ? "Yes": "No");
  for(k = 0; k < 2; k++) { 
    fprintf(fp, "%s IP Addresses:\n", k == SRC ? "Source" : "Destination"); 
    for (i = 0; i < fltrRPtr->totalAddresses[k]; i++) {
      for (j = 0; j < 4; j++) {		    /* octets */
	printMarks(fp, fltrRPtr->IP[k][i].octets[j], 
		   MAX_ADDRESSES * MARK_COMPRESS);
	fprintf(fp, j == 3 ? "\n" : ".");
      }
    }
  }
  fprintf(fp, "STime: ");
  printRange(fp, fltrRPtr->sTime);
  fprintf(fp, "\t ETime: ");
  printRange(fp, fltrRPtr->eTime);
  fprintf(fp, "\n Protocols:\t");
  printMarks(fp, fltrRPtr->protocols, MAX_PROTOCOLS * MARK_COMPRESS);
  fprintf(fp, "\n Source Ports:\t");
  printMarks(fp, fltrRPtr->srcPorts, MAX_PORTS * MARK_COMPRESS);
  fprintf(fp, "\n Dest Ports:\t");
  printMarks(fp, fltrRPtr->dstPorts, MAX_PORTS * MARK_COMPRESS);
  fprintf(fp, "\n Bytes:\t");
  printRange(fp, fltrRPtr->bytes);
  fprintf(fp, "\n Packets:\t");
  printRange(fp, fltrRPtr->packets);
  fprintf(fp, "\n Flows:\t");
  printRange(fp, fltrRPtr->flows);
  fprintf(fp, "\n Duration:\t");
  printRange(fp, fltrRPtr->duration);
  fprintf(fp, "\n");

  return;
}



