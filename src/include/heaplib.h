#ifndef _HEAPLIB_H
#define _HEAPLIB_H
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
**
** 1.5
** 2003/12/10 22:00:43
** thomasm
*/

/*@unused@*/ static char rcsID_HEAPLIB_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "heaplib.h,v 1.5 2003/12/10 22:00:43 thomasm Exp";




#define HEAP_OK 0

/* Unclassified error */
#define HEAP_ERR_MISC 1

/* Return value when passing a NULL HeapPtr pointer to heap_* function */
#define HEAP_ERR_NULL 2

/* Return value when attempting to add HeapNode to a full heap */
#define HEAP_ERR_FULL 3

/* Return value when attempting to get or to delete the top element of
 * an empty heap */
#define HEAP_ERR_EMPTY 4

/* Return value when heap iterator reaches end-of-data */
#define HEAP_NO_MORE_ENTRIES 5

/* Return value for heapCreate or heapResize when a there is a problem
 * with the requested size of the heap. */
#define HEAP_ERR_SIZE 6

/* The Nodes stored in the heap data structure */
typedef void* HeapNode;

/* Used to iterate over the entries in the heap */
typedef struct _heapIterator HeapIteratorType;
typedef HeapIteratorType *HeapIterator;

/* The signature of the comparator function that the caller must pass
 * to the heapCreate() function.  The function takes two HeapNodes,
 * node1 and node2, and returns:
 *  -- an integer value > 0 if node1 should be a closer to the root of
 *     the heap than node2
 *  -- an integer value < 0 if node2 should be a closer to the root of
 *     the heap than node1
 * For example: a heap with the lowest value at the root could return
 * 1 if node1<node2  */
typedef int (*HeapNodeCompFunc)(HeapNode node1, HeapNode node2);


/* The heap data structure */
typedef struct _heap {
  int max_size;
  int num_entries;
  HeapNodeCompFunc cmpfun;
  HeapNode *data;
} Heap;
typedef Heap* HeapPtr;


/* Creates a heap of capable of holding max_size HeapNodes.  cmpfun
 * determines how the nodes are ordered in the heap. */
HeapPtr heapCreate(int max_size, HeapNodeCompFunc cmpfun);

/* Destroy an existing heap.  The caller should make certain to free
 * all memory associated with the HeapNodes before calling this
 * function. */
void heapFree(HeapPtr heap);

/* Resizes the heap to new_max_size.  The new size must be greater
 * than the existing size; if it is not, HEAP_ERR_SIZE is returned. */
int heapResize(HeapPtr heap, int new_max_size);

/* Returns the maximum number of entries the heap can accommodate. */
#ifdef NDEBUG
#define heapGetMaxSize(h) ((h)->max_size)
#else
#define heapGetMaxSize(h) heapGetMaxSizeF(h)
#endif
int heapGetMaxSizeF(HeapPtr heap);

/* Returns the number of entries currently in the heap. */
#ifdef NDEBUG
#define heapGetNumberEntries(h) ((h)->num_entries)
#else
#define heapGetNumberEntries(h) heapGetNumberEntriesF(h)
#endif
int heapGetNumberEntriesF(HeapPtr heap);

/* Adds HeapNode to the heap.  Returns HEAP_ERR_FULL if the heap is
 * full. */
int heapInsert(HeapPtr heap, HeapNode new_node);

/* Sets the value of top_node to the HeapNode at the top of the heap.
 * Does not modify the heap.  Returns HEAP_ERR_EMPTY if the heap is
 * empty. */
int heapGetTop(HeapPtr heap, HeapNode *top_node);

/* Removes the HeapNode at the top of the heap.  If top_node is not
 * NULL, it is set to the removed node.  Returns HEAP_ERR_EMPTY if the
 * heap is empty. */
int heapExtractTop(HeapPtr heap, HeapNode *top_node);

/* Sorts the entries in the heap; a sorted heap is still a heap.
 * Useful to order the entries before using a HeapIterator. */
int heapSortEntries(HeapPtr heap);

/* Returns a HeapIterator that can be used to iterate over the nodes
 * in the heap.  Returns NULL if the iterator cannot be created.  If
 * direction is non-negative, the iterator starts at the root and
 * works toward the leaves; otherwise, the iterator works from the
 * leaves to the root.  The iterator visits all nodes on one level
 * before moving to the next.  By calling heapSortEntries() before
 * creating the iterator, the nodes will be traversed in the order
 * determined by the cmpfun. */
HeapIterator heapIteratorCreate(HeapPtr heap, int direction);

/* Frees the memory associated with the iterator */
void heapIteratorFree(HeapIterator iter);

/* While iterating, sets heap_node to the next HeapNode.  Returns
 * HEAP_OK if heap_node was set to the next node; returns
 * HEAP_NO_MORE_ENTRIES if all nodes have been visited; returns
 * HEAP_ERR_NULL if the iterator's heap has been freed. */
int heapIterate(HeapIterator iter, HeapNode *heap_node);

#endif /* _HEAPLIB_H */
