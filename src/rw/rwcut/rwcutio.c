/*
** Copyright (C) 2001-2004 by Carnegie Mellon University.
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
** 1.23
** 2004/02/18 15:07:23
** thomasm
*/


/*
** rwcutio.c
**      io routines in support of rwcut.
** Suresh Konda
**
*/

#include "silk.h"

#include <stdio.h>
#include <string.h>

#include "cut.h"
#include "filter.h"

RCSIDENT("rwcutio.c,v 1.23 2004/02/18 15:07:23 thomasm Exp");


/* For ICMP traffic the type and code are stored in a port field */
typedef union {
  uint16_t port;
  uint8_t  type_and_code[2];
} port_or_code_t;

/* local variables */
static char pText[64];               /* contains text to be printed */

/* local functions */
static char *dumpFlags(uint8_t flags);


/*
**
**  dumpPort
**
**  If the proto is icmp and the sportFlag is set, then dump in two
**  parts: type and code
**
**  Input:
**      sportFlag == 1 if sPort 0 else;
**      uint16_t  port number
**      uint8_t protocol
**  Output:
**      string rep of value in a static char buffer
**  Side Effect:
**      May fill pText with required string if this is icmp type/code
*/
char * dumpPort(int sportFlag, uint16_t port, uint8_t proto) {
  port_or_code_t sPort;

  sprintf(pText, "%u", port);
  return pText;

  if (sportFlag && proto == 1) {
    /* icmp */
    sPort.port =  port;
#if     IS_LITTLE_ENDIAN
    snprintf(pText, sizeof(pText), "%u %u", sPort.type_and_code[1],
             sPort.type_and_code[0]);
#else
    snprintf(pText, sizeof(pText), "%u %u", sPort.type_and_code[0],
               sPort.type_and_code[1]);
#endif
    return pText;
  }
}


/*
** writeColTitles:
**      write out the required column titles.
** Input: None
** Output: printed col titles
** NOTE: this must be idempotent.
*/
void writeColTitles(void) {
  static uint8_t wroteColTitles = 0;
  const char *cp;
  unsigned int i;
  cutDynlib_t *cutDL;

  if (wroteColTitles){
    return;
  }
  wroteColTitles = 1;
  if (!flags.printTitles) {
    return;
  }

  for (i = 0; i < maxFieldsSelected; i++) {
    /* Is fieldPositions[i] related to dynamic library ? */
    cutDL = cutFieldNum2Dynlib(fieldPositions[i]);
    if (NULL == cutDL) {
      cp = fieldTitles[fieldPositions[i]];
    } else {
      cutDL->fxn(fieldPositions[i] - cutDL->offset, pText,sizeof(pText), NULL);
      if (!flags.delimited) {
        /* trim title to min width required by the plugin */
        pText[fWidths[fieldPositions[i]]] = '\0';
      }
      cp = pText;
    }

    if (flags.delimited) {
      fprintf(stdout, "%s%c", cp, delimiter);
    } else {
      fprintf(stdout, "%*s%c", fWidths[fieldPositions[i]], cp, delimiter);
    }
  }

  fprintf(stdout, "\n");
  return;
}


/*
** dumprec:
**      dump the information the global rwrec according the user
**      options.
**      Basically, each desired field is dumped into the global pText
**      which is pointed to by cp.  All called functions must dump
**      their information into pRec.  Then pRec is printed out based
** Input: None
** Output: None
** Side Effects: printed record to stdout.
**      pText keeps getting filled etc.
*/
void dumprec(void) {
  register unsigned int i;
  char *cp;
  cutDynlib_t *cutDL;
#ifdef OSF1
  port_or_code_t pc;
#endif

  for (i = 0; i < maxFieldsSelected; i++) {
    cp = pText;
    switch (fieldPositions[i]) {
    case 0:
      /* sIP */
      if (flags.dottedip) {
        cp = num2dot(rwrec.sIP.ipnum);
      } else {
        sprintf(cp, "%u", rwrec.sIP.ipnum);
      }
      break;

    case 1:
      /* dIP */
      if (flags.dottedip) {
        cp = num2dot(rwrec.dIP.ipnum);
      } else {
        sprintf(cp, "%u", rwrec.dIP.ipnum);
      }
      break;

    case 2:
      /* sport.  If icmpTandC is set and this is icmp, write type here */
      if (flags.icmpTandC && rwrec.proto == 1) {
#if     IS_LITTLE_ENDIAN
#ifdef OSF1
        pc.port = rwrec.sPort;
        sprintf(cp, "%u", pc.type_and_code[1]);
#else
        sprintf(cp, "%u", ((port_or_code_t)rwrec.dPort).type_and_code[1]);
#endif
#else
        sprintf(cp, "%u", ((port_or_code_t)rwrec.dPort).type_and_code[0]);
#endif
      } else {
        sprintf(cp, "%u", rwrec.sPort);
      }
      break;

    case 3:
      /* dport.  If icmpTandC is set and this is icmp, write code here */
      if (flags.icmpTandC && rwrec.proto == 1) {
#if     IS_LITTLE_ENDIAN
#ifdef OSF1
        pc.port = rwrec.dPort;
        sprintf(cp, "%u", pc.type_and_code[1]);
#else
        sprintf(cp, "%u", ((port_or_code_t)rwrec.dPort).type_and_code[0]);
#endif
#else
        sprintf(cp, "%u", ((port_or_code_t)rwrec.dPort).type_and_code[1]);
#endif
      } else {
        sprintf(cp, "%u", rwrec.dPort);
      }
      break;

    case 4:
      /* proto */
      sprintf(cp, "%u", rwrec.proto);
      break;

    case 5:
      /* packets */
      sprintf(cp, "%u", rwrec.pkts);
      break;

    case 6:
      /* bytes */
      sprintf(cp, "%u", rwrec.bytes);
      break;

    case 7:
      /* flows */
      cp = dumpFlags(rwrec.flags);
      break;

    case 8:
      /* sTime */
      if (flags.timestamp) {
        cp = timestamp(rwrec.sTime);
      } else {
        sprintf(cp, "%u", rwrec.sTime);
      }
      break;

    case 9:
      /* elapsed/duration */
      sprintf(cp, "%u", rwrec.elapsed);
      break;

    case 10:
      /* end gmt */
      if (flags.timestamp) {
        cp = timestamp(rwrec.sTime + rwrec.elapsed);
      } else {
        sprintf(cp, "%u", rwrec.sTime + rwrec.elapsed);
      }
      break;

    case 11:
      /* sensor ID */
      if ( !flags.integerSensor ) {
        cp = (char*)sensorIdToName(rwrec.sID);
      } else if (SENSOR_INVALID_SID == rwrec.sID) {
        cp = "-1";
      } else {
        sprintf(cp, "%u", rwrec.sID);
      }
      break;

    case 12:
      /* input */
      sprintf(cp, "%u", rwrec.input);
      break;

    case 13:
      /* output */
      sprintf(cp, "%u", rwrec.output);
      break;

    case 14:
      /* next hop */
      if (flags.dottedip) {
        cp = num2dot(rwrec.nhIP.ipnum);
      } else {
        sprintf(cp, "%u", rwrec.nhIP.ipnum);
      }
      break;

    default:
      cutDL = cutFieldNum2Dynlib(fieldPositions[i]);
      cutDL->fxn(fieldPositions[i] - cutDL->offset, cp, sizeof(pText), &rwrec);
      break;
    } /* switch */

    if (flags.delimited) {
      fprintf(stdout, "%s%c", cp, delimiter);
    } else {
      fprintf(stdout, "%*s%c", fWidths[fieldPositions[i]], cp, delimiter);
    }
  } /* for */

  fprintf(stdout, "\n");
  return;
}


/*
** static char* dumpFlags(uint8_t flags)
**
** SUMMARY:
**
** Prints the targeted flags as a string.
**
** PARAMETERS:
**
** uint8_t flags: target flags.
**
** RETURNS:
**
** char* pointer to the targeted string.
**
** SIDE EFFECTS:
**
** fText gets rewritten with each call.
**
** NOTES:
**
**
*/
static char *dumpFlags(uint8_t flags) {
  static char fText[10] = "         "; /* ensure \0 is set */

  memset(fText,' ', sizeof(fText)-1); /* -1 for \0 */
  if (flags & URG_FLAG) {
    fText[5] = 'U';
  }
  if (flags & ACK_FLAG) {
    fText[4] = 'A';
  }
  if (flags & PSH_FLAG) {
    fText[3] = 'P';
  }
  if (flags & RST_FLAG) {
    fText[2] = 'R';
  }
  if (flags & SYN_FLAG) {
    fText[1] = 'S';
  }
  if (flags & FIN_FLAG) {
    fText[0] = 'F';
  }
  return fText;
}
