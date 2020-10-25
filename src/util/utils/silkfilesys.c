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
** 1.7
** 2004/03/10 21:29:31
** thomasm
*/

/*
**  a collection of utility routines.
**
**  Suresh L Konda
*/


#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>

#include "utils.h"
#define	NOZLIB

RCSIDENT("silkfilesys.c,v 1.7 2004/03/10 21:29:31 thomasm Exp");


/* local functions */
static char *getExecutablePath(const char *app_name, char *buf, size_t bufsize);


char *baseName(const char *fp) {
  static char rp[PATH_MAX];		/* return pointer */

  /* basename may modify the string, so make a copy */
  strncpy(rp, fp, PATH_MAX);
  return basename(rp);
}


char *dirName(const char *fp) {
  static char rp[PATH_MAX];		/* return pointer */

  /* dirname may modify the string, so make a copy */
  strncpy(rp, fp, PATH_MAX);
  return dirname(rp);
}


int isFIFO(const char *name) {
  struct stat stBuf;
  if (stat(name, &stBuf) == -1) {
    fprintf(stderr, "isFIFO: cannot stat [%s]: %s\n", name, strerror(errno));
    return 0;
  }
  return (S_ISFIFO(stBuf.st_mode));
}


int dirExists(const char *dName) {
  struct stat stBuf;
  if (stat(dName, &stBuf) == -1) {
    return 0;			/* does not exist */
  }
  /* return a 1 only if this is a directory */
  return S_ISDIR(stBuf.st_mode);
}


int fileExists(const char *fName) {
  struct stat stBuf;
  if (stat(fName, &stBuf) == -1) {
    return 0;			/* does not exist */
  }
  /* return a 1 only if this is a regular file */
  return (S_ISREG(stBuf.st_mode) || S_ISFIFO(stBuf.st_mode));
}


off_t fileSize(const char *fName) {
  struct stat stBuf;
  if (stat(fName, &stBuf) == -1) {
    return 0;			/* does not exist */
  }
  /* it exists. return the size */
  return stBuf.st_size;
}


/*
 *  int ret = silkFileLocks(int fd, short type, int cmd);
 *
 *  Perform a locking operation on the opened file represented by the
 *  file descriptor fd.  type is the type of lock, it should be one of
 *  F_RDLCK for a read lock, F_WRLCK for a write lock, or F_UNLCK to
 *  unlock a previously locked file.  cmd should be one of F_SETLKW to
 *  wait indefinitely for a lock, or F_SETLK to return immediately.
 *  Return 0 if successful, 1 otherwise.
 *
*/
int silkFileLocks(int fd, short type, int cmd) {
  struct flock lock;
  lock.l_type = type;
  lock.l_start = 0;		/* at SOF */
  lock.l_whence = SEEK_SET;	/* SOF */
  lock.l_len = 0;		/* EOF */

  if (fcntl(fd, cmd, &lock) != -1) {
    /* success */
    return 0;
  }

  /* print warning */
  if (type == F_UNLCK) {
    /* cannot unlock */
    fprintf(stderr, "Cannot release lock\n");
  } else if (cmd == F_SETLKW) {
    /* cannot get lock and willing to wait */
    fprintf(stderr, "Cannot get lock\n");
  }

  return 1;
}


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
char *findFile(const char *fName) {
  const char *app_name = skAppName();
  const size_t bufsize = PATH_MAX;
  char buf[bufsize+1];
  char fullPath[bufsize+1];
  char *cp1;

  /* Set first & last char in buffers to NUL for safety */
  buf[0] = buf[bufsize] = '\0';
  fullPath[0] = fullPath[bufsize] = '\0';

  /* check inputs */
  if (!fName) {
    return NULL;
  }

  /* Check in $SILK_PATH/share and $SILK_PATH */
  cp1 = getenv(ENV_SILK_PATH);
  if (cp1) {
    snprintf(buf, bufsize, "%s/share/%s", cp1, fName);
    if (fileExists(buf)) {
      return strdup(buf);
    }
    snprintf(buf, bufsize, "%s/%s", cp1, fName);
    if (fileExists(buf)) {
      return strdup(buf);
    }
  }

  /* Look in binarypath/../share.  First, get full path to the
   * executable. */

  if (app_name == (char *)NULL) {
    goto ERROR;
  }
  if (NULL == getExecutablePath(app_name, fullPath, bufsize)) {
    fullPath[0] = '\0';
    goto ERROR;
  }

  /* Next, Lop off the executable name */
  cp1 = strrchr(fullPath, '/');
  if (!cp1) {
    /* Weird */
    fprintf(stderr, "Cannot find parent dir of %s\n", fullPath);
    fullPath[0] = '\0';
    goto ERROR;
  }

  /* Finally, make new path: cp1 is pointing at final '/' in fullPath,
   * replace that with with "/../share/fName", then call realpath() to
   * make it pretty. */
  snprintf(cp1, (bufsize - (cp1-fullPath)), "/../share/%s", fName);
  if (realpath(fullPath, buf) && fileExists(buf)) {
    return strdup(buf);
  }

 ERROR:
  fprintf(stderr, ("Cannot find file '%s' in $" ENV_SILK_PATH "/share,"
                   " in $" ENV_SILK_PATH ","), fName);
  if (!app_name) {
    fprintf(stderr, " and application name not registered.\n");
  }
  else if ('\0' == fullPath[0]) {
    fprintf(stderr, " and cannot obtain full path to %s.\n", app_name);
  }
  else {
    fprintf(stderr, " nor in %s\n", fullPath);
  }
  return ((char*)NULL);
}


#ifdef	USE_ZLIB
/*
** gzopenFile:
** 	open file using gzopen with appropriate error handling.
** Input:
**	char * inFName;
**	int mode (0 == read; 1 == write)
**	gzFile ** the gzfile struct to be returned
** Output: 0 if OK; 1 else with appropriate error messages to stdout
*/
int gzopenFile(const char *inFName, int mode, gzFile **theFile) {

  if ( (theFile = gzopen(inFName, "rb")) == NULL ) {
    if (errno) {
      /* not a zlib error. */
      fprintf(stdout, "Unable to gzopen %s [%s]\n", inFName, strerror(errno));
    } else {
      fprintf(stdout, "Unable to gzopen %s [Z_MEM_ERROR]\n", inFName);
    }
    return 1;
  }
  return 0;
}
#endif


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
int openFile(const char *FName, int mode, FILE **fp, int *isPipe) {
  char cmd[512];

  /* check that the file exists if this is a read open */
  if (mode == 0 && fileExists(FName) == 0) {
    fprintf(stderr, "%s non-existant\n", FName);
    return 1;			/* error */
  }
  if (strstr(FName, ".gz")) {
    *isPipe = 1;
    snprintf(cmd, sizeof(cmd), "gzip %s %s", mode ? ">" : "-d -c", FName);
    *fp = popen(cmd, mode ? "w" : "r");
  } else {
    *fp = fopen(FName, mode ? "w" : "r");
    *isPipe = 0;
  }

  if (*fp == (FILE *)NULL) {
    fprintf(stderr, "Unable to open %s for %s\n",
	    FName, mode ? "writing" : "reading");
    return 1;			/* error */
  }
  return 0;
}


/*
** mkDirPath:
**	make full directory path including parent if required
** Input: char * path
** Output: 0 on success, 1 on failure
*/
int mkDirPath(const char *dirName){
  char *cp;
  char *buf;
  mode_t dirMode = S_IRWXU | S_IRGRP | S_IWGRP |  S_IXGRP | S_IROTH | S_IXOTH;

  /* try the full one once */
  if(dirExists(dirName) || mkdir(dirName, dirMode) == 0) {
    return 0;			/* ok */
  } 
  
  /* build the complete path as required  starting from the top */
  buf = strdup(dirName);
  if (!buf) {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  cp = buf + 1;
  while (1) {
    cp = strchr(cp, '/');
    if (!cp) {
      break;
    }
    *cp = '\0';			/* nail at dirsep */
    if (!dirExists(buf) &&
	(mkdir(buf, dirMode) == -1)) {
      fprintf(stderr, "error creating parent dir %s: [%s]\n", buf,
	      strerror(errno));
      free(buf);
      return 1;
    }
    *cp = '/';			/* add dirsep back */
    cp++;			/* and check string starting at next char */
  }
  /* all but the last component of the path was made. Make the last one now*/
  if (mkdir(buf, dirMode) == -1) {
    fprintf(stderr, "error creating parent dir %s: [%s]\n", buf,
	    strerror(errno));
    free(buf);
    return 1;
  }

  free(buf);
  return (!dirExists(dirName));			/* OK */
}


/*
** getExecutablePath
**	get the full path of the executable
** Input:
**	char *fPath: the executable as given in argv[0]
** Output:
**	char *newFullPath
**	NULL on error
*/
static char *getExecutablePath(const char *app_name, char *buf, size_t bufsize)
{
  const size_t lenAppName = strlen(app_name);
  char *cp1, *cp2;
  size_t len;

  buf[0] = '\0';

  if (lenAppName > bufsize) {
    fprintf(stderr, "getExecutablePath: bufsize too small\n");
    return NULL;
  }

  if (app_name[0] == '/') {
    /* an absolute path */
    strcpy(buf, app_name);
    if (fileExists(buf)) {
      return buf;
    }
  }

  if (strchr(app_name, '/') == (char *)NULL ) {
    /* no path at all. Try all directories in $PATH */
    cp1 = getenv("PATH");
    if (!cp1) {
      fprintf(stderr, "no $PATH");
      return (char *)NULL;
    }
    /* printf("looking along PATH %s\n", cp1); */
    while (cp1) {
      cp2 = strchr(cp1, ':');
      if (cp2) {
        len = cp2-cp1;
        cp2++;
      } else {
        len = strlen(cp1);
      }
      if (len + lenAppName + 2 < bufsize) {
        strncpy(buf, cp1, len);
        buf[len] = '/';
        strcpy(buf + len + 1, app_name);
        /*      printf("looking for %s\n", buf); */
        if (fileExists(buf)) {
          return buf;
        }
      }
      cp1 = cp2;
    }
  }

  /*
  ** neither an absolute path nor on  $PATH. Must be a relative path with
  ** ./ or ../ in it
  */
  if (!getcwd(buf, bufsize)) {
    perror(__FILE__ " getExecutablePath (getcwd)");
    buf[0] = '\0';
    return (char *)NULL;
  }
  len = strlen(buf);
  if (len + lenAppName + 2 < bufsize) {
    buf[len] = '/';
    strcpy(buf + len + 1, app_name);
    if (fileExists(buf)) {
      return buf;
    }
  }

  fprintf(stderr, "%s not found anywhere\n", app_name);
  buf[0] = '\0';
  return (char *)NULL;
}
