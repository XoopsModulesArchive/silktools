#ifndef _RWCOUNT_H
#define _RWCOUNT_H
/*
**  Copyright (C) 2001,2002,2003 by Carnegie Mellon University.
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
** 2003/12/10 22:00:47
** thomasm
*/

/*@unused@*/ static char rcsID_RWCOUNT_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "rwcount.h,v 1.9 2003/12/10 22:00:47 thomasm Exp";


/*
 * rwcount.h
 * header file for the rwcount utility.
*/


/*
 * General parms
*/

#define RWCO_VERSION 1
/*
 * argument parameters.
*/
#define EPOCH 0
#define INDEX 1
#define GMT 2
#define BINARY 3

/*
 * bin loading schemata
*/
#define LOAD_MEAN     0
#define LOAD_START    1
#define LOAD_END      2
#define LOAD_MIDDLE   3

/*
 * bin geometry.
*/
#define DEFAULT_BINSIZE 30
#define ROW_BINS 3
#define RECS     0
#define BYTES    1
#define PACKETS  2

/*
 * Application options.
*/
#define RWCO_BIN_SIZE 0
#define RWCO_BIN_LOAD 1
#define RWCO_SKIPZ 2
#define RWCO_DELIM 3
#define RWCO_PRINTFNAME 4
#define RWCO_NOTITLE 5
#define RWCO_EPOCHTIME 6
#define RWCO_BINTIME 7
#define RWCO_FIRSTEPOCH 8
/*
 * Miscellaneous handy constants
*/
#define DAY_SECS 86400

#define RWCO_ERR_GENERAL -1
#define RWCO_ERR_MALLOC  -2
#define RWCO_ERR_FILE    -3

#define RWCO_INTUITIVE 0
#define RWCO_TIMEBORDER 2

/*
 * Flags for use during the actual running of the application.
*/
extern int dryRunFlag,            /* dry run the data */
  printFNameFlag,                 /* print the filename */
  printStatsFlag,                 /* print statistics */
  printTitleFlag,
  binStampFlag;                   /* how to stamp the bins */
extern uint32_t defaultBinSize;            /* binsize, seconds */
extern uint32_t binLoadMode;        /* bin loading scheme */
extern uint32_t skipZeroFlag;                  /* Skip zero-entry records */
extern uint32_t zeroFlag;
extern char delimiter;
/*
 * countFile IO
 * data Structures
*/
typedef struct countFileHeader {
  genericHeader gHdr;
  uint32_t binSize, totalBins, initOffset;
} countFileHeader;

typedef struct countFile {
  countFileHeader *fileHeader;
  double *bins;
} countFile;

/*
 * functions
*/
#define RWCO_OVERWRITE 1
extern int readCountFile(char *, countFile *);
extern int writeCountFile(char *, countFile *, int);

#endif /* _RWCOUNT_H */
