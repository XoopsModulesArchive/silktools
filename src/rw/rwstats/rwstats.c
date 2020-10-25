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
** 1.10
** 2004/03/10 22:45:13
** thomasm
*/


/*
**  rwstats.c
**
**  Implementation of the rwstats suite application.
**
**  Reads packed files or reads the output from rwfilter and can
**  compute a battery of characterizations and statistics:
**
**  -- Top N or Bottom N SIPs with counts; count of unique SIPs
**  -- Top N or Bottom N DIPs with counts; count of unique DIPs
**  -- Top N or Bottom N SIP/DIP pairs with counts; count of unique
**     SIP/DIP pairs (for a limited number of records)
**  -- Top N or Bottom N Src Ports with counts; count of unique Src Ports
**  -- Top N or Bottom N Dest Ports with counts; count of unique Dest Ports
**  -- Top N or Bottom N Protocols with counts; count of unique protocols
**  -- For more continuous variables (bytes, packets, bytes/packet)
**     provide statistics such as min, max, quartiles, and intervals
**
**  Instead of specifying a Top N or Bottom N as an absolute number N,
**  the user may specify a cutoff threshold.  In this case, the Top N
**  or Bottom N required to print all counts meeting the threshold is
**  computed by the application.
**
**  Instead of specifying the threshold as an absolute count, the user
**  may specify the threshold as percentage of all input records.  For
**  this case, the absolute threshold is calculated and then that is
**  used to calculate the Top N or Bottom N.
**
**  The application will only do calculations and produce output when
**  asked to do so.  At least one argument is required to tell the
**  application what to do.
**
**  Ideas for expansion
**  -- Similarly for other variables, e.g., country code.
**  -- Output each type of data to its own file
**  -- Save intermediate data in files for faster reprocessing by this
**     application
**  -- Save intermediate data in files for processing by other
**     applications
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "utils.h"
#include "fglob.h"
#include "interval.h"
#include "rwpack.h"
#include "iochecks.h"
#include "heaplib.h"
#include "hashwrap.h"
#include "rwstats.h"

RCSIDENT("rwstats.c,v 1.10 2004/03/10 22:45:13 thomasm Exp");

/*
**  IMPLEMENTATION NOTES
**
**  For each input type (source ip, dest ip, source port, proto, etc),
**  there are two globals: g_limit_<type> contains the value the user
**  entered for the input type, and g_wanted_stat_<type> is a member
**  of the wanted_stat_type and says what the g_limit_<type> value
**  represents---e.g., the Top N, the bottom threshold percentage, etc.
**
**  The application takes input (either from stdin, fglob, or as files
**  on command line) and calls processFile() on each.  A count of each
**  unique source IP addresses is stored in the IpCounter hash table
**  g_counter_src_ip; Destinations IPs in g_counter_dest_ip; data for
**  flow between a Source IP and Destination IP pair are stored in
**  g_counter_pair_ip.
**
**  Since there are relatively few ports and protocols, two
**  65536-elements arrays, g_src_port_array and g_dest_port_array are
**  used to store a count of the records for each source and
**  destination port, respectively, and a 256-element array,
**  g_proto_array, is used to store a count of each protocol.
**
**  Minima, maxima, quartile, and interval data are stored for each of
**  bytes, packets, and bytes-per-packet for all flows--regardless of
**  protocol--and detailed for a limited number (RWSTATS_NUM_PROTO-1)
**  of protocols..  The minima and maxima are each stored in arrays
**  for each of bytes, packets, bpp.  For example g_bytes_min[0]
**  stores the smallest byte count regardless of protocol (ie, over
**  all protocols), and g_pkts_max[1] stores the largest packet count
**  for the first protocol the user specified.  The mapping from
**  protocol to array index is given by g_proto_to_stats_idx[], where
**  the index into g_proto_to_stats_idx[] returns an integer that is
**  the index into g_bytes_min[].  Data for the intervals is stored in
**  two dimensional arrays, where the first dimension is the same as
**  for the minima and maxima, and the second dimension is the number
**  of intervals, NUM_INTERVALS.
**
**  Once data is collected, it is processed.
**
**  For the IPs, the user is interested the number of unique IPs and
**  the IPs with the topN counts (things are similar for the bottomN,
**  but we use topN in this dicussion to keep things more clear).  In
**  the printTopIps() function, an array with 2*topN elements is
**  created and passed to calcTopIps(); that array will be the result
**  array and it will hold the topN IpAddr and IpCount pairs in sorted
**  order.  In calcTopIps(), a working array of 2*topN elements and a
**  Heap data structure with topN nodes are created.  The topN
**  IpCounts seen are stored as IpCount/IpAddr pairs in the
**  2*topN-element array (but not in sorted order), and the heap
**  stores pointers into that array with the lowest IpCount at the
**  root of the heap.  As the function iterates over the hash table,
**  it compares the IpCount of the current hash-table element with the
**  IpCount at the root of the heap.  When the IpCount of the
**  hash-table element is larger, the root of the heap is removed, the
**  IpCount/IpAddr pair pointed to by the former heap-root is removed
**  from the 2*topN-element array and replaced with the new
**  IpCount/IpAddr pair, and finally a new node is added to the heap
**  that points to the new IpCount/IpAddr pair.  This continues until
**  all hash-table entries are processed.  To get the list of topN IPs
**  from highest to lowest, calcTopIps() removes elements from the
**  heap and stores them in the result array from position N-1 to
**  position 0.
**
**  Finding the topN source ports, topN destination ports, and topN
**  protocols are similar to finding the topN IPs, except the ports
**  and protocols are already stored in an array, so pointers directly
**  into the g_src_port_array, g_dest_port_array, and g_proto_array
**  are stored in the heap.  When generating output, the number of the
**  port or protocol is determined by the diffence between the pointer
**  into the g_*_port_array or g_proto_array and its start.
**
**  Instead of specifying a topN, the user may specify a cutoff
**  threshold.  In this case, the topN required to print all counts
**  meeting the threshold is computed by looping over the IP
**  hash-table or port/protocol arrays and finding all entries with at
**  least threshold hits.
**
**  The user may specify a percentage threshold instead of an absolute
**  threshold.  Once all records are read, the total record count is
**  multiplied by the percentage threshold to get the absolute
**  threshold cutoff, and that is used to calculate the topN as
**  described in the preceeding paragraph.
**
**  For the continuous variables bytes, packets, bpp, most of the work
**  was done while reading the data, so processing is minimal.  Only
**  the quartiles must be calculated.
*/



/* local functions */

/* Functions that handle input */
static void processFile(char *curFName);

/* Functions to pass to the heaplib for ordering nodes */
static int rwstatsCompareCountsTop(HeapNode node1, HeapNode node2);
static int rwstatsCompareCountsBtm(HeapNode node1, HeapNode node2);

/* Functions for statistics computing */
static void updateStatistics(int proto, int proto_idx, rwRec *rwrec);


/* global variables.  these are all extern'ed in rwstats.h */

/* Total number of records read */
uint32_t g_record_count;

/* SOURCE IP: the counter, the initial size of counter, what type of
 * stat to compute, the limit for that stat as entered by user. */
IpCounter *g_counter_src_ip;
int g_init_size_counter_src_ip = 0xFFFFFF;  /* 2^24 - 1 */
wanted_stat_type g_wanted_stat_src_ip = RWSTATS_NONE;
uint32_t g_limit_src_ip = 0;

/* DEST IP: the counter, the initial size of counter, what type of
 * stat to compute, the limit for that stat as entered by user. */
IpCounter *g_counter_dest_ip;
int g_init_size_counter_dest_ip = 0xFFFFFF;  /* 2^24 - 1 */
wanted_stat_type g_wanted_stat_dest_ip = RWSTATS_NONE;
uint32_t g_limit_dest_ip = 0;

/* SOURCE/DESTINATION IP PAIRS: the counter, the initial size of
 * counter, what type of stat to compute, the limit for that stat as
 * entered by user */
TupleCounter *g_counter_pair_ip;
uint32_t g_init_size_counter_pair_ip = 0xFFFFFF;  /* 2^24 - 1 */
wanted_stat_type g_wanted_stat_pair_ip = RWSTATS_NONE;
uint32_t g_limit_pair_ip = 0;

/* SOURCE/DESTINATION PORT PAIRS: the counter, the initial size of
 * counter, what type of stat to compute, the limit for that stat as
 * entered by user */
#if USE_TUPLE_CTR_4_PORT_PAIR
TupleCounter *g_counter_pair_port;
#else
IpCounter *g_counter_pair_port;
#endif
uint32_t g_init_size_counter_pair_port = 0xFFFFF;  /* 2^20 - 1 */
wanted_stat_type g_wanted_stat_pair_port = RWSTATS_NONE;
uint32_t g_limit_pair_port = 0;

/* SOURCE PORTS: the counter, what type of stat to compute, the limit
 * for that stat as entered by user */
uint32_t g_src_port_array[65536];
wanted_stat_type g_wanted_stat_src_port = RWSTATS_NONE;
uint32_t g_limit_src_port = 0;

/* DEST PORTS: the counter, what type of stat to compute, the limit
 * for that stat as entered by user */
uint32_t g_dest_port_array[65536];
wanted_stat_type g_wanted_stat_dest_port = RWSTATS_NONE;
uint32_t g_limit_dest_port = 0;

/* PROTOCOL: the counter, what type of stat to compute, the limit for
 * that stat as entered by user */
uint32_t g_proto_array[256];
wanted_stat_type g_wanted_stat_proto = RWSTATS_NONE;
uint32_t g_limit_proto = 0;

/* Statistics (min, max, quartiles, intervals) for "continuous" values
 * (bytes, packets, bpp) can be computed over all protocols, and the
 * can be broken out for a limited number of specific protocols.  This
 * defines the size of the data structures to hold these statistics.
 * This is one more that the number of specific protocols allowed. */
#define RWSTATS_NUM_PROTO 9

/* The following is true if the stats are to be calculated over all
 * protocols. */
int g_do_stats_all_protos = 0;

/* These arrays hold the statistics.  Position 0 is for the
 * combination of all statistics. */
uint32_t g_bytes_min[RWSTATS_NUM_PROTO];
uint32_t g_bytes_max[RWSTATS_NUM_PROTO];
uint32_t g_byte_intervals[RWSTATS_NUM_PROTO][NUM_INTERVALS];

uint32_t g_pkts_min[RWSTATS_NUM_PROTO];
uint32_t g_pkts_max[RWSTATS_NUM_PROTO];
uint32_t g_pkt_intervals[RWSTATS_NUM_PROTO][NUM_INTERVALS];

uint32_t g_bpp_min[RWSTATS_NUM_PROTO];
uint32_t g_bpp_max[RWSTATS_NUM_PROTO];
uint32_t g_bpp_intervals[RWSTATS_NUM_PROTO][NUM_INTERVALS];

/* This maps the protocol number to the index in the above statistics
 * arrays.  If the value for a protocol is 0, the user did not request
 * detailed specs on that protocol. */
int16_t g_proto_to_stats_idx[256];

/* Whether to print input filenames */
int8_t g_print_filenames_flag = 0;

/* Whether to print column and section titles */
int8_t g_print_titles_flag = 1;

/* The delimiter string to print between columns */
char *g_delim = "|";

/* Whether to print IP addrs as integers */
int8_t g_integer_ip_flag = 0;

/* CIDR block mask for src and dest ips.  If 0, use all bits;
 * otherwise, the IP address should be bitwised ANDed with this
 * value. */
uint32_t g_cidr_src = 0;
uint32_t g_cidr_dest = 0;


iochecksInfoStructPtr ioISP;
FILE *outF;
char *outFPath = "stdout";
char *inFPath = NULL;
int outIsPipe;


/*
 * processFile:
 *      Read rwRec's from the named file and update the counters
 * Arguments:
 *      curFName -name of file to process
 * Returns: NONE.
 * Side Effects:
 *      Global counters, minima, maxima, intervals are modified.
*/
static void processFile(char *curFName)
{
  rwIOStructPtr rwIOSPtr;
  rwRec rwrec;
  register rwRec *rwrecPtr = &rwrec;
  register int proto_idx;

  if (! (rwIOSPtr = rwOpenFile(curFName, ioISP->inputCopyFD))) {
    fprintf(stderr, "Error opening file '%s'; file ignored.\n", curFName);
    return;
  }
  if (g_print_filenames_flag) {
    fprintf(stderr, "%s\n", curFName);
  }

  while (rwRead(rwIOSPtr, rwrecPtr)) {
    ++g_record_count;

    if (g_cidr_src) {
      rwrecPtr->sIP.ipnum &= g_cidr_src;
    }
    if (g_cidr_dest) {
      rwrecPtr->dIP.ipnum &= g_cidr_dest;
    }

    /* Increment counter for Source and Dest IP pairs */
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      if (g_record_count <= RWSTATS_MAX_RECORDS) {
        tuplectr_rec_inc(g_counter_pair_ip, rwrecPtr);
        if (g_record_count == RWSTATS_MAX_RECORDS) {
          /* we just reached the limit.  Print a warning */
          fprintf(stderr, "Input limit (%d) for Src+Dest Pairs reached\n",
                  RWSTATS_MAX_RECORDS);
        }
      }
    }

#if USE_TUPLE_CTR_4_PORT_PAIR
    /* Increment counter for Source and Dest Port pairs */
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      if (g_record_count <= RWSTATS_MAX_RECORDS) {
        tuplectr_rec_inc(g_counter_pair_port, rwrecPtr);
        /* print a warning if one wasn't printed above */
        if ((g_record_count == RWSTATS_MAX_RECORDS) &&
            (RWSTATS_NONE == g_wanted_stat_pair_ip)) {
          /* we just reached the limit.  Print a warning */
          fprintf(stderr, "Input limit (%d) for Src+Dest Pairs reached\n",
                  RWSTATS_MAX_RECORDS);
        }
      }
    }
#else
    /* Increment counter for Source and Dest Port pairs */
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      ipctr_inc(g_counter_pair_port, ((rwrecPtr->sPort<<16)|rwrecPtr->dPort));
    }
#endif /* USE_TUPLE_CTR_4_PORT_PAIR */

    /* Increment counters for Source IP and Dest IP */
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      ipctr_inc(g_counter_src_ip, rwrecPtr->sIP.ipnum);
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      ipctr_inc(g_counter_dest_ip, rwrecPtr->dIP.ipnum);
    }

    /* Increment Ports */
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      ++g_src_port_array[rwrecPtr->sPort];
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      ++g_dest_port_array[rwrecPtr->dPort];
    }

    /* Protocols */
    if (RWSTATS_NONE != g_wanted_stat_proto || g_do_stats_all_protos > 0) {
      ++g_proto_array[rwrecPtr->proto];
    }

    if (g_do_stats_all_protos > 0) {
      /* Do statistics for all regardless of protocol. Use
         256 as the protocol */
      updateStatistics(256, 0, rwrecPtr);
    }

    /* Compute statistics for specific protocol if requested */
    proto_idx = g_proto_to_stats_idx[rwrecPtr->proto];
    if (proto_idx) {
      updateStatistics(rwrecPtr->proto, proto_idx, rwrecPtr);
    }

  } /* while rwRead() */

  rwCloseFile(rwIOSPtr);
}


/*
 * updateStatistics:
 *      Update the minima, maxima, and intervals for bytes, packets,
 *      and bytes-per-packet for the specified protocol.
 * Arguments:
 *      proto -number of the protocol to be updated.  Used to
 *              determine which intervals to use.  This is equal to
 *              256 for the overall stats
 *      proto_idx -the index---determined by proto---into the
 *              various g_*_min, g_*_max, and g_*_intervals arrays.
 *      rwrecPtr -the rwrec data
 * Returns: NONE.
 * Side Effects:
 *      Global counters, minima, maxima, intervals are modified.
*/
static void updateStatistics(int proto, int proto_idx, rwRec *rwrecPtr)
{
  register uint32_t *byteIntervals;
  register uint32_t *pktIntervals;
  register uint32_t *bppIntervals;
  uint32_t bpp;
  int i;

  bpp = rwrecPtr->bytes / rwrecPtr->pkts;

  switch (proto) {
  case 6: /* tcp */
  case 256: /* all protocols: use tcp since dominant proto */
    byteIntervals = tcpByteIntervals;
    pktIntervals = tcpPktIntervals;
    bppIntervals = tcpBppIntervals;
    break;
  case 17: /* udp */
  case 1: /* icmp: use udp */
  default: /* other: use udp */
    byteIntervals = udpByteIntervals;
    pktIntervals = udpPktIntervals;
    bppIntervals = udpBppIntervals;
    break;
  }

  /* Find min/max/intervals for bytes, packets, bpp */
  if (rwrecPtr->bytes < g_bytes_min[proto_idx]) {
    g_bytes_min[proto_idx] = rwrecPtr->bytes;
  } else if (rwrecPtr->bytes > g_bytes_max[proto_idx]) {
    g_bytes_max[proto_idx] = rwrecPtr->bytes;
  }
  for (i = 0; i < NUM_INTERVALS; ++i) {
    if (rwrecPtr->bytes <= byteIntervals[i]) {
      g_byte_intervals[proto_idx][i]++;
      break;
    }
  }

  if (rwrecPtr->pkts < g_pkts_min[proto_idx]) {
    g_pkts_min[proto_idx] = rwrecPtr->pkts;
  } else if (rwrecPtr->pkts > g_pkts_max[proto_idx]) {
    g_pkts_max[proto_idx] = rwrecPtr->pkts;
  }
  for (i = 0; i < NUM_INTERVALS; ++i) {
    if (rwrecPtr->pkts <= pktIntervals[i]) {
      g_pkt_intervals[proto_idx][i]++;
      break;
    }
  }

  if (bpp < g_bpp_min[proto_idx]) {
    g_bpp_min[proto_idx] = bpp;
  } else if (bpp > g_bpp_max[proto_idx]) {
    g_bpp_max[proto_idx] = bpp;
  }
  for (i = 0; i < NUM_INTERVALS; ++i) {
    if (bpp <= bppIntervals[i]) {
      g_bpp_intervals[proto_idx][i]++;
      break;
    }
  }
}


/* rwstatsCompareCountsTop
 *      Called by heap library to compare Counts for the topN
 * Arguments:
 *      node1 - pointer to a uint32_t representing a count
 *      node2 - pointer to a uint32_t representing a count
 * Results:
 *      Returns 1 if the Count for node1 < that for node2;
 *      -1 if Count for node1 > that for node2.
 * Side effects: NONE.
*/
static int rwstatsCompareCountsTop(HeapNode node1, HeapNode node2)
{
  uint32_t a;
  uint32_t b;

  a = *((uint32_t*) node1);
  b = *((uint32_t*) node2);

  if (a > b) {
    return -1;
  }
  if (a < b) {
    return 1;
  }
  return 0;
}


/* rwstatsCompareCountsBtm
 *      Called by heap library to compare Counts for the bottomN
 * Arguments:
 *      node1 - pointer to a uint32_t representing a count
 *      node2 - pointer to a uint32_t representing a count
 * Results:
 *      Returns -1 if the Count for node1 < that for node2;
 *      1 if Count for node1 > that for node2.
 * Side effects: NONE.
*/
static int rwstatsCompareCountsBtm(HeapNode node1, HeapNode node2)
{
  uint32_t a;
  uint32_t b;

  a = *((uint32_t*) node1);
  b = *((uint32_t*) node2);

  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}


/*
 * calcTopIps
 *      Calculates the topN or bottomN IpAddr/IpCount key/value pairs
 *      in the ip_counter hash-table
 * Arguments:
 *      ip_counter - hash-table of IpAddr/IpCount key/value pairs
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      topn -the topN or bottomN to return
 *      topn_array -the array in which to return the topN/bottonN
 *              IpAddr/IpCount pairs.  size is 2*topn
 * Returns:
 *      the number of IpAddr/IpCount pairs stored in topn_array
 * Side Affects: NONE.
*/
uint32_t calcTopIps(IpCounter *ip_counter, const int8_t top_or_btm,
                  const uint32_t topn, uint32_t *topn_array)
{
  uint32_t *tmp_array;
  HeapPtr heap;
  uint32_t *heap_ptr;
  HASH_ITER iter;
  uint32_t ip_address;
  uint32_t ip_count;
  uint32_t heap_top_value;
  uint32_t heap_num_entries;
  uint32_t result_num_entries;

  /* Create the working array to store the IpCount/IpAddr pairs.  Note
   * that this tmp_array stores the count first.  Element 2n+1 is
   * an IpAddress; element 2n is the number of time the (2n+1)'th
   * IpAddr was seen in the input.  Elements in this array are in no
   * particular order.  Their ordering is maintained by pointers into
   * this array that are stored in the heap. */
  tmp_array = (uint32_t*) calloc(topn * 2, sizeof(uint32_t));
  if (NULL == tmp_array) {
    fprintf(stderr, "Cannot malloc ip array\n");
    exit(1);
  }

  /* Create the heap */
  if (top_or_btm > 0) {
    heap = heapCreate(topn, rwstatsCompareCountsTop);
  } else {
    heap = heapCreate(topn, rwstatsCompareCountsBtm);
  }
  if (NULL == heap) {
    fprintf(stderr, "Heap creation failed\n");
    exit(1);
  }

  /* The number of nodes currently in the heap. */
  heap_num_entries = 0;

  /* The value at the top of the heap--the smallest of the topN or the
   * largest of the bottomN; this value is used to avoid issuing
   * multiple heap lookups */
  heap_top_value = 0;

  /* Iterate over the hash-table. */
  iter = ipctr_create_iterator(ip_counter);
  while (ipctr_iterate(ip_counter, &iter,
                       &ip_address, &ip_count) != ERR_NOMOREENTRIES) {
    if (heap_num_entries < topn) {
      /* Heap is not yet full.  Add this entry into the tmp_array,
         and store a pointer to this data in the heap.*/
      tmp_array[2*heap_num_entries] = ip_count;
      tmp_array[2*heap_num_entries+1] = ip_address;
      heapInsert(heap, tmp_array + 2*heap_num_entries);
      ++heap_num_entries;
      if (heap_num_entries == topn) {
        /* we just filled the heap; now we start to compare hash-table
           elements with the IpCount stored in the root of the
           heap--the lowest count of the topN (or the highest of the
           bottomN).  Get the top node and cache its IpCount. */
        heapGetTop(heap, (HeapNode*)&heap_ptr);
        heap_top_value = *heap_ptr;
      }
    } else if ((top_or_btm > 0 && heap_top_value < ip_count) ||
               (top_or_btm <= 0 && heap_top_value > ip_count)) {
      /* The hash-table element we just read is "better" (for topN,
         higher than current heap-root's value; for bottomN, lower
         than current heap-root's value).  Remove the heap's root to
         make room for the new node */
      heapExtractTop(heap, NULL);
      /* heap_ptr points into the tmp_array.  replace the IP
         address and count in the tmp_array with this new
         value */
      *heap_ptr = ip_count;
      *(heap_ptr+1) = ip_address;
      /* insert this new value into the heap */
      heapInsert(heap, heap_ptr);
      /* the top may have changed; get the new top and its IpCount */
      heapGetTop(heap, (HeapNode*)&heap_ptr);
      heap_top_value = *heap_ptr;
    }
  }

  /* Cache the number of entries in the heap for our return value */
  result_num_entries = heap_num_entries;

  /* Remove the entries from the heap one at a time and put them into
     the topn_array.  They will come off from the lowest of the
     topN to the highest--or highest of the bottomN to the lowest. */
  while (heap_num_entries > 0) {
    heapExtractTop(heap, (HeapNode*)&heap_ptr);
    --heap_num_entries;
    topn_array[2*heap_num_entries] = *(heap_ptr + 1); /* ipaddr */
    topn_array[2*heap_num_entries + 1] = *heap_ptr;   /* count */
  }

  /* Clean up */
  heapFree(heap);
  free(tmp_array);

  return result_num_entries;
}


/*
 * calcTopPairsIp
 *      Calculates the topN or bottomN IpSrcAddr+IpDestAddr/IpCount
 *      key/value pairs in the g_counter_pair_ip hash-table
 * Arguments:
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      topn -the topN or bottomN to return
 *      topn_array -the array in which to return the topN/bottonN
 *              IpSrcAddr+IpDestAddr/IpCount triples. size is 3*topn
 * Returns:
 *      the number of IpSrcAddr+IpDestAddr/IpCount triples stored in
 *      topn_array
 * Side Affects: NONE.
*/
uint32_t calcTopPairsIp(const int8_t top_or_btm, const uint32_t topn,
                      uint32_t *topn_array)
{
  uint32_t *tmp_array;
  HeapPtr heap;
  uint32_t *heap_ptr;
  HASH_ITER iter;
  rwRec rwrec;
  uint32_t ip_count;
  uint32_t heap_top_value;
  uint32_t heap_num_entries;
  uint32_t result_num_entries;

  /* Create the working array to store the
   * IpCount/SrcIpAddr+DestIpAddr triples.  Note that this
   * tmp_array stores the count first.  Element 3n+1 is a
   * SrcIpAddr, element 3n+2 is a DestIpAddr; element 3n is the number
   * flows between them.  Elements in this array are in no particular
   * order.  Their ordering is maintained by pointers into this array
   * that are stored in the heap. */
  tmp_array = (uint32_t*) calloc(topn * 3, sizeof(uint32_t));
  if (NULL == tmp_array) {
    fprintf(stderr, "Cannot malloc ip pair array\n");
    exit(1);
  }

  /* Create the heap */
  if (top_or_btm > 0) {
    heap = heapCreate(topn, rwstatsCompareCountsTop);
  } else {
    heap = heapCreate(topn, rwstatsCompareCountsBtm);
  }
  if (NULL == heap) {
    fprintf(stderr, "Heap creation failed\n");
    exit(1);
  }

  /* The number of nodes in the heap. */
  heap_num_entries = 0;

  /* The IpCount of the root of the heap; this is used to avoid
   * multiple heap lookups */
  heap_top_value = 0;

  /* Iterate over the hash-table. */
  iter = tuplectr_create_iterator(g_counter_pair_ip);
  while (tuplectr_rec_iterate(g_counter_pair_ip, &iter,
                              &rwrec, &ip_count) != ERR_NOMOREENTRIES) {
    if (heap_num_entries < topn) {
      /* Heap is not yet full.  Add this entry into the
         tmp_array, and store a pointer to this data in the
         heap.*/
      tmp_array[3*heap_num_entries] = ip_count;
      tmp_array[3*heap_num_entries+1] = rwrec.sIP.ipnum;
      tmp_array[3*heap_num_entries+2] = rwrec.dIP.ipnum;
      heapInsert(heap, tmp_array + 3*heap_num_entries);
      ++heap_num_entries;
      if (heap_num_entries == topn) {
        /* we just filled the heap; now we start to compare hash-table
           elements with the IpCount stored in the root of the
           heap--the lowest count of the topN (or the highest of the
           bottomN).  Get the top node and cache its IpCount. */
        heapGetTop(heap, (HeapNode*)&heap_ptr);
        heap_top_value = *heap_ptr;
      }
    } else if ((top_or_btm > 0 && heap_top_value < ip_count) ||
               (top_or_btm <= 0 && heap_top_value > ip_count)) {
      /* The hash-table element we just read is "better" (for topN,
         higher than current heap-root's value; for bottomN, lower
         than current heap-root's value).  Remove the heap's root to
         make room for the new node */
      heapExtractTop(heap, NULL);
      /* heap_ptr points into the tmp_array.  replace the IP
         addresses and count in the tmp_array with this new
         value */
      *heap_ptr = ip_count;
      *(heap_ptr+1) = rwrec.sIP.ipnum;
      *(heap_ptr+2) = rwrec.dIP.ipnum;
      /* insert this new value into the heap */
      heapInsert(heap, heap_ptr);
      /* the top may have changed; get the new top and its IpCount */
      heapGetTop(heap, (HeapNode*)&heap_ptr);
      heap_top_value = *heap_ptr;
    }
  }

  /* Cache the number of entries in the heap for our return value */
  result_num_entries = heap_num_entries;

  /* Remove the entries from the heap one at a time and put them into
     the topn_array.  They will come off from the lowest of the
     topN to the highest--or highest of the bottomN to the lowest. */
  while (heap_num_entries > 0) {
    heapExtractTop(heap, (HeapNode*)&heap_ptr);
    --heap_num_entries;
    topn_array[3*heap_num_entries]   = *(heap_ptr+1); /* src ipaddr */
    topn_array[3*heap_num_entries+1] = *(heap_ptr+2); /* dest ipaddr */
    topn_array[3*heap_num_entries+2] = *heap_ptr;     /* count */
  }

  /* Clean up */
  heapFree(heap);
  free(tmp_array);

  return result_num_entries;
}


#if USE_TUPLE_CTR_4_PORT_PAIR
/*
 * calcTopPairsPort
 *      Calculates the topN or bottomN sPort+dPort/PortCount
 *      key/value pairs in the g_counter_pair_port hash-table
 * Arguments:
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      topn -the topN or bottomN to return
 *      topn_array -the array in which to return the topN/bottonN
 *              sPort+dPort/PortCount triples. size is 2*topn
 * Returns:
 *      the number of sPort+dPort/PortCount triples stored in
 *      topn_array.  sPort+dPort returned as (sPort<<16)|dPort
 * Side Affects: NONE.
*/
uint32_t calcTopPairsPort(const int8_t top_or_btm, const uint32_t topn,
                        uint32_t *topn_array)
{
  uint32_t *tmp_array;
  HeapPtr heap;
  uint32_t *heap_ptr;
  HASH_ITER iter;
  rwRec rwrec;
  uint32_t port_count;
  uint32_t heap_top_value;
  uint32_t heap_num_entries;
  uint32_t result_num_entries;

  /* Create the working array to store the PortCount/sPort+dPort
   * triples.  Note that this tmp_array stores the count first.
   * Element 2n+1 is (sPort<<16)|dPort; element 2n is the number flows
   * between them.  Elements in this array are in no particular order.
   * Their ordering is maintained by pointers into this array that are
   * stored in the heap. */
  tmp_array = (uint32_t*) calloc(topn * 2, sizeof(uint32_t));
  if (NULL == tmp_array) {
    fprintf(stderr, "Cannot malloc port pair array\n");
    exit(1);
  }

  /* Create the heap */
  if (top_or_btm > 0) {
    heap = heapCreate(topn, rwstatsCompareCountsTop);
  } else {
    heap = heapCreate(topn, rwstatsCompareCountsBtm);
  }
  if (NULL == heap) {
    fprintf(stderr, "Heap creation failed\n");
    exit(1);
  }

  /* The number of nodes in the heap. */
  heap_num_entries = 0;

  /* The PortCount of the root of the heap; this is used to avoid
   * multiple heap lookups */
  heap_top_value = 0;

  /* Iterate over the hash-table. */
  iter = tuplectr_create_iterator(g_counter_pair_port);
  while (tuplectr_rec_iterate(g_counter_pair_port, &iter,
                              &rwrec, &port_count) != ERR_NOMOREENTRIES) {
    if (heap_num_entries < topn) {
      /* Heap is not yet full.  Add this entry into the
         tmp_array, and store a pointer to this data in the
         heap.*/
      tmp_array[2*heap_num_entries] = port_count;
      tmp_array[2*heap_num_entries+1] = (rwrec.sPort << 16) | rwrec.dPort;
      heapInsert(heap, tmp_array + 2*heap_num_entries);
      ++heap_num_entries;
      if (heap_num_entries == topn) {
        /* we just filled the heap; now we start to compare hash-table
           elements with the PortCount stored in the root of the
           heap--the lowest count of the topN (or the highest of the
           bottomN).  Get the top node and cache its PortCount. */
        heapGetTop(heap, (HeapNode*)&heap_ptr);
        heap_top_value = *heap_ptr;
      }
    } else if ((top_or_btm > 0 && heap_top_value < port_count) ||
               (top_or_btm <= 0 && heap_top_value > port_count)) {
      /* The hash-table element we just read is "better" (for topN,
         higher than current heap-root's value; for bottomN, lower
         than current heap-root's value).  Remove the heap's root to
         make room for the new node */
      heapExtractTop(heap, NULL);
      /* heap_ptr points into the tmp_array.  replace the port numbers
         and count in the tmp_array with this new value */
      *heap_ptr = port_count;
      *(heap_ptr+1) = (rwrec.sPort << 16) | rwrec.dPort;
      /* insert this new value into the heap */
      heapInsert(heap, heap_ptr);
      /* the top may have changed; get the new top and its PortCount */
      heapGetTop(heap, (HeapNode*)&heap_ptr);
      heap_top_value = *heap_ptr;
    }
  }

  /* Cache the number of entries in the heap for our return value */
  result_num_entries = heap_num_entries;

  /* Remove the entries from the heap one at a time and put them into
     the topn_array.  They will come off from the lowest of the
     topN to the highest--or highest of the bottomN to the lowest. */
  while (heap_num_entries > 0) {
    heapExtractTop(heap, (HeapNode*)&heap_ptr);
    --heap_num_entries;
    topn_array[2*heap_num_entries]   = *(heap_ptr+1); /* src port+dest port */
    topn_array[2*heap_num_entries+1] = *heap_ptr;     /* count */
  }

  /* Clean up */
  heapFree(heap);
  free(tmp_array);

  return result_num_entries;
}
#endif /* USE_TUPLE_CTR_4_PORT_PAIR */


/*
 * calcTopPorts
 *      Calculates the topN or bottomN values in p_array---may be
 *      source ports, destination ports, or protocols
 * Arguments:
 *      p_array -array of port or protocol-counts.  the array's index
 *              is the port or protocol number
 *      p_array_size -the number of entries in the p_array
 *              (65536 for ports, 256 for protocols)
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      topn -the topN or bottomN to return
 *      topn_array -the array in which to return the topN/bottonN
 *              Port/Count pairs.  size is 2*topn
 * Returns:
 *      the number of Port/Count pairs stored in topn_array
 * Side Affects: NONE.
*/
uint32_t calcTopPorts(uint32_t* p_array, const int32_t p_array_size,
                    const int8_t top_or_btm, const uint32_t topn,
                    uint32_t *topn_array)
{
  HeapPtr heap;
  uint32_t *heap_ptr;
  uint32_t *idx_p;
  uint32_t heap_top_value;
  uint32_t heap_num_entries;
  uint32_t result_num_entries;

  /* Create the heap */
  if (top_or_btm > 0) {
    heap = heapCreate(topn, rwstatsCompareCountsTop);
  } else {
    heap = heapCreate(topn, rwstatsCompareCountsBtm);
  }
  if (NULL == heap) {
    fprintf(stderr, "Heap creation failed\n");
    exit(1);
  }

  /* The number of nodes in the heap. */
  heap_num_entries = 0;

  /* The value at the top of the heap--the smallest of the topN or the
   * largest of the bottomN; this value is used to avoid issuing
   * multiple heap lookups */
  heap_top_value = 0;

  /* Iterate over the elements in the port/protocol array */
  for (idx_p = p_array; idx_p < p_array + p_array_size; ++idx_p) {
    if (0 == *idx_p) {
      /* Skip ports/protocols with no hits */
      continue;
    }
    if (heap_num_entries < topn) {
      /* Heap is not yet full.  Add this entry to the heap. */
      heapInsert(heap, (HeapNode*)idx_p);
      ++heap_num_entries;
      if (heap_num_entries == topn) {
        /* we just filled the heap; now we start to compare the counts
           we read from the p_array with the count stored in the root
           of the heap--the lowest count of the topN (or the highest
           of the bottomN).  Get the top node and cache its count. */
        heapGetTop(heap, (HeapNode*)&heap_ptr);
        heap_top_value = *heap_ptr;
      }
    } else if ((top_or_btm > 0 && heap_top_value < *idx_p) ||
               (top_or_btm <= 0 && heap_top_value > *idx_p)) {
      /* The p_array element we just read is "better" (for topN,
         higher than current heap-root's value; for bottomN, lower
         than current heap-root's value).  Remove the heap's root to
         make room for the new node */
      heapExtractTop(heap, NULL);
      /* insert this new value into the heap */
      heapInsert(heap, (HeapNode*)idx_p);
      /* the top may have changed; get the new top and its value */
      heapGetTop(heap, (HeapNode*)&heap_ptr);
      heap_top_value = *heap_ptr;
    }
  }

  /* Cache the number of entries in the heap for our return value */
  result_num_entries = heap_num_entries;

  /* Remove the entries from the heap one at a time and put them into
     the topn_array.  They will come off from the lowest of the topN
     to the highest--or highest of the bottomN to the lowest.  The
     heap has pointers into the p_array.  Compute the difference
     between the pointer and the start of p_array to get the port or
     protocol (as an integer). */
  while (heap_num_entries > 0) {
    heapExtractTop(heap, (HeapNode*)&heap_ptr);
    --heap_num_entries;
    topn_array[2*heap_num_entries] = heap_ptr - p_array; /* port/proto */
    topn_array[2*heap_num_entries + 1] = *heap_ptr;      /* count */
  }

  /* Clean up */
  heapFree(heap);

  return result_num_entries;
}


int main(int argc, char **argv) {
  char *curFName;
  int ctr;

  /* Global setup */
  appSetup(argc, argv);


  /* Iterate over the files to process */
  if (fglobValid()) {
    while ((curFName = fglobNext())){
      processFile(curFName);
    }
  } else {
    for(ctr = ioISP->firstFile; ctr < ioISP->fileCount ; ctr++) {
      processFile(ioISP->fnArray[ctr]);
    }
  }

  /* Generate output */
  rwstatsGenerateOutput();

  /* Done, do cleanup */
  appTeardown();		/* AOK. Dump information */
  exit(0);

  return 0;
}
