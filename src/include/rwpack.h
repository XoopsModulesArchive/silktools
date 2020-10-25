#ifndef _RWPACK_H
#define _RWPACK_H
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
** 1.33
** 2004/03/18 21:15:54
** thomasm
*/

/*@unused@*/ static char rcsID_RWPACK_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "rwpack.h,v 1.33 2004/03/18 21:15:54 thomasm Exp";

#include <stdio.h>
#include "silk.h"

/* period definitions*/
#define PREVPREV 0
#define PREV  1
#define CUR   2
#define NEXT 3
/* num periods should be NEXT + 1 */
#define NUMPERIODS 4

/* # milliseconds of slop allowed in sysuptime checks in msecs*/
#define MAX_SUT_DELTA   500000
#define MAX_PKTS 1048576        /* 2^20 */
#define PKTS_DIVISOR 64         /* gives a max of 67,100,864 pkts */
#define BPP_BITS        6
#define BPP_PRECN       64 /* 2^BPP_BITS */
#define BPP_PRECN_DIV_2       32 /* 2^BPP_BITS/2 */
/*
** max elapsed time is the offset from the record end time and
** with the 30 minute flush can be at most 30*60 = 1800
*/
#define MAX_ELAPSED_TIME        2048 /* 2*11 */
/*
** max end time is offset from the file max end time and can be at
** most 5*60 = 300
*/
#define MAX_START_TIME          4096 /* 2^12 */
#define MAX_IP_NUMS 2           /*the max # of ip's passed to the packer*/
#define MAX_INDICES 256


#define SUCCESSFUL_TERM 0
#define GENERAL_ERROR 1
#define REBOOT_ERROR 2
#define REBOOT_GENERAL_ERROR 3

#define FT_PDU_V5_MAXFLOWS    30  /* max records in V5 packet, from the OSU flow tools project */
#define V5_PDU_LEN 1464

#define DIR_SUT  "sut"
#define FNPREFIX_SUT "sut"


#define  CONVERTTIME(r,t)       (r).t = rtr_unix_secs + ((r).t/1000) + \
                        ( (rtr_unix_msecs + ((r).t %1000)) >= 1000 ? 1 : 0);


/* The maximum size of a packed record.  All of the rw*Rec_V* below
 * should fit in uint8_t array of this size. */
#define SK_MAX_RECORD_SIZE 64


/* this is the genric header used in FT_RWROUTED, FT_RWNOTROUTED,
   and FT_RWSPLIT */
typedef struct rwFileHeaderV0 {
  genericHeader gHdr;
  uint32_t fileSTime;
}rwFileHeaderV0;

typedef rwFileHeaderV0 *rwFileHeaderV0Ptr;

typedef struct streamInfo {
  char *fName;
#ifdef  USE_FILDES
  int outFD;
#else
  FILE *outF;
#endif
  uint32_t count;
  uint32_t sTime;
} streamInfo;

/* FIXME. Ultimately, we'll want to split out the static type
 * definition information from the rwpack state, and move the static
 * configuration information to another file. */
typedef struct fileTypeInfo {
  char *className;
  char *dirPrefix;
  char *filePrefix;             /* file prefix */
  /* FIXME: We think the following field supersedes filePrefix */
  char *pathPrefix;
  int32_t filesPerDay;
  int32_t packType;             /* one of RW_XXXX from silk.h */
  streamInfo sInfo[NUMPERIODS];
  uint32_t maxETime;            /* utc end of the next file period */
} fileTypeInfo;

typedef struct v5Header {
  uint16_t version;
  uint16_t count;
  uint32_t sysUptime;
  uint32_t unix_secs;
  uint32_t unix_nsecs;
  uint32_t flow_sequence;
  uint8_t engine_type;
  uint8_t engine_id;
  uint16_t padding;
} v5Header;

typedef struct v5Record {
  ipUnion sIp;
  ipUnion dIp;
  ipUnion nexthop;
  uint16_t input;
  uint16_t output;
  uint32_t dPkts;
  uint32_t dOctets;
  uint32_t First;
  uint32_t Last;
  uint16_t sPort;
  uint16_t dPort;
  uint8_t pad1;
  uint8_t flags;
  uint8_t proto;
  uint8_t tos;
  uint16_t dst_as;
  uint16_t src_as;
  uint8_t dst_mask;
  uint8_t src_mask;
  uint16_t pad2;
} v5Record;

typedef struct v5PDU {
  v5Header hdr;
  v5Record data[FT_PDU_V5_MAXFLOWS];
} v5PDU;

typedef struct engineData {
  uint32_t next_seq;
  uint32_t missingRecCount;
  uint32_t pktCount;
  uint32_t recCount;
  uint32_t badRecCount;
  uint32_t prev_sysUptime;
} engineData;
typedef engineData * engineDataPtr;

typedef struct routerData {
  engineDataPtr eData[65536];
  uint16_t version;
  /* sysUptime: from prev pdu or disk file if not-initialized */
  uint32_t pktCount;
  uint32_t outofSeqCount;
  uint32_t badPktCount;
  uint32_t badRecCount;
  uint32_t misRecCount;
  uint16_t numEngines;
} routerData;

/* 28 bytes.  No padding */
typedef struct rwRtdRec_V1 {
#if     0
  ipUnion sIP;
  ipUnion dIP;
  ipUnion nhIP;

  uint16_t sPort;
  uint16_t dPort;

  uint32_t pef; /*uint32_t pkts:20; uint32_t elapsed:11; uint32_t pktsFlag:1;*/
  uint32_t sbb; /*uint32_t sTime:12; uint32_t bPPkt:14; uint32_t bPPFrac:6;*/

  uint8_t proto;
  uint8_t flags;
  uint8_t input;
  uint8_t output;

#endif
  uint8_t a[28];
} rwRtdRec_V1;

typedef rwRtdRec_V1 *rwRtdRecPtr_V1;

/*  23 bytes on disk. 24 bytes with 1 byte padding in memory*/
typedef struct rwNrRec_V1 {
#if     0
  ipUnion sIP;
  ipUnion dIP;

  uint16_t sPort;
  uint16_t dPort;

  uint32_t pef; /*uint32_t pkts:20; uint32_t elapsed:11; uint32_t pktsFlag:1;*/
  uint32_t sbb; /*uint32_t sTime:12; uint32_t bPPkt:14; uint32_t bPPFrac:6;*/

  uint8_t proto;
  uint8_t flags;
  uint8_t input;        /* output is always 0 */
  uint8_t pad;          /* wasted in mem  */
#endif
  uint8_t a[23];

} rwNrRec_V1;

typedef rwNrRec_V1 *rwNrRecPtr_V1;

/* 22 bytes on disk. 24 bytes with 2 bytes padding in memory*/
typedef struct rwSplitRec_V1 {
#if     0
  ipUnion sIP;
  ipUnion dIP;

  uint16_t sPort;
  uint16_t dPort;

  uint32_t pef; /*uint32_t pkts:20; uint32_t elapsed:11; uint32_t pktsFlag:1;*/
  uint32_t sbb; /*uint32_t sTime:12; uint32_t bPPkt:14; uint32_t bPPFrac:6;*/

  uint8_t proto;
  uint8_t flags;
  uint16_t padding;             /* wasted in mem */
#endif
  uint8_t a[22];

} rwSplitRec_V1;

typedef rwSplitRec_V1 *rwSplitRecPtr_V1;
/* 12 bytes */
typedef struct rwACLRec_V1 {
#if     0
  ipUnion sIP;
  ipUnion dIP;
  uint16_t dPort;
  uint8_t proto;
  uint8_t stuff;                /* multPktsFlag:1, flagsFlag:1, sTime:6 */
#endif
  uint8_t a[12];

} rwACLRec_V1;

typedef rwACLRec_V1 *rwACLRecPtr_V1;

/*
**  rw filter record is basically the same as rwRec except that
**  we do some packing but keep all fields full.
*/
typedef struct rwFilterRec_V1 {
#if     0
  ipUnion sIP;
  ipUnion dIP;

  uint16_t sPort;
  uint16_t dPort;

  uint8_t proto;
  uint8_t flags;
  uint8_t input;
  uint8_t output;

  ipUnion nhIP;
  uint32_t sTime;

  uint32_t pkts    :20;
  uint32_t elapsed :11;
  uint32_t pktsFlag: 1;

  uint32_t bPPkt   :14;
  uint32_t bPPFrac : 6;
  uint32_t sID     : 6;
  uint32_t pad2    : 6;         /* wasted in core and disk */
#endif
  uint8_t a[32];
} rwFilterRec_V1;
typedef rwFilterRec_V1 * rwRecFilterPtr_V1;

/* Updated version of filter record that stores 8-bit sensor ids in
** the least significant bits of the last 32-bit word:
 */
typedef struct rwFilterRec_V2 {

#if 0
   /* See V1 above */    
  uint32_t bPPkt   :14;
  uint32_t bPPFrac : 6;
  uint32_t pad2    : 4;         /* wasted in core and disk */
  uint32_t sID     : 8;
#endif    
  uint8_t a[32];
} rwFilterRec_V2; 

/*
**  rw web record is used to record web traffic
*/
typedef struct rwWebRec_V1 {
#if 0
  ipUnion sIP;
  ipUnion dIP;

  uint32_t pkts    :20;
  uint32_t elapsed :11;
  uint32_t pktsFlag: 1;

  uint32_t sTime   :12;
  uint32_t bPPkt   :14;
  uint32_t bPPFrac : 6;

  uint16_t port;                /* the non-web port */

  uint8_t wsPort    :1;         /* 0 if non-web-port is src, 1 if dst */
  uint8_t reserved  :1;
  uint8_t flags     :6;

  uint8_t wPort     :2;         /* 0 = 80 ; 1 = 443; 2 = 8080 */
  uint8_t padding   :6;
#endif
  uint8_t a[20];
} rwWebRec_V1;
typedef rwWebRec_V1 * rwWebRecPtr_V1;

/*
**  This is the generic record returned from ANY packed type.
**  37 bytes on disk.  40 in core with 3 bytes padding  In order
**  to enable a single fwrite, re-arranged on 6/6/2002 to write
**  37 contiguous bytes in one go.
*/
typedef struct rwRec {
  ipUnion sIP;
  ipUnion dIP;

  uint16_t sPort;
  uint16_t dPort;

  uint8_t proto;
  uint8_t flags;
  uint8_t input;
  uint8_t output;

  ipUnion nhIP;
  uint32_t sTime;

  uint32_t pkts;
  uint32_t bytes;
  uint32_t elapsed;

  uint8_t sID;
  uint8_t padding[3];

}rwRec;
typedef rwRec * rwRecPtr;

/*tyepdef int (* ()) rwrecReadFN  */

/* struct for rw file io */
typedef struct rwIOStruct {
  /*
  ** generic + whatever is specific to the file type. Responsibility of
  ** the opener to know what this is
  */
  void * hdr;
  uint32_t hdrLen;              /* the real length of the hdr */
  uint32_t (*rwReadFn)(struct rwIOStruct *, rwRecPtr);
  uint32_t (*rwWriteFn)(struct rwIOStruct *, rwRecPtr);
  uint32_t (*rwSkipFn)(struct rwIOStruct *, int);
  int (*closeFn)(FILE *);
  FILE *FD;
  FILE *copyInputFD;            /* file to  copy input to stdout */
  int32_t swapFlag;
  char *fPath;
  int eofFlag;
  uint8_t recLen;               /* fixed len of  recs of this type */
  uint8_t *origData;            /* copy of input date */
  /* sID required here for version 0 of all rw file types
     It will  be embedded into each record in later versions
  */
  uint8_t sID;
} rwIOStruct;
typedef rwIOStruct * rwIOStructPtr;


/* function declarations for handling PDUs */
void flowPktTimeSetup(v5Header *hdr);
extern uint32_t rtr_unix_secs;
extern uint32_t rtr_unix_msecs;


void logMsg(char *fmt, ...);

/* rwreader functions */
rwIOStructPtr rwOpenFile (char *fPath, FILE * copyInputFD);
rwIOStructPtr rwWriteOpen(char *fPath);
void rwCloseFile (rwIOStructPtr);
void  rwPrintHeader(rwIOStructPtr, FILE *);

/*  sID  stuff */
const char *sensorIdToName(uint8_t id);
uint8_t sensorNameToId(const char *name);


/* librw will set the sid field to this value for unknown sensor */
#define SENSOR_INVALID_SID 255

/* macros for accessing header fields */
#define rwIsCompressed(a)       ((a)->closeFn == pclose ? 1 : 0)
#define rwRead(a, b)            (a)->rwReadFn((a), (b))
#define rwWrite(a, b)           (a)->rwWriteFn((a), (b))
#define rwSkip(a, b)            (a)->rwSkipFn((a), (b))
#define rwGetIsBigEndian(a)     ((genericHeader *)a->hdr)->isBigEndian
#define rwGetFileType(a)        ((genericHeader *)a->hdr)->type
#define rwGetFileSTime(a)       ((rwFileHeader *)a->hdr)->fileSTime
#define rwGetFILE(a)            (a)->FD
#define rwGetFileName(a)        (a)->fPath
#define rwGetHeader(a)          (a)->hdr

#endif /* _RWPACK_H */
