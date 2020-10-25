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
** 1.3
** 2004/03/10 22:59:49
** thomasm
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rwpack.h"
#include "dynlib.h"
#include "filter.h"


/* local functions */


/*
** setup:
**	Called with no arguments.  Prepare  what is required here.
** Inputs: None
** Outputs: Return one of:
**	DYNLIB_FAILED:
**	  for whatever reason
**	DYNLIB_WILLPROCESS:
**	  if this dynamic library will check AND process the data
**	DYNLIB_WONTPROCESS:
**	  if this dynamic library will only check the record.
**
*/
int setup() {
  return DYNLIB_WONTPROCESS;
}

/*
** check:
** 	Check rwRecPtr.  Mayne process it as well.
** Input: rwRecPtr
** 	Output: 0 if passed (i.e., it pased the filter). 1 else
**	If you are also processing, return 0.
** Side Effects:
**
*/
int check(rwRecPtr rwRP) {
  if (rwRP->proto != 17) {
    return 1;			/* reject */
  }
  if (rwRP->dPort == 1434) {
    return 0;			/* wanted */
  }
  return 1;			/* reject also */
    
}

/*
** teardown:
**	Done with all input files.  Do the final processing.
**	If this lib was processing records, this is the only chance
**	to get out the results.
** Inputs: None
** Outputs: None
** NOTE: This must be idempotent.
*/

void teardown() {
  static int teardownFlag = 0;
  if (teardownFlag) {
    return;
  }
  return;
}
