#ifndef _BITS_H
#define _BITS_H
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
** 1.6
** 2003/12/10 22:00:39
** thomasm
*/

/*@unused@*/ static char rcsID_BITS_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "bits.h,v 1.6 2003/12/10 22:00:39 thomasm Exp";


#ifdef	BITS_INITIALIZE

#if	0
static uint32_t  bitmasks[32] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
  0x010000, 0x020000, 0x040000, 0x080000, 0x100000, 0x200000, 0x400000, 0x800000,
  0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000,
  0x40000000, 0x80000000
};
#define   SETBIT(a,b) a = a | bitmasks[b%32]
#define  TESTBIT(a,b) (a & bitmasks[b%32])
#define	CLEARBIT(a,b) a = a & (~bitmasks[b%32])
#endif


/* bitMask: masks to extract bit n from the given sized arg
   define format: bitNof<wordlength>[<wordlength>]
   where bitnumber is 1-8; 1-16; 1-32; 1-64
   and wordlength is 8, 16; 32; 64
*/
int bitNof8[8] = {
  0x80,
  0x40,
  0x20,
  0x10,
  0x08,
  0x04,
  0x02,
  0x01
};
int bitNof16[16] = {
  0x8000,
  0x4000,
  0x2000,
  0x1000,
  0x0800,
  0x0400,
  0x0200,
  0x0100,
  0x0080,
  0x0040,
  0x0020,
  0x0010,
  0x0008,
  0x0004,
  0x0002,
  0x0001
};
int bitNof32[32] = {
  0x80000000,
  0x40000000,
  0x20000000,
  0x10000000,
  0x08000000,
  0x04000000,
  0x02000000,
  0x01000000,
  0x00800000,
  0x00400000,
  0x00200000,
  0x00100000,
  0x00080000,
  0x00040000,
  0x00020000,
  0x00010000,
  0x00008000,
  0x00004000,
  0x00002000,
  0x00001000,
  0x00000800,
  0x00000400,
  0x00000200,
  0x00000100,
  0x00000080,
  0x00000040,
  0x00000020,
  0x00000010,
  0x00000008,
  0x00000004,
  0x00000002,
  0x00000001
};
#else
int bitNof8[8];
int bitNof16[16];
int bitNof32[32];
#endif

/*
**  masks for bit field manipulation: these masks will mask out the specified
**  number of bits from the right.  I.e., MASKARRAY_01 masks off the rightmost
**  bit; MASKARRAY_09 masks off the rightmost 9 bits etc.
*/
#define MASKARRAY_01	1
#define MASKARRAY_02	3
#define MASKARRAY_03	7
#define MASKARRAY_04	15
#define MASKARRAY_05	31
#define MASKARRAY_06	63
#define MASKARRAY_07	127
#define MASKARRAY_08	255
#define MASKARRAY_09	511

#define MASKARRAY_10	1023
#define MASKARRAY_11	2047
#define MASKARRAY_12	4095
#define MASKARRAY_13	8191
#define MASKARRAY_14	16383
#define MASKARRAY_15	32767
#define MASKARRAY_16	65535
#define MASKARRAY_17	131071
#define MASKARRAY_18	262143
#define MASKARRAY_19	524287

#define MASKARRAY_20	1048575
#define MASKARRAY_21	2097151
#define MASKARRAY_22	4194303
#define MASKARRAY_23	8388607
#define MASKARRAY_24	16777215
#define MASKARRAY_25	33554431
#define MASKARRAY_26	67108863
#define MASKARRAY_27	134217727
#define MASKARRAY_28	268435455
#define MASKARRAY_29	536870911

#define MASKARRAY_30	1073741823
#define MASKARRAY_31	2147483647

#endif /* _BITS_H */
