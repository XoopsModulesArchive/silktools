#ifndef _IPTREE_H
#define _IPTREE_H
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
** 1.8
** 2004/02/06 17:43:15
** thomasm
*/

/*@unused@*/ static char rcsID_IPTREE_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "iptree.h,v 1.8 2004/02/06 17:43:15 thomasm Exp";


/*
 * iptree.h
 *
 * Michael Collins
 * May 6th, 2003
 *
 * This is a tree structure for ip addresses.  It provides both
 * macro and microtree structures.
*/

#include "utils.h"

/*
 * Macrotree
 * a macrotree consists of a 64k-node root, followed by pointers to
 * macronodes, each of which contains 64k spots for marking addresses.
*/
typedef struct macroNode {
  uint16_t parentAddr;
  uint32_t addressBlock[2048];
} macroNode;

typedef struct macroTree {
  uint32_t totalAddrs;
  macroNode *macroNodes[65536];
} macroTree;

/*
 * Microtree
 * The microtree is a classic balanced binary tree; since it's always
 * constructed from a pre-sorted, address-defined macrotree, all we have to
 * do is define the nodal structures.
*/

typedef struct microNode {
  uint32_t addressTemplate; /* 24-bit address Template */
  uint32_t bytes[256];
  uint32_t pkts[256];
  uint32_t flows[256];
} microNode;

typedef struct microTree{
  uint32_t totalAddresses;
  microNode *rootNets[65535];
} microTree;

typedef struct macroTreeHeader{
  genericHeader gHdr;
} macroTreeHeader;

#define macroTreeCheckAddress(ipAddr, m) (m->macroNodes[(ipAddr >> 16)] == NULL ? 0 : macroTreeHasMark((ipAddr & 0xFFFF), m->macroNodes[(ipAddr >> 16)]->addressBlock))
#define macroTreeHasMark(space, dataBlock) (dataBlock[space >> 5] & (1 << (space & 0x1F)))
#define macroTreeMarkSpot(addr, dataBlock) (dataBlock[((addr & 0xFFFF) >> 5)] |= (1 << (addr & 0x1F)))
#define macroTreeAddAddress(addr, dt) (macroTreeMarkSpot(addr, dt->macroNodes[(addr >> 16)]->addressBlock))

int initMacroTree(macroTree *);
int freeMacroTree(macroTree*);
int readMacroTree(FILE *, macroTree *);
int writeMacroTree(FILE *, macroTree *);
void printMacroIPs(FILE *, macroTree *, int);
void skIptreePrintStatistics(FILE *outF,macroTree *tgtTree,int integerIP);
uint32_t countMacroIPs(macroTree *);

#endif /* _IPTREE_H */
