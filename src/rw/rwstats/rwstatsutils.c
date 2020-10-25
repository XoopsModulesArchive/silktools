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
** 1.11
** 2004/03/10 22:45:13
** thomasm
*/


/*
**  rwstatsutils.c
**
**  utility functions for the rwstats application.  See rwstats.c for
**  a full explanation.
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "fglob.h"
#include "interval.h"
#include "iochecks.h"
#include "utils.h"
#include "hashlib.h"
#include "hashwrap.h"
#include "rwstats.h"

RCSIDENT("rwstatsutils.c,v 1.11 2004/03/10 22:45:13 thomasm Exp");


/* local functions */
static void appUsageLong(void);
static void appOptionsUsage(void);
static int  appOptionsSetup(void);
static int  appOptionsHandler(clientData cData, int index, char *optarg);
static int  parseProtos(char *arg);

/* exported variables */

/* local variables */
static uint8_t abortFlag = 0;

typedef enum _rwstatsAppOptionEnum {
  STATOPT_SIP_TOP_N, STATOPT_SIP_TOP_THRESHOLD, STATOPT_SIP_TOP_PCT,
  STATOPT_SIP_BTM_N, STATOPT_SIP_BTM_THRESHOLD, STATOPT_SIP_BTM_PCT,

  STATOPT_DIP_TOP_N, STATOPT_DIP_TOP_THRESHOLD, STATOPT_DIP_TOP_PCT,
  STATOPT_DIP_BTM_N, STATOPT_DIP_BTM_THRESHOLD, STATOPT_DIP_BTM_PCT,

  STATOPT_PAIR_TOP_N, STATOPT_PAIR_TOP_THRESHOLD, STATOPT_PAIR_TOP_PCT,
  STATOPT_PAIR_BTM_N, STATOPT_PAIR_BTM_THRESHOLD, STATOPT_PAIR_BTM_PCT,

  STATOPT_SPORT_TOP_N, STATOPT_SPORT_TOP_THRESHOLD, STATOPT_SPORT_TOP_PCT,
  STATOPT_SPORT_BTM_N, STATOPT_SPORT_BTM_THRESHOLD, STATOPT_SPORT_BTM_PCT,

  STATOPT_DPORT_TOP_N, STATOPT_DPORT_TOP_THRESHOLD, STATOPT_DPORT_TOP_PCT,
  STATOPT_DPORT_BTM_N, STATOPT_DPORT_BTM_THRESHOLD, STATOPT_DPORT_BTM_PCT,

  STATOPT_PORTPAIR_TOP_N, STATOPT_PORTPAIR_TOP_THRESHOLD,
  STATOPT_PORTPAIR_TOP_PCT, STATOPT_PORTPAIR_BTM_N,
  STATOPT_PORTPAIR_BTM_THRESHOLD, STATOPT_PORTPAIR_BTM_PCT,

  STATOPT_PROTO_TOP_N, STATOPT_PROTO_TOP_THRESHOLD, STATOPT_PROTO_TOP_PCT,
  STATOPT_PROTO_BTM_N, STATOPT_PROTO_BTM_THRESHOLD, STATOPT_PROTO_BTM_PCT,

  STATOPT_OVERALL_STATS, STATOPT_DETAIL_PROTO_STATS,
  STATOPT_PRINT_FILENAMES, STATOPT_NO_TITLES, STATOPT_DELIMITED,
  STATOPT_INTEGER_IPS,

  STATOPT_CIDR_SRC, STATOPT_CIDR_DEST
} rwstatsAppOptionEnum;

static struct option appOptions[] = {
  {"sip-topn", REQUIRED_ARG, 0, STATOPT_SIP_TOP_N},
  {"sip-top-threshold", REQUIRED_ARG, 0, STATOPT_SIP_TOP_THRESHOLD},
  {"sip-top-pct", REQUIRED_ARG, 0, STATOPT_SIP_TOP_PCT},
  {"sip-btmn", REQUIRED_ARG, 0, STATOPT_SIP_BTM_N},
  {"sip-btm-threshold", REQUIRED_ARG, 0, STATOPT_SIP_BTM_THRESHOLD},
  {"sip-btm-pct", REQUIRED_ARG, 0, STATOPT_SIP_BTM_PCT},

  {"dip-topn", REQUIRED_ARG, 0, STATOPT_DIP_TOP_N},
  {"dip-top-threshold", REQUIRED_ARG, 0, STATOPT_DIP_TOP_THRESHOLD},
  {"dip-top-pct", REQUIRED_ARG, 0, STATOPT_DIP_TOP_PCT},
  {"dip-btmn", REQUIRED_ARG, 0, STATOPT_DIP_BTM_N},
  {"dip-btm-threshold", REQUIRED_ARG, 0, STATOPT_DIP_BTM_THRESHOLD},
  {"dip-btm-pct", REQUIRED_ARG, 0, STATOPT_DIP_BTM_PCT},

  {"pair-topn", REQUIRED_ARG, 0, STATOPT_PAIR_TOP_N},
  {"pair-top-threshold", REQUIRED_ARG, 0, STATOPT_PAIR_TOP_THRESHOLD},
  {"pair-top-pct", REQUIRED_ARG, 0, STATOPT_PAIR_TOP_PCT},
  {"pair-btmn", REQUIRED_ARG, 0, STATOPT_PAIR_BTM_N},
  {"pair-btm-threshold", REQUIRED_ARG, 0, STATOPT_PAIR_BTM_THRESHOLD},
  {"pair-btm-pct", REQUIRED_ARG, 0, STATOPT_PAIR_BTM_PCT},

  {"sport-topn", REQUIRED_ARG, 0, STATOPT_SPORT_TOP_N},
  {"sport-top-threshold", REQUIRED_ARG, 0, STATOPT_SPORT_TOP_THRESHOLD},
  {"sport-top-pct", REQUIRED_ARG, 0, STATOPT_SPORT_TOP_PCT},
  {"sport-btmn", REQUIRED_ARG, 0, STATOPT_SPORT_BTM_N},
  {"sport-btm-threshold", REQUIRED_ARG, 0, STATOPT_SPORT_BTM_THRESHOLD},
  {"sport-btm-pct", REQUIRED_ARG, 0, STATOPT_SPORT_BTM_PCT},

  {"dport-topn", REQUIRED_ARG, 0, STATOPT_DPORT_TOP_N},
  {"dport-top-threshold", REQUIRED_ARG, 0, STATOPT_DPORT_TOP_THRESHOLD},
  {"dport-top-pct", REQUIRED_ARG, 0, STATOPT_DPORT_TOP_PCT},
  {"dport-btmn", REQUIRED_ARG, 0, STATOPT_DPORT_BTM_N},
  {"dport-btm-threshold", REQUIRED_ARG, 0, STATOPT_DPORT_BTM_THRESHOLD},
  {"dport-btm-pct", REQUIRED_ARG, 0, STATOPT_DPORT_BTM_PCT},

  {"portpair-topn", REQUIRED_ARG, 0, STATOPT_PORTPAIR_TOP_N},
  {"portpair-top-threshold", REQUIRED_ARG, 0, STATOPT_PORTPAIR_TOP_THRESHOLD},
  {"portpair-top-pct", REQUIRED_ARG, 0, STATOPT_PORTPAIR_TOP_PCT},
  {"portpair-btmn", REQUIRED_ARG, 0, STATOPT_PORTPAIR_BTM_N},
  {"portpair-btm-threshold", REQUIRED_ARG, 0, STATOPT_PORTPAIR_BTM_THRESHOLD},
  {"portpair-btm-pct", REQUIRED_ARG, 0, STATOPT_PORTPAIR_BTM_PCT},

  {"proto-topn", REQUIRED_ARG, 0, STATOPT_PROTO_TOP_N},
  {"proto-top-threshold", REQUIRED_ARG, 0, STATOPT_PROTO_TOP_THRESHOLD},
  {"proto-top-pct", REQUIRED_ARG, 0, STATOPT_PROTO_TOP_PCT},
  {"proto-btmn", REQUIRED_ARG, 0, STATOPT_PROTO_BTM_N},
  {"proto-btm-threshold", REQUIRED_ARG, 0, STATOPT_PROTO_BTM_THRESHOLD},
  {"proto-btm-pct", REQUIRED_ARG, 0, STATOPT_PROTO_BTM_PCT},

  {"overall-stats", NO_ARG, 0, STATOPT_OVERALL_STATS},
  {"detail-proto-stats", REQUIRED_ARG, 0, STATOPT_DETAIL_PROTO_STATS},
  {"print-filenames", NO_ARG, 0, STATOPT_PRINT_FILENAMES},
  {"no-titles", NO_ARG, 0, STATOPT_NO_TITLES},
  {"delimited", OPTIONAL_ARG, 0, STATOPT_DELIMITED},
  {"integer-ips", NO_ARG, 0, STATOPT_INTEGER_IPS},

  {"cidr-src", REQUIRED_ARG, 0, STATOPT_CIDR_SRC},
  {"cidr-dest", REQUIRED_ARG, 0, STATOPT_CIDR_DEST},
  {0,0,0,0}			/* sentinal entry */
};

/* -1 to remove the sentinal */
static int appOptionCount = sizeof(appOptions)/sizeof(struct option) - 1;
static char *appHelp[] = {
  /* add help strings here for the applications options */
  "print topN (by rec-count) src ips",
  "print src ips with rec-count >= top-threshold",
  "print src ips with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) src ips.",
  "print src ips with rec-count <= btm-threshold",
  "print src ips with %of-rec-count <= btm-pct",

  "print topN (by rec-count) dest ips",
  "print dest ips with rec-count >= top-threshold",
  "print dest ips with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) dest ips.",
  "print dest ips with rec-count <= btm-threshold",
  "print dest ips with %of-rec-count <= btm-pct",

  "print topN (by rec-count) src+dest ip pairs",
  "print src+dest ip pairs with rec-count >= top-threshold",
  "print src+dest ip pairs with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) src+dest ip pairs.",
  "print src+dest ip pairs with rec-count <= btm-threshold",
  "print src+dest ip pairs with %of-rec-count <= btm-pct",

  "print topN (by rec-count) src ports",
  "print src ports with rec-count >= top-threshold",
  "print src ports with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) src ports.",
  "print src ports with rec-count <= btm-threshold",
  "print src ports with %of-rec-count <= btm-pct",

  "print topN (by rec-count) dest ports",
  "print dest ports with rec-count >= top-threshold",
  "print dest ports with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) dest ports.",
  "print dest ports with rec-count <= btm-threshold",
  "print dest ports with %of-rec-count <= btm-pct",

  "print topN (by rec-count) sPort+dPort pairs",
  "print sPort+dPort pairs with rec-count >= top-threshold",
  "print sPort+dPort pairs with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) sPort+dPort pairs.",
  "print sPort+dPort pairs with rec-count <= btm-threshold",
  "print sPort+dPort pairs with %of-rec-count <= btm-pct",

  "print topN (by rec-count) protos",
  "print protos with rec-count >= top-threshold",
  "print protos with %of-rec-count >= top-pct",
  "print bottomN (by rec-count) protos.",
  "print protos with rec-count <= btm-threshold",
  "print protos with %of-rec-count <= btm-pct",

  "whether to print overall stats for all protocols.  Def NO",
  "up to 8 protocols to print detailed stats for.  Def NONE",
  "print input filenames. Def. NO",
  "turn off section and column titles. Def: print titles",
  "disable fixed-width columns; optionally change delimiter (Def '|')",
  "print ips as integers.  Def: dotted decimal",

  "consider first N bits of src ip (sip-* & pair-* opts) Def 32",
  "consider first N bits of dest ip (dip-* & pair-* opts) Def 32",

  (char *)NULL
};


/*
 * appSetup
 *	do all the setup for this application include setting up required
 *	modules etc.
 * Arguments:
 *	argc, argv
 * Returns: None.
 * Side Effects:
 *	exits with code 1 if anything does not work.
*/
void appSetup(int argc, char **argv) {
  int proto_idx;
  skAppRegister(argv[0]);
  outF = stdout;		/* default */

  /* check that we have the same number of options entries and help*/
  if ((sizeof(appHelp)/sizeof(char *) - 1) != appOptionCount) {
    fprintf(stderr, "mismatch in option and help count\n");
    exit(1);
  }

  if ((argc - 1) < NUM_REQUIRED_ARGS) {
    appUsage();		/* never returns */
  }

  if (optionsSetup(appUsageLong)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);
  }

  /* allow for 0 input and 0 output pipes */
  ioISP = iochecksSetup(0, 0, argc, argv);

  if (fglobSetup()) {
    fprintf(stderr, "%s: unable to setup fglob module\n", skAppName());
    exit(1);
  }

  if (appOptionsSetup()) {
    fprintf(stderr, "%s: unable to setup app options\n", skAppName());
    exit(1);
  }

  /* Initialize the output field widths to their defaults */
  rwstatsSetColumnWidth(-1);

  /* parse options */
  if ( (ioISP->firstFile = optionsParse(argc, argv)) < 0) {
    appUsage();/* never returns */
  }


  /*
  ** Input file checks:  if libfglob is NOT used, then check
  ** if files given on command line.  Do NOT allow both libfglob
  ** style specs and cmd line files. If cmd line files are NOT given,
  ** then default to stdin.
  */
  if (fglobValid()) {
    if (ioISP->firstFile < argc) {
      fprintf(stderr, "Error: both fglob options & input files given\n");
      exit(1);
    }
  } else {
    if (ioISP->firstFile == argc) {
      if (FILEIsATty(stdin)) {
        appUsage();             /* never returns */
      }
      if (iochecksInputSource(ioISP, "stdin")) {
        exit(1);
      }
    }
  }

  /* check input files, pipes, etc */
  if (iochecksInputs(ioISP, fglobValid())) {
    appUsage();
  }

  /* Create IP counters for the sIP, dIP. If we cannot, exit the
   * application. */
  if (RWSTATS_NONE != g_wanted_stat_src_ip) {
    g_counter_src_ip = ipctr_create_counter(g_init_size_counter_src_ip);
    if (NULL == g_counter_src_ip) {
      fprintf(stderr, "Cannot create source IP counter.\n");
      exit(1);
    }
  }

  if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
    g_counter_dest_ip = ipctr_create_counter(g_init_size_counter_dest_ip);
    if (NULL == g_counter_dest_ip) {
      fprintf(stderr, "Cannot create destination IP counter.\n");
      exit(1);
    }
  }

  /* Create a counter for the flows between source and destination IPs */
  if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
    g_counter_pair_ip = tuplectr_create_counter(FIELD_SRC_IP | FIELD_DEST_IP,
                                                g_init_size_counter_pair_ip);
    if (NULL == g_counter_pair_ip) {
      fprintf(stderr, "Cannot create source/dest ip addr counter.\n");
      exit(1);
    }
  }

  /* Create a counter for the flows between source and destination
     ports.  Use an IP counter: map the two 16 bit ports into one 32
     bit number. */
  if (RWSTATS_NONE != g_wanted_stat_pair_port) {
    g_counter_pair_port = ipctr_create_counter(g_init_size_counter_pair_port);
    if (NULL == g_counter_pair_port) {
      fprintf(stderr, "Cannot create source/dest port counter.\n");
      exit(1);
    }
  }

  /* Set the minima to a big value, like INT_MAX */
  for (proto_idx = 0; proto_idx < RWSTATS_NUM_PROTO; ++proto_idx) {
    g_bytes_min[proto_idx] = INT_MAX;
    g_pkts_min[proto_idx] = INT_MAX;
    g_bpp_min[proto_idx] = INT_MAX;
  }

  if (atexit(appTeardown) < 0) {
    fprintf(stderr, "%s: unable to register appTeardown() with atexit()\n",
            skAppName());
    abortFlag = 1;		/* abort */
    appTeardown();
    exit(1);
  }

  return;			/* OK */
}


/*
 * appUsage:
 * 	print usage information to stderr and exit with code 1
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
void appUsage(void) {
  fprintf(stderr, "Use `%s --help' for usage\n", skAppName());
  exit(1);
}


/*
 * appUsageLong:
 * 	print usage information to stderr and exit with code 1.
 * 	passed to optionsSetup() to print usage when --help option
 * 	given.
 * Arguments: None
 * Returns: None
 * Side Effects: exits application.
*/
static void appUsageLong(void) {
  fprintf(stderr, "%s [app_opts] [fglob_opts] .... \n", skAppName());
  fglobUsage();
  appOptionsUsage();
  exit(1);
}


/*
 * appOptionsUsage:
 * 	print options for this app to stderr.
 * Arguments: None.
 * Returns: None.
 * Side Effects: None.
*/
static void appOptionsUsage(void) {
  register int i;
  fprintf(stderr, "\napp options:\n");
  for (i = 0; i < appOptionCount; i++ ) {
    fprintf(stderr, "--%s %s. %s\n", appOptions[i].name,
            appOptions[i].has_arg ? appOptions[i].has_arg == 1
            ? "Req. Arg" : "Opt. Arg" : "No Arg", appHelp[i]);
  }

  return;
}


/*
 * appOptionsSetup:
 * 	setup to parse application options.
 * Arguments: None.
 * Returns:
 *	0 OK; 1 else.
*/
static int  appOptionsSetup(void) {
  /* register the apps  options handler */
  if (optionsRegister(appOptions, (optHandler) appOptionsHandler,
                      (clientData) 0)) {
    fprintf(stderr, "%s: unable to register application options\n",
            skAppName());
    return 1;			/* error */
  }
  return 0;			/* OK */
}


/*
 * appOptionsHandler:
 * 	called for each option the app has registered.
 * Arguments:
 *	clientData cData: ignored here.
 *	int index: index into appOptions of the specific option
 *	char *optarg: the argument; 0 if no argument was required/given.
 * Returns:
 *	0 if OK. 1 else.
 * Side Effects:
 *	Relevant options are set.
*/
static int appOptionsHandler(clientData UNUSED(cData), int index,char *optarg){
  uint32_t val;
  char *post_optarg; /* used to see if strtoul processed optarg */

  if (index < 0 || index >= appOptionCount) {
    fprintf(stderr, "%s: invalid index %d\n", skAppName(), index);
    return 1;
  }

  switch (index) {
  case STATOPT_SIP_TOP_N:
    /* top N source IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      fprintf(stderr, "May only specify one --sip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal source IP count.\n");
      return 1;
    }
    g_wanted_stat_src_ip = RWSTATS_TOP_N;
    g_limit_src_ip = (uint32_t) val;
    break;

  case STATOPT_SIP_TOP_THRESHOLD:
    /* top threshold source IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      fprintf(stderr, "May only specify one --sip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal source IP threshold.\n");
      return 1;
    }
    g_wanted_stat_src_ip = RWSTATS_TOP_THRESH;
    g_limit_src_ip = (uint32_t) val;
    break;

  case STATOPT_SIP_TOP_PCT:
    /* top percentage source IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      fprintf(stderr, "May only specify one --sip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal source IP top percentage.\n");
      return 1;
    }
    g_wanted_stat_src_ip = RWSTATS_TOP_PERCENT;
    g_limit_src_ip = val;
    break;

  case STATOPT_SIP_BTM_N:
    /* bottom N source IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      fprintf(stderr, "May only specify one --sip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal source IP count.\n");
      return 1;
    }
    g_wanted_stat_src_ip = RWSTATS_BTM_N;
    g_limit_src_ip = (uint32_t) val;
    break;

  case STATOPT_SIP_BTM_THRESHOLD:
    /* bottom threshold source IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      fprintf(stderr, "May only specify one --sip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal source IP threshold.\n");
      return 1;
    }
    g_wanted_stat_src_ip = RWSTATS_BTM_THRESH;
    g_limit_src_ip = (uint32_t) val;
    break;

  case STATOPT_SIP_BTM_PCT:
    /* bottom percentage source IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_ip) {
      fprintf(stderr, "May only specify one --sip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal source IP bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_src_ip = RWSTATS_BTM_PERCENT;
    g_limit_src_ip = val;
    break;

  case STATOPT_DIP_TOP_N:
    /* top N destination IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      fprintf(stderr, "May only specify one --dip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal destination IP count.\n");
      return 1;
    }
    g_wanted_stat_dest_ip = RWSTATS_TOP_N;
    g_limit_dest_ip = (uint32_t) val;
    break;

  case STATOPT_DIP_TOP_THRESHOLD:
    /* top threshold destination IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      fprintf(stderr, "May only specify one --dip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal destination IP threshold.\n");
      return 1;
    }
    g_wanted_stat_dest_ip = RWSTATS_TOP_THRESH;
    g_limit_dest_ip = (uint32_t) val;
    break;

  case STATOPT_DIP_TOP_PCT:
    /* top percentage destination IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      fprintf(stderr, "May only specify one --dip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal destination IP top percentage.\n");
      return 1;
    }
    g_wanted_stat_dest_ip = RWSTATS_TOP_PERCENT;
    g_limit_dest_ip = val;
    break;

  case STATOPT_DIP_BTM_N:
    /* bottom N destination IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      fprintf(stderr, "May only specify one --dip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal destination IP count.\n");
      return 1;
    }
    g_wanted_stat_dest_ip = RWSTATS_BTM_N;
    g_limit_dest_ip = (uint32_t) val;
    break;

  case STATOPT_DIP_BTM_THRESHOLD:
    /* bottom threshold destination IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      fprintf(stderr, "May only specify one --dip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal destination IP threshold.\n");
      return 1;
    }
    g_wanted_stat_dest_ip = RWSTATS_BTM_THRESH;
    g_limit_dest_ip = (uint32_t) val;
    break;

  case STATOPT_DIP_BTM_PCT:
    /* bottom percentage destination IPs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_ip) {
      fprintf(stderr, "May only specify one --dip-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal destination IP bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_dest_ip = RWSTATS_BTM_PERCENT;
    g_limit_dest_ip = val;
    break;

  case STATOPT_PAIR_TOP_N:
    /* top N src ip+dest ip pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      fprintf(stderr, "May only specify one --pair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal src ip+dest ip pair count.\n");
      return 1;
    }
    g_wanted_stat_pair_ip = RWSTATS_TOP_N;
    g_limit_pair_ip = (uint32_t) val;
    break;

  case STATOPT_PAIR_TOP_THRESHOLD:
    /* top threshold src ip+dest ip pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      fprintf(stderr, "May only specify one --pair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal src ip+dest ip pair threshold.\n");
      return 1;
    }
    g_wanted_stat_pair_ip = RWSTATS_TOP_THRESH;
    g_limit_pair_ip = (uint32_t) val;
    break;

  case STATOPT_PAIR_TOP_PCT:
    /* top percentage src ip+dest ip pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      fprintf(stderr, "May only specify one --pair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal src ip+dest ip pair top percentage.\n");
      return 1;
    }
    g_wanted_stat_pair_ip = RWSTATS_TOP_PERCENT;
    g_limit_pair_ip = val;
    break;

  case STATOPT_PAIR_BTM_N:
    /* bottom N src ip+dest ip pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      fprintf(stderr, "May only specify one --pair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal src ip+dest ip pair count.\n");
      return 1;
    }
    g_wanted_stat_pair_ip = RWSTATS_BTM_N;
    g_limit_pair_ip = (uint32_t) val;
    break;

  case STATOPT_PAIR_BTM_THRESHOLD:
    /* bottom threshold src ip+dest ip pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      fprintf(stderr, "May only specify one --pair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal src ip+dest ip pair threshold.\n");
      return 1;
    }
    g_wanted_stat_pair_ip = RWSTATS_BTM_THRESH;
    g_limit_pair_ip = (uint32_t) val;
    break;

  case STATOPT_PAIR_BTM_PCT:
    /* bottom percentage src ip+dest ip pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_ip) {
      fprintf(stderr, "May only specify one --pair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal src ip+dest ip pair bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_pair_ip = RWSTATS_BTM_PERCENT;
    g_limit_pair_ip = val;
    break;

  case STATOPT_SPORT_TOP_N:
    /* top N source ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      fprintf(stderr, "May only specify one --sport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for source port top N.\n");
      return 1;
    }
    g_wanted_stat_src_port = RWSTATS_TOP_N;
    g_limit_src_port = val;
    break;

  case STATOPT_SPORT_TOP_THRESHOLD:
    /* top threshold source ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      fprintf(stderr, "May only specify one --sport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for source port top threshold.\n");
      return 1;
    }
    g_wanted_stat_src_port = RWSTATS_TOP_THRESH;
    g_limit_src_port = val;
    break;

  case STATOPT_SPORT_TOP_PCT:
    /* top percentage source ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      fprintf(stderr, "May only specify one --sport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal value for source port top percentage.\n");
      return 1;
    }
    g_wanted_stat_src_port = RWSTATS_TOP_PERCENT;
    g_limit_src_port = val;
    break;

  case STATOPT_SPORT_BTM_N:
    /* bottom N source ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      fprintf(stderr, "May only specify one --sport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for source port bottom N.\n");
      return 1;
    }
    g_wanted_stat_src_port = RWSTATS_BTM_N;
    g_limit_src_port = val;
    break;

  case STATOPT_SPORT_BTM_THRESHOLD:
    /* bottom threshold source ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      fprintf(stderr, "May only specify one --sport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for source port bottom threshold.\n");
      return 1;
    }
    g_wanted_stat_src_port = RWSTATS_BTM_THRESH;
    g_limit_src_port = val;
    break;

  case STATOPT_SPORT_BTM_PCT:
    /* bottom percentage source ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_src_port) {
      fprintf(stderr, "May only specify one --sport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal value for source port bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_src_port = RWSTATS_BTM_PERCENT;
    g_limit_src_port = val;
    break;

  case STATOPT_DPORT_TOP_N:
    /* top N destination ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      fprintf(stderr, "May only specify one --dport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for destination port top N.\n");
      return 1;
    }
    g_wanted_stat_dest_port = RWSTATS_TOP_N;
    g_limit_dest_port = val;
    break;

  case STATOPT_DPORT_TOP_THRESHOLD:
    /* top threshold destination ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      fprintf(stderr, "May only specify one --dport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for destination port top threshold.\n");
      return 1;
    }
    g_wanted_stat_dest_port = RWSTATS_TOP_THRESH;
    g_limit_dest_port = val;
    break;

  case STATOPT_DPORT_TOP_PCT:
    /* top percentage destination ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      fprintf(stderr, "May only specify one --dport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal value for destination port top percentage.\n");
      return 1;
    }
    g_wanted_stat_dest_port = RWSTATS_TOP_PERCENT;
    g_limit_dest_port = val;
    break;

  case STATOPT_DPORT_BTM_N:
    /* bottom N destination ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      fprintf(stderr, "May only specify one --dport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for destination port bottom N.\n");
      return 1;
    }
    g_wanted_stat_dest_port = RWSTATS_BTM_N;
    g_limit_dest_port = val;
    break;

  case STATOPT_DPORT_BTM_THRESHOLD:
    /* bottom threshold destination ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      fprintf(stderr, "May only specify one --dport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for destination port bottom threshold.\n");
      return 1;
    }
    g_wanted_stat_dest_port = RWSTATS_BTM_THRESH;
    g_limit_dest_port = val;
    break;

  case STATOPT_DPORT_BTM_PCT:
    /* bottom percentage destination ports */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_dest_port) {
      fprintf(stderr, "May only specify one --dport-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr,
              "Illegal value for destination port bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_dest_port = RWSTATS_BTM_PERCENT;
    g_limit_dest_port = val;
    break;

  case STATOPT_PORTPAIR_TOP_N:
    /* top N sPort+dPort pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      fprintf(stderr, "May only specify one --portpair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal sPort+dPort pair count.\n");
      return 1;
    }
    g_wanted_stat_pair_port = RWSTATS_TOP_N;
    g_limit_pair_port = (uint32_t) val;
    break;

  case STATOPT_PORTPAIR_TOP_THRESHOLD:
    /* top threshold sPort+dPort pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      fprintf(stderr, "May only specify one --portpair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal sPort+dPort pair threshold.\n");
      return 1;
    }
    g_wanted_stat_pair_port = RWSTATS_TOP_THRESH;
    g_limit_pair_port = (uint32_t) val;
    break;

  case STATOPT_PORTPAIR_TOP_PCT:
    /* top percentage sPort+dPort pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      fprintf(stderr, "May only specify one --portpair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal sPort+dPort pair top percentage.\n");
      return 1;
    }
    g_wanted_stat_pair_port = RWSTATS_TOP_PERCENT;
    g_limit_pair_port = val;
    break;

  case STATOPT_PORTPAIR_BTM_N:
    /* bottom N sPort+dPort pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      fprintf(stderr, "May only specify one --portpair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal sPort+dPort pair count.\n");
      return 1;
    }
    g_wanted_stat_pair_port = RWSTATS_BTM_N;
    g_limit_pair_port = (uint32_t) val;
    break;

  case STATOPT_PORTPAIR_BTM_THRESHOLD:
    /* bottom threshold sPort+dPort pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      fprintf(stderr, "May only specify one --portpair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal sPort+dPort pair threshold.\n");
      return 1;
    }
    g_wanted_stat_pair_port = RWSTATS_BTM_THRESH;
    g_limit_pair_port = (uint32_t) val;
    break;

  case STATOPT_PORTPAIR_BTM_PCT:
    /* bottom percentage sPort+dPort pairs */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_pair_port) {
      fprintf(stderr, "May only specify one --portpair-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal sPort+dPort pair bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_pair_port = RWSTATS_BTM_PERCENT;
    g_limit_pair_port = val;
    break;

  case STATOPT_PROTO_TOP_N:
    /* top N protocols */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_proto) {
      fprintf(stderr, "May only specify one --proto-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for protocol top N.\n");
      return 1;
    }
    g_wanted_stat_proto = RWSTATS_TOP_N;
    g_limit_proto = val;
    break;

  case STATOPT_PROTO_TOP_THRESHOLD:
    /* top threshold protocols */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_proto) {
      fprintf(stderr, "May only specify one --proto-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for protocol top threshold.\n");
      return 1;
    }
    g_wanted_stat_proto = RWSTATS_TOP_THRESH;
    g_limit_proto = val;
    break;

  case STATOPT_PROTO_TOP_PCT:
    /* top percentage protocols */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_proto) {
      fprintf(stderr, "May only specify one --proto-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal value for protocol top percentage.\n");
      return 1;
    }
    g_wanted_stat_proto = RWSTATS_TOP_PERCENT;
    g_limit_proto = val;
    break;

  case STATOPT_PROTO_BTM_N:
    /* bottom N protocols */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_proto) {
      fprintf(stderr, "May only specify one --proto-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for protocol bottom N.\n");
      return 1;
    }
    g_wanted_stat_proto = RWSTATS_BTM_N;
    g_limit_proto = val;
    break;

  case STATOPT_PROTO_BTM_THRESHOLD:
    /* bottom threshold protocols */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_proto) {
      fprintf(stderr, "May only specify one --proto-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val == ULONG_MAX) {
      fprintf(stderr, "Illegal value for protocol bottom threshold.\n");
      return 1;
    }
    g_wanted_stat_proto = RWSTATS_BTM_THRESH;
    g_limit_proto = val;
    break;

  case STATOPT_PROTO_BTM_PCT:
    /* bottom percentage protocols */
    if (!optarg) {
      return 1;
    }
    if (RWSTATS_NONE != g_wanted_stat_proto) {
      fprintf(stderr, "May only specify one --proto-* option\n");
      return 1;
    }
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 100) {
      fprintf(stderr, "Illegal value for protocol bottom percentage.\n");
      return 1;
    }
    g_wanted_stat_proto = RWSTATS_BTM_PERCENT;
    g_limit_proto = val;
    break;

  case STATOPT_OVERALL_STATS:
    /* combined stats for all protocols */
    g_do_stats_all_protos = 1;
    break;

  case STATOPT_DETAIL_PROTO_STATS:
    /* detailed stats for specific proto */
    if (0 != parseProtos(optarg)) {
      return 1;
    }
    g_do_stats_all_protos = 1;
    break;

  case STATOPT_PRINT_FILENAMES:
    /* whether to print input filenames */
    g_print_filenames_flag = 1;
    break;

  case STATOPT_NO_TITLES:
    /* whether to print column titles */
    g_print_titles_flag = 0;
    break;

  case STATOPT_DELIMITED:
    /* delimiter string & fixed width output columns */
    if (optarg) {
      MALLOCCOPY(g_delim, optarg);
    }
    rwstatsSetColumnWidth(1);
    break;

  case STATOPT_INTEGER_IPS:
    /* whether to print integer ips */
    g_integer_ip_flag = 1;
    break;

  case STATOPT_CIDR_SRC:
    /* length of CIDR for src ips */
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 32) {
      fprintf(stderr, "Src IP CIDR length must be between 1 and 32.\n");
      return 1;
    }
    g_cidr_src = ~0 << (32 - val);
    break;

  case STATOPT_CIDR_DEST:
    /* length of CIDR for dest ips */
    val = strtoul(optarg, &post_optarg, 10);
    if (post_optarg == optarg || val < 1 || val > 32) {
      fprintf(stderr, "Dest IP CIDR length must be between 1 and 32.\n");
      return 1;
    }
    g_cidr_dest = ~0 << (32 - val);
    break;

  default:
    appUsage();			/* never returns */
    return 0;
  }

  return 0;			/* OK */
}


/*
 * appTeardown:
 *	teardown all used modules and all application stuff.
 * Arguments: None.
 * Returns: None
 * Side Effects:
 * 	All modules are torn down. Then application teardown takes place.
 *	However, global variable abortFlag controls whether to dump info.
 * NOTE: This must be idempotent using static teardownFlag.
*/
void appTeardown(void) {
  static int teardownFlag = 0;

  if (0 != teardownFlag) {
    return;
  }
  teardownFlag = 1;

  fglobTeardown();
  iochecksTeardown(ioISP);
  if (outF != stdout) {
    outIsPipe ? pclose(outF) : fclose(outF);
  }
  return;
}


/*
 * parseProtos
 *      Discover which protos the user wants detailed stats for
 * Arguments:
 *      arg -the command line argument
 * Returns:
 *      0 if OK; 1 if error
 * Side Effects:
 *      Sets values in the global g_proto_to_stats_idx[]
*/
static int parseProtos(char *arg)
{
  uint8_t n;
  uint8_t i;
  uint8_t count;
  uint8_t *parsedList;
  int proto_idx = 1; /* 0 is global stats */

  parsedList = skParseNumberList(arg, 0, 255, &count);
  if (!parsedList) {
    return 1;
  }

  for (i = 0; i < count; ++i) {
    n = parsedList[i];
    if (0 != g_proto_to_stats_idx[n]) {
      fprintf(stderr, "Duplicate protocol %u ignored\n", n);
    } else if (proto_idx >= RWSTATS_NUM_PROTO) {
      fprintf(stderr, "You cannot specify more than %d protocols",
              RWSTATS_NUM_PROTO - 1);
      free(parsedList);
      return 1;
    } else {
      g_proto_to_stats_idx[n] = proto_idx;
      ++proto_idx;
    }
  }

  free(parsedList);
  return 0;
}


