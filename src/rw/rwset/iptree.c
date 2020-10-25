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
** 1.15
** 2004/03/10 23:07:18
** thomasm
*/

/*
 * iptree.c
 *
 * This is a data structure for providing marked sets of ip addresses
 * in a tree format.
 */

#include "silk.h"

#include <stdlib.h>
#include <string.h>

#include "iptree.h"

RCSIDENT("iptree.c,v 1.15 2004/03/10 23:07:18 thomasm Exp");


/*
 * NAME
 *
 * initMacroTree(macroTree *)
 *
 * ABSTRACT
 *
 * Initializes a macroTree as empty
 *
 * PARAMETERS
 *
 * macroTree *newTree - pointer to the tree to initialize
 *
 * RETURNS
 *
 * Nothing
 *
 * SIDE EFFECTS
 *
 * NONE.
 *
 * FAILS
 *
 * NOTES
 *
 * If you want to ditch a tree, use macroTreeFree
*/
int initMacroTree(macroTree *newTree) {
  newTree->totalAddrs = 0;
  memset(newTree->macroNodes, '\0', sizeof(uint32_t) * 65536);
  return 0;
}

/*
 * NAME
 *
 * deleteMacroTree(macroTree *oldTree)
 *
 * ABSTRACT
 *
 * macroTree deletion routine
 *
 * PARAMETERS
 * oldTree - tree to nuke
 *
 * RETURNS
 * nothing
 *
 * SIDE EFFECTS
 * Lots of free calls.
 *
 * FAILS
 *
 * NOTES
*/
int freeMacroTree(macroTree *oldTree) {
  int i;
  for(i = 0; i < 65535; i++) {
    if(oldTree->macroNodes[i] != NULL) {
      free(oldTree->macroNodes[i]);
      oldTree->macroNodes[i] = NULL;
    }
  }
  return 0;
}

/*
 * NAME
 *
 * writeMacroTree(FILE* dataFile, macroTree *oldTree)
 *
 * ABSTRACT
 *
 * This writes a macro tree to disk.  Macro trees are stored
 * in a class-b format of the following form:
 * <class b>
 * <number of class c's to write>
 * <class c's>
 * Where each class c is written as
 * <block in class b to write>
 * <bitmask>
 *
 * PARAMETERS
 *
 * RETURNS
 *
 * SIDE EFFECTS
 *
 * FAILS
 *
 * NOTES
*/
int writeMacroTree(FILE *dataFile, macroTree *tgtTree) {
  uint32_t i, j, jOffset;
  uint32_t addrSet;
  macroTreeHeader tgtHeader;
  macroNode *workNode;
  /*
   * Write header here
   * put in later.
   */
  PREPHEADER(&(tgtHeader.gHdr));
  tgtHeader.gHdr.type = FT_MACROTREE;
  /*
   * Blocks are written in the form
   * <a><b><c>0<block>
   * where a, b, and c are the first 3 octets.
   */
  fwrite((void *) (&tgtHeader), sizeof(macroTreeHeader), 1, dataFile);
  workNode = NULL;
  for(i = 0 ; i < 65536; i++) {
    workNode = tgtTree->macroNodes[i];
    if(workNode != NULL) {
      /*
       * non null node we now walk through the node to see what
       * is here, and write any non-null children to disk.
       */
      j = 0;
      do {
	if(workNode->addressBlock[j]) {
	  addrSet = ((i << 16) | ((j >> 3) << 8)) & 0xFFFFFF00;
	  fwrite((void *) &addrSet, sizeof(uint32_t), 1, dataFile);
	  jOffset = (j & 0x7F8);
	  fwrite((void *) (workNode->addressBlock + jOffset),
		 sizeof(uint32_t), 8, dataFile);
	  j = ((j & 0x7F8) + 8);
	} else {
	  j = j + 1;
	};
      } while (j < 2048);
    }
  }
  return 0;
}

/*
 * NAME
 *
 * readMacroTree(FILE *dataFile, macroTree *macroTree)
 *
 * ABSTRACT
 *
 * PARAMETERS
 *
 * RETURNS
 *
 * SIDE EFFECTS
 *
 * FAILS
 *
 * NOTES
*/
int readMacroTree(FILE *dataFile, macroTree *macroTree) {
  macroTreeHeader th;
  uint32_t rootAddr, blockStart;
  uint32_t tBuffer[9];
  int i;
  int swapFlag;

  /* Read header; determine if we need to set swapFlag */
  if (!fread((void *) &th, sizeof(macroTreeHeader), 1, dataFile)) {
    fprintf(stderr, "readMacroTree: cannot read macroTreeHeader\n");
    return 1;
  }

  if (CHECKMAGIC(&(th.gHdr))) {
    fprintf(stderr, "readMacroTree: invalid header\n");
    return 1;
  }

  if (FT_MACROTREE != th.gHdr.type) {
    fprintf(stderr, "readMacroTree: file is not an RWSET file\n");
    return 1;
  }

#if IS_LITTLE_ENDIAN
  swapFlag = th.gHdr.isBigEndian;
#else
  swapFlag = !th.gHdr.isBigEndian;
#endif

  /*
   * Now we loop, reading 36 bytes at a time.
   */
  if (!swapFlag) {
    while(fread((void *) tBuffer, sizeof(uint32_t), 9, dataFile)) {
      rootAddr = (tBuffer[0] & 0xFFFF0000) >> 16;
      blockStart = (tBuffer[0] >> 8) & 0xFF;
      if(macroTree->macroNodes[rootAddr] == ((macroNode *) NULL)) {
        macroTree->macroNodes[rootAddr] =(macroNode*)malloc(sizeof(macroNode));
        memset(macroTree->macroNodes[rootAddr]->addressBlock, '\0', 8192);
      }
      memcpy((macroTree->macroNodes[rootAddr]->addressBlock +(blockStart * 8)),
             tBuffer + 1, 32);
    }
  } else {
    while(fread((void *) tBuffer, sizeof(uint32_t), 9, dataFile)) {
      for (i = 0; i < 9; ++i) {
        tBuffer[i] = BSWAP32(tBuffer[i]);
      }
      rootAddr = (tBuffer[0] & 0xFFFF0000) >> 16;
      blockStart = (tBuffer[0] >> 8) & 0xFF;
      if(macroTree->macroNodes[rootAddr] == ((macroNode *) NULL)) {
        macroTree->macroNodes[rootAddr] =(macroNode*)malloc(sizeof(macroNode));
        memset(macroTree->macroNodes[rootAddr]->addressBlock, '\0', 8192);
      }
      memcpy((macroTree->macroNodes[rootAddr]->addressBlock +(blockStart * 8)),
             tBuffer + 1, 32);
    }
  }
  return 0;
}
/*
 * NAME printMacroIPs
 *
 * ABSTRACT
 *
 * prints out a set of IP addresses to the file descriptor provided in the
 * parameters.
 *
 * PARAMETERS
 *
 * FILE * outFile - targeted file descriptor
 * int integerIP - 0 to print as dotted quad; 1 to print as uint32_t
 *
 * RETURNS
 *
 * SIDE EFFECTS
 *
 * FAILS
 *
 * NOTES
*/
void printMacroIPs(FILE *outFile, macroTree *tgtTree, int integerIP) {
  int i, j;
  for(i = 0; i < 65536; i++) {
    if(tgtTree->macroNodes[i] != NULL) {
      for(j = 0; j < 65536; j++) {
	if(macroTreeHasMark(j, tgtTree->macroNodes[i]->addressBlock)) {
          if (integerIP) {
            fprintf(outFile, "%u\n", (i << 16) | j);
          } else {
            fprintf(outFile, "%d.%d.%d.%d\n", (i >> 8) & 0xFF,
                    i & 0xFF, (j >> 8) & 0xFF,
                    j & 0xFF);
          }
	}
      }
    }
  }
}

/*
 * NAME countMacroIPs
 *
 * ABSTRACT
 *   Returns a count of the number of IP addresses in the given macroTree
 *
 * PARAMETERS
 *   macroTree *tgtTree - macroTree to count IPs in
 *
 * RETURNS
 *   count of unique IPs
 *
 * SIDE EFFECTS
*/
uint32_t countMacroIPs(macroTree *tgtTree) {
  int i, j, k;
  uint32_t count = 0;

  for (i = 0; i < 65536; i++) {
    if (tgtTree->macroNodes[i] != NULL) {
      /* Break the abstraction to speed up the counting */
      for (j = 0; j < 65536 / 32; ++j) {
        if (tgtTree->macroNodes[i]->addressBlock[j]) {
          for (k = 0; k < 32; ++k) {
            if (tgtTree->macroNodes[i]->addressBlock[j] & (1 << k)) {
              ++count;
            }
          }
        }
      }
    }
  }

  return count;
}
/*
 * NAME skIptreePrintStatistics
 *
 * ABSTRACT
 *   Prints, to outF, statistics of the macroTree tgtTree.  Statistics
 *   printed are the minimum IP, the maximum IP, a count of the
 *   macroNodes (class B blocks) used, and a count of the
 *   addressBlocks (/27's) used.  If integerIP is 0, the min and max
 *   IPs are printed in dotted-quad form; otherwise they are printed
 *   as integers.
 *
 * PARAMETERS
 *   FILE *outF - stream to print to
 *   macroTree *tgtTree - macroTree to examine
 *   int integerIP - 0 to print IPs as dotted-quad; 1 to print as integer
 *
 * RETURNS
 *   count of Class B address blocks
 *
 * SIDE EFFECTS
*/
void skIptreePrintStatistics(FILE *outF,macroTree *tgtTree,int integerIP)
{
  int i, j, k;
  int printedMin = 0;
  int printedMax = 0;
  uint32_t macroNodeCount = 0;
  uint32_t addressBlockCount = 0;
  uint32_t macroNodeMax = 0;

  for (i = 0; i < 65536; i++) {
    if (tgtTree->macroNodes[i] != NULL) {
      ++macroNodeCount;
      macroNodeMax = i;
      for (j = 0; j < 65536 / 32; ++j) {
        if (tgtTree->macroNodes[i]->addressBlock[j]) {
          ++addressBlockCount;
          for (k = 0; k < 32; ++k) {
            if (tgtTree->macroNodes[i]->addressBlock[j] & (1 << k)) {
              if (!printedMin) {
                printedMin = 1;
                fprintf(outF, "\tminimumIP = ");
                if (integerIP) {
                  fprintf(outF, "%u\n", (i << 16) | (j * 32 + k));
                } else {
                  fprintf(outF, "%d.%d.%d.%d\n",
                          ((i >> 8) & 0xFF), (i & 0xFF),
                          (((j * 32) >> 8) & 0xFF), ((j * 32 + k) & 0xFF));
                }
              }
            }
          }
        }
      }
    }
  }

  if ( !macroNodeCount ) {
    /* empty tree */
    return;
  }

  i = macroNodeMax;
  for (j = 65536/32 - 1; j >= 0 && !printedMax; --j) {
    if (tgtTree->macroNodes[i]->addressBlock[j]) {
      for (k = 31; k >= 0; --k) {
        if (tgtTree->macroNodes[i]->addressBlock[j] & (1 << k)) {
          printedMax = 1;
          fprintf(outF, "\tmaximumIP = ");
          if (integerIP) {
            fprintf(outF, "%u\n", (i << 16) | (j * 32 + k));
          } else {
            printf("%d.%d.%d.%d\n", ((i >> 8) & 0xFF), (i & 0xFF),
                   (((j * 32) >> 8) & 0xFF), ((j * 32 + k) & 0xFF));
          }
          break;
        }
      }
    }
  }

  fprintf(outF, "\tcount of /16's = %u (%f %%)\n",
          macroNodeCount, (double)macroNodeCount/65536.0*100.0);
  fprintf(outF, "\tcount of /27's = %u (%f %%)\n",
          addressBlockCount,
          (double)addressBlockCount/(double)macroNodeCount/2048.0*100.0);

  return;
}

/*
 * NAME
 *
 * int readMacroAsMicro(FILE *dataFile, macroTree *dataTree)
 *
 * ABSTRACT
 *
 * reads a macroTree from disk and converts it into a microtree - a tree
 * that can be used for statistical information and analysis rather than
 * large scale crud.
 *
 * PARAMETERS
 *
 * FILE *dataFile - pointer to the macrotree in question
 * microTree *tgtTree - pointer to the microtree in question
 *
 * RETURNS
 *
 * 0 - successful read
 * 1 - some failure
 *
 * SIDE EFFECTS
 *
 * FAILS
 *
 * NOTES
*/
#if UNIMPLEMENTED
int readMacroAsMicro(FILE *dataFile, macroTree *dataTree) {
  /*
   * the first thing we do is identify all the class 16 subnets.
   */
}
#endif /* UNIMPLEMENTED */
