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
 * setcount.h
 * header file for sets.
 */
#ifndef _SETCOUNT_H
#define _SETCOUNT_H

/*@unused@*/ static char rcsID_RWADDRCOUNT_H[]
#ifdef __GNUC__
__attribute__((__unused__))
#endif
     = "rwaddrcount.h,v 1.2 2003/12/10 22:00:47 thomasm Exp";
#define MAX_STD_RECS          10
#define RWAC_PMODE_NONE       0 
#define RWAC_PMODE_IPS        1
#define RWAC_PMODE_RECS      2
#define RWAC_PMODE_STAT       3

#define RWACOPT_NOREPORT      0
#define RWACOPT_USEDEST       1
#define RWACOPT_PRINTIPS      2
#define RWACOPT_BYTEMIN       3
#define RWACOPT_PKTMIN        4
#define RWACOPT_RECMIN        5
#define RWACOPT_BYTEMAX       6
#define RWACOPT_PKTMAX        7
#define RWACOPT_RECMAX        8
#define RWACOPT_PRINTRECS     9
/*
 * 12/2 note - one of the problems I've had recently is a tendency to 
 * focus on doing the perfect hash.  The perfect hash is nice, but what 
 * we need RIGHT HERE RIGHT NOW is a tool that'll actually do the job.  Enough
 * mathematical wanking.
 *
 * So, this is a hash whose collision compensation algorithm is linear chaining.  I'm not happy
 * about it, but it'll do for now. 
 */
typedef struct countRecord {
  uint32_t value; /* IP address, or whatever we don't consider source or dest here*/
  uint32_t bytes; /* total number of bytes */
  uint32_t packets;/* total number of packets */
  uint32_t flows;  /* total number of records */
  uint32_t start; /* start time - epoch*/
  uint32_t end; /* total time lasted */
  struct countRecord *nextRecord; /* Pointer to the next record for collision */
} countRecord;
#endif
