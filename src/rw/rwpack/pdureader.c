/*
** Copyright (C) 2003-2004 by Carnegie Mellon University.
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
** 1.8
** 2004/02/18 20:59:16
** thomasm
*/

/*
**  Application commentary goes here
**
*/


/* Includes */
#include "silk.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>

#include "utils.h"
#include "rwpack.h"
#include "log.h"

RCSIDENT("pdureader.c,v 1.8 2004/02/18 20:59:16 thomasm Exp");

#define WEBP(p) ((p) == 80 || (p) == 443 || (p) == 8080)
#define IS_WEB_PORT(p) WEBP(p)
#define IS_WEB(r) ((r).proto == 6 && (IS_WEB_PORT((r).sPort) || IS_WEB_PORT((r).dPort)))
#define WEBPORT(p) (((p) == 80) ? 0 : (((p) == 443) ? 1 : (((p) == 8080) ? 2 : 3)))
#define WEB_PORT(p) WEBPORT(p)

typedef struct pdubuffer {
  uint32_t maxsize;             /* Maximum number of cells */
  uint32_t size;                /* Current number of cells in use */
  uint32_t head;                /* Index of next unused cell */
  uint32_t tail;                /* Index of oldest unprocessed used cell */
  v5PDU *data;                  /* The cell data */
  pthread_mutex_t mutex;        /* Mutex */
  pthread_cond_t cond;          /* Condition variable */
} pdubuffer;

/*
**  +---+---+---+---+---+---+---+
**  |   | u | d | d | d | f |   |
**  +---+---+---+---+---+---+---+
**  t              h
**
**  d = data
**  f = being filled
**  u = data-in-use
**  t = tail index
**  h = head index
**  <blank> = unused
**
*/

typedef void (*cleanupHandler)(void *);

pdubuffer pdubuf;

extern uint8_t reading;
extern int rcvrSock;

static uint32_t count;
static uint32_t inPkts;
static uint32_t procPkts;
static uint32_t badPkts;
static int32_t missingRecs; /* NOTE: signed int to allow for out of seq pkts */

/* these could be static, but they are declared "extern" in rwpack.h */
uint32_t rtr_unix_secs;
uint32_t rtr_unix_msecs;

static uint32_t flowSeqNumbers[65536];
static uint32_t engineMasks[2048]; /* 65536 bits */

static pthread_t pdureader;

extern int processRecord(v5Record *v5RPtr);

#define CHECK_MARK(e) (engineMasks[e>>5] & (1 << (e & 0x1f)))
#define SET_MARK(e)   (engineMasks[e>>5] |= (1 << (e & 0x1f)))

/*
** macro uses globals rtr_unix_secs and rtr_unix_msecs to compute the
** epoch seconds for the given field.  All are assumed to be in the
** local endian order.
*/
#define  CONVTIME(t)    t = rtr_unix_secs + (t/1000) + \
                        ( (rtr_unix_msecs + (t %1000)) >= 1000 ? 1 : 0);

/* Initialize a pdubuffer */
void initPdubuffer(uint32_t kilobytes, pdubuffer *buf)
{
  memset(buf, 0, sizeof(pdubuffer));
  buf->maxsize = 1024 * kilobytes / sizeof(v5PDU);
  if (buf->maxsize < 3) {
    fprintf(stderr, "PDU buffer too small");
    exit(1);
  }
  buf->data = (v5PDU *)malloc(sizeof(v5PDU) * buf->maxsize);
  if (!buf->data) {
    fprintf(stderr, "Unable to allocate PDU buffer.");
    exit(1);
  }
  pthread_mutex_init(&buf->mutex, NULL);
  pthread_cond_init(&buf->cond, NULL);
}

/* Destroy a pdubuffer */
void destroyPdubuffer(pdubuffer *buf)
{
  free(buf->data);
  pthread_mutex_destroy(&buf->mutex);
  pthread_cond_destroy(&buf->cond);
}


/* Get a pointer to the next pdu on the tail (kills the previous one).
   Blocks on empty.  returns in-use cell (u) */
v5PDU *nextPduTail(pdubuffer *buf)
{
  v5PDU *retval;

  pthread_cleanup_push((cleanupHandler)pthread_mutex_unlock,
                       (void *)&buf->mutex);
  pthread_mutex_lock(&buf->mutex);
  while (!buf->size) {
    /* Wait on empty */
    pthread_cond_wait(&buf->cond, &buf->mutex);
  }
  if (buf->size == buf->maxsize - 2) {
    /* We were full, we will be no longer after this */
    pthread_cond_signal(&buf->cond); /* Won't actually take place
                                        until we release mutex */
  }
  retval = &buf->data[buf->tail++];
  buf->tail %= buf->maxsize;
  buf->size--;
  pthread_cleanup_pop(1);
  return retval;
}

/* Get a pointer to the next pdu on the head.
   Blocks on full.  returns to-be-filled cell (f) */
v5PDU *nextPduHead(pdubuffer *buf)
{
  v5PDU *retval;

  pthread_cleanup_push((cleanupHandler)pthread_mutex_unlock,
                       (void *)(&buf->mutex));
  pthread_mutex_lock(&buf->mutex);
  buf->head = (buf->head + 1) % buf->maxsize;
  retval = &buf->data[buf->head];
  while (buf->size == buf->maxsize - 2) {
    logMsg("PDU buffer full");
    pthread_cond_wait(&buf->cond, &buf->mutex);
  }
  if (!buf->size) {
    /* Used to be empty */
    pthread_cond_signal(&buf->cond); /* Won't actually take place
                                        until we release mutex */
  }
  buf->size++;
  pthread_cleanup_pop(1);
  return retval;
}

/*
** readerSetup:
**      Initialize all reader stuff.
*/
int readerSetup(void) {
  static int pduInit = 0;

  memset(engineMasks, 0, sizeof(engineMasks));
  memset(flowSeqNumbers, 0, sizeof(flowSeqNumbers));
  count = 0;

  if (!pduInit) {
    initPdubuffer(80000, &pdubuf); /* Should handle over 30
                                          seconds of data */
    pduInit = 1;
  }

  return 0;
}

/* pdu-reading thread */
void *readpdu(void * UNUSED(dummy)) {
  v5PDU *pdu;
  sigset_t sigs;
#if     DEBUGGING
  uint32_t ns;                  /* new seq num */
#endif

  sigfillset(&sigs);
  pthread_sigmask(SIG_SETMASK, &sigs, NULL); /* Ignore all signals */

  /* In this thread, allow cancelling at any cancellation point */
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  pdu = &pdubuf.data[pdubuf.head];              /* Initially, head */

  logMsg("PDU reader has started.");

  while (reading) {
    if (recv(rcvrSock, (void *)pdu, V5_PDU_LEN, 0 /*flags*/) == -1) {
      /* error reading from udp socket.  Shutdown and restart */
      logMsg("udp read error: %s", strerror(errno));
      continue;
    }
    inPkts++;
    pdu = nextPduHead(&pdubuf);
  }

  logMsg("PDU reader has stopped.");

  return NULL;
}

/*
** flowPktTimeSetup
**      Setup to calculate the record's utc times based on the header info.
**      This must be done once for each udp packet before
**              before using CONVTIME macro.
** Input: v5Header *;
** Output: None
** Side Effects:
**       globals hdr_unix_secs and hdr_unix_msecs are set
** NOTE:   * from flow-tools:  lib/ftio.c:
** function: ftltime
**
** Flow exports represent time with a combination of uptime of the
** router, real time, and offsets from the router uptime.  ftltime
** converts from the PDU to a standard unix seconds/milliseconds
** representation
**
*/
void flowPktTimeSetup(v5Header *hdr) {
  /* GMT when router was last re-booted based on this PDU */
  uint32_t hdr_sys_secs, hdr_sys_msecs;
  uint32_t hdr_unix_secs, hdr_unix_usecs;

  /* sysUpTime is in milliseconds, convert to seconds/milliseconds */
  hdr_sys_secs = hdr->sysUptime / 1000;
  hdr_sys_msecs = hdr->sysUptime % 1000;

  /* unix seconds/nanoseconds to seconds/milliseconds */
  hdr_unix_secs = hdr->unix_secs;
  hdr_unix_usecs = hdr->unix_nsecs / 1000000L;

  /* subtract sysUpTime from unix seconds */
  hdr_unix_secs -= hdr_sys_secs;

  /* borrow a sec? */
  if (hdr_sys_msecs > hdr_unix_usecs) {
    --hdr_unix_secs;
    hdr_unix_usecs += 1000;
  }
  hdr_unix_usecs -= hdr_sys_msecs;

  rtr_unix_secs = hdr_unix_secs;
  rtr_unix_msecs = hdr_unix_usecs;
  return;

}

/*
** reader:
**      Reads PDU.
**      Packs and write all real records according to type.
**      If full (i.e., past the high water mark), close file and put
**      into send queue at tail.
**      Open a new file.
**      if sender is stopped, start sender.
*/
void *processor(void * UNUSED(dummy)) { 
  pthread_attr_t attr;
  sigset_t sigs;
  v5PDU *pdu;
  v5Record *v5RPtr;
  uint16_t engine;
  uint8_t i;

  sigfillset(&sigs);
  sigdelset(&sigs, SIGPIPE);
  pthread_sigmask(SIG_SETMASK, &sigs, NULL); /* Only handle SIGPIPE */

  /* We don't want this thread to be arbitrarily cancelled except in
     very particular circumstances.  So, turn off cancelling for now,
     and set the cancel type to deferred for the one point we will
     have cancelling enabled.  Normal cancellation should happen by
     setting the variable `reading' to false. */
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

  logMsg("PDU handler started");

  /* Start the reader actual reading thread.  No need to wait (join)
     on it, so create it as a detached thread. */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&pdureader, &attr, readpdu, NULL);

  while (reading) {
    /* We could block in nextPduTail waiting for new data to come in
       from the reading thread.  Since we don't want to end up in a
       state where we might wait forever (in the case that no data
       *will* come in the the reading thread), enable thread
       cancellation around this point.  Once done, re-disable it.  */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pdu = nextPduTail(&pdubuf);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    procPkts++;

    pdu->hdr.count = ntohs(pdu->hdr.count);

    if (pdu->hdr.count > FT_PDU_V5_MAXFLOWS) {
      /* reject packet */
      badPkts++;
      continue;
    }

    pdu->hdr.sysUptime = ntohl(pdu->hdr.sysUptime);
    pdu->hdr.unix_secs = ntohl(pdu->hdr.unix_secs);
    pdu->hdr.unix_nsecs = ntohl(pdu->hdr.unix_nsecs);
    pdu->hdr.flow_sequence = ntohl(pdu->hdr.flow_sequence);
    flowPktTimeSetup(&pdu->hdr);

    /* handle seq numbers here */
    engine = (pdu->hdr.engine_type << 8) | pdu->hdr.engine_id;

    if (CHECK_MARK(engine)) {
      /* check hdr seqnum against expected seq num */
      if (pdu->hdr.flow_sequence < flowSeqNumbers[engine]) {
        /*
        ** Out of sequence packet.  Reduce missing flow count.
        ** However, do not change the expected seq num.
        */
        missingRecs -= pdu->hdr.count;
      } else {
        /* Check if this is a later packet and handling missing recs */
        if (pdu->hdr.flow_sequence > flowSeqNumbers[engine]) {
          /* Increase missing flow count */
          missingRecs += pdu->hdr.flow_sequence - flowSeqNumbers[engine];
        }
        /* Update the next expected seq */
        flowSeqNumbers[engine] = pdu->hdr.flow_sequence + pdu->hdr.count;
      }
    } else {
      /* A new engine. Mark and record */
      SET_MARK(engine);
      flowSeqNumbers[engine] = pdu->hdr.flow_sequence + pdu->hdr.count;
    }
  
    /* Loop through records */
    for (i=0; i < pdu->hdr.count; i++) {
      v5RPtr = &pdu->data[i];

      v5RPtr->sIp.ipnum = ntohl(v5RPtr->sIp.ipnum);
      v5RPtr->dIp.ipnum = ntohl(v5RPtr->dIp.ipnum);
      v5RPtr->input = ntohs(v5RPtr->input);
      v5RPtr->output = ntohs(v5RPtr->output);
      v5RPtr->dPkts = ntohl(v5RPtr->dPkts);
      v5RPtr->dOctets = ntohl(v5RPtr->dOctets);
      v5RPtr->First = ntohl(v5RPtr->First);
      v5RPtr->Last = ntohl(v5RPtr->Last);

      if ((v5RPtr->First > v5RPtr->Last) ||
          (v5RPtr->dPkts) < 1 ||
          (v5RPtr->dOctets) < 1 ||
          (v5RPtr->input >= MAX_INDICES) || 
          (v5RPtr->output >= MAX_INDICES)) {
        /* Handle bad record */
        continue;
      }

      v5RPtr->sPort = ntohs(v5RPtr->sPort);
      v5RPtr->dPort = ntohs(v5RPtr->dPort);

      CONVTIME(v5RPtr->First);
      CONVTIME(v5RPtr->Last);

      processRecord(v5RPtr);
    }
  }

  logMsg("PDU handler stopped");

  return NULL;
}
