#ifndef __HASHLIB_UTILS__H
#define __HASHLIB_UTILS__H
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

**  1.6
**  2003/12/10 22:00:43
** thomasm
*/

/*@unused@*/ static char rcsID__HASHLIB_UTILS__H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "hashlib_utils.h,v 1.6 2003/12/10 22:00:43 thomasm Exp";


/* File: hashlib_utils.h */



#include <stdio.h>

#ifdef ENABLE_LOGGING
FILE *g_ilog_fp;
#define OPEN_LOG(fp, fn_str, flags_str) fp = fopen(fn_str, flags_str)
#define LOG0(fp, fs) fprintf(fp, fs)
#define LOG1(fp, fs, arg1) fprintf(fp, fs, arg1)
#define LOG2(fp, fs, arg1, arg2) fprintf(fp, fs, arg1, arg2)
#define LOG3(fp, fs, arg1, arg2, arg3) fprintf(fp, fs, arg1, arg2, arg3)
#define LOG_BYTES(fp, dp, n) { int j; for (j=0;j<n;j++) { fprintf (fp, "%02x ", dp[j]); } }
#define CLOSE_LOG(fp) fclose(fp)
#else
#define OPEN_LOG(fp,file_name_str, flags_str)
#define LOG0(fp, fs)
#define LOG1(fp, fs, a1)
#define LOG2(fp, fs, a1, a2)
#define LOG3(fp, fs, a1, a2, a3)
#define LOG_BYTES(fp, data_ptr, num_bytes)
#define CLOSE_LOG(fp)
#endif

#endif /* __HASHLIB_UTILS__H */
