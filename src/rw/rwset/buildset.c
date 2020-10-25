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
** 1.9
** 2004/03/10 23:07:18
** thomasm
*/

/*
 *
 * Quick application to make sure I can open and read a set.
*/
#include "silk.h"

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "iptree.h"


#define checkMark(f,i) ( (f[i/32] & (1 << (i % 32))) ? 1 : 0 )
RCSIDENT("buildset.c,v 1.9 2004/03/10 23:07:18 thomasm Exp");


/*
 * usage is the standard usage struct.
*/
char usage[80] = "buildset <input file> <output file>";
/*
 * globals - m is the tree you're creating by calling the application,
*/
macroTree m;
int currentLine;
/*
 * Given a collection of numbers written in comma, dash format
 * (i.e., 3,4,5-22), this generates a marker table
 * which marks as 1 any value to be checked.
*/
int parseMarks(char *b, uint32_t *fields, uint32_t size) {
  uint32_t n, m, i;
  char *sp, *ep;
  int fieldCount;
  sp = b;
  fieldCount = 0;
  memset(fields, 0, size/8);

  /*ensure that the only characters are 0-9 , (comma) and - (hyphen) */
  while(*sp) {
    n = strtoul(sp, &ep, 10);
    if (sp == ep || n > size) {
      fprintf(stderr, "Error in field allocation; greater than %d", size);
      memset(fields, 0, size/8); /* cleanup */
      return -1;
    }
    fields[n/32] |= (1 << (n %32));
    fieldCount++;
    switch (*ep) {
    case ',':
      sp = ep + 1;
      break;

    case '-':
      sp = ep + 1;
      m = strtoul(sp, &ep, 10);
      if (sp == ep || m > size) {
	fprintf(stderr, "Error in field allocation; greater than %d", size);
        memset(fields, 0, size/8); /* cleanup */
	return -1;
      }
      /* AJY 12.12.01 added check for semantic correctness (e.g. x >=y) */
      if (n > m) {
	fprintf(stderr, "Input is incorrectly constructed: the beginning value %d is greater than the ending value %d\n", n, m);
        memset(fields, 0, size/8); /* cleanup */
	return -1;
      }
      for (i = n; i <= m; i++) {
	fields[i /32] |= (1 << (i %32));
	fieldCount++;
      }
      if (*ep == '\0') {
	return 0;
      }
      if (*ep == ',') {
	sp = ep + 1;
      } else {
	fprintf(stderr, "Error in field allocation; greater than %d\n", size);
        memset(fields, 0, size/8); /* cleanup */
	return 1;
      }
      break;

    case '\0':
      return 0;

    default:
      /* AJY modified 12.12.01 to dump ep */
      fprintf(stderr, "Unparsable character:%c\n", *ep);
      memset(fields, 0, size/8); /* cleanup */
      return 1;
    }
  }
  return 0;
}

/*
 * uint32_t parseIP(char *ipString)
 *
 * SUMMARY:
 *
 * parseIP uses the ip address specified in ipString to determine what
 * spots to mark in a macrotree.
 *
 * PARAMETERS:
 *
 * ipString: a string representation of the IP address in ,- list format.
 *
 * RETURNS:
 *
 * Error code - 0 on success, nonzero on failure.
 *
 * Failures occur because of:
 * a) faulty parsing
 * b) too many IP addresses added.
 *
 * SIDE EFFECTS:
 *
 * on success, the macrotree will get an address set added to it.
 *
 * NOTES:
 *
 *
*/
uint32_t parseIP(char *ipString) {
  int ipLen,maskSize,i, lastPos;
  size_t j;
  char tmpString[256];
  char maxString[6] = "0-255\0";
  int ipLatch[4];
  macroNode *tgtBlock;
  uint32_t numericAddress;
  uint32_t octets[4][8];
  ipLen = strlen(ipString);
  maskSize = 0;

  /*
   * There's some c-weenieism going on here so I'll explain.  We can't
   * use gettoken due to the side effects, so this
   * following string isolates all the strings separated by .'s (I can't
   * use strtoul because the values are unlikely to be numbers).  The
   * loop uses j as its current holder and lastPost as a pointer to where
   * j starts out each time.  The while loop, in turn, terminates on a
   * peiord each time.  The increment instructions jump past the period,
   * and set lastPos to the position of j after the period.
   */
  for(i = 0, j = 0, lastPos = 0; i < 4; i++, lastPos = ++j) {
    /*
     * This is the only place where using strtoul won't
     * work since the values for parseIP are actually mark lists
     * (comma, dash lists).  To fix that I'm using a simple iterator).
     */
    while( (ipString[j] != '.') && (j < strlen(ipString))) {
      j++;
    }
    if((j == strlen(ipString)) && i < 3) {
      return 1;
    } else {
      memset(tmpString, '\000', 256);
      strncpy(tmpString, ipString + lastPos, j - lastPos);
    }
    /*
     * Handles the wildcard specifications discussed earlier in the
     * specification.  Compelte
     *
     */
    if(!tmpString || !strcmp(tmpString,"x") || !strcmp(tmpString, "X")) {
      strncpy(tmpString, maxString, 6);
    }
    if(parseMarks(tmpString, octets[i], 256)) {
      return 1;
    }
  }
  /*
   * Now we actually mark the ip addresses in question
   */
  for(ipLatch[0] = 0; ipLatch[0] < 256 ; ipLatch[0]++) {
    if(checkMark(octets[0], ipLatch[0])) {
      for(ipLatch[1] = 0 ; ipLatch[1] < 256; ipLatch[1]++) {
	if(checkMark(octets[1], ipLatch[1])) {
	  for(ipLatch[2] = 0; ipLatch[2] < 256; ipLatch[2]++)
	    if(checkMark(octets[2], ipLatch[2])) {
	      for(ipLatch[3] = 0; ipLatch[3] < 256; ipLatch[3]++) {
		if(checkMark(octets[3], ipLatch[3])) {
		  numericAddress = (ipLatch[0] << 24) + (ipLatch[1] << 16) +
		    (ipLatch[2] << 8) + ipLatch[3];
		  tgtBlock = m.macroNodes[(numericAddress >> 16)];
		  if(!tgtBlock) {
		    tgtBlock = (macroNode *) malloc(sizeof(macroNode));
		    memset(tgtBlock->addressBlock, '\0', 8192);
		    tgtBlock->parentAddr = (numericAddress >> 16);
		    m.macroNodes[(numericAddress >> 16)] = tgtBlock;
		  };
		  macroTreeAddAddress(numericAddress, (&m));
		}
	      }
	    }
	}
      }
    }
  };
  return 0;
}



int main(int argc, char **argv) {
  FILE *asciiFile;
  FILE *treeFile;
  int fileSize;
  char dataBuffer[255];
  struct stat tmpStat;
  if(argc < 3) {
    fprintf(stderr, "%s\n", usage);
    return(0);
  } else {
    if(!strcmp(argv[1], "stdin")) {
      asciiFile = stdin;
    } else {
      if ((asciiFile = fopen(argv[1],"r")) == NULL) {
	fprintf(stderr, "Error opening file %s, exiting\n", argv[1]);
	exit(-1);
      }
    }
    memset(dataBuffer, '\0', 255);
    initMacroTree(&m);
    currentLine = 0;
    while(fgets(dataBuffer, 255 ,asciiFile) != NULL) {
      currentLine++;
      fileSize = strlen(dataBuffer);
      if(dataBuffer[fileSize -1 ] == '\n') {
	dataBuffer[fileSize - 1] = '\0';
      };
      /*
       * Now I'm going to do a couple of quick checks on the text.  As a rule,
       * blank lines are skipped, lines containing # are skipped.
       */
      if(dataBuffer[0] == '#') continue;
      if(parseIP(dataBuffer)) {
	fprintf(stderr, "Error in line %d: %s\n", currentLine, dataBuffer);
	exit(-1);
      };
      memset(dataBuffer, '\0', 255);
    }
    fclose(asciiFile);
  }
  if(stat(argv[2], &tmpStat) == 0) {
    fprintf(stderr, "Error, file %s exists, please delete or move the file before continuing\n", argv[2]);
    exit(-1);
  };
  if (NULL == (treeFile = fopen(argv[2], "wb"))) {
    fprintf(stderr, "Error opening output file %s, exiting\n", argv[2]);
    exit(-1);
  }
  writeMacroTree(treeFile, &m);
  fclose(treeFile);
  exit(0);
  return(0);
}
