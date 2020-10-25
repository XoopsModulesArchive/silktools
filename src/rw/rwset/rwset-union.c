/*
**  Copyright (C) 2001-2004 by Carnegie Mellon University.
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
** 2004/03/10 23:07:18
** thomasm
*/

/*
 * Application to create the union of two or more rwset files
 */

#include "silk.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "iptree.h"

RCSIDENT("rwset-union.c,v 1.4 2004/03/10 23:07:18 thomasm Exp");


const char usageFormat[] =
  ("%s: <output-file> <input-file1> <input-file2> ... <input-fileN>\n"
   "  Merge the inputs into the output\n");


int main(int argc, char **argv) {
  macroTree newTree;
  macroTree tmpTree;
  FILE *dataFile;
  const char *outputFile = argv[1];
  char *fileName;
  int i, j, k;

  skAppRegister(argv[0]);

  /* Need output file and at least two input files.  Also, if first
   * option starts with a hyphen, print usage. */
  if(argc < 4 || '-' == argv[1][0]) {
    fprintf(stderr, usageFormat, skAppName());
    return 1;
  }

  /* Initialize trees */
  initMacroTree(&newTree);
  initMacroTree(&tmpTree);

  /* Handle first input file (at argv[2]) */
  fileName = argv[2];
  dataFile = fopen(fileName, "rb");
  if (NULL == dataFile) {
    fprintf(stderr, "%s: Error opening input file '%s': %s\n",
            skAppName(), fileName, strerror(errno));
    return 1;
  }
  if (0 != readMacroTree(dataFile, &newTree)) {
    fprintf(stderr, "%s: Error reading tree from file '%s'\n",
            skAppName(), fileName);
    return 1;
  }
  fclose(dataFile);

  /* Now handle remaining input files */
  for (i = 3; i < argc; ++i) {
    /* Open file and read tree */
    fileName = argv[i];
    dataFile = fopen(fileName, "rb");
    if (NULL == dataFile) {
      fprintf(stderr, "%s: Error opening input file '%s': %s\n",
              skAppName(), fileName, strerror(errno));
      return 1;
    }
    if (0 != readMacroTree(dataFile, &tmpTree)) {
      fprintf(stderr, "%s: Error reading tree from file '%s'\n",
              skAppName(), fileName);
      return 1;
    }
    fclose(dataFile);

    /* Do the union */
    for (j = 0; j < 65536; ++j) {
      if (NULL != tmpTree.macroNodes[j]) {
        if (NULL != newTree.macroNodes[j]) {
          /* need to merge */
          for (k = 0; k < 2048; ++k) {
            newTree.macroNodes[j]->addressBlock[k] |=
              tmpTree.macroNodes[j]->addressBlock[k];
          }
        } else {
          /* copy block from tmpTree to newTree */
          newTree.macroNodes[j] = (macroNode*) malloc(sizeof(macroNode));
          memcpy(newTree.macroNodes[j], tmpTree.macroNodes[j],
                 sizeof(macroNode));
        }
      }
    }

    freeMacroTree(&tmpTree);
  }

  /* write newTree to the output file */
  dataFile = fopen(outputFile, "wb");
  if (NULL == dataFile) {
    fprintf(stderr, "%s: Error opening output file '%s': %s\n",
            skAppName(), outputFile, strerror(errno));
    return 1;
  }
  if (0 != writeMacroTree(dataFile, &newTree)) {
      fprintf(stderr, "%s: Error writing tree to file '%s'\n",
              skAppName(), outputFile);
      return 1;
  }
  if (0 != fclose(dataFile)) {
    fprintf(stderr, "%s: Cannot close output file '%s': %s\n",
            skAppName(), outputFile, strerror(errno));
    return 1;
  }

  return 0;
}
