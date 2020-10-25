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
** 1.7
** 2004/03/10 21:20:15
** thomasm
*/

/*
**  times.c
**
**  various utility functions for dealing with time
**
**  Suresh L Konda
**  1/24/2002
*/


#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

#include "utils.h"

RCSIDENT("times.c,v 1.7 2004/03/10 21:20:15 thomasm Exp");


/* MAX_YEARS max number of years we use for fglobbing */
#define MAX_YEARS 5


char *timestamp(uint32_t t) {
  struct tm *tRec;
  static char ts[24];
  tRec = gmtime((time_t *)&(t));
  sprintf(ts, "%02u/%02u/%04u %02u:%02u:%02u", tRec->tm_mon + 1,
	  tRec->tm_mday, tRec->tm_year + 1900,
	  tRec->tm_hour, tRec->tm_min, tRec->tm_sec);
  return ts;
}


/* Inverse of gmtime(); convert a struct tm to time_t */
time_t sk_timegm(struct tm *tm)
{
  time_t ret;
  char *tz;
  /* Some OSes will put this string directly into the environment--as
   * opposed to making a copy of it--so we make it static. */
  static char envbuf[1024];
  const size_t envbuf_size = sizeof(envbuf);

  /* get old timezone */
  tz = getenv("TZ");

  /* make TZ empty (UTC) and set timezone there */
  if (putenv("TZ=")) {
    fprintf(stderr, "sk_timegm(): Out of memory!\n");
    exit(EXIT_FAILURE);
  }
  tzset();

  /* do the work */
  ret = mktime(tm);

  /* restore TZ and timezone. Magic 6 is space we allow for "TZ=" */
  if (tz && (strlen(tz) < (envbuf_size - 6))) {
    sprintf(envbuf, "TZ=%s", tz);
    putenv(envbuf);
  } else {
    /* unset */
    putenv("TZ");
  }
  tzset();

  return ret;
}


/*
 *  the following two functions were taken from
 *
 *   http://aa.usno.navy.mil/faq/docs/JD_Formula.html
 *   which was based on the work by:
 *
 *   Fliegel and van Flandern (1968) published compact computer
 *   algorithms for converting between Julian dates and Gregorian
 *   calendar dates. Their algorithms were presented in the Fortran
 *   programming language, and take advantage of the truncation feature
 *   of integer arithmetic.
 *
 *   The following Fortran code modules are based on these
 *   algorithms. In this code, YEAR is the full representation of the
 *   year, such as 1970, 2000, etc. (not a two-digit abbreviation);
 *   MONTH is the month, a number from 1 to 12; DAY is the day of the
 *   month, a number in the range 1-31; and JD is the the Julian date at
 *   Greenwich noon on the specified YEAR, MONTH, and DAY.
 *
 *   Conversion from a Gregorian calendar date to a Julian date. Valid for
 *   any Gregorian calendar date producing a Julian date greater than zero:
 *
 *   SLK:
 *   This was modified from the fortran program using signed ints and with a
 *   base of 1999/12/1 being 0.  Using jdate.c this was calculated to be:
 *
 *   Julian Date of 1999/12/1 is   2451514
 *   
*/

static int32_t jdate1999Dec1 = 2451514;

int32_t julianDate(int yr, int mo, int day) {
#if	0  
  int32_t i;
  i = (day-32075+1461*(yr+4800+(mo-14)/12)/4+367*(mo-2-(mo-14)/12*12)
       /12-3*((yr+4900+(mo-14)/12)/100)/4);
  return ( i - jdate1999Dec1);
#else
  return ((day-32075+1461*(yr+4800+(mo-14)/12)/4+367*(mo-2-(mo-14)/12*12)
       /12-3*((yr+4900+(mo-14)/12)/100)/4) - jdate1999Dec1);
#endif
}

void gregorianDate(int32_t jd, int32_t *yr, int32_t *mo, int32_t *day) {
  int L, N, I, J, K;

  /* reset to the original 0 point */
  jd  += jdate1999Dec1;

  L= jd+68569;
  N= 4*L/146097;
  L= L-(146097*N+3)/4;
  I= 4000*(L+1)/1461001;
  L= L-1461*I/4+31;
  J= 80*L/2447;
  K= L-2447*J/80;
  L= J/11;
  J= J+2-12*L;
  I= 100*(N-49)+I+L;

  *yr = I;
  *mo = J;
  *day = K;
  return;
}


/*
 *  maxDayInMonth:
 *  	Use fixed array and julianDate and gregorianDate to compute
 *	the maximum day in a given month/year
 *	exit(1) on failure.
 *
 *  Months are in the 1..12 range and NOT 0..11
 *
*/
int maxDayInMonth(int yr, int mo) {
  static int maxDaysInMonths[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  int gy, gm, gday, jd, day;

  /* if not feb, return fixed value from array */
  if (mo != 2) {
    return maxDaysInMonths[mo-1];
  }
  for (day = 30; day > 26; day--) {
    jd = julianDate(yr, mo, day);
    gregorianDate(jd, &gy, &gm, &gday);
    if (gy == yr && gm == mo ) {
      return day;
    }
  }
  fprintf(stderr, "Bad logic in maxDayInMonth\n");
  exit(1);
  return 0;			/* keep gcc happy */
}


/*
** getJDFromDate:
** 	Given a string of YYYY/MM/DD:HH:MM:SS
**	return a julian date calculated from 0 for 12/1/1999.
** 	If the hour (etc) are given return the hour in the
** 	int pointer vaiable
** Input:
**	Date string
** Outputs:
**	JD as returned value.
** Side Effect:
**	the int pointer argument is filled with the hour if given
**	or -1 otherwise.
**	Exits on error.
*/
int getJDFromDate(char *line, int *hour) {
  int yr, mo, day;
  char *cp, *ep;
  cp = line;
  yr = (int) strtol(cp, &ep, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Invalid year in %s\n", line);
    exit(1);
  }
  if (yr < 1999 || yr > 1999 + MAX_YEARS) {
    fprintf(stderr, "Year < 1999 or > %d\n", 1999 + MAX_YEARS);
    exit(1);
  }
  
  cp = ep + 1;
  mo = (int) strtol(cp, &ep, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Invalid month in %s\n", line);
    exit(1);
  }
  if (mo < 1 || mo > 12) {
    fprintf(stderr, "Month < 1 or > 12\n");
    exit(1);
  }
  
  cp = ep + 1;
  day = (int) strtol(cp, &ep, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Invalid day in %s\n", line);
    exit(1);
  }
  if (day < 1 || day > 31) {
    fprintf(stderr, "Month < 1 or > 31\n");
    exit(1);
  }

  /* grab hour if available */
  if (*ep == ':') {
    cp = ep + 1;
    *hour = (int) strtol(cp, (char **)NULL, 10);
    if (errno == ERANGE) {
      fprintf(stderr, "Invalid hour in %s\n", line);
      exit(1);
    }
    if (*hour < 0 || *hour > 23 ) {
      fprintf(stderr, "Invalid hour in %s. Must be in 0-23.\n", line);
      exit(1);
    }
      
  } else {
    *hour = -1;			/* default */
  }
  return julianDate(yr, mo, day);
}
