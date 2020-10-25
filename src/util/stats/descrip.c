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
** 1.4
** 2004/03/10 22:23:52
** thomasm
*/



/*
 *  descrip.c
 *
 *  calculate descriptive statistics for the given input stream.  The
 *  data are assumed to be separated by spaces. The number of
 *  variables and observations have to be deduced by the
 *  program. However, all records are assumed to have the same number
 *  of variables.  Finally, all variables are assumed to be int's
 *  only.
 *
 */


#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include "utils.h"

RCSIDENT("descrip.c,v 1.4 2004/03/10 22:23:52 thomasm Exp");


#define	INITIAL_SIZE	4096
#define GROWTH_FACTOR   2

typedef long WORD;
typedef struct descriptiveStatsStruct {
  uint64_t sumX;
  uint64_t sumX2;
  uint64_t sumX3;
  uint64_t sumX4;
  uint32_t min;
  uint32_t max;
  uint32_t (*data)[1];
}descriptiveStatsStruct;
typedef descriptiveStatsStruct *descriptiveStatsStruct_Ptr;

/* module globals */
static int shutdownFlag = 0;
static char *inFName;
static FILE *inF = (FILE *)NULL;
static int inIsPipe;
static int numVars;
static int numObs;
static descriptiveStatsStruct (*stats)[1];
static int32_t curSize;		/* size of all the data arrays in the structs */
static int32_t curIndex;		/* where to put the stuff */

/* 	internal functions */
int descrip(FILE *inFile);
static void calcStats(void);

/* external functions */
float fmedian(float x[], int n);
float imedian(int32_t x[], int n);
void lqsort(char *a, size_t n, size_t es);

/*#ifdef	TEST_DESCRIP */
#if	1
void shutdown(void) {
  if (shutdownFlag) {
    return;
  }
  shutdownFlag = 1;
  if (inFName) {
    if (inIsPipe) {
      pclose(inF);
    } else {
      fclose(inF);
    }
    inF = (FILE *)NULL;
  }
  return;
}

void usage(void) {
  fprintf(stderr, "Usage: %s <inputfile>\n", skAppName());
  exit(1);

}

void setup(int argc, char **argv) {
  skAppRegister(argv[0]);
  if (argc < 2) {
    usage();
  }

  MALLOCCOPY(inFName, argv[1]);

  if (openFile(inFName, 0 /* read */, &inF, &inIsPipe)) {
    exit(1);
  }
  if (atexit(shutdown)) {
    fprintf(stderr, "Unable to set shutdown function\n");
    exit(1);
  }
  return;
}


int main(int argc, char **argv) {
  setup(argc, argv);		/* never returns on error */

  descrip(inF);
  shutdown();
  exit(0);
  return 0;
}
#endif

int descrip(FILE *inFile) {
  char line[512];
  char *cp, *ep;
  uint32_t i, obs;
  uint64_t p;			/* temp vars for square, cube etc. */
  descriptiveStatsStruct_Ptr dsPtr;


  numVars = numObs = 0;

  /* read one record to find out how many variable there are and set up */
  if (! fgets(line, sizeof(line), inFile)) {
    fprintf(stderr, "Unable to read from inputfile: [%s]\n", strerror(errno));
    return 1;
  }

  cp = line;
  while(1) {
    i = (int32_t) strtoul(cp, &ep, 10);
    numVars++;
    if (*ep == '\0' || *ep == '\n') {
      break;
    }
    cp = 1 + ep;
  }
  if ( ! (stats = (descriptiveStatsStruct (*)[1]) malloc(numVars * sizeof(descriptiveStatsStruct)))) {
    fprintf(stderr, "Out of mem\n");
    return 1;
  }

  memset(stats, 0, sizeof(numVars * sizeof(descriptiveStatsStruct)));
  for (i = 0; i < numVars; i++) {
    (*stats)[i].min = 0xFFFFFFFF;
  }

  numObs = 1;
  curSize = 0;
  curIndex = 0;

  while (1) {
    /* get space for the data arrays in the struct in the stats array */
    if (curIndex >= curSize) {
      fprintf(stderr, "Growing from %u", curSize);
      curSize = curSize ? curSize * GROWTH_FACTOR : INITIAL_SIZE;
      fprintf(stderr, " to %u\n", curSize);
      for (i = 0; i < numVars; i++) {
	dsPtr = &(*stats)[i];
	if ( ! ( dsPtr->data = realloc(dsPtr->data, (sizeof(int32_t) * curSize)))) {
	  fprintf(stderr, "Out of mem after %u records\n", numObs);
	  return 1;
	}
      }
    }

    cp = line;
    for (i = 0; i < numVars; i++) {
        obs = (int32_t) strtoul(cp, &ep, 10);
	cp = 1 + ep;

	/* calculate the various numbers for each var */
	dsPtr = &(*stats)[i];
	dsPtr->sumX += obs;
	dsPtr->sumX2 += (p = obs * obs);
	dsPtr->sumX3 += p*obs;
	dsPtr->sumX4 += p*p;
	if (dsPtr->min > obs) {
	  dsPtr->min = obs;
	}
	if (dsPtr->max < obs) {
	  dsPtr->max = obs;
	}
	(*dsPtr->data)[curIndex] = obs;
    }

    curIndex++;

    /* get the next line */
    if (! fgets(line, sizeof(line), inFile)) {
      break;			/* EOF */
    }
    numObs++;
  }

  calcStats();
  return 0;
}


static void calcStats(void) {
  uint32_t i,j;
  double mean, var, median, kurt, skew, stddev, meanSQ, p;
  double s, s2, s3, s4;
  descriptiveStatsStruct_Ptr dsPtr;

  fprintf(stdout, "Number of observations: %u\n", numObs);
  fprintf(stdout, "Min Max Mean Median Variance StdDev Skewness Kurtosis\n");

  for (i = 0; i < numVars; i++) {
    dsPtr = &(*stats)[i];
    s = (double)dsPtr->sumX;
    s2 = (double)dsPtr->sumX2;
    s3 = (double)dsPtr->sumX3;
    s4 = (double)dsPtr->sumX4;
    mean = s/numObs;
    var =  (s2 - (numObs * mean * mean) ) / (numObs - 1);
    stddev = sqrt(var);
    skew = (( s3 - (3 * mean * s2) + (3 * (meanSQ=mean*mean)*s))/numObs)
      - (meanSQ * mean) / (( p = ( ((s2 - (numObs * meanSQ))/ (numObs - 1)) )) * sqrt(p));

    kurt = (( (s4 - (4 * mean * s3) + (6 * meanSQ * s2))
	      - (4 * mean * meanSQ * s)/numObs) + (meanSQ * meanSQ))
      / ((p=((s2 - (numObs * meanSQ))/(numObs-1)))*p);
    /* qsort((char *)(*dsPtr->data), numObs, sizeof(uint32_t), cmp); */
    lqsort((char *)(*dsPtr->data), numObs, sizeof(uint32_t));
    j = numObs/2;
    if ( numObs % 2) {
      /* odd number. median is the middle number */
      median = (double) (*dsPtr->data)[j];
    } else {
      /* interpolate */
      median = ( (double) (*dsPtr->data)[j-1] + (double) (*dsPtr->data)[j])/2.0;
    }
    fprintf(stdout, "Var %u %u %u %f %f %f %f %f %f\n", i, dsPtr->min, dsPtr->max,
	    mean, median, var, stddev, skew, kurt);
    fprintf(stdout, "Imedian vs. median: %f %f \n", imedian((*dsPtr->data), numObs),
	    median);
  }
  return;
}
