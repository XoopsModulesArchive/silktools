#ifndef _RWSWAPBYTES_H
#define _RWSWAPBYTES_H
/*
**  Copyright (C) 2003 by Carnegie Mellon University.
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
** 2003/12/10 22:00:50
** thomasm
** opensrc-sourceforge-20040319
*/

/*@unused@*/ static char rcsID_RWSWAPBYTES_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "rwswapbytes.h,v 1.4 2003/12/10 22:00:50 thomasm Exp";

/*
 * rwswapbytes
 *
 * Read any rw file (rwpacked file, rwfilter output, etc) and output a
 * file in the specified byte order.
 *
 * This header for is for application use only.
 *
*/

extern char *appName;
extern FILE *outF;
void swapRouted(rwIOStructPtr rwIOSPtr);
void swapNotRouted(rwIOStructPtr rwIOSPtr);
void swapSplit(rwIOStructPtr rwIOSPtr);
void swapAcl(rwIOStructPtr rwIOSPtr);
void swapFilter(rwIOStructPtr rwIOSPtr);
void swapWeb(rwIOStructPtr rwIOSPtr);
void swapGeneric(rwIOStructPtr rwIOSPtr);
void fileCopy(rwIOStructPtr rwIOSPtr);

#endif /* _RWSWAPBYTES_H */