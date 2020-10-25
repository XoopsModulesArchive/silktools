/*
** Copyright (C) 2001-2004  by Carnegie Mellon University.
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
** 2004/03/10 22:23:52
** thomasm
*/



/*
 *   iqsort()
 *
 *   A la Bentley and McIlroy in Software - Practice and Experience,
 *   Vol. 23 (11) 1249-1265. Nov. 1993
 *
 *   MODIFIED TO DEAL WITH UNSIGNED INTEGER ARRAYS ONLY.
 */

#include "silk.h"

#include <stddef.h>

typedef long WORD;
#define	W		sizeof(WORD)	/* must be a power of 2 */
#define SWAPINIT(a, es)	swaptype = \
			(( (a-(char *)0) | es) % W) ? 2 : (es > W) ? 1 : 0
#define exch(a, b, t)	(t = a, a = b, b = t)
/* since swaptype is always 0, change the macro below to avoid the comparision*/
/*#define swap(a, b)	swaptype != 0 ? swapfunc(a, b, es, swaptype):(void )exch(*(WORD*)(a), *(WORD*)(b), t)*/

#define swap(a, b)	(void )exch(*(WORD*)(a), *(WORD*)(b), t)

/* swap sequences of records */
#define vecswap(a, b, n)	if ( n > 0) swapfunc(a, b, n, swaptype)

/* since swaptype is always 0, change the macro below to avoid the comparision*/
/* #define	PVINIT(pv, pm)	if (swaptype != 0) pv = a, swap(pv, pm); else pv = (char *)&v, v = *(WORD*)pm */
#define	PVINIT(pv, pm)	pv = (char *)&v, v = *(WORD*)pm

#define min(a, b)	a <= b ? a : b

/* cmp(a,b)  a > b ? 1 : a < b ? -1 : 0 */

/* CMP compares entries in a pointer to an array of ints */
/* #define CMP(a, b)	(( (**(gwPackedRec_28Ptr (*)[1]) (a))->epochST > (**(gwPackedRec_28Ptr (*)[1]) (b))->epochST) ? 1 : ( (**(gwPackedRec_28Ptr (*)[1]) (a))->epochST < (**(gwPackedRec_28Ptr (*)[1]) (b))->epochST) ? -1 : 0) */


/*
 * Change below to 0 to get a function instead of a macro
 *
*/

#if	1

#define CMP(a,b)  (int)(*(uint32_t *)(a) - *(uint32_t *)(b))
#define MED3(a,b,c)	(CMP((a),(b)) < 0 ? (CMP((b),(c)) < 0 ? (b) : CMP((a),(c)) < 0 ? (c): (a)) : (CMP((b),(c))> 0 ? (b) : CMP((a),(c)) > 0 ? (c): (a)))
#else
#define CMP cmp
#define MED3	med3

int cmp(uint32_t *a, uint32_t *b) {
  register int i;
  i =  *a - *b;
  return i; 
}

static char *med3(char *a, char *b, char *c)
{
  register char *cp;
  cp = CMP(a, b) < 0 ?
	   (CMP(b, c) < 0 ? b : CMP(a, c) < 0 ? c : a)
    : (CMP(b, c) > 0 ? b : CMP(a, c) > 0 ? c : a);
  return cp;
}

#endif

static void swapfunc(char *a, char *b, size_t n, int swaptype)
{
  if( swaptype <= 1)
  { /* this is what will be true for us in this case always */
    WORD t;
    for( ; n > 0; a += W, b += W, n -= W)
      exch(*(WORD*)a, *(WORD*)b, t);
  }
  else
  {
    char t;
    for( ; n > 0; a += 1, b += 1, n -= 1)
      exch(*a, *b, t);
  }
}

void lqsort(char *a, size_t n, size_t es)
{
  char *pa, *pb, *pc, *pd, *pl, *pm, *pn, *pv;
  int r, swaptype;
  WORD t, v;
  size_t s;

/*  SWAPINIT(a, es); always swaptype = 0 */
  swaptype = 0;
  if(n < 7)
  {
    /* use insertion sort on smallest arrays */
    for (pm = a + es; pm < a + n*es; pm += es)
      for(pl = pm; pl > a && CMP(pl-es, pl) > 0; pl -= es)
	swap(pl, pl-es);
    return;
  }

  pm = a + (n/2)*es;		/* small arrays middle element */
  if ( n > 7) 			/* SLK What about n == 7 ? */
  {
    pl = a;
    pn = a + (n - 1)*es;
    if( n > 40 )
    {
      /* big arays.  Pseudomedian of 9 */
      s = (n / 8) * es;
      pl = MED3(pl, pl + s, pl + 2 * s);
      pm = MED3(pm - s, pm, pm + s);
      pn = MED3(pn - 2 * s, pn - s, pn);
    }
    pm = MED3(pl, pm, pn);	/* mid-size, med of 3 */
  }
  PVINIT(pv, pm);		/* pv points to the partition value */
  pa = pb = a;
  pc = pd = a + (n - 1) * es;
  for(;;)
  {
    while(pb <= pc && (r = CMP(pb, pv)) <= 0)
    {
      if (r == 0)
      {
	swap(pa, pb);
	pa += es;
      }
      pb += es;
    }
    while (pc >= pb && (r = CMP(pc, pv)) >= 0)
    {
      if( r == 0)
      {
	swap(pc, pd);
	pd -= es;
      }
      pc -= es;
    }
    if( pb > pc)
      break;
    swap(pb, pc);
    pb += es;
    pc -= es;
  }
  pn = a + n * es;
  s = min(pa - a, pb - pa);
  vecswap(a, pb - s, s);
  s = min(pd - pc, pn -pd -es);
  vecswap(pb, pn - s, s);
  if (( s = pb - pa) > es)
    lqsort(a, s/es, es);
  if (( s = pd - pc) > es)
    lqsort(pn - s, s/es, es);

  return;
}
