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
** 1.10
** 2004/03/10 21:38:57
** thomasm
*/

/*
**  mapsid.c
**    dump a map of sid integers to names from silk_site.h
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


#define DECLARE_SENSORID_VARIABLES
#include "silk_site.h"

#include "utils.h"

RCSIDENT("mapsid.c,v 1.10 2004/03/10 21:38:57 thomasm Exp");


int mapName(char *name) {
  int i;
  upper(name);
  for (i  = 0; i < SENSOR_COUNT; i++ ) {
    if (strcmp(name, sensorInfo[i].sensorName) == 0) {
      fprintf(stdout, "%s -> %2d\n", sensorInfo[i].sensorName, i);
      return 0;
    }
  }
  fprintf(stderr, "Invalid sensor name %s\n", name);
  return 1;
}
  

int main(int argc, char **argv) {
  int i, count, sid;
  char *cp;
  int sidNameFlag = 0;

  skAppRegister(argv[0]);
  count = 0;
  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      /* see if the input is chars or a number */
      cp = &argv[i][0];
      while (*cp) {
        if (!isdigit((int)*cp)) {
          sidNameFlag = 1;
          break;
        }
        cp++;
      }
      if(sidNameFlag) {
        mapName(argv[i]);
      } else {
        sid = (int)strtoul(argv[i], NULL, 10);
        if (sid < 0 || sid >= SENSOR_COUNT) {
          fprintf(stderr, "Invalid sensor number %d\n", sid);
        } else {
          fprintf(stdout, "%3d -> %s\n", sid, sensorInfo[sid].sensorName);
        }
      }
    }
  } else {
    /* no args. dump all */
    for (i  = 0; i < SENSOR_COUNT; i++ ) {
      count++;
      fprintf(stdout, "%3d -> %s\n", i, sensorInfo[i].sensorName);
    }
    fprintf(stdout, "Total sensors %d\n", count);
  }
  exit(0);
}

    
