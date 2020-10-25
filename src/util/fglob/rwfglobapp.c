/*
**  Copyright (C) 2003-2004 by Carnegie Mellon University.
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
** 1.3
** 2004/03/10 22:29:58
** thomasm
**
*/

/*
**  rwfglob
**
**  A utility to simply print to stdout the list of files that fglob
**  would normally return.  If CHECK_BLOCK_COUNT is defined, this
**  application code will stat() each file and print the string "ON
**  TAPE" if its block size is 0.
*/


#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"
#include "rwpack.h"
#include "fglob.h"

RCSIDENT("rwfglobapp.c,v 1.3 2004/03/10 22:29:58 thomasm Exp");

/* local defines and typedefs */

#define	NUM_REQUIRED_ARGS 2

/* if true, stat() to get block size and print ON TAPE if size==0 */
#define CHECK_BLOCK_COUNT 1


/* exported variables */


/* required by optionsSetup: this app has no options beyond those that
 * the fglob library provides */
void no_op(void) {}


/*
 * printFileNames()
 *
 *  output function: simply prints the name of each globbed file
*/
static void printFileNames(void) {
  char *fname;
  int numFiles = 0;

  while( (fname = fglobNext()) != (char *)NULL ) {
    fprintf(stdout, "%s\n", fname);
    numFiles++;
  }

  fprintf(stdout, "globbed %d files\n", numFiles);
}


/*
 * printFileNamesCheckBlocks()
 *
 *  output function: for each globbed file; stat() it and check if its
 *  block size; if * zero, flag the file as being "ON TAPE".
*/
static void printFileNamesCheckBlocks(void) {
  struct stat statbuf;
  char *fname;
  int numFiles = 0;
  int numOnTape = 0;

  while( (fname = fglobNext()) != (char *)NULL ) {
    if (-1 == stat(fname, &statbuf)) {
      /* should never happen; fglob wouldn't have returned it */
      fprintf(stderr, "Cannot stat '%s'\n", fname);
      exit(1);
    }
    if (0 == statbuf.st_blocks) {
      fprintf(stdout, "%s  \t*** ON_TAPE ***\n", fname);
      ++numOnTape;
    } else {
      fprintf(stdout, "%s\n", fname);
    }
    numFiles++;
  }

  fprintf(stdout, "globbed %d files; %d on tape\n", numFiles, numOnTape);
}


int main(int argc, char **argv)
{
  skAppRegister(argv[0]);

  if (argc < NUM_REQUIRED_ARGS) {
    fglobUsage();
    return 1;
  }

  if (optionsSetup(no_op)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    return 1;
  }
  
  if (fglobSetup()){
    fprintf(stderr, "fglob: error\n");
    return 1;
  }

  if (optionsParse(argc, argv) < 0) {
    fglobUsage();
    return 1;
  }

#if CHECK_BLOCK_COUNT
  printFileNamesCheckBlocks();
#else /* CHECK_BLOCK_COUNT */
  printFileNames();
#endif /* CHECK_BLOCK_COUNT */
  
  fglobTeardown();

  return 0;
}
