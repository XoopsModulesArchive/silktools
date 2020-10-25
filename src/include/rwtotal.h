#ifndef _RWTOTAL_H
#define _RWTOTAL_H
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
** 1.10
** 2004/03/15 18:21:51
** collinsm
*/

/*@unused@*/ static char rcsID_RWTOTAL_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "rwtotal.h,v 1.10 2004/03/15 18:21:51 collinsm Exp";


/*
 * rwtotal.h
 * Various handy header information for rwtotal.
*/


#define RWTO_NONE -1
#define RWTO_COPT_PORT 1
#define RWTO_COPT_ADD8 2
#define RWTO_COPT_ADD16 3
#define RWTO_COPT_LADD8 4
#define RWTO_COPT_LADD16 5
#define RWTO_COPT_PROTO 6
#define RWTO_COPT_ADD24 7
#define RWTO_COPT_DURATION 8
#define RWTO_COPT_ICMP 9
#define RWTO_COPT_BYTES 10
#define RWTO_COPT_PACKETS 11

#define RWTO_SIP_8 0
#define RWTO_SIP_16 1
#define RWTO_SIP_L8 2
#define RWTO_SIP_L16 3
#define RWTO_DIP_8 4
#define RWTO_DIP_16 5
#define RWTO_DIP_L8 6
#define RWTO_DIP_L16 7
#define RWTO_SPORT 8
#define RWTO_DPORT 9
#define RWTO_PROTO 10
#define RWTO_DELIM 11
#define RWTO_SKIPZ 12
#define RWTO_NOTITLES 13
#define RWTO_PRINTFILENAMES 14
#define RWTO_COPYINPUT 15
#define RWTO_SIP_24 16
#define RWTO_DIP_24 17
#define RWTO_ICMPCODE 18
#define RWTO_DURATION 19
#define RWTO_BYTES 20
#define RWTO_PACKETS 21

#define COUNTBINS 256 * 256 * 256
#define C_RECS 0
#define C_BYTES 1
#define C_PKTS 2

extern int countMode;
extern int skipFlag;
extern int noTitlesFlag;
extern int destFlag;
extern char delimiter;
extern int printFileNamesFlag;
extern uint32_t totalRecs;
#endif /* _RWTOTAL_H */
