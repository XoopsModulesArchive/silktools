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
** 1.9
** 2004/03/10 21:05:43
** thomasm
*/


/*
** log.c
** Library to support file-based logging.  Cobbled together based on the
** report.c code written by M. Duggan.
** Suresh Konda
** 4/23/2003
*/


#include "silk.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "utils.h"
#include "log.h"

RCSIDENT("log.c,v 1.9 2004/03/10 21:05:43 thomasm Exp");

#ifndef MAILX
#ifdef	LINUX
#define MAILX "mail"
#else
#define MAILX "mailx"
#endif
#endif

static int rolloverHour;
static char *logFilePrefix;
static char *logDir;
static FILE *logFS;
static char *curLogFilename;
static time_t rolloverTime;
static int mailAvailableFlag;
/* rotate mutex */
static pthread_mutex_t rotatemutex = PTHREAD_MUTEX_INITIALIZER; 
/* logging mutex */
static pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;

static int openLogFS(void);
static void rotateLogs(void);
static void setRolloverTime(void);
static void *compressLogs(void*);

int logSetup(const char *in_logFilePrefix, const char *in_logDir,
	     int rHour, int maFlag) {

  if (!dirExists((char *)in_logDir)) {
    fprintf(stderr, "Dir %s does not exist\n", in_logDir);
    return 1;
  }
  MALLOCCOPY(logFilePrefix, in_logFilePrefix);
  MALLOCCOPY(logDir, in_logDir);
  curLogFilename = (char *)malloc(32 + strlen(logFilePrefix) + strlen(logDir));
  if (openLogFS()) {
    free(logFilePrefix);
    free(logDir);
    free(curLogFilename);
    logFilePrefix = (char*)NULL;
    return 1;
  }
  rolloverHour = rHour;
  setRolloverTime();
  mailAvailableFlag = maFlag;
  return 0;
}

void logTeardown(void) {
  if(logFilePrefix) {
    free(logFilePrefix);
    free(logDir);
    free(curLogFilename);
    logFilePrefix = (char*)NULL;
  }

  if(logFS != stderr) {
    fclose(logFS);
    logFS = (FILE *)NULL;
  }
  return;
}


static int openLogFS(void) {
  char date[9];
  struct tm *tmNow;
  time_t t;
  t = time((time_t*)NULL);
  tmNow = localtime(&t);
  pthread_mutex_lock (&rotatemutex);
  strftime(date, sizeof(date), "%Y%m%d", tmNow);
  sprintf(curLogFilename, "%s/%s-%s.log", logDir, logFilePrefix, date);
  pthread_mutex_unlock (&rotatemutex);
  if ( (logFS = fopen(curLogFilename, "a")) == (FILE *)NULL) {
    fprintf(stderr, "Cannot open log file %s\n", curLogFilename);
    logFS = stderr;
    return 1;
  }
  return 0;
}

static void rotateLogs(void) {
  pthread_attr_t compAttr;
  pthread_t compThread;
  if (logFS) {
    fclose(logFS);
  }
  logFS = (FILE*)NULL;
  pthread_attr_init (&compAttr);
  pthread_attr_setdetachstate (&compAttr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock (&rotatemutex);
  if (pthread_create(&compThread, &compAttr, compressLogs, (void *)NULL) < 0) {
    fprintf (stderr, "error forking compressLog\n");
    pthread_mutex_unlock (&rotatemutex);
  }
  if (openLogFS()) {
    return;
  }
  setRolloverTime();
  return;
}

static void setRolloverTime(void) {
  time_t tNow;
  struct tm *tmNow, tmCopy;

  tNow = time((time_t*)NULL);
  tmNow = localtime(&tNow);
  memcpy(&tmCopy, tmNow, sizeof(tmCopy));
  tmCopy.tm_sec = 59;
  tmCopy.tm_min = 59;
  tmCopy.tm_hour = 23;
  rolloverTime = mktime(&tmCopy) + (rolloverHour * 60 * 60);
  return;
}

void vlogMsg(char *fmt, va_list args)
{
  time_t ct;
  struct tm *t;
  char stime[10];

  pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock,
		       (void *)&logmutex);
  pthread_mutex_lock(&logmutex);

  ct = time(NULL);
  t = localtime(&ct);
  strftime(stime, sizeof(stime), "%T", t);
  fprintf(logFS, "%s: ", stime);
  vfprintf(logFS, fmt, args);
  fprintf(logFS, "\n");
  fflush(logFS);
  if (rolloverTime < ct) {
    rotateLogs();
  }
  pthread_cleanup_pop(1);
  return;
}

/* Subject may not include double quotes */
int vreport(char *user, char *subject, char *message, va_list args)
{
  char buf[4096];
  size_t rv;
  FILE *f;
  int ev;
  int nleft;
  char *pos;
  char command[4096];

  if (logFS) {
    vlogMsg(message, args);
  }

  if (mailAvailableFlag) {
    vsnprintf(buf, sizeof(buf), message, args);
    buf[sizeof(buf) - 1] = 0;
    pos = buf;
    nleft = strlen(buf);
    snprintf(command, sizeof(command), "%s -s \"%s\" %s", MAILX,
	     subject, user);
    if ((f = popen(command, "w")) == NULL) {
      return -1;
    }
    while (nleft && (rv = fwrite(buf, 1, strlen(buf), f))) {
      pos += rv;
      nleft -= rv;
    }
    if (nleft) {
      return -1;
    }
    fwrite("\n", 1, 1, f);
    fflush(f);
    ev = pclose(f);
    return (ev);
  } else {
    return 0;			/* OK */
  }
}

void logMsg(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vlogMsg(fmt, args);
  va_end(args);
}

int report(char *user, char *subject, char *message, ...)
{
  int retval;
  va_list args;
  va_start(args, message);
  retval = vreport(user, subject, message, args);
  va_end(args);
  return retval;
}


static void *compressLogs (void UNUSED(*dummy)) {
  char command[PATH_MAX + 5];
  memset (&command, 0 , strlen(command));
  sprintf (command , "%s %s","gzip",  curLogFilename);
  pthread_mutex_unlock (&rotatemutex);
  if (system(command) <0 ) {
    fprintf (stderr, "compressing error\n");
  }
  return NULL;
}

