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
** 1.22
** 2004/03/10 22:23:52
** thomasm
*/

/*
 *  filteropts.c
 *
 *  Suresh Konda
 *
 *  parsing and setting up filter options
 *
 *  10/21/01
 *
 */

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include <regex.h>

#include "utils.h"
#include "filter.h"

RCSIDENT("filteropts.c,v 1.22 2004/03/10 22:23:52 thomasm Exp");


#define checkMark(f,i) ( (f[i/32] & (1 << (i % 32))) ? 1 : 0 )

#define miniMax(t, min, max) ((t == FILTER_USE_MIN) ? min : (t == FILTER_USE_MAX) ? max : ((max - min) << 1))

/* locals */

/* exported */
filterRulesPtr fltrRulesPtr = (filterRulesPtr) NULL;

/*
** IMPORTANT:
** Keep this array in exact sync with the enum ruleTypes declared in filter.h
*/

static struct option filterOptions[] =
{
  {"stime",             REQUIRED_ARG, 0, STIME},
  {"etime",             REQUIRED_ARG, 0, ENDTIME},
  {"duration",          REQUIRED_ARG, 0, DURATION},
  {"sport",             REQUIRED_ARG, 0, SPORT},
  {"dport",             REQUIRED_ARG, 0, DPORT},
  {"protocol",          REQUIRED_ARG, 0, PROTOCOL},
  {"bytes",             REQUIRED_ARG, 0, BYTES},
  {"packets",           REQUIRED_ARG, 0, PKTS},
  {"bytes-per-packet",  REQUIRED_ARG, 0, BYTES_PER_PKT},
  {"saddress",          REQUIRED_ARG, 0, SADDRESS},
  {"daddress",          REQUIRED_ARG, 0, DADDRESS},
  {"not-saddress",      REQUIRED_ARG, 0, NOT_SADDRESS},
  {"not-daddress",      REQUIRED_ARG, 0, NOT_DADDRESS},
  {"tcp-flags",         REQUIRED_ARG, 0, TCP_FLAGS},
  {"input-index",       REQUIRED_ARG, 0, INPUT_INTERFACES},
  {"output-index",      REQUIRED_ARG, 0, OUTPUT_INTERFACES},
  {"next-hop-id",       REQUIRED_ARG, 0, NEXT_HOP_ID},
  {"aport",             REQUIRED_ARG, 0, APORT},
#if     ADD_SENSOR_RULE
  {"sensors",           REQUIRED_ARG, 0, SENSORS},
#endif
  {"syn-flag",          REQUIRED_ARG, 0, OPT_SYN},
  {"ack-flag",          REQUIRED_ARG, 0, OPT_ACK},
  {"fin-flag",          REQUIRED_ARG, 0, OPT_FIN},
  {"psh-flag",          REQUIRED_ARG, 0, OPT_PSH},
  {"urg-flag",          REQUIRED_ARG, 0, OPT_URG},
  {"rst-flag",          REQUIRED_ARG, 0, OPT_RST},
  {0, 0, 0, 0}
};


static const char *filterHelp[] = {
  "date range YYYY/MM/DD:HH:MM:SS-YYYY/MM/DD:HH:MM:SS", /* STIME */
  "date range YYYY/MM/DD:HH:MM:SS-YYYY/MM/DD:HH:MM:SS", /* ENDTIME */
  "numeric range N-N", /* DURATION */
  "list in 0-65535", /* SPORT */
  "list in 0-65535", /* DPORT */
  "list in 0-255", /* PROTOCOL */
  "numeric range N-M", /* BYTES */
  "numeric range N-M", /* PKTS */
  "numeric range N-M", /* BYTES_PER_PKT */
  "1-10 dotted ip # with each octet being a list", /* SADDRESS */
  "1-10 dotted ip # with each octet being a list", /* DADDRESS */
  "1-10 dotted # with each octet being a list", /* NOT_SADDRESS */
  "1-10 dotted # with each octet being a list", /* NOT_DADDRESS */
  "list in [UAPSRF] where\n  U=URG; S=SYN; A=ACK; R=RST; P=PSH; F=FIN", /* TCP_FLAGS */
  "list in 1-255", /* INPUT_INTERFACES */
  "list in 1-255", /* OUTPUT_INTERFACES */
  "1 dotted # with each octet being a list", /* NEXT_HOP_ID */
  "list in 0-65535", /* APORT */
#if     ADD_SENSOR_RULE
  "list in 0-254", /* SENSORS */
#endif
  "SYN: 0 for packets without SYN flag, 1 for packets with one", /* OPT_SYN */
  "ACK: 0 for packets without ACK flag, 1 for packets with one", /* OPT_ACK */
  "FIN: 0 for packets without FIN flag, 1 for packets with one", /* OPT_FIN */
  "PSH: 0 for packets without PSH flag, 1 for packets with one", /* OPT_PSH */
  "URG: 0 for packets without URG flag, 1 for packets with one", /* OPT_URG */
  "RST: 0 for packets without RST flag, 1 for packets with one", /* OPT_RST */
  (char *)NULL
};

/*
 * the number of options is 1 less than the number of entries in the array
 * because of the sentinel
 */
static uint32_t optionCount = sizeof(filterOptions)/sizeof(struct option) - 1;

/* local functions*/
static int validChars(const char *input, const char *pattern);

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
  memset(fields, 0, size/32);

  /*ensure that the only characters are 0-9 , (comma) and - (hyphen) */
  if(!validChars(b, "[^0-9\\,\\-]")){
    fprintf(stderr,"invalid input to parseMarks().  The only legal inputs are the numbers 0 through 9, a comma, and a hyphen.\n");
    return 1;
  }

  while(*sp) {
    n = strtoul(sp, &ep, 10);
    if (sp == ep || n > size) {
      fprintf(stderr, "Error in field allocation; greater than %d\n", size);
      memset(fields, 0, size/32); /* cleanup */
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
        fprintf(stderr, "Error in field allocation; greater than %d\n", size);
        memset(fields, 0, size/32); /* cleanup */
        return -1;
      }
      /* AJY 12.12.01 added check for semantic correctness (e.g. x >=y) */
      if (n > m) {
        fprintf(stderr, "Input is incorrectly constructed: the beginning value %d is greater than the ending value %d\n", n, m);
        memset(fields, 0, size/32); /* cleanup */
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
        memset(fields, 0, size/32); /* cleanup */
        return 1;
      }
      break;

    case '\0':
      return 0;

    default:
      /* AJY modified 12.12.01 to dump ep */
      fprintf(stderr, "Unparsable character:%c\n", *ep);
      memset(fields, 0, size/32); /* cleanup */
      return 1;
    }
  }
  return 0;
}


/*
 * uint32_t parseIP(char *ipString, int place, filterRules *rules)
 *
 * SUMMARY:
 *
 * parseIP converts a string represetnation of a range of IP addresses into
 * the appropriate mark matrix.
 *
 * PARAMETERS:
 *
 * ipString: a string representation of the IP address in ,- list format.
 * int place: Source or Destination address.
 * filterRules *rules: pointer to the filterRules array
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
 * On success, modifies the rules to add a new IP address:
 *             The counter
 *
 * NOTES:
 *
 *
 */
uint32_t parseIP(char *ipString, int place, filterRules *rules) {
  int ipLen,maskSize,i, lastPos, currentIP;
  size_t j;
  char smallIP[4][1024];
  char tmpString[256];
  const char maxString[] = "0-255";
  ipLen = strlen(ipString);
  maskSize=0;
  currentIP = rules->totalAddresses[place]++;


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
    memset(smallIP[i], 0, 1024);
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
      strcpy(tmpString, maxString);
    }
    if(parseMarks(tmpString, rules->IP[place][currentIP].octets[i],
                  MAX_ADDRESSES * 32 - 1)) {
      rules->totalAddresses[place]--;
      return 1;
    }
  }
  return 0;
}

/*
 * uint32_t parseSingleAddress(char *ipString, addressRange *tgtAddress)
 *
 * SUMMARY:
 *
 * parses an ip string and deposits the results into the addressRagne
 * specified by the pointer.
 *
 * PARAMETERS:
 *
 * ipString: char * pointer to the representation fo the IP address
 * addressRange *tgtAddress: tgted address range.
 *
 *
 * RETURNS:
 *
 * status variable.  0 on successs, one on failure.
 *
 * SIDE EFFECTS:
 *
 * IMPLICIT CLEAR->tgtAddress
 * REWRITE: tgtAddress
 *
 * NOTES:
 * stylistic note to msyelf; I need an addendum function style.
 *
 * This routine is a modified version of parseIP intended for single
 * IP addresses (outside of our normal IP array).  We havent' encountered
 * a need for this routine before because up until this point, we've
 * always been dealing with IP arrays.  Nexthopid changes this however,
 * ergo this routine.
 *
 * TODO: Move invocation of this into parseIP
 */

uint32_t parseSingleAddress(char *ipString, addressRange *tgtAddress) {
  int ipLen,maskSize,i,lastPos;
  size_t j;
  char smallIP[4][1024];
  char tmpString[256];
  char maxString[] = "0-255";
  ipLen = strlen(ipString);
  maskSize=0;
  for(i = 0, j = 0, lastPos = 0; i < 4; i++, lastPos = ++j) {
    memset(smallIP[i], 0, 1024);
    memset(tgtAddress, 0, sizeof(tgtAddress));
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
      memset(tmpString, '\000', 15);
      strcpy(tmpString, maxString);
    }
    if(parseMarks(tmpString, tgtAddress->octets[i],
                  MAX_ADDRESSES * 32 - 1)) {
      return 1;
    }
  }
  return 0;
}


/*
 * uint32_t parseStartTime(char *timeString, uint32_t *tgtTime)
 *
 * SUMMARY:
 *
 * parses a single time string(YYYY/MM/DD:HH:MM:SS) into epoch time.
 * this particular method is syntax-tight; that is, it expects the
 * full string and will fail if the full string is not provided.
 *
 * PARAMETERS:
 *
 * char *timeString - ostensible time value.
 * uint32_t *tgtTime - buffer to write the new time to.
 * RETURNS:
 *
 * 0 on success
 * 1 on parse error or semantic error
 *
 * SIDE EFFECTS:
 *
 * fills the tgtTime buffer
 *
 * NOTES:
 *
 * formerly called "parseSingleTime" and was changed to reflect
 * some of our
 */
uint32_t parseStartTime(char *timeString, uint32_t *timeBuffer) {
  static struct tm tgtTime;
  char *tmpString;
  char *strEnd;
  uint32_t buffer, i;
  /*ensure that the only characters are 0-9 , forward slash and a colon */

  if(!validChars(timeString, "[^0-9\\/\\:]")){
    fprintf(stderr,"invalid input to parseStartTime().  The only legal inputs are the numbers 0 through 9, a forward slash and a colon\n.");
    return 1;
  }
  memset(&tgtTime, '\000', sizeof(struct tm));
  tmpString = timeString;
  for(i=0; i<6;i++) {
    buffer = strtoul(tmpString, &strEnd, 10);
    if (tmpString == strEnd) {
      fprintf(stderr, "Expecting digit but got '%c'\n", *tmpString);
      return 1;
    }
    tmpString = (strEnd + 1);
    switch(i) {
    case 0:
      if (buffer < 1970 || buffer > 3000) {
        fprintf(stderr, "Invalid year: %d, please pick a number between 1970 and 3000\n", buffer);
        return 1;
      }
      tgtTime.tm_year = buffer - 1900;
      break;
    case 1:
      if (buffer > 12 || buffer < 1) {
        fprintf(stderr, "Invalid month: %d, please pick a number between 1 and 12\n", buffer);
        return 1;
      }
      tgtTime.tm_mon = buffer - 1;
      break;
    case 2:
      if (buffer > 31 || buffer < 1) {
        fprintf(stderr, "Invalid day: %d, please pick a number between 1 and 31\n", buffer);
        return 1;
      }
      tgtTime.tm_mday = buffer;
      break;
    case 3:
      if (buffer > 23) {
        fprintf(stderr, "Invalid hour: %d, please pick a number between 0 and 23\n", buffer);
        return 1;
      }
      tgtTime.tm_hour = buffer;
      break;
    case 4:
      if(buffer > 59) {
        fprintf(stderr, "Invalid minute: %d, please pick a number between 0 and 59\n", buffer);
        return 1;
      }
      tgtTime.tm_min = buffer;
      break;
    case 5:
      if(buffer > 59) {
        fprintf(stderr, "Invalid second: %d, please pick a number between 0 and 59\n", buffer);
        return 1;
      }
      tgtTime.tm_sec = buffer;
      /*
       * Assume that the time is always zulu, and adjust the clock accordingly
       */

      tgtTime.tm_isdst = 0;
      break;
    }
  }

  *timeBuffer = SK_TIME_GM(&tgtTime);
  /* ERROR check on timeBuffer.  Return 1 on an invalid value.*/
  if ((time_t)*timeBuffer == (time_t)-1) {
    return 1;
  }

  return 0;
}

/*
 * uint32_t parseEndTime(char *timeString, uint32_t *tgtTime, uint32_t position)
 *
 * SUMMARY:
 *
 * parses a single time string(YYYY/MM/DD:HH:MM:SS) into epoch time.
 * unlike parseStartTime, this method is looser.  If it runs out of
 * elements (YYYY/MM/DD), it estimates the next logical time based
 * on the position variable -
 *
 *
 * PARAMETERS:
 *
 * char *timeString - ostensible time value.
 * uint32_t *tgtTime - buffer to write the new time to.
 *
 * RETURNS:
 *
 * 0 on success
 * 1 on parse error
 *
 * SIDE EFFECTS:
 *
 * fills the tgtTime buffer
 *
 * NOTES:
 *
 * What this does, as with the IP parser, is generate a syntactically
 * correct string that fills up the rest of the time buffer with the
 * expected final time.
 */
uint32_t parseEndTime(char *timeString, uint32_t *tgtTime, uint32_t position) {
  char timeBuffer[80], result[80];
  char tmpString[25];
  int i, mday, month,lastPos;
  size_t j;

  time_t tms;
  memset(timeBuffer, '\0', sizeof(timeBuffer));
  memset(result, '\0', sizeof(result));
  month = 0;
  /*
   * We'll just do a next element search here, this routine is
   * pretty complex, and it's just as well to replace the
   * gettokens here.
   */
  for(i = 0, j = 0, lastPos = 0; i<6; i++, lastPos = ++j) {
    while((j < strlen(timeString)) && (timeString[j] != (i < 2 ? '/' : ':')))
      j++;
    memset(tmpString, '\000', 25);
    strncpy(tmpString, timeString + lastPos, j - lastPos);
    if(! tmpString) {
      break;
    } else {
      strcat(timeBuffer, tmpString);
      strcat(timeBuffer, i < 2 ? "/" : ":");
      if (i == 2) {
        month = strtoul(tmpString, NULL, 10);
      }
    }
    if( !*(timeString + j + 1))
      break;
  }
  i++;


  if(!month) {
    month = miniMax(position, 1, 12);
  }
  if(month == 2) {
    if(!strncmp("2000",timeString,4)) {
      mday = 29;
    } else {
      mday = 28;
    }
  } else {
    if((month == 9) || (month == 4) || (month == 6)) {
      mday = 30;
    }
  }

  j = (int)strtoul(timeString, (char **)NULL, 10); /* temp location for year */
  mday = maxDayInMonth(j, month);
  switch(i) {
  case 6:
    break;
  case 5:
    sprintf(result, "%d", miniMax(position, 0, 59));
    break;
  case 4:
    sprintf(result, "%d:%d", miniMax(position, 0, 59),
            miniMax(position, 0, 59));
    break;
  case 3:
    sprintf(result, "%d:%d:%d", miniMax(position, 0, 23),
            miniMax(position , 0 , 59), miniMax(position, 0, 59));
    break;
  case 2:
    sprintf(result, "%d:%d:%d:%d", miniMax(position, 1, mday),
            miniMax(position, 0, 23), miniMax(position, 0, 59),
            miniMax(position, 0, 59));
    break;
  case 1:
    sprintf(result,"%d/%d:%d:%d:%d", month, miniMax(position, 1, mday),
            miniMax(position, 0, 23), miniMax(position, 0, 59),
            miniMax(position, 0, 59));
    break;
  case 0:
    time(&tms);
    strftime(result, 80, "%G/%m/%d:%H:%M:%S", gmtime(&tms));
    break;
  }
  strcat(timeBuffer, result);

  /* AJY 12.12.01 commented out line below; looks like debugging info that's hanging around */
  /*fprintf(stderr, "Revised buffer is %s\n", timeBuffer);*/

  return(parseStartTime(timeBuffer, tgtTime));
}

/*
 * int parseTime(char *timeString, valueRange *tgtTime)
 *
 * SUMMARY:
 *
 * Parses a time string in YYYY/MM/DD:HH:MM:SS[-YYYY/MM/DD:HH:MM:SS] format
 * rejects badly formatted times.
 *
 * PARAMETERS:
 *
 * char *timeString = a string containing a time value to parse
 * valueRange *tgtTime = pointer to the time field itself.
 *
 * RETURNS:
 *
 * 0 on success
 * 1 on a badly formatted string.
 *
 * SIDE EFFECTS:
 *
 * tgtTime is set to [start, start] in the case of only one
 * value being passed.
 *
 * on failures, tgtTime does not change.
 *
 * NOTES:
 *
 *
 */
uint32_t parseTime(char *timeString, valueRange *tgtTime) {
  char *tmpString;
  char fBuffer[64];
  uint32_t buffer, stringIndex;
  /* isolate times into start and end strings, if possible.*/
  tmpString = strchr(timeString, '-');
  if (tmpString) {
    memset(fBuffer, '\000', 64);
    stringIndex = (tmpString - timeString) / sizeof(char);
    strncpy(fBuffer, timeString, stringIndex);
    if(parseStartTime(fBuffer, &buffer))
        return 1;
    tgtTime->min = buffer;
    if (stringIndex < strlen(timeString)) {
      memset(fBuffer, 0, 64);
      strcpy(fBuffer, tmpString + 1);
      if (parseEndTime(fBuffer, &buffer, FILTER_USE_MAX))
        return 1;
      tgtTime->max = buffer;
    } else {
      tgtTime->max = tgtTime->min;
    }
  }
  else
    return 1;
  if(tgtTime->max < tgtTime->min){
    fprintf(stderr, "The starting time is before the ending time.\n");
    return 1;
  }

  return 0;
}

/*
 * uint32_t parseRange(char *rangeString, valueRange *tgtRange)
 *
 * SUMMARY:
 *
 * parses a range of values (i.e.: a-b)
 *
 * PARAMETERS:
 *
 * char *rangeString: a string containing a representation of the range.
 * valueRange *tgtRange: the resulting range.
 *
 * RETURNS:
 *
 * 0 on success; 1 on failure.
 *
 * SIDE EFFECTS:
 *
 * valueRange is altered.
 *
 * NOTES:
 * AJY modified 12.11.2001 to add fail-fast input checking.  End result - the input must take the format of x-y s.t. x,y exist in Nat. Numbers and x<=y.  All else dies with a message to stderr.
 *
 */
uint32_t parseRange(char *rangeString, valueRange *tgtRange) {
  char *tmpString;
  uint64_t buffer;
  /* input validation case 1:  lack of hyphen */
  if(memchr(rangeString, '-', sizeof(char) * strlen(rangeString)) == NULL){
    return 1;
  }
  /* case 2: rangeString is of the form -y */
  if(rangeString[0] == '-'){
    return 1;
  }
  /* case 3: rangeString is of the form x- */
  if(rangeString[strlen(rangeString)-1] == '-'){
    return 1;
  }
  /* end input validation */


  buffer = 0;
  /*
   * tmpstring will be set to the pointer to '-' if it exists in
   * this range.  If it's set to NULL, then we just to the min=max
   * approach (i.e., 100 = 100-100), if it's set to -, then we
   * look for a maximum (first checking that '-'++ is not null, if
   * it is (i.e., ASCIIZ), we do min = max again.  Otherwise,
   * we parse the presumed (digit) and exit.
   */

  buffer = (uint64_t) strtoul(rangeString, &tmpString, 10);
  /*
   *  Invalid digit.
   */
  if (buffer == ULONG_MAX)
    return 1;
  tgtRange->min = buffer;
  if (*tmpString) {
    /*
     * If the value held at tmpString is non-null, then
     * it must be a dash.
     * If the dash is followed by a null (terminated), then
     * we use max-max again.
     */
    if (*tmpString == '-' && ( *(tmpString + 1)))
      buffer = (uint64_t) strtoul(tmpString + 1, NULL, 10);
    else
      return 1;
    if(buffer == ULONG_MAX) {
      fprintf(stderr, "ERROR: parsing range value %s\n", rangeString);
      return 1;
    }
    tgtRange->max = buffer;
  } else
    return 1;
  if(tgtRange->max < tgtRange->min) {
    fprintf(stderr, "ERROR: Range maximum (%llu) < range minimum (%llu)\n",tgtRange->max, tgtRange->min);
    return 1;
  }
  return 0;
}

/*
 * uint32_t parseDecimalRange(char *rangeString, valueRange *tgtRange)
 *
 * SUMMARY:
 *
 * parses a range of values (i.e.: a-b)
 *
 * PARAMETERS:
 *
 * char *rangeString: a string containing a representation of the range.
 * valueRange *tgtRange: the resulting range.
 *
 * RETURNS:
 *
 * 0 on success; 1 on failure.
 *
 * SIDE EFFECTS:
 *
 * valueRange is altered.
 *
 * NOTES:
 *
 * rename this to parsefixedpointrange at some point.
 */
uint32_t parseDecimalRange(char *rangeString, valueRange *tgtRange) {
  char *tmpString;
  double buffer;

  buffer = 0.0;

  /* input validation case 1:  lack of hyphen */
  if(memchr(rangeString, '-', sizeof(char) * strlen(rangeString)) == NULL){
    return 1;
  }
  /* case 2: rangeString is of the form -y */
  if(rangeString[0] == '-'){
    return 1;
  }
  /* case 3: rangeString is of the form x- */
  if(rangeString[strlen(rangeString)-1] == '-'){
    return 1;
    }
  /* end input validation */
  /* alright, strtod is an incredibly annoying function, since
   * its error code is 0 (which is also a valid value>  To get
   * around that, you set errno at the start of the invocation
   */
  errno = 0;
  buffer = strtod(rangeString, &tmpString);
  if (errno) {
    return 1;
  }
  tgtRange->min = (uint32_t) (buffer * PRECISION);
  if (*tmpString) {
    if (*tmpString == '-' && (*(tmpString + 1))) {
      buffer = ( strtod(tmpString + 1, NULL) * PRECISION);
      if (errno) {
        return 1;
      }
    }
    else
      return 1;
  } else {
    return 1;
  }
  tgtRange->max = (uint32_t) buffer;
  if(tgtRange->max < tgtRange->min) {
    fprintf(stderr, "ERROR: Range maximum < range minimum\n");
    return 1;
  }
  return 0;
}

/*
 * uint32_t parseTCPFlags(char *tgtString)
 *
 * SUMMARY:
 *
 * generates a binary representation of the TCP flags.
 *
 * PARAMETERS:
 *
 * tgtString: tcp flags to be parsed by the routine.
 *
 * RETURNS:
 *
 * an integer representation of the flag mask.
 *
 * SIDE EFFECTS:
 *
 * None to speak of.
 *
 * NOTES:
 *
 *
 */
uint32_t parseTCPFlags(char *tgtString) {
  uint8_t i;
  uint32_t buffer = 0;
  for(i = 0; i < strlen(tgtString); i++) {
    switch(tgtString[i]) {
    case 'U':
      buffer |= URG_FLAG;
      break;
    case 'A':
      buffer |= ACK_FLAG;
      break;
    case 'P':
      buffer |= PSH_FLAG;
      break;
    case 'R':
      buffer |= RST_FLAG;
      break;
    case 'S':
      buffer |= SYN_FLAG;
      break;
    case 'F':
      buffer |= FIN_FLAG;
      break;
    }
  }
  return buffer;
}



/* int validChars(const char *input, const char *pattern)
 *
 * tests input to see if characters exist that are not in POSIX regex
 * pattern.
 *
 * returns: 1 if no characters not in regex exist
 *          0 if characters not in regex exist or if error occurs
 * input:   input: an input string
 *          pattern: a valid POSIX regex pattern describing THE
 *          COMPLEMENT OF WHAT IS ACCEPTED.  should be [^...]
 *
 * example: testing date strings for invalid characters - input =
 * 2001/11/1:10:45:20 the only acceptable symbols are 0 through 9,
 * forward slash, and colon.  pattern = [^0-9:\\/]
 */
static int validChars(const char *input, const char *pattern){
  regmatch_t *result;
  regex_t *regex;
  size_t errorLength;
  int errorNumber;
  char *errorBuffer;

  regex = (regex_t *)malloc(sizeof(regex_t));
  errorNumber=0;
  errorLength=0;

  /*compile regex */
  if((errorNumber=regcomp(regex, pattern, REG_EXTENDED)) != 0){
    /* error condition in regex compilation - print error to stderr and die */
    errorLength = regerror(errorNumber, regex, NULL, 0);
    errorBuffer = malloc(errorLength);
    regerror(errorNumber, regex, errorBuffer, errorLength);

    /* dump to stderr */
    fprintf(stderr, "error while compiling regular expression - error message is: %s\n", errorBuffer);

    /* cleanup and quit */
    regfree(regex);
    free(errorBuffer);
    return 0;
  }

  /* regex compiled successfully.  now allocate memory for regex testing */
  if((result=(regmatch_t *)malloc(sizeof(regmatch_t))) == 0){
    /*error allocating memory */
    fprintf(stderr,"can't allocate memory to run regex test\n");
    return 0;
  }

  /* finally, run regex and report results. */
  if(regexec(regex, input, errorLength, result, 0) == 0){
    /* note that finding the invalid character is not supported, as the regex was run with a final arg of 0, to maximize performance. */
    fprintf(stderr,"invalid character found in input string %s\n", input);
    return 0;
  }

  return 1;
}


/* public functions */
void filterUsage(void) {
  register uint32_t i;
  fprintf(stderr, "\nfilter options:\n");
  for (i = 0; i < optionCount; i++) {
    fprintf(stderr, "--%s %s. %s\n", filterOptions[i].name,
            filterOptions[i].has_arg ? filterOptions[i].has_arg == 1
            ? "Req. Arg" : "Opt. Arg" : "No Arg", filterHelp[i]);
  }
}

int  filterGetRuleCount(void) {
  return fltrRulesPtr->maxRulesUsed;
}



/*
 * static int parseFilterOptions(clientData 0, int index, char *optarg)
 *
 * SUMMARY:
 *
 * Called by the generic options parser when the user gives a filter
 * option on the command line. Index is the index of the entry in the
 * filterOptions array.
 * optarg, when valid, points to the argument to the option.
 *
 * PARAMETERS:
 * clientData  0: ignored here
 * int index: index into filterOptions array for this option
 * char *optarg:  option argument.
 *
 * RETURNS:
 *
 * on failure, returns 1
 * on success, returns 0
 *
 * SIDE EFFECTS:
 *
 * Fills up the relevant sections of fltrRulesPtr with the parsed
 * rules.  Triggers flags, &c.
 *
 * NOTES:
 *
 */
static int parseFilterOptions(clientData UNUSED(cData),int index,char *optarg){
  register int i;

#if     0
  if (index < 0 || index >= optionCount) {
    fprintf(stderr, "parseFilterOptions: invalid index %d, count is %d\n",
            index, optionCount);
    return 1;
  }
#endif

  /* check that this is not a repeated rule */
  for (i = 0; i < fltrRulesPtr->maxRulesUsed; i++ ) {
    if (index == fltrRulesPtr->ruleSet[i]) {
      fprintf(stderr, "Rule %s has already been set\n",
            filterOptions[index].name);
      return 1;
    }
  }

  switch (index) {

  case STIME:
    if (parseTime(optarg, &(fltrRulesPtr->sTime))) {
      fprintf(stderr, "Error parsing start time\n");
      return(1);
    };
    break;

  case ENDTIME:
    if (parseTime(optarg, &(fltrRulesPtr->eTime))) {
      fprintf(stderr, "Error parsing end time\n");
      return(1);
    };
    break;

  case DURATION:
    if(parseRange(optarg, &(fltrRulesPtr->duration))) {
      fprintf(stderr, "Error parsing duration\n");
      return(1);
    }
    break;

  case SPORT:
    if(parseMarks(optarg, fltrRulesPtr->srcPorts,
                  MAX_PORTS * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing source port\n");
      return(1);
    };
    break;

  case DPORT:
    if(parseMarks(optarg, fltrRulesPtr->dstPorts,
                  MAX_PORTS * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing destination port\n");
      return(1);
    };
    break;

  case PROTOCOL:
    if(parseMarks(optarg, fltrRulesPtr->protocols,
                  MAX_PROTOCOLS * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing protocol\n");
      return(1);
    }
    break;

  case BYTES:
    if (parseRange(optarg, &(fltrRulesPtr->bytes))) {
      fprintf(stderr, "Error parsing byte range\n");
      return(1);
    }
    break;

  case PKTS:
    if(parseRange(optarg, &(fltrRulesPtr->packets))) {
      fprintf(stderr, "Error parsing packet range\n");
      return(1);
    };
    break;

  case BYTES_PER_PKT:
    if(parseDecimalRange(optarg, &(fltrRulesPtr->bytesPerPacket))) {
      fprintf(stderr, "Error parsing average number of bytes per packet\n");
      return(1);
    }
    break;

  case SADDRESS:
    if(parseIP(optarg, SRC, fltrRulesPtr)) {
      fprintf(stderr, "Error parsing source IP address\n");
      return(1);
    }
    break;

  case DADDRESS:
    if(parseIP(optarg, DST, fltrRulesPtr)) {
      fprintf(stderr, "Error parsing destination IP address\n");
      return(1);
    }
    break;

  case NOT_SADDRESS:
    if(parseIP(optarg, SRC, fltrRulesPtr)) {
      fprintf(stderr, "Error parsing not source IP address\n");
      return(1);
    }
    fltrRulesPtr->negateIP[SRC] = 1;
    break;

  case NOT_DADDRESS:
    if(parseIP(optarg, DST, fltrRulesPtr)) {
      fprintf(stderr, "Error parsing not destination IP address\n");
      return(1);
    }
    fltrRulesPtr->negateIP[DST] = 1;
    break;

  case TCP_FLAGS:
    fltrRulesPtr->tcpFlags = parseTCPFlags(optarg);
    break;

  case INPUT_INTERFACES:
    if (parseMarks(optarg, fltrRulesPtr->inputInterfaces,
                   MAX_INTERFACES * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing input interfaces\n");
      return(1);
    }
    break;

  case OUTPUT_INTERFACES:
    if (parseMarks(optarg, fltrRulesPtr->outputInterfaces,
                   MAX_INTERFACES * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing output interfaces\n");
      return(1);
    }
    break;

  case NEXT_HOP_ID:
    if(parseSingleAddress(optarg, &(fltrRulesPtr->nextHop))) {
      fprintf(stderr, "Error parsing next hop IP address\n");
      return(1);
    }
    break;

  case APORT:
    if(parseMarks(optarg, fltrRulesPtr->anyPorts,
                  MAX_PORTS * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing destination port\n");
      return(1);
    };
    break;

#if     ADD_SENSOR_RULE
  case SENSORS:
    if(parseMarks(optarg, fltrRulesPtr->sensors,
                  MAX_SENSORS * MARK_COMPRESS)) {
      fprintf(stderr, "Error parsing sensor\n");
      return(1);
    }
    break;
    /*#endif */
#endif

    /*
     * This section is all flag wonking
     */
  case OPT_SYN:
    i = (int)strtol(optarg, (char **)NULL, 10);
    switch(i) {
    case 1:
      fltrRulesPtr->flagMark |= SYN_FLAG;
    case 0:
      fltrRulesPtr->flagCare |= SYN_FLAG;
      break;
    default:
      fprintf(stderr, "Error parsing syn flag value: %s\n", optarg);
      return(1);
    }
    break;

  case OPT_ACK:
    i = (int)strtol(optarg, (char **)NULL, 10);
    switch(i) {
    case 1:
      fltrRulesPtr->flagMark |= ACK_FLAG;
    case 0:
      fltrRulesPtr->flagCare |= ACK_FLAG;
      break;
    default:
      fprintf(stderr, "Error parsing ack flag value: %s\n", optarg);
      return(1);
    }
    break;

  case OPT_FIN:
    i = (int)strtol(optarg, (char **)NULL, 10);
    switch(i) {
    case 1:
      fltrRulesPtr->flagMark |= FIN_FLAG;
    case 0:
      fltrRulesPtr->flagCare |= FIN_FLAG;
      break;
    default:
      fprintf(stderr, "Error parsing fin flag value: %s\n", optarg);
      return(1);
    }
    break;

  case OPT_PSH:
    i = (int)strtol(optarg, (char **)NULL, 10);
    switch(i) {
    case 1:
      fltrRulesPtr->flagMark |= PSH_FLAG;
    case 0:
      fltrRulesPtr->flagCare |= PSH_FLAG;
      break;
    default:
      fprintf(stderr, "Error parsing psh flag value: %s\n", optarg);
      return(1);
    }
    break;

  case OPT_URG:
    i = (int)strtol(optarg, (char **)NULL, 10);
    switch(i) {
    case 1:
      fltrRulesPtr->flagMark |= URG_FLAG;
    case 0:
      fltrRulesPtr->flagCare |= URG_FLAG;
      break;
    default:
      fprintf(stderr, "Error parsing urg flag value: %s\n", optarg);
      return(1);
    }
    break;

  case OPT_RST:
    i = (int)strtol(optarg, (char **)NULL, 10);
    switch(i) {
    case 1:
      fltrRulesPtr->flagMark |= RST_FLAG;
    case 0:
      fltrRulesPtr->flagCare |= RST_FLAG;
      break;
    default:
      fprintf(stderr, "Error parsing rst flag value: %s\n", optarg);
      return(1);
    }
    break;

  } /* switch */

  fltrRulesPtr->ruleSet[fltrRulesPtr->maxRulesUsed] = index;
  fltrRulesPtr->maxRulesUsed++;

  return 0;                     /* OK */
}


/*
 * int filterSetup()
 *
 * SUMMARY:
 *
 * Called by the application to let the filter library setup for
 * options processing.
 *
 * RETURNS:
 *
 * on failure, returns 1
 * on success, returns 0
 *
 * SIDE EFFECTS:
 *
 * Initializes the filter library options with the options processing library.
 *
 * NOTES:
 *
 */
int filterSetup(void) {
  register int c;

  c = (sizeof(filterHelp)/sizeof(char *) - 1);
#if     1
  if ( c  != (int)optionCount) {
    fprintf(stderr, "filterSetup: mismatch in options and help %d vs %d\n",
            optionCount, c);
    return 1;
  }
#endif

  if (optionsRegister(filterOptions, (optHandler)parseFilterOptions,
                      (clientData)NULL)) {
    fprintf(stderr, "filterSetup: Unable to register with options\n");
    return 1;
  }

  fltrRulesPtr = (filterRulesPtr) calloc(1, sizeof(filterRules));
  return 0;
}

/*
 * void filterTeardown()
 *
 */
void filterTeardown(void) {
  if (fltrRulesPtr) {
    free(fltrRulesPtr);
    fltrRulesPtr = (filterRulesPtr) NULL;
  }
  return;
}
