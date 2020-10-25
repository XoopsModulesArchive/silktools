#ifndef _UTILS_H
#define _UTILS_H
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
** 1.23
** 2004/03/10 21:36:09
** thomasm
*/

/*@unused@*/ static char rcsID_UTILS_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "utils.h,v 1.23 2004/03/10 21:36:09 thomasm Exp";


/*
 *  a collection of utility routines.
 *
 *  Suresh L Konda
 */


#include "silk.h"

#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef	USE_ZLIB
#include <zlib.h>
#endif

#if SK_NEED_GETOPT
#include "gnu_getopt.h"
#else
#include <getopt.h>
#endif

/* Application name registration */
extern char *skAppName();
extern void skAppRegister(char *name);

/*
**
**    options.c
**
*/

/* long option arg types */
#define NO_ARG		0
#define	REQUIRED_ARG	1
#define	OPTIONAL_ARG	2
#define HELPVALUE	0xFFFF


/* argument checking used by checkArg() */
#define INTEGER_ONLY	0
#define UNSIGNED_ONLY	1
#define FLOAT_ONLY	2


/* name of option for app to load a dynamlic library */
#define OPT_DYNAMIC_LIBRARY "dynamic-library"


/* generic types for dealing with opaque handles */
typedef void *clientData;
typedef int (* optHandler)(clientData cData, int optIndex, char *optarg);
typedef void (* usageFn)(void);


int optionsSetup(usageFn fn);
/* returns 0 (ok), 1 (not ok) */

void optionsTeardown(void);

int optionsRegister(void *inOptions, optHandler handler, clientData cData);

int optionsParse(int argc, char **argv);
/* returns optind */


int checkArg(const char *arg, int argType);
/*
** checkArg
**	check if the given argument is correct depending on the argType
** Inputs:
**	char *arg; int argType
** Output:
**	0 if ok. 1 else
*/




/*
**
**    times.c
**
*/

int32_t julianDate(int yr, int mo, int day);
/* jdate with 1999/12/1 == 0 */


void gregorianDate(int32_t jd, int32_t *yr, int32_t *mo, int32_t *day);
/* with jdate with 1999/12/1 == 0 */


#if SK_HAVE_TIMEGM
#  define SK_TIME_GM(tm) timegm(tm)
#else
#  define SK_TIME_GM(tm) sk_timegm(tm)
#endif
time_t sk_timegm(struct tm *tm);
/*
 *  Does the reverse of gmtime().  Takes a time structure and returns
 *  seconds since the epoch in the UTC timezone
 */


char *timestamp(uint32_t t);
/*
 *  Treats t as seconds since the UNIX epoch and returns a pointer to
 *  a static string buffer containing the UTC time in the form:
 *
 *      "mm/dd/yyyy HH:MM:SS"
 */


int getJDFromDate(char *line, int *hour);
/*
 * getJDFromDate:
 * 	Given a string of YYYY/MM/DD:HH:MM:SS
 *	return a julian date calculated from 0 for 12/1/1999.
 * 	If the hour (etc) are given return the hour in the
 * 	int pointer vaiable
 * Input:
 *	Date string
 * Outputs:
 *	JD as returned value.
 * Side Effect:
 *	the int pointer argument is filled with the hour if given
 *	or -1 otherwise.
 *	Exits on error.
 */


int maxDayInMonth(int yr, int mo);
/*
 *  maxDayInMonth:
 *  	Use fixed array and julianDate and gregorianDate to compute
 *	the maximum day in a given month/year
 *	exit(1) on failure.
 *
 *  Months are in the 1..12 range and NOT 0..11
 *
 */


/*
**
**    silkfilesys.c
**
*/

char *baseName(const char *fp);
/*
 *  Strip directory prefix from the file path fp.  Returns a pointer
 *  to a static string buffer.
 */


char *dirName(const char *fp);
/*
 *  Strip file name suffix from the file path fp.  Returns a pointer
 *  to a static string buffer.
 */


#define FILEIsATty(fd)		isatty(fileno(fd))
/*
 *  Returns 1 if the FILE* fd is a tty, 0 otherwise
 */


int isFIFO(const char *name);
/*
 *  Returns 1 if name exists and is a FIFO; returns 0 otherwise.
 *  Prints message to stderr if stat(name) fails.
 */


int dirExists(const char *dName);
/*
 *  Returns 1 if dName exists and is a directory; returns 0 otherwise.
 */


int fileExists(const char *fName);
/*
 *  Returns 1 if fName exists and is a regular file; returns 0
 *  otherwise.
 */


off_t fileSize(const char *fName);
/*
 *  Returns the size of the file fName.  Returns 0 if file is empty or
 *  if it does not exist; use fileExists() to check for existence
 */


#define getRLockFD(fd)         silkFileLocks((fd), F_RDLCK, F_SETLK)
#define getRLockFILE(fp)       getRLockFD(fileno(fp))
/*
 *  int ret = getRLockFD(int fd);           // file descriptor
 *  int ret = getRLockFILE(FILE *fp);       // file pointer
 *
 *  Get an exclusive read lock on the opened file.  It does not wait.
 *
 *  Returns: 0 OK; 1 failed;
 *
 */


#define getRLockFD_Wait(fd)    silkFileLocks((fd), F_RDLCK, F_SETLKW)
#define getRLockFILE_Wait(fp)  getRLockFD_Wait(fileno(fp))
/*
 *  int ret = getRLockFD_Wait(int fd);      // file descriptor
 *  int ret = getRLockFILE_Wait(FILE *fp);  // file pointer
 *
 *  Get an exclusive read lock on the opened file.  It waits till it
 *  gets the lock indefinitely.
 *
 *  Returns: 0 OK; 1 failed;
 *
 */


#define getWLockFD(fd)         silkFileLocks((fd), F_WRLCK, F_SETLK)
#define getWLockFILE(fp)       getWLockFD(fileno(fp))
/*
 *  int ret = getWLockFD(int fd);           // file descriptor
 *  int ret = getWLockFILE(FILE *fp);       // file pointer
 *
 *  Get an exclusive write lock on the opened file.  It does not wait.
 *
 *  Returns: 0 OK; 1 failed;
 *
 */


#define getWLockFD_Wait(fd)    silkFileLocks((fd), F_WRLCK, F_SETLKW)
#define getWLockFILE_Wait(fp)  getWLockFD_Wait(fileno(fp))
/*
 *  int ret = getWLockFD_Wait(int fd);      // file descriptor
 *  int ret = getWLockFILE_Wait(FILE *fp);  // file pointer
 *
 *  Get an exclusive write lock on the opened file.  It waits till it gets
 *  the lock indefinitely.
 *
 *  Input: the file descriptor
 *
 *  Returns: 0 OK; 1 failed;
 *
 */


#define releaseLockFD(fd)      silkFileLocks((fd), F_UNLCK, F_SETLK)
#define releaseLockFILE(fp)    releaseLockFD(fileno(fp))
/*
 *  int ret = releaseLockFD(int fd);        // file descriptor
 *  int ret = releaseLockFILE(FILE *fp);    // file pointer
 *
 *  Release a previously locked file.
 *
 *  Returns: 0 OK; 1 failed;
 *
 */


int silkFileLocks(int fd, short type, int cmd);
/*
 *  Perform a locking operation on the specified file.  For use by the
 *  convenience macros above, which see.
 */


char *findFile(const char *fName);
/*
 *  findFile(fName);
 *
 *  Find the given file in one of several places:
 *
 *  -- See if the environment variable named by the cpp macro
 *     ENV_SILK_PATH (normally SILK_PATH) is defined.  If so, look in
 *     $SILK_PATH/share/fName
 *  -- For historical reasons, look in $SILK_PATH/fName
 *  -- Take the full path to the application (/yadda/yadda/bin/app),
 *     lop off the app's parent directory (/yadda/yadda), and check
 *     the "/share" subdir (/yadda/yadda/share/fName
 *
 *  Inputs:
 *	filename to find
 *	
 *  Outputs:
 *  	Return char * if found NULL else.  When non-null is returned,
 *  	the caller should free() the char* when finished.
 */


char *skutilsFindPluginPath(const char *dlPath, char *path, size_t path_len);
/*
 *  skutilsFindPluginPath(dlPath, path, path_len);
 *
 *  Find the given plugin path, dlPath, in one of several places:
 *
 *  -- If dlPath contains a slash, return NULL.
 *  -- See if the environment variable named by the cpp macro
 *     ENV_SILK_PATH (normally SILK_PATH) is defined.  If so, look in
 *     $SILK_PATH/share/fName
 *  -- For historical reasons, look in $SILK_PATH/fName
 *  -- Take the full path to the application (/yadda/yadda/bin/app),
 *     lop off the app's parent directory (/yadda/yadda), and check
 *     the "/share" subdir (/yadda/yadda/share/fName
 *
 *  Inputs:
 *	- name or path of plug-in to find
 *      - buffer to hold the path
 *      - length of the buffer
 *	
 *  Outputs:
 *  	Return NULL if dlPath was not found; otherwise return a char*
 *  	which is the buffer passed into the subroutine.
 */




#ifdef	USE_ZLIB
int gzopenFile(const char *inFName, int mode, gzFile **theFile);
#endif
/*
 * gzopenFile:
 * 	open file using gzopen with appropriate error handling.
 * Input:
 *	char * inFName;
 *	int mode (0 == read; 1 == write)
 *	gzFile ** the gzfile struct to be returned
 * Output: 0 if OK; 1 else with appropriate error messages to stdout
 */


int openFile(const char *FName, int mode, FILE **fp, int *isPipe);
/*
 *  openFile:
 *  	open file as pipe or regular depending on whether it is a gzipped
 *	file or not.
 *  Input:
 *  	char * fileName, int mode , **FILE, *isPipe
 *	where mode = 0 == read; 1 == write.
 *  Output:
 *  	0/1 sucess/failure
 *  Side Effects:
 *  	filePointer and isPipe pointers set
 */


int mkDirPath(const char *dirName);
/*
 *  mkDirPath:
 *	make full directory path including parent if required
 *  Input: char * path
 *  Output: 0 on success, 1 on failure
 */



/*
**
**    silkstring.c
**
*/


#define MALLOCCOPY(a,b) {if (!((a)=strdup(b))) {fprintf(stderr,"MALLOCCOPY: Out of memory\n");exit(1);}}


char *num2dot(uint32_t ip);
/*
 *  Converts the integer form of an IPv4 IP address to the dotted-quad
 *  version.  ip is taken to be in native byte order; returns a
 *  pointer to a static string buffer.
 *
 *  Returns NULL on error.
 */


uint32_t dot2num(const char *ip);
/*
 *  Converts the string buffer ip containing an IPv4 dotted-quad IP
 *  address into an integer in the native byte order.
 *
 *  Returns 0xFFFFFFFF on error.
 */


char *tcpflags_string(const uint8_t flags);
/*
 *  Return a 6 character string denoting which TCP flags are set.  If
 *  all flags are on, UAPRSF is returned.  For any flag that is off, a
 *  space appears in place of the character.  Returns a pointer to a
 *  static buffer.
 */


int strip(char *line);
/*
 *  Strips all whitespace from the start and end of line.  Modifies
 *  line in place.
 */


void lower(char *cp);
/*
 *  Converts uppercase letters in cp to lowercase.  Modifies the
 *  string in place.
 */


void upper(char *cp);
/*
 *  Converts lowercase letters in cp to uppercase.  Modifies the
 *  string in place.
 */


uint8_t *skParseNumberList(const char *input, uint8_t minValue,
                           uint8_t maxValue, uint8_t *valueCount);
/*
 * skParseNumberList
 *
 *    Given a list (i.e., comma or hyphen delimited set of
 *    non-negative integers), return an array whose values are the
 *    numbers the list contains, breaking ranges into a list of
 *    numbers.  If duplicates appear in the input, they will appear in
 *    the return value.  Order is maintained.  Thus the input
 *    "4,3,2-6" will return an array containing {4,3,2,3,4,5,6}.
 *
 *    INPUT:
 *      input -- the string buffer to be parsed
 *      minValue -- the minimum allowed value in user input
 *      maxValue -- the maximum allowed value in user input
 *      valueCount -- the address of a uint16_t in which to store the
 *      number valid elements in the returned array.
 *
 *    The caller should free() the returned array when finished
 *    processing.
 *
 *    On error, NULL is returned and *valueCount will be set to 0.
 */

#endif /* _UTILS_H */
