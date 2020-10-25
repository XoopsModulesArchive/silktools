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
**
** 1.10
** 2004/03/10 22:45:13
** thomasm
*/


/*
**  rwstatsutils.c
**
**  Input/output functions for the rwstats application.  See rwstats.c
**  for a full explanation.
**
*/

#include "silk.h"

#include <stdio.h>

#include "iochecks.h"
#include "interval.h"
#include "rwpack.h"
#include "utils.h"
#include "hashlib.h"
#include "hashwrap.h"
#include "rwstats.h"

RCSIDENT("rwstatsio.c,v 1.10 2004/03/10 22:45:13 thomasm Exp");


/* local functions */

/* Functions for IP output */
static void printTopIps(IpCounter *ip_counter,
                        const wanted_stat_type wantedstat, const uint32_t limit);

/* Functions for Src+Dest IP Pair output */
static void printTopPairsIp(void);

/* Functions for Src+Dest Port Pair output */
static void printTopPairsPort(void);

/* Functions for Port and Protocol output */
static void printTopPorts(uint32_t* port_array, const int32_t p_array_size,
                          const wanted_stat_type wantedstat,
                          const uint32_t limit);

/* Functions for statistics computing and output */
static void printIntervals(int proto, int proto_idx);

/* Functions to take a record count or a percentage of total records
   and return an absolute topN or bottomN */
static uint32_t ipThresholdToCount(IpCounter *ip_counter, const int8_t top_or_btm,
                                 const uint32_t threshold);
static uint32_t pairThresholdToCount(TupleCounter *tuple_counter,
                                   const int8_t top_or_btm,
                                   const uint32_t threshold);
static uint32_t arrayThresholdToCount(uint32_t* p_array, const int p_array_size,
                                    const int8_t top_or_btm,
                                    const uint32_t threshold);

/* types of columns we output */
enum width_type {
  WIDTH_COUNT, WIDTH_IPADD, WIDTH_PORT, WIDTH_PROTO, WIDTH_INTVL,
  WIDTH_PCT
};

/* output column widths.  mapped to width_type */
static int g_width[6];


/*
 * rwstatsSetColumnWidth
 *      sets the width of the output columns
 * Arguments:
 *      width -an integer.  if >0, use this value as the output column
 *              width, otherwise use the default output width
 * Returns: None.
 * Side Effects:
 *      sets the global values in g_width
*/
void rwstatsSetColumnWidth(int width)
{
  unsigned int i;
  int default_width[] = {
    12, /* WIDTH_COUNT: count */
    16, /* WIDTH_IPADD: ip addr (string or integer) */
     6, /* WIDTH_PORT:  port or protocol number */
     6, /* WIDTH_PROTO: port or protocol number */
    16, /* WIDTH_INTVL: interval maximum */
    11, /* WIDTH_PCT:   percentage value (add one for title width) */
  };

  if (width > 0) {
    for (i = 0; i < sizeof(g_width)/sizeof(g_width[0]); ++i) {
      g_width[i] = width;
    }
  } else {
    for (i = 0; i < sizeof(g_width)/sizeof(g_width[0]); ++i) {
      g_width[i] = default_width[i];
    }
  }
}


/*
 * rwstatsGenerateOutput
 *      Print the output
 * Arguments: NONE.
 * Returns: NONE
 * Side Effects: NONE.
*/
void rwstatsGenerateOutput(void)
{
  int proto;
  int proto_idx;

  if (g_print_titles_flag) {
    fprintf(outF, "INPUT SIZE: %u records\n", g_record_count);
  }

  /* Return if no records were read */
  if (0 == g_record_count) {
    return;
  }

  if (RWSTATS_NONE != g_wanted_stat_src_ip) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nSOURCE IPs: ");
    }
    printTopIps(g_counter_src_ip, g_wanted_stat_src_ip, g_limit_src_ip);
  }

  if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nDESTINATION IPs: ");
    }
    printTopIps(g_counter_dest_ip, g_wanted_stat_dest_ip, g_limit_dest_ip);
  }

  if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nSOURCE IP/DEST IP PAIRS: ");
    }
    printTopPairsIp();
  }

  if (RWSTATS_NONE != g_wanted_stat_src_port) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nSOURCE PORTS:  ");
    }
    printTopPorts(g_src_port_array, 65536, g_wanted_stat_src_port,
                  g_limit_src_port);
  }

  if (RWSTATS_NONE != g_wanted_stat_dest_port) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nDESTINATION PORTS:  ");
    }
    printTopPorts(g_dest_port_array, 65536, g_wanted_stat_dest_port,
                  g_limit_dest_port);
  }

  if (RWSTATS_NONE != g_wanted_stat_pair_port) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nSOURCE PORT/DEST PORT PAIRS: ");
    }
    printTopPairsPort();
  }

  if (RWSTATS_NONE != g_wanted_stat_proto) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nPROTOCOLS:  ");
    }
    printTopPorts(g_proto_array, 256, g_wanted_stat_proto, g_limit_proto);
  }

  if (g_do_stats_all_protos > 0) {
    if (g_print_titles_flag) {
      /* No trailing newline here; the print...() method adds it */
      fprintf(outF, "\nFLOW STATISTICS--ALL PROTOCOLS:  ");
    }
    printIntervals(256, 0);
  }

  for (proto = 0; proto < 256; ++proto) {
    proto_idx = g_proto_to_stats_idx[proto];
    if (proto_idx > 0) {
      if (g_print_titles_flag) {
        /* No trailing newline here; the print...() method adds it */
        fprintf(outF, "\nFLOW STATISTICS--PROTOCOL %d:  ", proto);
      }
      printIntervals(proto, proto_idx);
    }
  }
}


/*
 * printTopIps
 *      Prints the topN or bottomN IpAddr/IpCount key/value pairs in
 *      the ip_counter hash-table
 * Arguments:
 *      ip_counter - hash-table of IpAddr/IpCount key/value pairs
 *      wantedstat -the type of statistic the user requested
 *      limit -the value the user entered as her limit; the type of
 *              the value is determined by the wantedstat argument;
 *              for example, limit could be the topN, the top
 *              threshold cutoff, or the bottom threshold percentage
 * Returns: NONE.
 * Side Affects:
 *      Prints output to outF.
*/
static void printTopIps(IpCounter *ip_counter,
                        const wanted_stat_type wantedstat, const uint32_t limit)
{
  const int8_t top_or_btm = (wantedstat > RWSTATS_NONE);
  uint32_t topn;
  uint32_t unique_entries;
  uint32_t *topn_array;
  uint32_t topn_found;
  uint32_t i;
  double percent;
  double cumul_pct;

  /* Given the statistic the user wants, convert the "limit" value to
     an actual topN or bottomN */
  switch (wantedstat) {
  case RWSTATS_TOP_N:
  case RWSTATS_BTM_N:
    /* If user gave a count, we are set */
    topn = limit;
    break;
  case RWSTATS_TOP_THRESH:
    /* Convert number of records to topN */
    topn = ipThresholdToCount(ip_counter, top_or_btm, limit);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u\n", limit);
      return;
    }
    break;
  case RWSTATS_TOP_PERCENT:
    /* Convert percertage of records to topN */
    topn = ipThresholdToCount(ip_counter, top_or_btm,
                              g_record_count * 0.01 * limit);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u%%\n", limit);
      return;
    }
    break;
  case RWSTATS_BTM_THRESH:
    /* Convert number of records to bottomN */
    topn = ipThresholdToCount(ip_counter, top_or_btm, limit);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u\n", limit);
      return;
    }
    break;
  case RWSTATS_BTM_PERCENT:
    /* Convert percentage of records to bottomN */
    topn = ipThresholdToCount(ip_counter, top_or_btm,
                              g_record_count * 0.01 * limit);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u%%\n", limit);
      return;
    }
    break;
  case RWSTATS_NONE:
  default:
    fprintf(stderr, "Impossible situation!");
    abort();
  }

  /* Create an array to hold the topN or btmN IP addresses and their
     counts */
  topn_array = (uint32_t*) calloc(topn * 2, sizeof(uint32_t));
  if (NULL == topn_array) {
    fprintf(stderr, "Cannot malloc ip array\n");
    exit(1);
  }

  /* Call the function to do the actual topN/btmN */
  topn_found = calcTopIps(ip_counter, top_or_btm, topn, topn_array);

  /* Get a count of unique IPs */
  unique_entries = ipctr_count_entries(ip_counter);

  /* Print results */
  if (g_print_titles_flag) {
    switch (wantedstat) {
    case RWSTATS_TOP_N:
      fprintf(outF, "Top %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_TOP_THRESH:
      fprintf(outF, "Top %u (threshold %u) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_TOP_PERCENT:
      fprintf(outF, "Top %u (threshold %u%%) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_BTM_N:
      fprintf(outF, "Bottom %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_BTM_THRESH:
      fprintf(outF, "Bottom %u (threshold %u) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_BTM_PERCENT:
      fprintf(outF, "Bottom %u (threshold %u%%) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_NONE:
      /* keep gcc quiet */
      break;
    }
    fprintf(outF,   "%-*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_IPADD], "ip_addr", g_delim,
            g_width[WIDTH_COUNT], "rec_count", g_delim,
            1+g_width[WIDTH_PCT], "%_of_input", g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }

  cumul_pct = 0.0;
  for (i = 0; i < 2*topn_found; i += 2) {
    percent = 100.0 * (double)topn_array[i+1] / (double)g_record_count;
    cumul_pct += percent;
    if (g_integer_ip_flag == 0) {
      fprintf(outF, "%-*s%s%*u%s%*.6f%%%s%*.6f%%\n",
              g_width[WIDTH_IPADD], num2dot(topn_array[i]), g_delim,
              g_width[WIDTH_COUNT], topn_array[i+1], g_delim,
              g_width[WIDTH_PCT],percent,g_delim,g_width[WIDTH_PCT],cumul_pct);
    } else {
      fprintf(outF, "%*u%s%*u%s%*.6f%%%s%*.6f%%\n",
              g_width[WIDTH_IPADD], topn_array[i], g_delim,
              g_width[WIDTH_COUNT], topn_array[i+1], g_delim,
              g_width[WIDTH_PCT],percent,g_delim,g_width[WIDTH_PCT],cumul_pct);
    }
  }

  /* Clean up */
  free(topn_array);
}


/*
 * printTopPairsIp
 *      Prints the topN or bottomN IpSrcAddr+IpDestAddr/IpCount
 *      key/value pairs in the g_counter_pair_ip hash-table
 * Arguments: NONE.
 * Returns: NONE.
 * Side Affects:
 *      Prints output to outF.
*/
static void printTopPairsIp(void)
{
  const int8_t top_or_btm = (g_wanted_stat_pair_ip > RWSTATS_NONE);
  uint32_t topn;
  uint32_t *topn_array;
  uint32_t topn_found;
  uint32_t unique_entries;
  uint32_t i;
  double percent;
  double cumul_pct;

  /* Given the statistic the user wants, convert the "limit" value to
     an actual topN or bottomN */
  switch (g_wanted_stat_pair_ip) {
  case RWSTATS_TOP_N:
  case RWSTATS_BTM_N:
    /* If user gave a count, we are set */
    topn = g_limit_pair_ip;
    break;
  case RWSTATS_TOP_THRESH:
    /* Convert number of records to topN */
    topn = pairThresholdToCount(g_counter_pair_ip, top_or_btm,g_limit_pair_ip);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u\n",
              g_limit_pair_ip);
      return;
    }
    break;
  case RWSTATS_TOP_PERCENT:
    /* Convert percertage of records to topN */
    topn = pairThresholdToCount(g_counter_pair_ip, top_or_btm,
                                g_record_count*0.01*g_limit_pair_ip);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u%%\n",
              g_limit_pair_ip);
      return;
    }
    break;
  case RWSTATS_BTM_THRESH:
    /* Convert number of records to bottomN */
    topn = pairThresholdToCount(g_counter_pair_ip, top_or_btm,g_limit_pair_ip);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u\n",
              g_limit_pair_ip);
      return;
    }
    break;
  case RWSTATS_BTM_PERCENT:
    /* Convert percentage of records to bottomN */
    topn =pairThresholdToCount(g_counter_pair_ip, top_or_btm,
                               g_record_count*0.01*g_limit_pair_ip);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u%%\n",
              g_limit_pair_ip);
      return;
    }
    break;
  case RWSTATS_NONE:
  default:
    fprintf(stderr, "Impossible situation!");
    abort();
  }

  /* Create an array to hold the topN or btmN srcIP+destIP addresses
     and their counts */
  topn_array = (uint32_t*) calloc(topn * 3, sizeof(uint32_t));
  if (NULL == topn_array) {
    fprintf(stderr, "Cannot malloc ip array\n");
    exit(1);
  }

  /* Call the function to do the actual topN/btmN */
  topn_found = calcTopPairsIp(top_or_btm, topn, topn_array);

  /* Get a count of unique flows */
  unique_entries = tuplectr_count_entries(g_counter_pair_ip);

  /* Print results */
  if (g_print_titles_flag) {
    switch (g_wanted_stat_pair_ip) {
    case RWSTATS_TOP_N:
      fprintf(outF, "Top %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_TOP_THRESH:
      fprintf(outF, "Top %u (threshold %u) of %d unique\n",
              topn, g_limit_pair_ip, unique_entries);
      break;
    case RWSTATS_TOP_PERCENT:
      fprintf(outF, "Top %u (threshold %u%%) of %d unique\n",
              topn, g_limit_pair_ip, unique_entries);
      break;
    case RWSTATS_BTM_N:
      fprintf(outF, "Bottom %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_BTM_THRESH:
      fprintf(outF, "Bottom %u (threshold %u) of %d unique\n",
              topn, g_limit_pair_ip, unique_entries);
      break;
    case RWSTATS_BTM_PERCENT:
      fprintf(outF, "Bottom %u (threshold %u%%) of %d unique\n",
              topn, g_limit_pair_ip, unique_entries);
      break;
    case RWSTATS_NONE:
      /* keep gcc quiet */
      break;
    }
    if (g_record_count > RWSTATS_MAX_RECORDS) {
      fprintf(outF, "Only first %d records considered\n", RWSTATS_MAX_RECORDS);
    }
    fprintf(outF, "%-*s%s%-*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_IPADD], "src_ip_addr", g_delim,
            g_width[WIDTH_IPADD], "dest_ip_addr", g_delim,
            g_width[WIDTH_COUNT], "num_pairs", g_delim,
            1+g_width[WIDTH_PCT], "%_of_input", g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }

  cumul_pct = 0.0;
  for (i = 0; i < 3*topn_found; i += 3) {
    percent = 100.0 * (double)topn_array[i+2] / (double)g_record_count;
    cumul_pct += percent;
    /* use two printf's since num2dot uses static buffer */
    if (g_integer_ip_flag == 0) {
      fprintf(outF, "%-*s%s",
              g_width[WIDTH_IPADD], num2dot(topn_array[i]), g_delim);
      fprintf(outF, "%-*s%s%*u%s%*.6f%%%s%*.6f%%\n",
              g_width[WIDTH_IPADD], num2dot(topn_array[i+1]), g_delim,
              g_width[WIDTH_COUNT], topn_array[i+2], g_delim,
              g_width[WIDTH_PCT],percent,g_delim,g_width[WIDTH_PCT],cumul_pct);
    } else {
      fprintf(outF, "%*u%s%*u%s%*u%s%*.6f%%%s%*.6f%%\n",
              g_width[WIDTH_IPADD], topn_array[i], g_delim,
              g_width[WIDTH_IPADD], topn_array[i+1], g_delim,
              g_width[WIDTH_COUNT], topn_array[i+2], g_delim,
              g_width[WIDTH_PCT],percent,g_delim,g_width[WIDTH_PCT],cumul_pct);
    }
  }

  /* Clean up */
  free(topn_array);
}


/*
 * printTopPairsPort
 *      Prints the topN or bottomN SrcPort+DestPort/PortCount
 *      key/value pairs in the g_counter_pair_port hash-table
 * Arguments: NONE.
 * Returns: NONE.
 * Side Affects:
 *      Prints output to outF.
*/
static void printTopPairsPort(void)
{
  const int8_t top_or_btm = (g_wanted_stat_pair_port > RWSTATS_NONE);
  uint32_t topn;
  uint32_t *topn_array;
  uint32_t topn_found;
  uint32_t unique_entries;
  uint32_t i;
  double percent;
  double cumul_pct;

  /* Given the statistic the user wants, convert the "limit" value to
     an actual topN or bottomN */
  switch (g_wanted_stat_pair_port) {
  case RWSTATS_TOP_N:
  case RWSTATS_BTM_N:
    /* If user gave a count, we are set */
    topn = g_limit_pair_port;
    break;
  case RWSTATS_TOP_THRESH:
    /* Convert number of records to topN */
    topn = ipThresholdToCount(g_counter_pair_port, top_or_btm,
                              g_limit_pair_port);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u\n",
              g_limit_pair_port);
      return;
    }
    break;
  case RWSTATS_TOP_PERCENT:
    /* Convert percertage of records to topN */
    topn = ipThresholdToCount(g_counter_pair_port, top_or_btm,
                              g_record_count*0.01*g_limit_pair_port);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u%%\n",
              g_limit_pair_port);
      return;
    }
    break;
  case RWSTATS_BTM_THRESH:
    /* Convert number of records to bottomN */
    topn = ipThresholdToCount(g_counter_pair_port, top_or_btm,
                              g_limit_pair_port);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u\n",
              g_limit_pair_port);
      return;
    }
    break;
  case RWSTATS_BTM_PERCENT:
    /* Convert percentage of records to bottomN */
    topn = ipThresholdToCount(g_counter_pair_port, top_or_btm,
                              g_record_count*0.01*g_limit_pair_port);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u%%\n",
              g_limit_pair_port);
      return;
    }
    break;
  case RWSTATS_NONE:
  default:
    fprintf(stderr, "Impossible situation!");
    abort();
  }

  /* Create an array to hold the topN or btmN sPort+dPort and their
     counts */
  topn_array = (uint32_t*) calloc(topn * 2, sizeof(uint32_t));
  if (NULL == topn_array) {
    fprintf(stderr, "Cannot malloc port pair array\n");
    exit(1);
  }

  /* Call the function to do the actual topN/btmN */
  topn_found = calcTopIps(g_counter_pair_port, top_or_btm, topn, topn_array);

  /* Get a count of unique flows */
  unique_entries = ipctr_count_entries(g_counter_pair_port);

  /* Print results */
  if (g_print_titles_flag) {
    switch (g_wanted_stat_pair_port) {
    case RWSTATS_TOP_N:
      fprintf(outF, "Top %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_TOP_THRESH:
      fprintf(outF, "Top %u (threshold %u) of %d unique\n",
              topn, g_limit_pair_port, unique_entries);
      break;
    case RWSTATS_TOP_PERCENT:
      fprintf(outF, "Top %u (threshold %u%%) of %d unique\n",
              topn, g_limit_pair_port, unique_entries);
      break;
    case RWSTATS_BTM_N:
      fprintf(outF, "Bottom %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_BTM_THRESH:
      fprintf(outF, "Bottom %u (threshold %u) of %d unique\n",
              topn, g_limit_pair_port, unique_entries);
      break;
    case RWSTATS_BTM_PERCENT:
      fprintf(outF, "Bottom %u (threshold %u%%) of %d unique\n",
              topn, g_limit_pair_port, unique_entries);
      break;
    case RWSTATS_NONE:
      /* keep gcc quiet */
      break;
    }
#if USE_TUPLE_CTR_4_PORT_PAIR
    if (g_record_count > RWSTATS_MAX_RECORDS) {
      fprintf(outF, "Only first %d records considered\n", RWSTATS_MAX_RECORDS);
    }
#endif
    fprintf(outF, "%-*s%s%-*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_PORT], "s_Port", g_delim,
            g_width[WIDTH_PORT], "d_Port", g_delim,
            g_width[WIDTH_COUNT], "num_pairs", g_delim,
            1+g_width[WIDTH_PCT], "%_of_input", g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }

  cumul_pct = 0.0;
  for (i = 0; i < 2*topn_found; i += 2) {
    percent = 100.0 * (double)topn_array[i+1] / (double)g_record_count;
    cumul_pct += percent;
    fprintf(outF, "%*u%s%*u%s%*u%s%*.6f%%%s%*.6f%%\n",
            g_width[WIDTH_PORT], (topn_array[i] >> 16), g_delim,
            g_width[WIDTH_PORT], (topn_array[i] & 0xFFFF), g_delim,
            g_width[WIDTH_COUNT], topn_array[i+1], g_delim,
            g_width[WIDTH_PCT],percent,g_delim, g_width[WIDTH_PCT], cumul_pct);
  }

  /* Clean up */
  free(topn_array);
}


/*
 * printTopPorts
 *      Prints the topN or bottomN values in p_array---may be source
 *      ports, destination ports, or protocols
 * Arguments:
 *      p_array -array of port or protocol-counts.  the array's index
 *              is the port or protocol number
 *      p_array_size -the number of entries in the p_array
 *              (65536 for ports, 256 for protocols)
 *      wantedstat -the type of statistic the user requested
 *      limit -the value the user entered as her limit; the type of
 *              the value is determined by the wantedstat argument;
 *              for example, limit could be the topN, the top
 *              threshold cutoff, or the bottom threshold percentage
 * Returns: NONE
 * Side Affects:
 *      Prints output to outF
*/
static void printTopPorts(uint32_t *p_array, const int32_t p_array_size,
                          const wanted_stat_type wantedstat,const uint32_t limit)
{
  const int8_t top_or_btm = (wantedstat > RWSTATS_NONE);
  uint32_t *topn_array;
  uint32_t topn;
  uint32_t topn_found;
  int32_t unique_entries = 0;
  uint32_t i;
  double percent;
  double cumul_pct;

  /* Given the statistic the user wants, convert the "limit" value to
     an actual topN or bottomN */
  switch (wantedstat) {
  case RWSTATS_TOP_N:
  case RWSTATS_BTM_N:
    /* If user gave a count, we are set */
    topn = limit;
    break;
  case RWSTATS_TOP_THRESH:
    /* Convert number of records to topN */
    topn = arrayThresholdToCount(p_array, p_array_size, 1, limit);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u\n", limit);
      return;
    }
    break;
  case RWSTATS_TOP_PERCENT:
    /* Convert percertage of records to topN */
    topn = arrayThresholdToCount(p_array, p_array_size, 1,
                                 g_record_count * 0.01 * limit);
    if (topn < 1) {
      fprintf(outF, "No counts above threshold of %u%%\n", limit);
      return;
    }
    break;
  case RWSTATS_BTM_THRESH:
    /* Convert number of records to bottomN */
    topn = arrayThresholdToCount(p_array, p_array_size, -1, limit);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u\n", limit);
      return;
    }
    break;
  case RWSTATS_BTM_PERCENT:
    /* Convert percentage of records to bottomN */
    topn = arrayThresholdToCount(p_array, p_array_size, -1,
                                 g_record_count * 0.01 * limit);
    if (topn < 1) {
      fprintf(outF, "No counts below threshold of %u%%\n", limit);
      return;
    }
    break;
  case RWSTATS_NONE:
  default:
    fprintf(stderr, "Impossible situation!");
    abort();
  }

  /* Create an array to hold the topN or btmN ports/protos and their
     counts */
  topn_array = (uint32_t*) calloc(topn * 2, sizeof(uint32_t));
  if (NULL == topn_array) {
    fprintf(stderr, "Cannot malloc port/proto array\n");
    exit(1);
  }

  topn_found = calcTopPorts(p_array, p_array_size, top_or_btm, topn,
                            topn_array);

  /* Compute the unique number of entries in array */
  for (i = 0; i < (uint32_t)p_array_size; ++i) {
    if (0 == p_array[i]) {
      /* Skip ports/protocols with no hits */
      continue;
    }
    /* Increment the number of unique ports/protocols seen */
    ++unique_entries;
  }

  /* Print results. */
  if (g_print_titles_flag) {
    switch (wantedstat) {
    case RWSTATS_TOP_N:
      fprintf(outF, "Top %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_TOP_THRESH:
      fprintf(outF, "Top %u (threshold %u) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_TOP_PERCENT:
      fprintf(outF, "Top %u (threshold %u%%) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_BTM_N:
      fprintf(outF, "Bottom %u of %d unique\n", topn, unique_entries);
      break;
    case RWSTATS_BTM_THRESH:
      fprintf(outF, "Bottom %u (threshold %u) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_BTM_PERCENT:
      fprintf(outF, "Bottom %u (threshold %u%%) of %d unique\n",
              topn, limit, unique_entries);
      break;
    case RWSTATS_NONE:
      /* keep gcc quiet */
      break;
    }
    fprintf(outF, "%*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_PORT], "number", g_delim,
            g_width[WIDTH_COUNT], "rec_count", g_delim,
            1+g_width[WIDTH_PCT], "%_of_input", g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }

  cumul_pct = 0.0;
  for (i = 0; i < 2*topn_found; i += 2) {
    percent = 100.0 * (double)topn_array[i+1] / (double)g_record_count;
    cumul_pct += percent;
    fprintf(outF, "%*d%s%*u%s%*.6f%%%s%*.6f%%\n",
            g_width[WIDTH_PORT], topn_array[i], g_delim,
            g_width[WIDTH_COUNT], topn_array[i+1], g_delim,
            g_width[WIDTH_PCT],percent,g_delim, g_width[WIDTH_PCT], cumul_pct);
  }
}


/*
 * printIntervals
 *      Print min, max, and intervals for bytes, packets, and bpp for
 *      the given protocol
 * Arguments:
 *      proto - the protocol to print
 *      proto_idx - the index to use into the g_*_min, g_*_max,
 *              g_*_interval arrays to access data for this protocol
 * Returns: NONE
 * Side Effects:
 *      Prints output to outF
*/
static void printIntervals(int proto, int proto_idx)
{
  register uint32_t *byteIntervals;
  register uint32_t *pktIntervals;
  register uint32_t *bppIntervals;
  register double *quartiles;
  uint32_t rec_count;
  int i;
  char *col_title;
  double percent;
  double cumul_pct;

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

  if (256 == proto) {
    rec_count = g_record_count;
    col_title = "%_of_input";
  } else {
    rec_count = g_proto_array[proto];
    col_title = "%_of_proto";
  }
  fprintf(outF, "%d records\n", rec_count);

  if (0 == rec_count) {
    /* no records, so no data to print */
    return;
  }

  /* Compute and print min/max, Quartiles, and Intervals for each of
     bytes, packets, and bytes/packet. */
  cumul_pct = 0.0;
  quartiles = intervalQuartiles(g_byte_intervals[proto_idx],
                                byteIntervals, NUM_INTERVALS);
  fprintf(outF, "*BYTES min %u; max %u\n",
          g_bytes_min[proto_idx], g_bytes_max[proto_idx]);
  fprintf(outF, "  quartiles LQ %.5f Med %.5f UQ %.5f UQ-LQ %.5f\n",
          quartiles[0], quartiles[1], quartiles[2],
          (quartiles[2] - quartiles[0]));
  if (g_print_titles_flag) {
    fprintf(outF, "%*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_INTVL], "interval_max", g_delim,
            g_width[WIDTH_COUNT], "count<=max", g_delim,
            1+g_width[WIDTH_PCT], col_title, g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }
  for (i = 0; i < NUM_INTERVALS; ++i) {
    percent = 100.0*(double)g_byte_intervals[proto_idx][i]/(double)rec_count;
    cumul_pct += percent;
    fprintf(outF, "%*u%s%*u%s%*.6f%%%s%*.6f%%\n",
            g_width[WIDTH_INTVL], byteIntervals[i], g_delim,
            g_width[WIDTH_COUNT], g_byte_intervals[proto_idx][i], g_delim,
            g_width[WIDTH_PCT],percent, g_delim, g_width[WIDTH_PCT],cumul_pct);
  }

  cumul_pct = 0.0;
  quartiles = intervalQuartiles(g_pkt_intervals[proto_idx],
                                pktIntervals, NUM_INTERVALS);
  fprintf(outF, "*PACKETS min %u; max %u\n",
          g_pkts_min[proto_idx], g_pkts_max[proto_idx]);
  fprintf(outF, "  quartiles LQ %.5f Med %.5f UQ %.5f UQ-LQ %.5f\n",
          quartiles[0], quartiles[1], quartiles[2],
          (quartiles[2] - quartiles[0]));
  if (g_print_titles_flag) {
    fprintf(outF, "%*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_INTVL], "interval_max", g_delim,
            g_width[WIDTH_COUNT], "count<=max", g_delim,
            1+g_width[WIDTH_PCT], col_title, g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }
  for (i = 0; i < NUM_INTERVALS; ++i) {
    percent = 100.0*(double)g_pkt_intervals[proto_idx][i]/(double)rec_count;
    cumul_pct += percent;
    fprintf(outF, "%*u%s%*u%s%*.6f%%%s%*.6f%%\n",
            g_width[WIDTH_INTVL], pktIntervals[i], g_delim,
            g_width[WIDTH_COUNT], g_pkt_intervals[proto_idx][i], g_delim,
            g_width[WIDTH_PCT],percent, g_delim, g_width[WIDTH_PCT],cumul_pct);
  }

  cumul_pct = 0.0;
  quartiles = intervalQuartiles(g_bpp_intervals[proto_idx],
                                byteIntervals, NUM_INTERVALS);
  fprintf(outF, "*BYTES/PACKET min %u; max %u\n",
          g_bpp_min[proto_idx], g_bpp_max[proto_idx]);
  fprintf(outF, "  quartiles LQ %.5f Med %.5f UQ %.5f UQ-LQ %.5f\n",
          quartiles[0], quartiles[1], quartiles[2],
          (quartiles[2] - quartiles[0]));
  if (g_print_titles_flag) {
    fprintf(outF, "%*s%s%*s%s%*s%s%*s\n",
            g_width[WIDTH_INTVL], "interval_max", g_delim,
            g_width[WIDTH_COUNT], "count<=max", g_delim,
            1+g_width[WIDTH_PCT], col_title, g_delim,
            1+g_width[WIDTH_PCT], "cumul_%");
  }
  for (i = 0; i < NUM_INTERVALS; ++i) {
    percent = 100.0*(double)g_bpp_intervals[proto_idx][i]/(double)rec_count;
    cumul_pct += percent;
    fprintf(outF, "%*u%s%*u%s%*.6f%%%s%*.6f%%\n",
            g_width[WIDTH_INTVL], bppIntervals[i], g_delim,
            g_width[WIDTH_COUNT], g_bpp_intervals[proto_idx][i], g_delim,
            g_width[WIDTH_PCT],percent, g_delim, g_width[WIDTH_PCT],cumul_pct);
  }
}


/*
 * ipThresholdToCount
 *      Return the topN/bottomN required to print all IpAddr/IpCount
 *      key/value pairs whose IpCount is at-least/no-more-than
 *      threshold
 * Arguments:
 *      ip_counter - hash-table of IpAddr/IpCount key/value pairs
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      threshold -find IpCounts with at-least/no-more-than this many
 *              hits
 * Returns:
 *      number of IpAddr/IpCount pairs whose IpCount was
 *      at-least/no-more-than threshold
 * Side Affects: NONE.
*/
static uint32_t ipThresholdToCount(IpCounter *ip_counter, const int8_t top_or_btm,
                                 const uint32_t threshold)
{
  HASH_ITER iter;
  uint32_t ip_address;
  uint32_t ip_count;
  register uint32_t count = 0;

  /* Iterate over the hash-table. */
  iter = ipctr_create_iterator(ip_counter);
  if (top_or_btm > 0) {
    while (ipctr_iterate(ip_counter, &iter,
                         &ip_address, &ip_count) != ERR_NOMOREENTRIES) {
      if (ip_count >= threshold) {
        ++count;
      }
    }
  } else {
    while (ipctr_iterate(ip_counter, &iter,
                         &ip_address, &ip_count) != ERR_NOMOREENTRIES) {
      if (ip_count <= threshold) {
        ++count;
      }
    }
  }

  return count;
}


/*
 * pairThresholdToCount
 *      Return the topN/bottomN required to print all
 *      SrcXX+DestXX/PairCount key/value pairs in the tuple_counter
 *      hash-table whose PairCount is at-least/no-more-than threshold.
 *      XX can be either IpAddr or Port.
 * Arguments:
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      threshold -find PairCounts with at-least/no-more-than this
 *              many hits
 * Returns:
 *      number of SrcXX+DestXX/PairCount pairs whose PairCount was
 *      at-least/no-more than threshold
 * Side Affects: NONE.
*/
static uint32_t pairThresholdToCount(TupleCounter *tuple_counter,
                                   const int8_t top_or_btm,
                                   const uint32_t threshold)
{
  HASH_ITER iter;
  rwRec rwrec;
  register rwRec *rwrecPtr = &rwrec;
  uint32_t pair_count;
  register uint32_t count = 0;

  /* Iterate over the hash-table. */
  iter = tuplectr_create_iterator(tuple_counter);
  if (top_or_btm > 0) {
    while (tuplectr_rec_iterate(tuple_counter, &iter,
                                rwrecPtr, &pair_count) != ERR_NOMOREENTRIES) {
      if (pair_count >= threshold) {
        ++count;
      }
    }
  } else {
    while (tuplectr_rec_iterate(tuple_counter, &iter,
                                rwrecPtr, &pair_count) != ERR_NOMOREENTRIES) {
      if (pair_count <= threshold) {
        ++count;
      }
    }
  }

  return count;
}


/*
 * arrayThresholdToCount
 *      Return the topN/bottomN required to print all source ports,
 *      destination ports, or protocols in the p_array whose count
 *      is at-least/no-more-than threshold.
 * Arguments:
 *      p_array -array of port or protocol -counts.  the array's
 *              index is the port or protocol number
 *      p_array_size -the number of entries in the p_array
 *              (65536 for ports, 256 for protocols)
 *      top_or_btm -whether to compute for topN(1) or bottomN(0)
 *      threshold -find counts with at-least/no-more-than this many
 *              hits
 * Returns:
 *      number of ports or protocols whose count was
 *      at-least/no-more-than threshold
 * Side Affects: NONE.
*/
static uint32_t arrayThresholdToCount(uint32_t* p_array, const int p_array_size,
                                    const int8_t top_or_btm,
                                    const uint32_t threshold)
{
  uint32_t *idx_p;
  register uint32_t count = 0;

  if (top_or_btm > 0) {
    /* Iterate over the elements in the array */
    for (idx_p = p_array; idx_p < p_array + p_array_size; ++idx_p) {
      if (*idx_p >= threshold) {
        ++count;
      }
    }
  } else {
    /* Iterate over the elements in the array */
    for (idx_p = p_array; idx_p < p_array + p_array_size; ++idx_p) {
      if (*idx_p > 0 && *idx_p <= threshold) {
        ++count;
      }
    }
  }

  return count;
}


