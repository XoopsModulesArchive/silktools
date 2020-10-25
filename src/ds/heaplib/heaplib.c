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
** 1.6
** 2004/03/11 14:23:04
** thomasm
*/

/*
**  heaplib.c
**
**  Implementation of a heap data structure.
**
**  A heap data structure is an ordinary binary tree with two
**  properties. The shape property and the heap property.
**
**  The shape property states that the tree is perfectly balanced and
**  that the elements at the bottom level are pushed as far to the
**  left as possible; the tree has no holes and there are leaf
**  elements on at most two levels of the tree.
**
**  The heap property simply states that every element of the tree is
**  larger than any of its descendants if they exists. In particularly
**  the largest element of the heap is the root element. Of course the
**  opposite ordering also defines a heap. Depending on the ordering,
**  a heap is called a max-heap or a min-heap respectively.
**
**  This implementation uses 1 as the root of the heap; for any node
**  n, its parent node is n/2, and its children are 2n and 2n+1
**
*/


#include "silk.h"

RCSIDENT("heaplib.c,v 1.6 2004/03/11 14:23:04 thomasm Exp");

#include <stdlib.h>
#include <assert.h>

#include "heaplib.h"


struct _heapIterator {
  HeapPtr heap;
  int position;
  int increment;
};


HeapPtr heapCreate(int max_size, HeapNodeCompFunc cmpfun)
{
  HeapPtr heap;
  HeapNode *data;

  if (max_size < 1) {
    return (HeapPtr)NULL;
  }
  assert(NULL != cmpfun);

  heap = (HeapPtr) malloc(sizeof(Heap));
  if (NULL == heap) {
    return (HeapPtr)NULL;
  }

  /* Heap code is based on array whose first index is 1 */
  data = (HeapNode*) calloc((1+max_size), sizeof(HeapNode));
  if (NULL == data) {
    free(heap);
    return (HeapPtr)NULL;
  }

  heap->max_size = max_size;
  heap->num_entries = 0;
  heap->cmpfun = cmpfun;
  heap->data = data;

  return heap;
}


void heapFree(HeapPtr heap)
{
  if (NULL == heap) {
    return;
  }

  if (NULL != heap->data) {
    free(heap->data);
    heap->data = NULL;
  }
  free(heap);

  heap = NULL;
}


int heapResize(HeapPtr heap, int new_max_size)
{
  HeapNode *data;

  assert(NULL != heap);

  if (new_max_size <= heap->max_size ) {
    return HEAP_ERR_SIZE;
  }

  /* Heap code is based on array whose first index is 1 */
  data = (HeapNode*) realloc(heap->data, (1+new_max_size) * sizeof(HeapNode));
  if (NULL == data) {
    return HEAP_ERR_MISC;
  }

  heap->max_size = new_max_size;
  heap->data = data;

  return HEAP_OK;
}


int heapGetMaxSizeF(HeapPtr heap)
{
  assert(NULL != heap);
  return heap->max_size;
}


int heapGetNumberEntriesF(HeapPtr heap)
{
  assert(NULL != heap);
  return heap->num_entries;
}


int heapInsert(HeapPtr heap, HeapNode new_node)
{
  int i;
  int j;

  assert(NULL != heap);

  if (heap->num_entries >= heap->max_size) {
    return HEAP_ERR_FULL;
  }

  ++heap->num_entries;

  for (j = heap->num_entries; j > 1; j = i) {
    /* i is j's parent */
    i = j/2;
    if ((*heap->cmpfun)(heap->data[i], new_node) >= 0) {
      break;
    }
    heap->data[j] = heap->data[i];
  }
  heap->data[j] = new_node;

  return HEAP_OK;
}


static void _heapSiftup(HeapNode *heap_data, int start_idx, int last_idx,
                        HeapNodeCompFunc cmpfun)
{
  HeapNode temp;
  int j;

  while ((j = 2*start_idx) <= last_idx) {
    if ((j < last_idx) && (0 > (*cmpfun)(heap_data[j], heap_data[j+1]))) {
      j++;
    }
    if (0 <= (*cmpfun)(heap_data[start_idx], heap_data[j])) {
      break;
    }
    temp = heap_data[j];
    heap_data[j] = heap_data[start_idx];
    heap_data[start_idx] = temp;
    start_idx = j;
  }
}


int heapGetTop(HeapPtr heap, HeapNode *top_node)
{
  assert(NULL != heap);
  assert(NULL != top_node);
  if (heap->num_entries < 1) {
    return HEAP_ERR_EMPTY;
  }

  *top_node = heap->data[1];
  return HEAP_OK;
}


int heapExtractTop(HeapPtr heap, HeapNode *top_node)
{
  HeapNode *data;

  assert(NULL != heap);
  if (heap->num_entries < 1) {
    return HEAP_ERR_EMPTY;
  }

  data = heap->data;
  if (NULL != top_node) {
    *top_node = data[1];
  }

  data[1] = data[heap->num_entries];
  --heap->num_entries;
  _heapSiftup(data, 1, heap->num_entries, heap->cmpfun);

  return HEAP_OK;
}


int heapSortEntries(HeapPtr heap)
{
  HeapNode *data;
  HeapNode item;
  int num_entries;
  int i;

  assert(NULL != heap);

  num_entries = heap->num_entries;
  if (num_entries < 1) {
    return HEAP_ERR_EMPTY;
  }

  /* sort the entries by removing entries from the top of the heap and
     sticking them into the data[] array from back to front */
  data = heap->data;
  for (i = num_entries; i > 1; --i) {
    item = data[1];
    data[1] = data[i];
    data[i] = item;
    _heapSiftup(data, 1, i-1, heap->cmpfun);
  }

  /* the entries are sorted in data[] but they are in the reverse
     order from how the cmpfun expects them, so reverse them */
  for (i = 1; i <= (num_entries/2); ++i) {
    item = data[i];
    data[i] = data[num_entries+1-i];
    data[num_entries+1-i] = item;
  }

  return HEAP_OK;
}


HeapIterator heapIteratorCreate(HeapPtr heap, const int direction)
{
  HeapIterator iter;

  assert(NULL != heap);

  iter = (HeapIterator) malloc(sizeof(HeapIteratorType));
  if (NULL == iter) {
    return (HeapIterator)NULL;
  }

  iter->heap = heap;
  if (direction >= 0) {
    iter->position = 1;
    iter->increment = 1;
  } else {
    iter->position = heap->num_entries;
    iter->increment = -1;
  }

  return iter;
}


void heapIteratorFree(HeapIterator iter)
{
  if (NULL == iter) {
    return;
  }

  free(iter);
  iter = NULL;
}


int heapIterate(HeapIterator iter, HeapNode *heap_node)
{
  HeapPtr heap;

  assert (NULL != iter);

  heap = iter->heap;
  if (NULL == heap) {
    return HEAP_ERR_NULL;
  }

  if (iter->position < 1 || iter->position > heap->num_entries) {
    return HEAP_NO_MORE_ENTRIES;
  }
  *heap_node = heap->data[iter->position];

  iter->position += iter->increment;

  return HEAP_OK;
}
