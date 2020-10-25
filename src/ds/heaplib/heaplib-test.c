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
** 1.3
** 2003/12/10 22:00:32
** thomasm
*/

/*
**  heaplib-test.c
**
**  a simple testing harness for the heaplib data structure library
**
*/

#include <stdio.h>
#include <stdlib.h>

#include "heaplib.h"

/*@unused@*/ static char* __rcsID
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "heaplib-test.c,v 1.3 2003/12/10 22:00:32 thomasm Exp";

int compare(HeapNode node1, HeapNode node2)
{
  int a;
  int b;

  a = *(((int*) node1)+1);
  b = *(((int*) node2)+1);
  if (a > b) {
    return -1;
  }
  if (a < b) {
    return 1;
  }
  return 0;
}

int main(void)
{
#define DATA_SIZE 15
  HeapPtr heap;
  int data[2*DATA_SIZE] = {
    201, 34, 202, 56, 203,  2,
    204, 65, 205,  3, 206,  5,
    207,  8, 208, 74, 209, 32,
    210, 78, 211, 79, 212, 80,
    213,  5, 214,  5, 215,  1};
  int i;
  int *iptr;
  int k;
  int* top;
  int status;
  int resized_heap = 0;
  int heap_init_size = DATA_SIZE/3;
  HeapIterator iter_down;
  HeapIterator iter_up;

  heap = heapCreate(heap_init_size, compare);
  if (NULL == heap) {
    printf("Cannot create heap\n");
    exit(1);
  }

  for (i = 0, iptr = data; i < DATA_SIZE; ++i, iptr += 2) {
    printf("\n** adding %d/%d...", data[2*i], data[2*i+1]);
    status = heapInsert(heap, (HeapNode)iptr);
    if (HEAP_OK == status) {
      printf("OK\n");
    } else if (HEAP_ERR_FULL != status) {
      printf("NOPE. Got wierd error status %d\n", status);
    } else if (!resized_heap) {
      printf("NOPE. Heap full.\nGoing to try to resize the heap...");
      resized_heap = 1;
      if (HEAP_OK == heapResize(heap, 2*heap_init_size)) {
        printf("resized!\n");
      } else {
        printf("unable to resize!\n");
      }
    } else {
      printf("NOPE. Heap full.  Comparing with the top.\n");
      heapGetTop(heap, (HeapNode*)&top);
      if (0 >= compare(top, iptr)) {
        printf("Not added to heap since <= top (%d/%d) [%d]\n",
               *top, *(top+1), compare(top, iptr));
      } else {
        printf("Removing top of heap (%d/%d)...", *top, *(top+1));
        heapExtractTop(heap, NULL);
        if (HEAP_OK == heapInsert(heap, (HeapNode)iptr)) {
          printf("OK\n");
        } else {
          printf("Problem adding '%d/%d' to heap\n", data[2*i], data[2*i+1]);
        }
      }
    }
    printf("heap %d/%d\n", heap->num_entries, heap->max_size);
    for (k = 1; k <= heap->num_entries; ++k) {
      printf("%5d  %d/%d\n", k,
             *((int*)(heap->data[k])), *(((int*) (heap->data[k]))+1));
    }
  }

  printf("\n** Sorting the heap...");
  if (HEAP_OK == heapSortEntries(heap)) {
    printf("OK\n");
  } else {
    printf("Got error\n");
  }
  printf("heap %d/%d\n", heap->num_entries, heap->max_size);
  for (k = 1; k <= heap->num_entries; ++k) {
    printf("%5d  %d/%d\n", k,
           *((int*)(heap->data[k])), *(((int*) (heap->data[k]))+1));
  }

  printf("\n** Iterating over the heap...\n");
  iter_down = heapIteratorCreate(heap, 1);
  iter_up = heapIteratorCreate(heap, -1);
  while (heapIterate(iter_down, (HeapNode*)&iptr) == HEAP_OK) {
    printf("Down: %d/%d\t\t", *iptr, *(iptr+1));
    heapIterate(iter_up, (HeapNode*)&iptr);
    printf("Up: %d/%d\n", *iptr, *(iptr+1));
  }
  heapIteratorFree(iter_down);
  heapIteratorFree(iter_up);

  printf("\nRemoving sorted data from the heap:\n");
  while(HEAP_OK == heapExtractTop(heap, (HeapNode*)&top)) {
    printf("%d/%d\n", *top, *(top+1));
  }


  heapFree(heap);

  exit(0);
}
