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

**  1.12
**  2004/03/18 16:18:45
** thomasm
*/

/*
 * rwtotal.c
 *
 * This is an analysis package which totals up various values in a
 * packfile, breaking them up by some combination of fields.
*/


/* includes */
#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "rwpack.h"
#include "utils.h"
#include "iochecks.h"
#include "rwtotal.h"

RCSIDENT("rwtotal.c,v 1.12 2004/03/18 16:18:45 thomasm Exp");


/* imported variables */
int minFile;
int tallySrc;
/* imported functions */
extern char * dumpPort(int sportFlag, uint16_t port, uint8_t proto);
void appTeardown(void);
void appSetup(int argc, char **argv);	/* never returns */
void setCounters(void);
void dumpCounts(void);
void process(rwRec *);
/* exported functions */

/* exported variables */
rwRec rwrec;
rwIOStructPtr rwIOSPtr;
char delimiter='|';
int countMode, skipFlag, destFlag, noTitlesFlag;
int printFileNamesFlag = 0;
int(*totalFunction) (rwRec *);

uint64_t *countRecs;
uint32_t totalRecs;
extern iochecksInfoStructPtr ioISP;
/* local functions */
static void countFile(char *inFName);

int main(int argc, char **argv) {
  int counter;

  appSetup(argc, argv);			/* never returns */

  /*
   * Alright, note that I'm actually condascending to use a malloc
   * here.  Depending on what countRecs is doing (protocol is 256
   * slots, /24's is 16 million); the number of bins can really
   * fluctuate.  The array is unidimensional.
   */
  countRecs = (uint64_t *) malloc(sizeof(uint64_t) * 3 * totalRecs);

  memset(countRecs, 0 , sizeof(uint64_t) * totalRecs * 3);
  /* set up checking and processing handlers */
  /* get files to process from the globber or the command line */
  /* done */
  for (counter = ioISP->firstFile;
       counter < ioISP->fileCount;
       counter++) {
    countFile(ioISP->fnArray[counter]);
  }
  dumpCounts();
  appTeardown();
  exit(0);
}

void countFile(char *inFName) {
  uint32_t pointer = 0;
  if ( (rwIOSPtr = rwOpenFile(inFName, ioISP->inputCopyFD))
       == (rwIOStructPtr)NULL) {
    fprintf(stderr, "%s: unable to open %s\n", skAppName(), inFName);
    exit(1);
  }
  if (printFileNamesFlag) {
    fprintf(stderr, "%s\n", inFName);
  }

  /*
   * Alright, children, what follows is an object lesson in
   * throwing away readability in favor of performance.
   * I have removed the former largely by unrolling a bunch
   * of function calls and the like into the hideous agglomeration
   * below, I will describe as it continues
   */
  if(destFlag) {
    /*
     * Since we're always working with either SIP or DIP, the source/dest
     * switch is done at the start.
     */
    while (rwRead(rwIOSPtr, &rwrec)) {
      switch(countMode) {
      case RWTO_COPT_ICMP:
      case RWTO_COPT_PORT:
	pointer = rwrec.dPort;
	break;
      case RWTO_COPT_ADD8:
	pointer = rwrec.dIP.O.o1;
	break;
	/* At some point, I have to think about the sanity of ipUnion's
	 * endianness*/
      case RWTO_COPT_ADD16:
	pointer = (rwrec.dIP.O.o1 << 8) + rwrec.dIP.O.o2;
	break;
      case RWTO_COPT_LADD8:
	pointer = rwrec.dIP.O.o4;
	break;
      case RWTO_COPT_LADD16:
	pointer = (rwrec.dIP.O.o3 << 8) + rwrec.dIP.O.o4;
	break;
      case RWTO_COPT_PROTO:
	pointer = rwrec.proto;
	break;
      case RWTO_COPT_ADD24:
	pointer = (((rwrec.dIP.O.o1 << 16) & 0xFF0000)  |
		   ((rwrec.dIP.O.o2 << 8) & 0xFF00) |
		   (rwrec.dIP.O.o3));
	break;
      case RWTO_COPT_DURATION:
	pointer = rwrec.elapsed;
	break;
      case RWTO_COPT_BYTES:
	pointer = rwrec.bytes < (totalRecs - 1) ? rwrec.bytes : totalRecs - 1;
	break;
      case RWTO_COPT_PACKETS:
	pointer = rwrec.pkts < (totalRecs - 1)? rwrec.pkts : totalRecs - 1;
	break;
      }
      pointer *= 3;
      countRecs[pointer]++;
      countRecs[pointer +  1] += rwrec.bytes;
      countRecs[pointer +  2] += rwrec.pkts;
    }
  } else {
    while(rwRead(rwIOSPtr, &rwrec)) {
      switch(countMode) {
      case RWTO_COPT_PORT:
	pointer = rwrec.sPort;
	break;
      case RWTO_COPT_ADD8:
	pointer = rwrec.sIP.O.o1;
	break;
	/* At some point, I have to think about the sanity of ipUnion's
	 * endianness*/
      case RWTO_COPT_ADD16:
	pointer = (rwrec.sIP.O.o1 << 8) + rwrec.sIP.O.o2;
	break;
      case RWTO_COPT_ADD24:
	pointer = (((rwrec.sIP.O.o1 << 16) & 0xFF0000)  |
		   ((rwrec.sIP.O.o2 << 8) & 0xFF00) |
		   (rwrec.sIP.O.o3));
	break;
      case RWTO_COPT_LADD8:
	pointer = rwrec.sIP.O.o4;
	break;
      case RWTO_COPT_LADD16:
	pointer = (rwrec.sIP.O.o3 << 8) + rwrec.sIP.O.o4;
	break;
      case RWTO_COPT_PROTO:
	pointer = rwrec.proto;
	break;
      case RWTO_COPT_ICMP:
	pointer = rwrec.dPort;
	break;
      case RWTO_COPT_DURATION:
	pointer = rwrec.elapsed;
	break;
      case RWTO_COPT_BYTES:
	pointer = rwrec.bytes < (totalRecs - 1) ? rwrec.bytes : totalRecs - 1;
	break;
      case RWTO_COPT_PACKETS:
	pointer = rwrec.pkts < (totalRecs - 1)? rwrec.pkts : totalRecs - 1;
	break;
      }
      pointer *= 3;
      countRecs[pointer]++;
      countRecs[pointer + 1] += rwrec.bytes;
      countRecs[pointer + 2] += rwrec.pkts;
    }
  }
  rwCloseFile(rwIOSPtr);
  rwIOSPtr = (rwIOStructPtr) NULL;
  return;
}


void dumpCounts(void) {
  uint32_t i;
  uint32_t max, t;
  /*
   * Print out title
   */
  if(!noTitlesFlag) {
    fprintf(stdout, "%11s%c%15s%c%15s%c%15s\n",
	    "Key", delimiter,
	    "Records", delimiter,
	    "Bytes", delimiter,
	    "Packets");
  }
  switch(countMode) {
  case RWTO_COPT_PORT:
  case RWTO_COPT_ADD16:
  case RWTO_COPT_LADD16:
  case RWTO_COPT_ICMP:
    max = 65536;
    break;
  case RWTO_COPT_ADD24:
  case RWTO_COPT_BYTES:
  case RWTO_COPT_PACKETS:
    max = 65536 * 256;
    break;
  case RWTO_COPT_DURATION:
    max = 4096;
    break;
  default:
    max = 256;
  }
  max = max * 3;
  switch(countMode) {
  case RWTO_COPT_ADD24:
    for(i = 0; i < max ; i += 3) {
      if(skipFlag && (!(countRecs[i] +
			countRecs[i + 1] +
			countRecs[i + 2]))) continue;
      t = i / 3;
      fprintf(stdout, "%3u.%3u.%3u%c%15llu%c%15llu%c%15llu\n",
	      t >> 16, ((t & 0xFF00) >> 8), t & 0xFF, delimiter,
	      countRecs[i],delimiter,
	      countRecs[i + 1],delimiter,
	      countRecs[i + 2]);
    };
    break;
  case RWTO_COPT_ICMP:
    for(i = 0; i < max ; i += 3) {
      if(skipFlag && (!(countRecs[i] +
			countRecs[i + 1] +
			countRecs[i + 2]))) continue;
      t = i / 3;
      fprintf(stdout, "    %3u %3u%c%15llu%c%15llu%c%15llu\n",
	      (t & 0xFF00) >> 8, (t & 0x00FF), delimiter,
	      countRecs[i],delimiter,
	      countRecs[i + 1],delimiter,
	      countRecs[i + 2]);
    };
    break;
  case RWTO_COPT_ADD16:
  case RWTO_COPT_LADD16:
    for(i = 0; i < max ; i += 3) {
      if(skipFlag && (!(countRecs[i] +
			countRecs[i + 1] +
			countRecs[i + 2]))) continue;
      fprintf(stdout, "    %3u.%3u%c%15llu%c%15llu%c%15llu\n", ((i/3) >> 8),
	      (i/3) - (((i/3) >> 8) << 8), delimiter,
	      countRecs[i],delimiter,
	      countRecs[i + 1],delimiter,
	      countRecs[i + 2]);
    };
    break;
  default:
    for(i = 0; i < max ; i += 3 ) {
      if(skipFlag && (!(countRecs[i] +
			countRecs[i + 1] +
			countRecs[i + 2]))) continue;
      fprintf(stdout, "%11u%c%15llu%c%15llu%c%15llu\n", i / 3, delimiter,
	      countRecs[i],delimiter,
	      countRecs[i + 1],delimiter,
	      countRecs[i + 2]);
    }
  }
}

