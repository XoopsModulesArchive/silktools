#ifndef _RWSTATS_H
#define _RWSTATS_H
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
** 1.9
** 2003/12/10 22:00:50
** thomasm
*/

/*@unused@*/ static char rcsID_RWSTATS_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "rwstats.h,v 1.9 2003/12/10 22:00:50 thomasm Exp";


/*
**  rwstats.h
**
**  Header file for the rwstats application.  See rwstats.c for a full
**  explanation.
**
*/



/* The type of statistic the user may ask us to compute for each of
 * source ip, dest ip, source port, dest port, protocol,
 * sip+dip-pairs */
typedef enum _wanted_stat {
  RWSTATS_BTM_N = -1, RWSTATS_BTM_THRESH = -2, RWSTATS_BTM_PERCENT = -3,
  RWSTATS_NONE = 0,
  RWSTATS_TOP_N = 1,  RWSTATS_TOP_THRESH = 2,  RWSTATS_TOP_PERCENT = 3
} wanted_stat_type;


/* Total number of records read */
extern uint32_t g_record_count;

/* SOURCE IP: the counter, the initial size of counter, what type of
 * stat to compute, the limit for that stat as entered by user. */
extern IpCounter *g_counter_src_ip;
extern int g_init_size_counter_src_ip;
extern wanted_stat_type g_wanted_stat_src_ip;
extern uint32_t g_limit_src_ip;

/* DEST IP: the counter, the initial size of counter, what type of
 * stat to compute, the limit for that stat as entered by user */
extern IpCounter *g_counter_dest_ip;
extern int g_init_size_counter_dest_ip;
extern wanted_stat_type g_wanted_stat_dest_ip;
extern uint32_t g_limit_dest_ip;

/* The max number of records to accept for Src/Dest pairs.  All other
 * input is unlimited */
#define RWSTATS_MAX_RECORDS 50000000   /* 50,000,000 */

/* SOURCE/DESTINATION IP PAIRS: the counter, the initial size of
 * counter, what type of stat to compute, the limit for that stat as
 * entered by user */
extern TupleCounter *g_counter_pair_ip;
extern uint32_t g_init_size_counter_pair_ip;
extern wanted_stat_type g_wanted_stat_pair_ip;
extern uint32_t g_limit_pair_ip;

/* SOURCE/DESTINATION PORT PAIRS: the counter, the initial size of
 * counter, what type of stat to compute, the limit for that stat as
 * entered by user */
#if USE_TUPLE_CTR_4_PORT_PAIR
extern TupleCounter *g_counter_pair_port;
#else
extern IpCounter *g_counter_pair_port;
#endif
extern uint32_t g_init_size_counter_pair_port;
extern wanted_stat_type g_wanted_stat_pair_port;
extern uint32_t g_limit_pair_port;

/* SOURCE PORTS: the counter, what type of stat to compute, the limit
 * for that stat as entered by user */
extern uint32_t g_src_port_array[];
extern wanted_stat_type g_wanted_stat_src_port;
extern uint32_t g_limit_src_port;

/* DEST PORTS: the counter, what type of stat to compute, the limit
 * for that stat as entered by user */
extern uint32_t g_dest_port_array[];
extern wanted_stat_type g_wanted_stat_dest_port;
extern uint32_t g_limit_dest_port;

/* PROTOCOL: the counter, what type of stat to compute, the limit for
 * that stat as entered by user */
extern uint32_t g_proto_array[];
extern wanted_stat_type g_wanted_stat_proto;
extern uint32_t g_limit_proto;

/* Statistics (min, max, quartiles, intervals) for "continuous" values
 * (bytes, packets, bpp) can be computed over all protocols, and the
 * can be broken out for a limited number of specific protocols.  This
 * defines the size of the data structures to hold these statistics.
 * This is one more that the number of specific protocols allowed. */
#define RWSTATS_NUM_PROTO 9

/* The following is true if the stats are to be calculated over all
 * protocols. */
extern int g_do_stats_all_protos;

/* These arrays hold the statistics.  Position 0 is for the
 * combination of all statistics. */
extern uint32_t g_bytes_min[];
extern uint32_t g_bytes_max[];
extern uint32_t g_byte_intervals[][NUM_INTERVALS];

extern uint32_t g_pkts_min[];
extern uint32_t g_pkts_max[];
extern uint32_t g_pkt_intervals[][NUM_INTERVALS];

extern uint32_t g_bpp_min[];
extern uint32_t g_bpp_max[];
extern uint32_t g_bpp_intervals[][NUM_INTERVALS];

/* This maps the protocol number to the index in the above statistics
 * arrays.  If the value for a protocol is 0, the user did not request
 * detailed specs on that protocol. */
extern int16_t g_proto_to_stats_idx[];

/* Whether to print input filenames */
extern int8_t g_print_filenames_flag;

/* Whether to print column and section titles */
extern int8_t g_print_titles_flag;

/* The delimiter string to print between columns */
extern char *g_delim;

/* Whether to print IP addrs as integers */
extern int8_t g_integer_ip_flag;

/* CIDR block mask for src and dest ips.  If 0, use all bits;
 * otherwise, the IP address should be bitwised ANDed with this
 * value. */
extern uint32_t g_cidr_src;
extern uint32_t g_cidr_dest;


extern iochecksInfoStructPtr ioISP;
extern FILE *outF;
extern char *outFPath;
extern int outIsPipe;

#define	NUM_REQUIRED_ARGS	1


/* from rwstatsutils.c */
void appUsage(void);			/* never returns */
void appSetup(int argc, char **argv);	/* never returns */
void appTeardown(void);

/* from rwstatsio.c */
void rwstatsSetColumnWidth(int width);
void rwstatsGenerateOutput(void);

/* from rwstats.c */
uint32_t calcTopIps(IpCounter *ip_counter, const int8_t top_or_btm,
                  const uint32_t topn, uint32_t *topn_array);
uint32_t calcTopPairs(const int8_t top_or_btm, const uint32_t topn,
                    uint32_t *topn_array);
uint32_t calcTopPairsIp(const int8_t top_or_btm, const uint32_t topn,
                      uint32_t *topn_array);
#if USE_TUPLE_CTR_4_PORT_PAIR
uint32_t calcTopPairsPort(const int8_t top_or_btm, const uint32_t topn,
                        uint32_t *topn_array);
#endif
uint32_t calcTopPorts(uint32_t* p_array, const int32_t p_array_size,
                    const int8_t top_or_btm, const uint32_t topn,
                    uint32_t *topn_array);


#endif /* _RWSTATS_H */
