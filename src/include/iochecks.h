#ifndef _IOCHECKS_H
#define _IOCHECKS_H
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
** 1.7
** 2003/12/10 22:00:44
** thomasm
*/

/*@unused@*/ static char rcsID_IOCHECKS_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "iochecks.h,v 1.7 2003/12/10 22:00:44 thomasm Exp";



#include "silk.h"

#define MAX_PASS_DESTINATIONS 2
#define MAX_FAIL_DESTINATIONS 2
#define MAX_ALL_DESTINATIONS  1


typedef struct iochecksInfoStruct {
  uint8_t devnullUsed;
  uint8_t passCount;
  uint8_t failCount;
  uint8_t stdinUsed;
  uint8_t stderrUsed;
  uint8_t stdoutUsed;
  uint8_t maxPassDestinations;
  uint8_t maxFailDestinations;
  int firstFile;
  int fileCount;
  FILE * passFD[MAX_PASS_DESTINATIONS];
  char * passFPath[MAX_PASS_DESTINATIONS];
  int  passIsPipe[MAX_PASS_DESTINATIONS];
  FILE * failFD[MAX_FAIL_DESTINATIONS];
  char * failFPath[MAX_FAIL_DESTINATIONS];
  int  failIsPipe[MAX_FAIL_DESTINATIONS];
  int inputPipeFlag;
  FILE * inputCopyFD;
  char * inputCopyFPath;
  char **fnArray;
  char **argv;
  int  argc;
} iochecksInfoStruct;
typedef iochecksInfoStruct * iochecksInfoStructPtr;

iochecksInfoStructPtr iochecksSetup(int maxPass, int maxFail, int argc,
                                    char **argv);
int iochecksFailDestinations(iochecksInfoStructPtr ioISP, char *fPath, int ttyOK);
int iochecksPassDestinations(iochecksInfoStructPtr ioISP, char *fPath, int ttyOK);
int iochecksAllDestinations(iochecksInfoStructPtr ioISP, char *fPath);
int iochecksInputSource(iochecksInfoStructPtr ioISP, char *fPath);
int iochecksInputs(iochecksInfoStructPtr ioISP, int zeroFilesOK);
void iochecksTeardown(iochecksInfoStructPtr ioISP);

#endif /*  _IOCHECKS_H */
