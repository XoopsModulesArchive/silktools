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
** 1.6
** 2004/03/03 20:26:50
** thomasm
*/

/*
**  A collection of utility routines to manipulate strings
**
*/


#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"
#include "filter.h"

RCSIDENT("silkstring.c,v 1.6 2004/03/03 20:26:50 thomasm Exp");



uint32_t dot2num(const char *ip) {
  struct in_addr inp;

  if (inet_pton(AF_INET, ip, &inp) < 0) {
    return 0xFFFFFFFF;
  }
  return ntohl(inp.s_addr);
}


char *num2dot(uint32_t ip) {
  static char dotted[INET_ADDRSTRLEN];

  ip = htonl(ip);
  return (char*)inet_ntop(AF_INET, &ip, dotted, INET_ADDRSTRLEN);
}


/* integer to UAPRSF string */
char *tcpflags_string(const uint8_t flags) {
  static char flag_string[7];
  const int flag_string_len = sizeof(flag_string);

  memset(flag_string, ' ', flag_string_len);
  flag_string[flag_string_len - 1] = '\0';

  if (flags & URG_FLAG) {
    flag_string[5] = 'U';
  }
  if (flags & ACK_FLAG) {
    flag_string[4] = 'A';
  }
  if (flags & PSH_FLAG) {
    flag_string[3] = 'P';
  }
  if (flags & RST_FLAG) {
    flag_string[2] = 'R';
  }
  if (flags & SYN_FLAG) {
    flag_string[1] = 'S';
  }
  if (flags & FIN_FLAG) {
    flag_string[0] = 'F';
  }
  return flag_string;
}


/* strip whitespace of line in-place; return length */
int strip(char *line) {
  char *sp, *ep;
  int len;

  sp  = line;
  while( *sp && isspace((int)*sp) ) {
    sp++;
  }
  /* at first non-space char OR at end of line */
  if (*sp == '\0') {
    /* line full of white space. nail at beginning and return with 0 */
    line[0] = '\0';
    return 0;
  }

  /* figure out where to stop the line */
  ep = sp + strlen(sp) - 1;
  while( isspace((int)*ep) && (ep > sp) ) {
    ep--;
  }
  /* ep at last non-space char. Nail after */
  ep++;
  *ep = '\0';

  len = (int)(ep - sp);
  if (sp == line) {
    /* no shifting required */
    return(len);
  }

  memmove(line, sp, len+1);
  return(len);
}
  

void lower(char *cp) {
  while (*cp) {
    if (isupper((int)*cp)) {
      *cp = *cp + 32;
    }
    cp++;
  }
  return;
}


void upper(char *cp) {
  while (*cp) {
    if (islower((int)*cp)) {
      *cp = *cp - 32;
    }
    cp++;
  }
  return;
}


/* parse string "4,3,2-6" to array {4,3,2,3,4,5,6}.  See header for details */
uint8_t *skParseNumberList(const char *input, uint8_t minValue,
                           uint8_t maxValue, uint8_t *valueCount)
{
  unsigned long n, rangeStart, i;
  const char *sp;
  char *ep;
  /* max number of fields user can give: this is the maximum value
   * that valueCount can hold. */
  const uint16_t countUpperLimit = 255; /* (1<<(8*sizeof(*valueCount)))-1;*/
  uint8_t *numberListArray = NULL; /* returned array */
  uint8_t count = 0; /* returned count */
  uint16_t arraySize;
  int numEntries;
  /* the last thing we successfully parsed. NUMBER is a single number
   * or start of a range; RANGE is the upper limit of a range. */
  enum {COMMA, HYPHEN, NUMBER, RANGE} parseState;

  /* check input */
  if (!input || !maxValue || !valueCount) {
    fprintf(stderr, "Coder error: input or fieldCount NULL or maxValue 0\n");
    return NULL;
  }

  if (maxValue <= minValue) {
    fprintf(stderr, "Coder error: maxValue <= minValue\n");
    goto ERROR;
  }

  /* Set initial length of numberListArray as the number of possible
   * values and malloc something that size.  Duplicate values allow
   * arraySize to be larger that the possible values, and we will grow
   * the array if necessary. */
  arraySize = 1 + maxValue - minValue;
  numberListArray = calloc(arraySize, sizeof(uint8_t));
  if (!numberListArray) {
    fprintf(stderr, "Out of memory! numberListArray=calloc()\n");
    goto ERROR;
  }

  /* this is non-zero when the number we are about to parse is the
   * upper half of the range m-n */
  sp = input;
  rangeStart = 0;
  parseState = COMMA;

  /* parse user input */
  while(*sp) {
    if (!isdigit(*sp)) {
      if (parseState == HYPHEN) {
        /* digit must follow a hyphen */
        fprintf(stderr,
                "parse error: expecting digit; saw '%c' at position %u\n",
                *sp, 1+(sp - input));
        goto ERROR;
      }
      if (*sp == ',') {
        /* allow multiple commas: 3,,4 */
        parseState = COMMA;
        ++sp;
        continue;
      }
      if (parseState == RANGE) {
        /* range must end with a comma */
        fprintf(stderr,
                "parse error: expecting comma; saw '%c' at position %u\n",
                *sp, 1+(sp - input));
        goto ERROR;
      }
      if (parseState == COMMA) {
        /* a digit or a comma(handled above) must follow a comma */
        fprintf(stderr,
                "parse error: expecting digit; saw '%c' at position %u\n",
                *sp, 1+(sp - input));
        goto ERROR;
      }
      /* if we are here, parseState must be NUMBER */
      if (*sp == '-') {
        parseState = HYPHEN;
        ++sp;
        continue;
      }
      fprintf(stderr,
              "parse error: expecting digit,comma,hyphen; saw '%c' at position %u\n",
              *sp, 1+(sp - input));
      goto ERROR;
    }

    /* parseState is either COMMA or HYPHEN */

    n = strtoul(sp, &ep, 10);
    if (sp == ep) {
      fprintf(stderr, "cannot parse '%s' as an integer\n", sp);
      goto ERROR;
    }
    if ( !(minValue <= n && n <= maxValue) ) {
      fprintf(stderr, "value '%lu' out of range.  use %u <= n <= %u\n",
              n, minValue, maxValue);
      goto ERROR;
    }

    sp = ep;

    if (parseState == COMMA) {
      /* not parsing the upper half of a range */
      parseState = NUMBER;
      numEntries = 1;
      rangeStart = n;
    } else {
      parseState = RANGE;
      numEntries = n - rangeStart;

      if (n < rangeStart) {
        /* user entered 3-2 */
        fprintf(stderr, "bad range %lu-%lu\n", rangeStart, n);
        goto ERROR;
      }

      if (rangeStart == n) {
        /* user entered 3-3, ignore second '3' */
        continue;
      }

      /* we added rangeStart on the previous iteration, so add
       * rangeStart+1 through n now; */
      rangeStart++;
    }

    /* check number of fields user gave */
    if ((count + numEntries) > countUpperLimit) {
      fprintf(stderr, "Too many fields provided. Only %u fields allowed\n",
              countUpperLimit);
      goto ERROR;
    }

    /* check if we need to grow array? */
    while ((count + numEntries) > arraySize) {
      numberListArray = realloc(numberListArray, 2*arraySize);
      if (!numberListArray) {
        fprintf(stderr, "Out of memory! numberListArray=realloc()\n");
        goto ERROR;
      }
      memset((numberListArray+arraySize), 0, arraySize);
      arraySize *= 2;
    }

    /* add entries */
    for (i = rangeStart; i <= n; ++i) {
      numberListArray[count] = i;
      ++count;
    }
  } /* while(*sp) */

  *valueCount = count;
  return numberListArray;

 ERROR:
  if (numberListArray) {
    free(numberListArray);
  }
  *valueCount = 0;
  return NULL;
}
