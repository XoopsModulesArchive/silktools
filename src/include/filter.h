#ifndef _FILTER_H
#define _FILTER_H
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
** 1.14
** 2004/01/15 17:57:29
** thomasm
*/

/*@unused@*/ static char rcsID_FILTER_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "filter.h,v 1.14 2004/01/15 17:57:29 thomasm Exp";

#include "silk.h"


#if	IS_LITTLE_ENDIAN
#define SHIFT_OPERATOR <<
#else
#define SHIFT_OPERATOR >>
#endif

/* Whether the filter library knows how to filter on sensor-id.
 * Currently we rely on fglob to filter by sensor */
#define	ADD_SENSOR_RULE	0


#define SRC 0
#define DST 1
#define PAIRS 2
#define MARK_COMPRESS 32
#define MAX_PORTS 2048
#define MAX_PROTOCOLS 8		/* 256/32 */
#define NEGATE_FLAG 128
#define MAX_ADDRESSES 8		/* 256/32 */
#define MAX_IP_RULES 10
#define MAX_INTERFACES 32 /*1024/32*/
#define MAX_SENSORS 8 /*256/32*/

/*
 * flag definitions.
*/
#define URG_FLAG 1 << 5		/* 32  */
#define ACK_FLAG 1 << 4		/* 16  */
#define PSH_FLAG 1 << 3		/* 8 */
#define RST_FLAG 1 << 2		/* 4 */
#define SYN_FLAG 1 << 1		/* 2 */
#define FIN_FLAG 1		/* 1 */

/* 0	"      " */
/* 1	"     F" */
/* 2	"    S " */
/* 3	"    SF" */
/* 4	"   R  " */
/* 5	"   R F" */
/* 6	"   RS " */
/* 7	"   RSF" */
/* 8	"  P   " */
/* 9	"  P  F" */
/* 10	"  P S " */
/* 11	"  P SF" */
/* 12	"  PR  " */
/* 13	"  PR F" */
/* 14	"  PRS " */
/* 15	"  PRSF" */
/* 16	" A    " */
/* 17	" A   F" */
/* 18	" A  S " */
/* 19	" A  SF" */
/* 20	" A R  " */
/* 21	" A R F" */
/* 22	" A RS " */
/* 23	" A RSF" */
/* 24	" AP   " */
/* 25	" AP  F" */
/* 26	" AP S " */
/* 27	" AP SF" */
/* 28	" APR  " */
/* 29	" APR F" */
/* 30	" APRS " */
/* 31	" APRSF" */
/* 32	"U     " */
/* 33	"U    F" */
/* 34	"U   S " */
/* 35	"U   SF" */
/* 36	"U  R  " */
/* 37	"U  R F" */
/* 38	"U  RS " */
/* 39	"U  RSF" */
/* 40	"U P   " */
/* 41	"U P  F" */
/* 42	"U P S " */
/* 43	"U P SF" */
/* 44	"U PR  " */
/* 45	"U PR F" */
/* 46	"U PRS " */
/* 47	"U PRSF" */
/* 48	"UA    " */
/* 49	"UA   F" */
/* 50	"UA  S " */
/* 51	"UA  SF" */
/* 52	"UA R  " */
/* 53	"UA R F" */
/* 54	"UA RS " */
/* 55	"UA RSF" */
/* 56	"UAP   " */
/* 57	"UAP  F" */
/* 58	"UAP S " */
/* 59	"UAP SF" */
/* 60	"UAPR  " */
/* 61	"UAPR F" */
/* 62	"UAPRS " */
/* 63	"UAPRSF" */

/*
 * deletion by mcollins on 10/11/01
 * there used to be a collectionw of constants here:
 * NUM_RULE_CLASSES and a collection of flags for each
 * rule (SRCIP, &c).  These were used to determine which
 * negation flags were set, but our new version uses
 * negation flags in exactly one case, therefore they are
 * now moot.
*/

#define NUM_RULE_CLASSES 36

#define MINUS_INFINITY 0
#define PLUS_INFINITY  0xFFFFFFFF

#define FILTER_USE_MIN 0
#define FILTER_USE_MAX 1
#define FILTER_USE_MID 2

/* in this context, precision is the mutliplier that we're using
 * to make fixed-point numbers 'work'.  10,000d translates into 4
 * digits.
*/

#define PRECISION      10000

/* anchor the starting time at 0 and use HELP as the sentinal */
typedef enum {
  STIME = 0, ENDTIME, DURATION, SPORT, DPORT, PROTOCOL, BYTES, PKTS,
  BYTES_PER_PKT, SADDRESS, DADDRESS, NOT_SADDRESS, NOT_DADDRESS, TCP_FLAGS,
  INPUT_INTERFACES, OUTPUT_INTERFACES, NEXT_HOP_ID, APORT, SENSORS,
  OPT_SYN, OPT_ACK, OPT_FIN, OPT_PSH, OPT_URG, OPT_RST, HELP
} ruleType;

typedef struct {
  uint32_t octets[4][MAX_ADDRESSES];
} addressRange;

typedef struct {
  uint64_t max;
  uint64_t min;
} valueRange;

typedef struct {
  /*
   * IP address count and values
   */
  uint8_t totalAddresses[PAIRS];
  uint8_t negateIP[PAIRS];
  addressRange IP[PAIRS][MAX_IP_RULES];
  uint32_t srcPorts[MAX_PORTS];
  uint32_t dstPorts[MAX_PORTS];
  uint32_t anyPorts[MAX_PORTS];
  uint32_t protocols[MAX_PROTOCOLS];
  valueRange sTime, eTime, bytes, packets, flows, duration;
  valueRange packetsPerFlow, bytesPerPacket, bytesPerFlow, bytesPerSecond;
  uint8_t tcpFlags;
  uint8_t flagCare, flagMark;
  uint32_t sensors[MAX_SENSORS];
  uint32_t inputInterfaces[MAX_INTERFACES]; /* these are both value ranges and I'm using the array */
  uint32_t outputInterfaces[MAX_INTERFACES];   /* as a cheat to use the value list operations */
  addressRange nextHop;
  /*
  **  entry in ruleSet[i] ==  ruleNumber if desired.  maxRuleseUsd is the
  **  number of rules actually used in this run
  */
  uint8_t ruleSet[NUM_RULE_CLASSES];
  uint8_t maxRulesUsed;

} filterRules;

typedef filterRules *filterRulesPtr;

typedef struct filterHeaderV0 {
  /* number of filters applied to this file */
  uint8_t filterCount;
  /* ptr to array of ptrs to filterRules */
  filterRulesPtr  (*frArray)[1];
} filterHeaderV0;

typedef filterHeaderV0 * filterHeaderV0Ptr;

/* struct to hold information for the filter information header */
typedef struct filterInfoV1 {
  uint16_t byteCount;             /* # of bytes in this information set */
  char info[4];                 /* keep the compiler happy */
} filterInfoV1;
typedef filterInfoV1 * filterInfoV1Ptr;

/* struct to hold the filter header itself for FT_RWFILTER. */
typedef struct filterHeaderV1 {
  genericHeader gHdr;
  uint32_t filterCount;                /* # of filters in this file */
  /* array of pointers to filterInfo structs */
  filterInfoV1Ptr fiArray[256];
  /* length of all filter information structs */
  uint32_t totFilterLen;
} filterHeaderV1;
typedef filterHeaderV1 * filterHeaderV1Ptr;

int parseMarks(char *, uint32_t *, uint32_t);
int checkMark(uint32_t*, uint16_t);
int checkRWRules(void *tmpRec);

/*int checkAddress(uint32_t tgtAddress, int swapFlag, addressRange *maskRange); */
/* SLK int validChars(char *input, char *pattern); */

void filterPrintRuleSet(FILE *fp, filterRulesPtr fltrRPtr);
int filterWriteHeaderV1(filterHeaderV1Ptr FHPtr, FILE *fp);
int filterAddFInfoToHeaderV1 (filterHeaderV1Ptr, int, char **);
void  filterPrintHeaderV1(filterHeaderV1Ptr fHPtr, FILE *outF);
int filterReadHeaderV1(filterHeaderV1Ptr FHPtr, FILE *fp, int swapFlag);

int checkAddress(uint32_t tgtAddress, addressRange *maskRange);
uint32_t parseTime(char *timeString, valueRange *tgtTime);
uint32_t parseRange(char *rangeString, valueRange *tgtRange);
uint32_t parseDecimalRange(char *rangeString, valueRange *tgtRange);
uint32_t parseIP(char *ipString, int place, filterRules *rules);
uint32_t parseStartTime(char *timeString, uint32_t *tgtTime);
uint32_t parseEndTime(char *timeString, uint32_t *tgtTime, uint32_t position);
uint32_t parseTCPFlags(char *flagString);
/*
 * routines having to do with the filter options.  Stored in filteropts.c
*/

void filterUsage(void);
int  filterGetRuleCount(void);
int  filterSetup(void);
void filterTeardown(void);

#endif /*  _FILTER_H */
